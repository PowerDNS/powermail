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
#include "session.hh"
#include "ahuexception.hh"
#include "argsettings.hh"
#include "ppsession.hh"
#include "logger.hh"
#include "misc.hh"
#include "config.h"
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <cstdio>
#include <signal.h>
#include "pplistenerdefaults.h"
#include "common.hh"

extern Logger L;

static void *serveConnection(void *p)
{
  pthread_detach(pthread_self());
  Session* s;
  try {
    s=(Session *)p;
    s->setTimeout(0);
    PPListenSession pp(s->getRemote());
    
    // L<<"New connect from "<<s->getRemote()<<endl;
    
    s->putLine(pp.getBanner());
    
    string line, response;
    int res;
    PPListenSession::quit_t quit;
    int fd;

    for(;;){
      s->getLine(line); // will throw exception on EOF
      bool more=true;
      while(more) {
	res=pp.giveLine(line,response,quit,fd,more);
	if(res)
	  s->putLine(response+"\n");
      }
	    
      
      if(quit==PPListenSession::quitNow)
	break;

      
      if(fd>0) {
	s->sendFile(fd);
	s->putLine(".\n");
	close(fd);
	fd=-1;
      }
      
    if(quit==PPListenSession::pleaseQuit)
      break;
    if(quit==PPListenSession::pleaseDie)
      exit(0);
    
    }
    // L<<"Closed connection from "<<s->getRemote()<<endl;
    s->close();
    delete s;
    s=0;
  }
  catch(SessionTimeoutException &e) {
    L<<Logger::Error<<"Timeout in pplistener";
    if(s)
      L<<Logger::Error<<" from "<<s->getRemote();
    L<<": "<<e.d_reason<<endl;
  }
  catch(SessionException &e) {
    L<<Logger::Error<<"Fatal error";
    if(s)
      L<<Logger::Error<<" from "<<s->getRemote();
    L<<": "<<e.d_reason<<endl;
  }
  catch(AhuException &e) {
    L<<Logger::Error<<"Fatal error";
    if(s)
      L<<Logger::Error<<" from "<<s->getRemote();
    L<<": "<<e.d_reason<<endl;
  }
  if(s) {
    s->close();
    delete s;
  }
  return 0;
}

static void doCommands(const vector<string>&commands)
{
  int ret=1;
  const string &command=commands[0];
  try {

    string line;
    cerr<<"pplistener";
    if(!args().paramString("config-name").empty())
      cerr<<"-"<<args().paramString("config-name");
    cerr<<": ";    

    Session *s;
    try {
      s=new Session(args().paramString("listen-address"),args().paramAsNum("listen-port"));
      s->getLine(line);
      s->putLine("pass "+args().paramString("password")+"\r\n");
      s->getLine(line);
      if(line.find("+OK")) {
	L<<Logger::Error<<"Password not accepted: "<<line<<endl;
	exit(1);
      }
    }
    catch(SessionException &e) {
      if(command=="start") {
	cerr<<"started"<<endl;
	return;
      }
      if(command=="stop") {
	cerr<<"not running"<<endl;
	exit(0);
      }
      cerr<<"Can't connect to PowerSMTP ("<<args().paramString("listen-address")<<":"<<args().paramAsNum("listen-port")<<")";
      cerr<<". Unable to send command"<<endl;
      exit(1);
    }
    if(command=="start") {
      cerr<<"already running"<<endl;
      exit(1);
    }
    else
    if(command=="stop") {
      s->putLine("control stop\r\n");
      s->getLine(line); // why twice?

      if(!line.find("+OK")) {
	cerr<<"stopped pplistener"<<endl;
	ret=0;
      }
      else {
	stripLine(line);
	cerr<<"pplistener did not stop: "<<line<<endl;
      }
      s->close();
      delete s;
    }
    else
    if(command=="status") {
      s->putLine("control status\r\n");
      s->getLine(line);
      if(!line.empty() && !line.find("+OK")) {
	stripLine(line);
	cerr<<"pplistener functioning ok: "<<line<<endl;
	ret=0;
      }
      else {
	stripLine(line);
	cerr<<"pplistener not ok: "<<line<<endl;
	ret=2;
      }
      s->close();
      delete s;
    }
    else {
      L<<Logger::Error<<"Unknown command '"<<command<<"' passed on startup"<<endl;
    }
  }
  catch(SessionException &e) {
    L<<Logger::Error<<"Error sending data to powersmtp: "<<e.d_reason<<endl;
    if(command=="stop")
      ret=0;
  }
  exit(ret);
}

int PPListenerMain(int argc, char **argv)
{
  L.setName("pplistener");
  args().addParameter("config-dir","Location to read configuration files from",SYSCONFDIR);
  args().addParameter("config-name","Name of this virtual configuration","");
  args().addParameter("listen-port","Port on which to listen for new connections","1101");
  args().addParameter("listen-address","Address on which to listen for new connections","0.0.0.0");
#ifdef PPLISTENERUID
  args().addParameter("run-as-uid","User-id to run as, after acquiring socket",PPLISTENERUID);
#else
  args().addParameter("run-as-uid","User-id to run as, after acquiring socket","0");
#endif

#ifdef PPLISTENERGID
  args().addParameter("run-as-gid","Group-id to run as, after acquiring socket",PPLISTENERGID);
#else
  args().addParameter("run-as-gid","Group-id to run as, after acquiring socket","0");
#endif

  args().addParameter("min-kbytes-free","When this many kilobytes are available, we unset the Full flag","5000");
  args().addParameter("password","Password that clients need to send before they can retrieve and store messages","");
  args().addParameter("mail-root","Location of all email messages. Must be readable, writable and executable by the chosen UID & GID!",LOCALSTATEDIR"/messages");
  args().addCommand("help","Display this helpful message");
  args().addCommand("version","Display version information");
  args().addCommand("make-config","Output a configuration file conformant to current settings, which you can also specify on the commandline");
  args().addSwitch("daemon","Run as a daemon",true);
  L.toConsole(Logger::Warning);

  vector<string>commands;
  try {
    string response;
    args().preparseArgs(argc, argv,"config-dir");
    if(args().parseFiles(response, (args().paramString("config-dir")+"/pplistener.conf").c_str(),0))
      L<<Logger::Warning<<"Warning: unable to open a configuration file"<<endl;
    else
      L<<Logger::Info<<"Read configuration from "<<response<<endl;

    args().parseArgs(argc, argv);
    if(args().commandGiven("help")) {
      cerr<<args().makeHelp()<<endl;
      exit(0);
    }
    if(args().commandGiven("version")) {
      cerr<<"pplistener version "<<VERSION<<". This is $Id: pplistener.cc,v 1.3 2002-12-24 20:05:55 ahu Exp $"<<endl;
      exit(0);
    }
    if(args().commandGiven("make-config")) {
      cout<<args().makeConfig()<<endl;
      exit(0);
    }
    args().getRest(argc,argv,commands);
  }
  catch(ArgumentException &e) {
    cerr<<"Unable to parse arguments:"<<endl<<e.txtReason()<<endl;
    exit(0);
  }
  
  if(!commands.empty())
    doCommands(commands);

  try {  // check if pplistener can function
    PPListenSession *pps=new PPListenSession("0.0.0.0");
    delete pps;
  }
  catch(AhuException &e) {
    L<<Logger::Error<<"Unable to launch: "<<e.d_reason<<endl;
    L<<Logger::Error<<"pplistener exiting"<<endl;
    exit(0);
  }
  signal(SIGPIPE,SIG_IGN);
  try {
    Server *s=new Server(args().paramAsNum("listen-port"),args().paramString("listen-address"));
    dropPrivs(args().paramAsNum("uid"),args().paramAsNum("gid"));
    
    if(args().switchSet("daemon")) {
      L<<Logger::Info<<"Going to background"<<endl;
      daemonize();
      L.toConsole(false);
    }
    else
      L.toConsole(true);

    Session *client;
    pthread_t tid;
    while((client=s->accept())) {
      pthread_create(&tid, 0 , &serveConnection, (void *)client);
    }
  }
  catch(SessionTimeoutException &e) {
    cerr<<"Timeout"<<endl;
  }
  catch(SessionException &e) {
    L<<Logger::Error<<"Fatal error in pplistener: "<<e.d_reason<<endl;
  }
  catch(Exception &e) {
    cerr<<e.reason<<endl;
  }
  return 0;
}
