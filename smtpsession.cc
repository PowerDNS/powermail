/*
    PowerMail versatile mail receiver
    Copyright (C) 2002  PowerDNS.COM BV

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "smtpsession.hh"
#include "userbase.hh"
#include "misc.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "megatalker.hh"
#include <ctype.h>
#include "lock.hh"
#include "pool.hh"
#include "common.hh"
#include "config.h"
extern PoolClass<UserBase> *UBP;
extern Logger L;

int SmtpSession::s_counter;
pthread_mutex_t SmtpSession::s_counterlock= PTHREAD_MUTEX_INITIALIZER;

SmtpSession::SmtpSession(const string &remote): d_fromset(false), d_indata(false), d_megatalker(0)
{
  d_databytes=0;
  d_remote=remote;
}

SmtpSession::~SmtpSession()
{
  if(d_megatalker)
    delete d_megatalker;
}

bool SmtpSession::rsetCommand(string &response)
{
  rsetSession();
  response="250 Ok";
  return true;
}

void SmtpSession::rsetSession()
{
  d_from="";
  d_quotaneeded=true;
  d_kbroom=0;
  d_databytes=0;
  d_fromset=false;
  d_recipients.clear();
  if(d_megatalker)
    delete d_megatalker;
  d_megatalker=0;
}

string SmtpSession::makeIndex()
{

  ostringstream o;
  struct timeval tv;
  gettimeofday(&tv,0);
  {
    Lock l(&s_counterlock);
    o<<tv.tv_sec<<":"<<tv.tv_usec<<":"<<getHostname()<<"."<<s_counter++;
  }
  return o.str();
}
  
bool SmtpSession::eatData(const string &line, string &response)
{
  if(line=="DATA") {
    if(!d_fromset) {
      response="503 Please specify MAIL From: first";
      return true;
    }
    else if(d_recipients.empty() && !d_megatalker) { // no forwards either
      response="503 Please specify destination(s) first";
      return true;
    }
    else {
      try {
	if(!d_megatalker) 
	  d_megatalker=new MegaTalker;
	
	if(!d_recipients.empty()) // perhaps there are only forwards
	  d_megatalker->openStorage(); // select & connect to storage backends
	else
	  d_quotaneeded=false;

	d_databytes=0;
	if(d_megatalker->isDegraded()) {
	  response="450 Operating in degraded (read-only) mode, try again later";
	  return true;
	}

	for(vector<MboxData>::const_iterator i=d_recipients.begin();
	    i!=d_recipients.end();++i) {
	  if(i->isForward==false && i->mbQuota<=0) {
	    L<<Logger::Warning<<"Mailbox "<<i->canonicalMbox<<" has unlimited quota, no quota for entire message"<<endl;
	    d_quotaneeded=false;
	  }
	  else if(d_quotaneeded) {
	    size_t kbUsage=0;

	    if(determineKbUsage(i->canonicalMbox,&kbUsage, response)) 
	      return true; // error

	    L<<Logger::Warning<<"Mailbox "<<i->canonicalMbox<<" has "<<kbUsage<<" kbytes in use, "
	     <<i->mbQuota*1000-kbUsage<<" available"<<endl;
	    
	    d_kbroom=max((signed int)(i->mbQuota*1000-kbUsage), d_kbroom);
	  }
	  d_megatalker->addRecipient(i->canonicalMbox, d_index);
	}

	d_megatalker->startData(); 
	if(d_megatalker->isDegraded()) {
	  response="450 Operating in degraded (read-only) mode, try again later";
	  return true;
	} else {
	  response="354 Enter message, ending with '.' on a line by itself";
	  if(d_quotaneeded) {
	    if(d_kbroom<=0) {
	      response="552 Recipient(s) over quota, no space available";
	      L<<Logger::Error<<"No space available for <"<<d_index<<">"<<endl;
	      return true;
	    }
	    if(d_esmtpkbsize && (signed int)d_esmtpkbsize>d_kbroom) {
	      response="552 A message of that size ("+itoa(d_esmtpkbsize)+"kbytes) exceeds remaining quota ("+itoa(d_kbroom)+"kb)";
	      L<<Logger::Error<<"Message <"<<d_index<<"> has ESMTP size that exceeds quota ("<<d_kbroom<<"kb)"<<endl;
	      return true;
	    }
	    if(d_esmtpkbsize && args().paramAsNum("max-msg-kb") && (signed int)d_esmtpkbsize>args().paramAsNum("max-msg-kb")) {
	      response="552 A message of "+itoa(d_esmtpkbsize)+"kbytes exceeds maximum message size ("
					  +args().paramString("max-msg-kb")+"kb)";

	      L<<Logger::Error<<"Message <"<<d_index<<"> has ESMTP size that exceeds max message size ("
	       <<args().paramAsNum("max-msg-kb")<<"kb)"<<endl;
	      return true;
	    }
	    L<<Logger::Warning<<"Effective quota for <"<<d_index<<">: "<<d_kbroom<<" kbytes"<<endl;
	    response+=". Quota available: "+itoa(d_kbroom)+" kilobytes";
	  }
	  d_indata=true;
	  d_inheaders=true;
	  d_multipart=false;
	  d_boundary="";
	  sendReceivedLine();
	}
      }
      catch(MegaTalkerException &e) {
	response="450 Backend error: "+e.getReason();
      }
    }
    return true;
  } 
  return false; // not DATA
}

void SmtpSession::sendReceivedLine()
{

  /* Received: from exit.powerdns.com (exit.powerdns.com [213.244.168.242])
      by outpost.powerdns.com (Postfix) with ESMTP id 0AE2CC6469
      for <ahu@ds9a.nl>; Wed, 21 Nov 2001 17:32:44 +0100 (CET)
  */

  string line="Received: from "+d_remote+" by "+getHostname()+" (PowerMail) with id <"+d_index+">\r\n";
  d_megatalker->msgAddLine(line);
}

bool SmtpSession::eatDataLine(const string &line,string &response) 
{
  if(!(line[0]=='.' && (line[1]=='\r' || line[1]=='\n'))) {
    // give to pptalker
    if(!d_quotaneeded || (signed int)(d_databytes/1000)<d_kbroom)
      d_megatalker->msgAddLine(line);

    if(d_inheaders && !line.find("Content-Type: multipart/mixed")) {
      // Content-Type: multipart/mixed; boundary="45Z9DzgjV8m4Oswq"
      unsigned int pos=line.find("boundary=\"");
      if(pos!=string::npos) {
	d_boundary=line.substr(pos+10);

	pos=d_boundary.find("\"");
	if(pos!=string::npos)
	  d_boundary=d_boundary.substr(0,pos);
      }
      
      d_multipart=true;
    }

    if(d_inheaders && (line[0]=='\n' || line[1]=='\n')) {
      if(!d_from.find("ahu@") && !args().paramString("advertisement").empty()) {
	if(d_multipart)
	  d_megatalker->msgAddLine("--"+d_boundary+"\r\nContent-Type: text/plain; charset=us-ascii\r\n"
				  "Content-Disposition: inline\r\n\r\n");

	d_megatalker->msgAddLine(args().paramString("advertisement")+"\r\n\r\n");
      }

      d_inheaders=false;
    }
			       

    d_databytes+=line.size();
    return false;
  }

  d_indata=false;
  response="450 Temporary failure";
  try {
    if(d_quotaneeded && (signed int)(d_databytes/1000)>d_kbroom) {
      L<<Logger::Warning<<"Message "<<d_index<<" larger than quota ("<<d_databytes/1000<<"kb > "<<d_kbroom<<"kb), dropped"<<endl;
      response="552 Message of "+itoa(d_databytes/1000)+" kilobytes exceeded remaining quota ("+itoa(d_kbroom)+" kilobytes)";
      d_megatalker->msgRollback();
      rsetSession(); 
      return true;
    }
    if(args().paramAsNum("max-msg-kb") && (signed int)(d_databytes/1000)>args().paramAsNum("max-msg-kb")) {
      L<<Logger::Warning<<"Message "<<d_index<<" larger than maximum message size ("<<d_databytes/1000<<"kb > "
       <<args().paramAsNum("max-msg-kb")<<"kb), dropped"<<endl;
      response="552 Message of "+itoa(d_databytes/1000)+" kilobytes exceeded maximum message size ("+
	args().paramString("max-msg-kb")+" kilobytes), not delivered";
      d_megatalker->msgRollback();
      rsetSession(); 
      return true;
    }

    d_megatalker->msgDone();

    if(d_megatalker->isDegraded()) {
      L<<Logger::Warning<<"Failed to deliver message <"<<d_index<<"> redundantly to all recipients"<<endl;
      response="450 Message was not delivered redundantly, going to readonly mode. Try again later";
      d_megatalker->msgRollback();
      rsetSession(); 
      return true;
    }
    rsetSession();    
  }
  catch(MegaTalkerException &e) {
    response="450 Temporary backend error: "+e.getReason();
    return true;
  }
    
  response="250 Delivered message <"+d_index+"> successfully to all recipients"; // if ok
  L<<Logger::Warning<<"Delivered message <"<<d_index<<"> successfully to all recipients"<<endl;
  return true;
}

bool SmtpSession::topCommands(const string &line, string &response, quit_t &quit)
{
  if(line=="HELP") {
    response="214 EHLO HELP HELO RSET MAIL RCPT DATA NOOP QUIT";
    return true;
  }
  if(line=="NOOP") {
    response="250 done";
    return true;
  }
  if(line=="QUIT") {
    response="221 bye";
    quit=pleaseQuit;
    return true;
  }

  vector<string>words;
  stringtok(words,line);

  if(words.size()>1 && words[0]=="HELO") {
    L<<Logger::Warning<<"HELO '"<<line.substr(5)<<"' from "<<d_remote<<endl;
    response="250 Hi "+words[1];
    return true;
  }
  if(words.size()>1 && words[0]=="EHLO") {
    L<<Logger::Warning<<"EHLO '"<<line.substr(5)<<"' from "<<d_remote<<endl;
    response="250-"+getHostname()+"\r\n";
    response+="250-SIZE";
    if(args().paramAsNum("max-msg-kb")) {
      response+=" "+args().paramString("max-msg-kb")+"000";
    }
    response+="\r\n250-PIPELINING\r\n250 HELP";
    return true;
  }
  if(!line.find("RSET")) {
    return rsetCommand(response);;
  }

 	
  return false;
}

string SmtpSession::extractAddress(const string &line)
{
  string ret;
  bool ltseen=false;
  string::const_iterator i=line.begin();

  while(i!=line.end() && isspace(*i)) 
    ++i;

  if(*i=='<') {
    ltseen=true;
    ++i;
  }

  for(;i!=line.end();++i) {
    if(*i=='>' || (!ltseen && isspace(*i)))
      return ret;
    
    ret+=*i;
  }
  return ret;
}

bool SmtpSession::eatFrom(const string &line, string &response)
{
  if(!line.find("MAIL FROM:")) { // MAIL FROM:<adres> SIZE=213123 
    if(!d_fromset) {
      d_index=makeIndex();
      d_from=extractAddress(line.substr(10));
      d_fromset=true;
      d_quotaneeded=true;
      d_kbroom=0;
      d_esmtpkbsize=0;

      response="250 Started message <"+d_index+"> for sender <"+d_from+">";
      L<<Logger::Warning<<"Started message <"<<d_index<<"> for sender <"<<d_from<<">"<<endl;

      vector<string>words;
      stringtok(words,line);
      if(!words.empty()) {
	string lastword=words[words.size()-1];
	if(!(lastword.find("SIZE="))) {
	  d_esmtpkbsize=atoi(lastword.substr(5).c_str())/1024;
	  response+=" with ESMTP size of "+itoa(d_esmtpkbsize)+" kbytes";
	}
      }
      

    } 
    else
      response="503 Sender already given";
      
    return true;
  }
  return false;
}

int SmtpSession::determineKbUsage(const string &mbox, size_t *usage, string &response)
{
  *usage=0;
  MegaTalker::mboxInfo_t data;
  try {
    if(!d_megatalker) {
      L<<Logger::Error<<"megatalker zero"<<endl;
      exit(1);
    }
    d_megatalker->getMsgs(mbox, data);
  }
  catch(MegaTalkerException &e) {
    response="450 Unable to determine quota: "+e.getReason();
    return -1;
  }
  for(MegaTalker::mboxInfo_t::const_iterator i=data.begin();
      i!=data.end();
      ++i) {
    *usage+=max((unsigned int)4,(i->size/1024)); // penalize small files
  }

  return 0;
}


bool SmtpSession::eatTo(const string &line, string &response)
{
  if(!line.find("RCPT TO:")) {
    if(!d_fromset) {
      response="503 First specify sender";
    }
    else {
      // check existence here, retrieve quota, store all these

      string d_to=extractAddress(line.substr(8));

      MboxData md;
      int res;
      //      L<<Logger::Error<<"Starting lookup for destination '"<<d_to<<"'"<<endl;

      // semantics - check for a direct hit first, and see that it is
      // then check for a *@variant

      string tries[2];
      tries[0]=d_to;
      tries[1]="*";
      unsigned int offset=d_to.find('@');
      if(offset!=string::npos) {
	tries[1]+=d_to.substr(offset);
      }
      
      bool exists=false,pwcorrect;
      const string *dest;
      for(dest=tries;dest!=&tries[2];++dest) {
	PoolClass<UserBase>::handle ubh=UBP->get();
	res=ubh.d_thing->mboxData(*dest,md,"",response,exists,pwcorrect);

	if(res<0) { // temporary
	  response="450 "+response;

	  L<<Logger::Error<<"Temporary database error for message <"<<d_index<<">"<<endl;
	  return true; // temporary failure -> quit even trying
	}

	if(exists)
	  break;
      }
      if(exists && *dest!=d_to)
	L<<Logger::Warning<<"Fallback match of '"<<d_to<<"' as '"<<*dest<<"'"<<endl;

      if(!exists) {
	response="553 "+response;
	L<<Logger::Warning<<"No such account in message <"<<d_index<<"> to <"<<d_to<<">"<<endl;
	return true;
      }

      // check here if this is a real mbox or a forward
      if(!md.isForward) { // real 
	d_recipients.push_back(md);
	response="250 Added recipient '"+d_to+"' to message <"+d_index+">";
      } else {  
	try {
	  if(!d_megatalker)
	    d_megatalker=new MegaTalker;

	  d_megatalker->addRemote(d_from,md.fwdDest,d_index);
	  response="250 Will forward message <"+d_index+"> to <"+md.fwdDest+">";
	  L<<"Forwarding <"<<d_index<<"> from <"<<d_from<<"> originally for <"<<d_to<<"> to <"<<md.fwdDest<<">"<<endl;
	}
	catch(MegaTalkerException &e) {
	  response="450 Not able to forward to <"+md.fwdDest+">: "+e.getReason();
	  return true;
	}
      }
    }
    return true;
  } 

  return false;
}

void SmtpSession::strip(string &line)
{
  unsigned int pos=line.find_first_of("\r\n");
  if(pos!=string::npos) {
    line.resize(pos);
  }
  else
    L<<Logger::Error<<"UNTERMINATED LINE SPOTTED! "<<line.size()<<" bytes long"<<endl;
}

//! Uppercase the first word of a line, or up to a possible colon
void SmtpSession::firstWordUC(string &line)
{
  bool hasColon=line.find(":")!=string::npos;

  for(string::iterator i=line.begin();i!=line.end();++i) {
    if((*i==':') || (!hasColon && isspace(*i)))
      return;
    *i=toupper(*i);
  }
}
    
bool SmtpSession::controlCommands(const string &line, string &response, quit_t &quit)
{
  response="450 Unknown error processing control command";
  vector<string> words;
  stringtok(words,line);
  if(words.empty())
    return false;

  if(words[0]!="CONTROL")
    return false;

  if(d_remote.find("127.0.0.1:") && d_remote.find(args().paramString("listen-address")+":")) {
    L<<Logger::Error<<"Remote "<<d_remote<<" tried a control command: "<<line<<endl;
    return false;
  }
    
  if(words.size()<2) {
    response="500 Missing control command";
    return true;
  }
  else if(words[1]=="stop") {
    response="200 Bye!";
    quit=pleaseDie;
  }
  else if(words[1]=="status") {
    response="200 OK!";
  }
  else {
    response="500 Unknown control command '"+words[1]+"'";
  }
  return true;
}

bool SmtpSession::giveLine(string &line, string &response, quit_t &quit)
{
  quit=noQuit;

  if(d_indata) 
    return eatDataLine(line, response);

  strip(line);
  string origline(line);

  firstWordUC(line);
  if(controlCommands(line,response,quit)) // stop, start, status 
    return true;

  if(topCommands(line, response, quit)) // RSET, HELP, QUIT
    return true;

  if(eatFrom(line,response))
    return true;

  if(eatTo(line,response))
    return true;

  if(eatData(line,response))
    return true;
  
  response="502 Not Implemented";
  L<<"Remote "<<d_remote<<" sent unparsed line: '"<<origline<<"'"<<endl;

  return true;
}

string SmtpSession::getBanner()
{
  return "220 "+getHostname()+" PowerSMTP "VERSION" ESMTP\r\n";
}
