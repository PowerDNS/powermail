/*
    PowerMail versatile mail receiver
    Copyright (C) 2002-2007  PowerDNS.COM BV

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
#include "smtptalker.hh"
#include "session.hh"
#include "logger.hh"
#include "ahuexception.hh"
#include <sstream>
#include "misc.hh"

extern Logger L;

SmtpTalker::SmtpTalker(const string &dest, u_int16_t port, const string &from, int timeout) : d_inmsg(0)
{
  d_session=0;
  try {
    d_session=new Session(dest, port, timeout);
    d_session->beVerbose();
    string line;

    d_session->getLine(line);
    if(line.find("220 ")) {
      d_session->close();
      delete d_session;
      d_session=0;
      throw SmtpTalkerException("Unable to connect: "+line);
    }
    d_session->putLine("HELO "+getHostname()+"\r\n");
    d_session->getLine(line);
    if(line.find("2")) {
      d_session->close();
      delete d_session;
      d_session=0;
      throw SmtpTalkerException("Sent HELO, did not get correct response: "+line);
    }
    d_session->putLine("MAIL From:<"+from+">\r\n");
    d_session->getLine(line);
    if(line.find("2")) {
      d_session->close();
      delete d_session;
      d_session=0;
      throw SmtpTalkerException("Sent HELO, did not get correct response: "+line);
    }
  }
  catch (SessionException &e){
    if(d_session) {
      d_session->close();
      delete d_session;
      d_session=0;
    }
    throw SmtpTalkerException("Session error connecting: "+e.d_reason);
  }
}

SmtpTalker::~SmtpTalker()
{
  if(d_session) {
    d_session->close();
    delete d_session;
    d_session=0;
  }
}

int SmtpTalker::addRecipient(const string &recip, const string &)
{
  if(d_inmsg) 
    throw SmtpTalkerException("SmtpTalker accepts only a single recipient at a time!");
  string line;
  try {
    d_session->putLine("rcpt to:<"+recip+">\r\n");
    d_session->getLine(line);
    
    if(line.find("2")) 
      throw SmtpTalkerException("Unable to add recipient: '"+recip+"' "+line);
    startData();
  }
  catch(SessionException &e) {
    throw SmtpTalkerException("Session Error: "+e.d_reason);
  }

  return 0;
}

void SmtpTalker::startData()
{
  if(d_inmsg)
    return;

  string line;
  try {
    d_session->putLine("DATA\r\n");
    d_session->getLine(line);
  }
  catch(SessionException &e) {
    throw SmtpTalkerException("Session Error adding a line: "+e.d_reason);
  }
   
  if(line.find("3")) 
    throw SmtpTalkerException("Unable start sending message data: "+line);
  d_inmsg++;
}

int SmtpTalker::msgAddLine(const string &line)
{
  if(!d_inmsg++) 
    startData();
  
  try {
    d_session->putLine(line);
  }
  catch(SessionException &e) {
    throw SmtpTalkerException("Session Error adding a line: "+e.d_reason);
  }
  
  return 0;
}

int SmtpTalker::msgDone()
{
  try {
    d_session->putLine(".\r\n");
    string line;
    d_session->getLine(line);
    if(line.find("2")) {
      stripLine(line);
      throw SmtpTalkerException("Remote indicated error accepting message: "+line);
    }

  }
  catch(SessionException &e) {
    throw SmtpTalkerException("Session Error committing message: "+e.d_reason);
  }
  return 0;
}

int SmtpTalker::msgRollback()
{
  try {
    d_session->close();
  }
  catch(AhuException &e) {
  }
  return 0;
}
