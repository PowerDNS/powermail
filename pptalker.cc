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
#include "pptalker.hh"
#include "session.hh"
#include "logger.hh"
#include "ahuexception.hh"
#include <sstream>
#include "misc.hh"

extern Logger L;

PPTalker::PPTalker(const string &dest, u_int16_t port, const string &password, int timeout) : d_inmsg(0)
{
  d_session=0;
  d_usage=0;
  try {
    d_ppname=dest+":"+itoa(port);
    d_session=new Session(dest, port, timeout);
    d_session->beVerbose();
    string line;
    d_session->getLine(line);

    if(line.find("+OK")) {
      d_session->close();
      delete d_session;
      throw PPTalkerException("Unable to connect: "+line);
    }

    d_session->putLine("pass "+password+"\n");
    d_session->getLine(line);
    if(line.find("+OK")) {
      d_session->close();
      delete d_session;
      throw PPTalkerException("Unable to authenticate with password '"+password+"': "+line);
    }

  }
  catch (SessionException &e){
    if(d_session) {
      d_session->close();
      delete d_session;
    }
    throw PPTalkerException(e.d_reason);
  }
}

PPTalker::~PPTalker()
{
  if(d_session) {
    d_session->putLine("quit\n");
    d_session->close();
    delete d_session;
    d_session=0;
  }
}

int PPTalker::addRecipient(const string &recip, const string &index)
{
  string line;
  try {
    d_session->putLine("rcpt "+recip+" "+index+"\n");
    d_session->getLine(line);
    
    if(line.find("+OK")) 
      throw PPTalkerException("Unable to add recipient: '"+recip+"' "+line);
  }
  catch(AhuException &e) {
    throw PPTalkerException("Session Error: "+e.d_reason);
  }

  return 0;
}

int PPTalker::delMsg(const string &recip, const string &index)
{
  string line;
  try {
    d_session->putLine("del "+recip+" "+index+"\n");
    d_session->getLine(line);
    if(line.find("+OK"))
      throw PPTalkerException("Unable to delete '"+index+"' from mailbox '"+
			      recip+"'");
  }
  catch(AhuException &e) {
    throw PPTalkerException("Session Error: "+e.d_reason);
  }

  return 0;
}

void PPTalker::listMboxes(vector<string> &mboxes)
{
  string line;
  try {
    d_session->putLine("dir\n");
    d_session->getLine(line);
    
    if(line.find("+OK"))
      throw PPTalkerException("Unable to request listing of mboxes: "+line);
    
    for(;;){
      d_session->getLine(line);
      stripLine(line);
      if(line==".")
	break;
      mboxes.push_back(line);
    }
  }
  catch(AhuException &e) {
    throw PPTalkerException("Session Error listing mboxes: "+e.d_reason);
  }
}

int PPTalker::nukeMbox(const string &mbox)
{
  string line;
  try {
    d_session->putLine("nuke "+mbox+"\n");
    d_session->getLine(line);
    if(line.find("+OK"))
      throw PPTalkerException("Unable to purge mailbox '"+mbox+"': "+line);
  }
  catch(AhuException &e) {
    throw PPTalkerException("Session Error nuking mailbox: "+e.d_reason);
  }

  return 0;
}


void PPTalker::startData()
{
  string line;
  try {
    d_session->putLine("put\n");
    d_session->getLine(line);
  }
  catch(SessionException &e) {
    throw PPTalkerException("Session Error adding a line: "+e.d_reason);
  }
   
  if(line.find("+OK")) 
    throw PPTalkerException("Unable start sending message data: "+line);
  d_inmsg=true;
}

int PPTalker::msgAddLine(const string &line)
{
  if(!d_inmsg++) 
    startData();
  
  try {
    d_session->putLine(line);
  }
  catch(AhuException &e) {
    throw PPTalkerException("Session Error adding a line: "+e.d_reason);
  }
  
  return 0;
}

void PPTalker::setReadonly(bool flag)
{
  try {
    string line;
    d_session->putLine(flag ? "readonly\n" : "readwrite\n");
    d_session->getLine(line);
    if(line.find("+OK")) 
	throw PPTalkerException("Unable to set readonly state: "+line);
  
  }  
  catch(SessionException &e) {
    throw PPTalkerException("Session Error during readonly change: "+e.d_reason);
  }
}

void PPTalker::getMsgs(const string &mbox, mboxInfo_t& mbi)
{
  try {
    string line;
    d_session->putLine("ls "+mbox+"\n");
    d_session->getLine(line);
    if(line.find("+OK")) {
	d_usage--;
	throw PPTalkerException("Unable to list mailbox '"+mbox+"': "+line);
    }
    
    d_session->getLine(line);
    while(line!=".\n") {
      msgInfo_t msgi;
      istringstream i(line);
      i>>msgi.index;
      i>>msgi.size;
      msgi.ppname=d_ppname;
      
      mbi.push_back(msgi);

      d_session->getLine(line);
      // push back into mbi
    }
    d_usage--;
  }
  catch(SessionException &e) {
    throw PPTalkerException("Session Error adding a line: "+e.d_reason);
  }
}

void PPTalker::blastMessageFD(const string &address, const string &index, size_t length, int fd, int limit)
{
  try {
    string line;
    d_session->putLine("get "+address+" "+index+"\n");
    d_session->getLine(line);
    if(line.find("+OK"))
      throw PPTalkerException("Unable to retrieve message "+address+" "+index);
  
    if(limit==-1)
      line="+OK "+itoa(length)+"\r\n";
    else
      line="+OK\r\n";

    write(fd,line.c_str(),line.size());
    bool inheaders=true;
    int lineno=0;

    for(;;) {
      d_session->getLine(line);
      
      if(limit>=0) {
	if(inheaders && (line[0]=='\r' || line[1]=='\n'))
	  inheaders=false;
	else if(!inheaders)
	  lineno++;
      }
	

      if(line[0]=='.' && (line[1]=='\r' || line[1]=='\n')) {
	write(fd, ".\r\n",3);
	break;
      }

      if(limit<0 || lineno<=limit)
	write(fd, line.c_str(),line.length());
      
    }
  }
  catch(SessionException &e) {
    throw PPTalkerException("Session Error retrieving message: "+e.d_reason);
  }
}

void PPTalker::stat(int *kbytes, int *inodes,double *load)
{
  try {
    d_session->putLine("stat\n");
    string line;
    d_session->getLine(line);
    if(line.find("+OK"))
      throw PPTalkerException("Remote indicated error accepting ping: "+line);

    //+OK 122669 kbytes 1096077 inodes 1.13 load

    vector<string>words;
    stringtok(words,line);
    if(words.size()<3)
      throw PPTalkerException("Invalid response to stat request");

    *kbytes=atoi(words[1].c_str());
    if(inodes && words.size()>4)
      *inodes=atoi(words[3].c_str());
    if(load && words.size()>6)
      *load=strtod(words[5].c_str(),0);
  }
  catch(SessionException &e) {
    throw PPTalkerException("Session Error doing ping: "+e.d_reason);
  }
}

void PPTalker::ping(bool &readonly)
{
  try {
    if(!d_session)
      throw PPTalkerException("Unable to ping, no session");

    d_session->putLine("ping\n");
    string line;
    d_session->getLine(line);
    if(line.find("+OK"))
      throw PPTalkerException("Remote indicated error accepting ping: "+line);

    readonly=(line.find("READONLY")!=string::npos);
  }
  catch(SessionException &e) {
    throw PPTalkerException("Session Error doing ping: "+e.d_reason);
  }
}


int PPTalker::msgDone()
{
  try {
    d_session->putLine(".\n");
    string line;
    d_session->getLine(line);
    if(line.find("+OK"))
      throw PPTalkerException("Remote indicated error accepting message: "+line);

    d_session->putLine("commit\n");
    
    d_session->getLine(line);
    if(line.find("+OK"))
      throw PPTalkerException("Remote indicated error accepting committing message: "+line);

  }
  catch(AhuException &e) {
    throw PPTalkerException("Session Error adding a line: "+e.d_reason);
  }
  
  return 0;
}

int PPTalker::msgRollback()
{
  try {
    d_session->putLine(".\n");
    string line;
    d_session->getLine(line);
    if(line.find("+OK"))
      return 0; // killed anyhow :-)

    d_session->putLine("rollback\n");
    
    d_session->getLine(line);
    line.find("+OK");
  }
  catch(AhuException &e) {
  }
  
  return 0;
}


#ifdef DRIVER

void print(const string &s)
{
  cout<<s<<endl;
}

int main()
{
  L.toConsole(true);
  try {
    PPTalker PP(0,1100);

    
    PP.addRecipient("ahu@ds9a.nl",itoa(time(0)));
    PP.addRecipient("dave@powerdns.com",itoa(time(0)*2));

    PP.msgAddLine("From: ahu@ds9a.nl");
    PP.msgAddLine("To: ahu@ds9a.nl");
    PP.msgAddLine("");
    PP.msgAddLine("Hallo, hallo!");
    PP.msgAddLine("Regel 2");
    PP.msgDone();
    PPTalker::mboxInfo_t data;
    PP.getMsgs("ahu@ds9a.nl", data);
    PPTalker::mboxInfo_t::const_iterator i;
    for(i=data.begin();
	i!=data.end();
	++i)
	cout<<"'"<<i->index<<"', "<<i->size<<endl;

    --i;
    PP.blastMessageFD("ahu@ds9a.nl", i->index,0);

    
  }
  catch(PPTalkerException &e) {
    L<<"Fatal error: "<<e.getReason()<<endl;
  }
    
  
}
#endif
