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
#include "imapsession.hh"
#include "userbase.hh"
#include "delivery.hh"
#include "logger.hh"
#include "misc.hh"
#include <sys/types.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sstream>
#include <ctype.h>
#include "argsettings.hh"
#include "pool.hh"
#include "common.hh"

extern PoolClass<UserBase>* UBP;
extern Logger L;

// UIDL, TOP 

void IMAPSession::strip(string &line)
{
  unsigned int pos=line.find_first_of("\r\n");
  if(pos!=string::npos) {
    line.resize(pos);
  }

}

IMAPSession::~IMAPSession()
{
  if(d_megatalker)
    delete d_megatalker;
}

void IMAPSession::firstWordUC(string &line)
{
  for(string::iterator i=line.begin();i!=line.end();++i) {
    if(*i==' ')
      return;
    *i=toupper(*i);
  }
}

int IMAPSession::delDeleted(string &response)
{
  try {
    for(msgData_t::const_iterator i=d_msgData.begin();i!=d_msgData.end();++i) {
      if(i->d_deleted) {
	d_megatalker->delMsg(i->d_fname);
	d_deleted++;
      }
    }
  }
  catch(MegaTalkerException &e) {
    response="-ERR "+e.getReason();
    return -1;
  }

  return 0;
}

bool IMAPSession::topCommands(const string &line, string &response, quit_t &quit)
{
  if(line=="NOOP") {
    response="+OK Done";
    return true;
  }
  if(line=="QUIT") {
    if(delDeleted(response))
      return true; 

    response="+OK ";
    if(d_login)
      L<<Logger::Error<<"Mailbox '"<<d_user<<"' logging out, retrieved: "<<d_retrieved<<", deleted: "<<d_deleted<<endl;
    quit=pleaseQuit;
    return 1;
  }
  
  if(line=="RSET") {
    for(msgData_t::iterator i=d_msgData.begin();i!=d_msgData.end();++i) {
      i->d_deleted=false;
    }
    response="+OK ";
    return true;
  }

  return false;
}

bool IMAPSession::unauthCommands(const string &command, const string &line, string &response, quit_t &quit)
{
  cout<<"got command '"<<command<<"'"<<endl;
  if(command=="CAPABILITY") {
    d_session->putLine("* CAPABILITY IMAP4rev1\r\n");
    /*
      C: A001 CAPABILITY
      S: * CAPABILITY IMAP4rev1 CHILDREN NAMESPACE THREAD=ORDEREDSUBJECT THREAD=REFERENCES SORT
      S: A001 OK CAPABILITY completed
    */
    response="OK CAPABILITY completed";
    return true;
  }
  else if(command=="LOGOUT") {
    d_session->putLine("* BYE PowerImap shutting down\r\n");
    /*
      C: A001 CAPABILITY
      S: * CAPABILITY IMAP4rev1 CHILDREN NAMESPACE THREAD=ORDEREDSUBJECT THREAD=REFERENCES SORT
      S: A001 OK CAPABILITY completed
    */
    response="OK LOGOUT done";
    quit=pleaseQuit;
    return true;
  }
  else if(command=="LOGIN") {
    if(d_login) {
      response="NO Already logged in";
      return true;
    }

    vector<string>parts;
    stringtok(parts,line," \"");
    if(parts.size()!=3) {
      response="Malformed LOGIN line";
      return true;
    }
    d_user=parts[1];
    string d_pass=parts[2];

    string::size_type pos=d_user.find("%");
    if(pos!=string::npos)
      d_user[pos]='@';

    bool exists=false, pwcorrect=false;
    MboxData md;
    int res;

    string tries[2];
    tries[0]=d_user;
    tries[1]="*";
    unsigned int offset=d_user.find('@');
    if(offset!=string::npos) {
      tries[1]+=d_user.substr(offset);
    }
    else
      tries[1]="*@"+d_user;

    string *dest;
    for(dest=tries;dest!=&tries[2];++dest) {
      PoolClass<UserBase>::handle ubh=UBP->get();
      res=ubh.d_thing->mboxData(*dest,md,d_pass, response, exists, pwcorrect); // sets response!
      if(res<0) {
	response="NO "+response;
	sleep(1); // thwart dictionary attacks
	return 1;
      }
      if(exists) {
	d_user=md.canonicalMbox; // we may have been renamed by the userbase!
	break;
      }
    }

    if(exists && *dest!=d_user) {
      L<<Logger::Warning<<"Fallback match of '"<<d_user<<"' as '"<<*dest<<"'"<<endl;
    }

    response="NO Invalid credentials (wrong username or password?)";
    if(exists && pwcorrect) {
      L<<Logger::Warning<<"Mailbox '"<<d_user<<"' logged in"<<endl;
      d_login=true;
      
      try {
	d_megatalker=0;
	d_megatalker=new MegaTalker;
      }
      catch(MegaTalkerException &e) {
	L<<"Fatal error connecting to MegaTalker: "<<e.getReason()<<endl;
	response="NO Error connecting to storage backends.";
	return 1;
      }
      if(scanDir(response))
	return false;

      response="OK LOGIN Ok.";
    }
    else if(exists && !pwcorrect) {
      L<<Logger::Warning<<"Mailbox '"<<d_user<<"' exists but tried wrong password"<<endl;
    }
    else
      L<<Logger::Warning<<"No such mailbox '"<<d_user<<"'"<<endl;
    return 1;
  }
  return 0;
}

bool IMAPSession::unSelectedCommands(const string &command, const string &line, string &response, quit_t &quit)
{
  if(!d_login)
    return false;

  if(command=="SELECT" || command=="EXAMINE") {
    vector<string>parts;
    stringtok(parts,line," \"");
    if(parts.size()!=2) {
      response="NO Malformed SELECT line";
      return true;
    }    
    if(parts[1]!="INBOX") {
      response="NO Invalid mailbox";
      return true;
    }

    if(scanDir(response))
      return true;

    ostringstream selstr;
    selstr<<"* FLAGS (\\Deleted)\r\n* OK [PERMANENTFLAGS (\\Deleted)] Limited\r\n* "<<d_msgData.size()<<" EXISTS\r\n";
    selstr<<"* 0 RECENT\r\n* OK [UIDVALIDITY "<<time(0)<<"] Ok\r\n";
    d_session->putLine(selstr.str());

    response="OK [READ-WRITE] Ok";

    if(command=="SELECT") 
      response="OK [READ-WRITE] Ok";
    else
      response="OK [READ-ONLY] Ok"; // should enforce this btw

    return true;
  }
  return false;
}
int IMAPSession::scanDir(string &response)
{
  MegaTalker::mboxInfo_t data;
  try {
    d_megatalker->getMsgs(d_user, data);
  }
  catch(MegaTalkerException &e) {
    response="NO "+e.getReason();
    return true;
  }
  
  MsgDatum md;

  for(MegaTalker::mboxInfo_t::const_iterator i=data.begin();
      i!=data.end();
      ++i) {
    md.d_size=i->size;
    md.d_fname=i->index;
    md.d_deleted=false;
    d_msgData.push_back(md);
  }
  d_scanned=1;
  return false;
}    

bool IMAPSession::controlCommands(const string &line, string &response, quit_t &quit)
{
  response="-ERR Unknown error processing control command";
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
    response="-ERR Missing control command";
    return true;
  }

  else if(words[1]=="stop") {
    response="+OK BYE!";
    quit=pleaseDie;
  }
  else if(words[1]=="status") {
    response="+OK OK!";
  }
  else {
    response="-ERR Unknown control command '"+words[1]+"'";
  }
  return true;
}


bool IMAPSession::giveLine(string &line, string &response, quit_t &quit, int &fd)
{
  quit=noQuit;
  fd=0;
  strip(line);

  vector<string>parts;
  stringtok(parts,line);
  string tag="*";
  if(!parts.empty())
    tag=parts[0];

  if(parts.size()<2) {
    response=tag+" NO Error in IMAP command received by server.";
    return true;
  }
  
  string::size_type cmdOffset=parts[0].length();
  for(;cmdOffset<line.length();++cmdOffset) 
    if(!isspace(line[cmdOffset]))
      break;
  
  string command=line.substr(cmdOffset);
  string firstW=command.substr(0,command.find_first_of(" \t"));
  cout<<"command: '"<<command<<"', tag: '"<<tag<<"'"<<endl;

  if(unauthCommands(firstW,command,response,quit) || unSelectedCommands(firstW,command,response,quit)) {
    response=tag+" "+response;
    return true;
  }
  
  response=tag+" Not Implemented or Not Authorized Yet";
  L<<Logger::Error<<"Unable to parse or process line '"<<line<<"'"<<endl;
  return true;
}

string IMAPSession::getBanner()
{
  return string("* OK PowerImap ready. Copyright 2002 PowerDNS.COM BV.")+"\r\n";
}
