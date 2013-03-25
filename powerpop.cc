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
#include <stdlib.h>
#include "ahuexception.hh"
#include "popsession.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "misc.hh"
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <cstdio>
#include <signal.h>
#include <exception>
#include "userbase.hh"
#include "config.h"
#include "megatalker.hh"
#include "common.hh"

extern Logger L;

static void *serveConnection(void *p)
{
  pthread_detach(pthread_self());
  Session *s=(Session *)p;
  s->setTimeout(1800);
  PopSession pop(s->getSocket(),s->getRemote());
  L<<Logger::Warning<<"Session start for "<<s->getRemote()<<endl;    
  s->putLine(pop.getBanner());

  string line, response;
  int res;
  PopSession::quit_t quit;
  int fd;
  try {
    for(;;) {
      s->getLine(line);
      res=pop.giveLine(line,response,quit,fd);
      
      if(quit==PopSession::quitNow)
	break;

      if(res && !response.empty())
	s->putLine(response+"\r\n");
      
      if(fd>0) {
	s->sendFile(fd);
	s->putLine(".\r\n");
	close(fd);
	fd=-1;
      }
      
      if(quit==PopSession::pleaseQuit)
	break;
      if(quit==PopSession::pleaseDie)
	exit(0);
      
    }
    L<<Logger::Warning<<"Session from "<<s->getRemote()<<" signed off"<<endl;

  }
  catch(SessionTimeoutException &e) {
    L<<Logger::Warning<<"PowerPOP session thread timeout: "<<e.d_reason<<endl;
  }
  catch(SessionException &e) {
    L<<Logger::Error<<"PowerPOP session thread error: "<<e.d_reason<<endl;
  }
  catch(exception &e) {
    L<<Logger::Error<<"STL error: "<<e.what()<<endl;
  }
  catch(...) {
    L<<Logger::Error<<"Unexpected exception in PowerPOP"<<endl;
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
    cerr<<"powerpop";
    if(!args().paramString("config-name").empty())
      cerr<<"-"<<args().paramString("config-name");
    cerr<<": ";    

    Session *s;
    try {
      s=new Session(args().paramString("listen-address"),args().paramAsNum("listen-port"));
      s->getLine(line);

    }
    catch(SessionException &e) {
      if(command=="start") {
	cerr<<"started"<<endl;
	return; // means 'go'
      }
      if(command=="stop") {
	cerr<<"not running"<<endl;
	exit(0);
      }

      cerr<<"PowerPOP not running: can't connect to "<<args().paramString("listen-address")<<":"<<args().paramAsNum("listen-port")<<": "<<
	e.d_reason;
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
      
      s->getLine(line);
      if (!line.find("+OK ")) {
	cerr<<"stopped powerpop"<<endl;
	ret=0;
      }
      else {
	stripLine(line);
	cerr<<"powerpop did not stop: "<<line<<endl;
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
	cerr<<"PowerPOP functioning ok: "<<line<<endl;
	ret=0;
      }
      else {
	stripLine(line);
	cerr<<"PowerPOP not ok: "<<line<<endl;
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


void daemonize(void)
{
  if(fork())
    exit(1); // bye bye


  setsid(); 
  if(!chdir("/"))  // silences warning, if this fails we are toast anyhow
    umask(0);

  //  for(int i=0;i<sysconf(_SC_OPEN_MAX);i++)
  //  close(i);
}




int PowerPopMain(int argc, char **argv)
{
  L.setName("powerpop");
  args().addParameter("config-dir","Location to read configuration files from",SYSCONFDIR);
  args().addParameter("config-name","Name of this virtual configuration","");

  args().addParameter("listen-port","Port on which to listen for new connections","110");
  args().addParameter("listen-address","Address on which to listen for new connections","0.0.0.0");
  args().addParameter("run-as-uid","User-id to run as, after acquiring socket","0");
  args().addParameter("run-as-gid","Group-id to run as, after acquiring socket","0");

  UserBaseArguments();

  args().addParameter("pplistener-password","Password for the pptalker to connect to the pplisteners","");

  args().addParameter("backends","Comma or space separated list of PPListener backends","1:1:127.0.0.1");
  args().addParameter("state-dir","Directory to store queued operations",LOCALSTATEDIR"/state");
  args().addParameter("loglevel","Amount of logging to do - 9 is highest","3");

  args().addCommand("help","Display this helpful message");
  args().addCommand("version","Display version information");
  args().addCommand("make-config","Output a configuration file conformant to current settings, which you can also specify on the commandline");
  args().addSwitch("daemon","Run as a daemon",true);

  L.toConsole(Logger::Warning);
  signal(SIGPIPE,SIG_IGN);
  vector<string>commands;
  try {
    string response;

    args().preparseArgs(argc, argv,"help");
    if(args().commandGiven("help")) {
      cerr<<args().makeHelp()<<endl;
      exit(0);
    }
    args().preparseArgs(argc, argv,"loglevel");
    L.toConsole((Logger::Urgency)(args().paramAsNum("loglevel")));  

    args().preparseArgs(argc, argv,"make-config");
    if(args().commandGiven("make-config")) {
      cout<<args().makeConfig()<<endl;
      exit(0);
    }
    args().preparseArgs(argc, argv,"version");
    if(args().commandGiven("version")) {
      cerr<<"powerpop version "<<VERSION<<". This is $Id: powerpop.cc,v 1.5 2009-05-28 20:12:27 ahu Exp $"<<endl;
      exit(0);
    }

    args().preparseArgs(argc, argv,"config-dir");
    if(args().parseFiles(response, (args().paramString("config-dir")+"/power.conf").c_str(),0))
      L<<Logger::Error<<"Warning: unable to open common configuration file"<<endl;
    else
      L<<Logger::Notice<<"Read common configuration from "<<response<<endl;

    if(args().parseFiles(response, (args().paramString("config-dir")+"/powerpop.conf").c_str(),0))
      L<<Logger::Error<<"Warning: unable to open powerpop configuration file"<<endl;
    else
      L<<Logger::Notice<<"Read configuration from "<<response<<endl;

    args().parseArgs(argc, argv);

    if(args().commandGiven("make-config")) {
      cout<<args().makeConfig()<<endl;
      exit(0);
    }
    if(!args().paramString("config-name").empty())
      L.setName("powerpop-"+args().paramString("config-name"));    
    args().getRest(argc,argv,commands);
  }
  catch(ArgumentException &e) {
    cerr<<"Unable to parse arguments:"<<endl<<e.txtReason()<<endl;
    exit(0);
  }
  if(!commands.empty())
    doCommands(commands);


  vector<string> backends;
  stringtok(backends, args().paramString("backends"),", ");
  try {
    MegaTalker::setBackends(backends, args().paramString("pplistener-password"));
    MegaTalker::setRedundancy(2);
    MegaTalker::setStateDir(args().paramString("state-dir"));
  }
  catch(MegaTalkerException &e) {
    L<<Logger::Error<< e.getReason()<<endl;
    L<<Logger::Error<<"PowerPOP exiting"<<endl;
    exit(0);
  }

  try {
    startUserBase();
    Server *s=new Server(args().paramAsNum("listen-port"));

    dropPrivs(args().paramAsNum("run-as-uid"), args().paramAsNum("run-as-gid"));
    
    if(args().switchSet("daemon")) {
      L<<Logger::Info<<"Going to background"<<endl;
      daemonize();
      L.toConsole(false);
    }
    else
      L.toConsole(true);

    MegaTalker::startStateThread();

    Session *client;
    pthread_t tid;
    while((client=s->accept())) {
      pthread_create(&tid, 0 , &serveConnection, (void *)client);
    }
  }
  catch(SessionTimeoutException &e) {
    L<<Logger::Error<<"Timeout in powerpop main"<<endl;
  }
  catch(SessionException &e) {
    L<<Logger::Error<<"Fatal TCP error in powerpop main: "<<e.d_reason<<endl;
  }
  catch(Exception &e) {
    L<<Logger::Error<<"Fatal error in powerpop main: "<<e.reason<<endl;
  }
  L<<Logger::Error<<"PowerPOP exiting"<<endl;
  return 0;
}
