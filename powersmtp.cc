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
#include "smtpsession.hh"
#include "logger.hh"
#include "misc.hh"
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <cstdio>
#include <signal.h>
#include "config.h"
#include "megatalker.hh"
#include "userbase.hh"
#include "common.hh"

extern Logger L;
extern UserBase *UB;

static void *serveConnection(void *p)
{
  pthread_detach(pthread_self());

  Session *s=(Session *)p;
  s->setTimeout(1800);
  try {
    SmtpSession smtp(s->getRemote());
    L<<Logger::Warning<<"Session start for "<<s->getRemote()<<endl;    
    s->putLine(smtp.getBanner());
  
    string line, response;
    int res;
    SmtpSession::quit_t quit;
    for(;;) {
      s->getLine(line);
      res=smtp.giveLine(line,response,quit);
      if(quit==SmtpSession::quitNow)
	break;
      
      if(res)
	s->putLine(response+"\r\n");
      
      if(quit==SmtpSession::pleaseQuit)
	break;
      if(quit==SmtpSession::pleaseDie)
	exit(0);

    }
    L<<Logger::Warning<<"Session from "<<s->getRemote()<<" signed off"<<endl;

    s->close();
    delete s;
    return 0;
  }

  catch(SessionTimeoutException &e) {
    L<<Logger::Warning<<e.d_reason<<endl;
    s->close();
    delete s;
    return 0;
  }
  catch(SessionException &e) {
    L<<Logger::Error<<"Fatal error in smtp session with "<<s->getRemote()<<": "<<e.d_reason<<endl;
    s->close();
    delete s;

    return 0;
  }

  catch(...) {
    L<<Logger::Error<<"Caught unknown exception from SMTP session"<<endl;
    s->close();
    delete s;
    return 0;
  }
  return 0;
}

static void doCommands(const vector<string>&commands)
{
  int ret=1;
  const string &command=commands[0];
  try {

    string line;
    cerr<<"powersmtp";
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
      s->getLine(line);
      if(line[0]=='2') {
	cerr<<"stopped powersmtp"<<endl;
	ret=0;
      }
      else {
	stripLine(line);
	cerr<<"powersmtp did not stop: "<<line<<endl;
      }
      s->close();
      delete s;
    }
    else
    if(command=="status") {
      s->putLine("control status\r\n");
      s->getLine(line);
      if(!line.empty() && line[0]=='2') {
	stripLine(line);
	cerr<<"PowerSMTP functioning ok: "<<line<<endl;
	ret=0;
      }
      else {
	stripLine(line);
	if(!line.empty() && line[0]=='5')
	  cerr<<"PowerSMTP not running at "
	      <<args().paramString("listen-address")<<":"<<args().paramAsNum("listen-port")<<", probably another SMTP server!"<<endl;
	else
	  cerr<<"PowerSMTP not ok: "<<line<<endl;
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

int PowerSmtpMain(int argc, char **argv)
{
  L.setName("powersmtp");
  args().addParameter("config-dir","Location to read configuration files from",SYSCONFDIR);
  args().addParameter("config-name","Name of this virtual configuration","");


  args().addParameter("advertisement","Advertisement to insert on received messages","");
  args().addParameter("listen-port","Port on which to listen for new connections","25");
  args().addParameter("listen-address","Address on which to listen for new connections","0.0.0.0");
  args().addParameter("run-as-uid","User-id to run as, after acquiring socket","0");
  args().addParameter("run-as-gid","Group-id to run as, after acquiring socket","0");

  UserBaseArguments();

  args().addParameter("pplistener-password","Password for the pptalker to connect to the pplisteners","");
  args().addParameter("backends","Comma separated list of PPListener backends","1:1:127.0.0.1");
  args().addParameter("max-msg-kb","Maximum size of messages in kb, 0 for unlimited","25000");

  args().addParameter("smtp-sender-address","Address of our SMTP email sender","127.0.0.1");
  args().addParameter("smtp-sender-port","Address of our SMTP email sender","2500");

  args().addParameter("redundancy-target","Number of backends required per message for succesful delivery","1");

  args().addCommand("help","Display this helpful message");
  args().addCommand("version","Display version information");
  args().addCommand("make-config","Output a configuration file conformant to current settings, which you can also specify on the commandline");
  args().addSwitch("daemon","Run as a daemon",true);
  args().addSwitch("spread-disk","Given equal priority backend servers, favor those with emptier disks",true);
  args().addSwitch("spread-load","Given equal priority backend servers, favor those with lower load",false);

  signal(SIGPIPE,SIG_IGN);

  L.toConsole(Logger::Warning);
  vector<string>commands; // will be found on the commandline
  try {
    string response;

    args().preparseArgs(argc, argv,"help");
    if(args().commandGiven("help")) {
      cerr<<args().makeHelp()<<endl;
      exit(0);
    }
    args().preparseArgs(argc, argv,"make-config");
    if(args().commandGiven("make-config")) {
      cout<<args().makeConfig()<<endl;
      exit(0);
    }
    args().preparseArgs(argc, argv,"config-dir");
    if(args().parseFiles(response, (args().paramString("config-dir")+"/power.conf").c_str(),0))
      L<<Logger::Error<<"Warning: unable to open common configuration file"<<endl;
    else
      L<<Logger::Notice<<"Read common configuration from "<<response<<endl;

    if(args().parseFiles(response, (args().paramString("config-dir")+"/powersmtp.conf").c_str(),0))
      L<<Logger::Error<<"Warning: unable to open powersmtp configuration file"<<endl;
    else
      L<<Logger::Notice<<"Read configuration from "<<response<<endl;


    args().parseArgs(argc, argv);

    if(args().commandGiven("help")) {
      cerr<<args().makeHelp()<<endl;
      exit(0);
    }

    if(args().commandGiven("version")) {
      cerr<<"powersmtp version "<<VERSION<<". This is $Id: powersmtp.cc,v 1.2 2002-12-04 16:32:49 ahu Exp $"<<endl;
      exit(0);
    }

    if(!args().paramString("config-name").empty())
      L.setName("powersmtp-"+args().paramString("config-name"));
    
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
    MegaTalker::setRedundancy(args().paramAsNum("redundancy-target"));
  }
  catch(MegaTalkerException &e) {
    cerr<<e.getReason()<<endl;
    exit(0);
  }

  try {
    startUserBase();

    Server *s=new Server(args().paramAsNum("listen-port"),args().paramString("listen-address"));

    dropPrivs(args().paramAsNum("uid"), args().paramAsNum("gid"));
    
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
    L<<Logger::Error<<"Fatal Timeout: "<<e.d_reason<<endl;
  }
  catch(SessionException &e) {
    L<<Logger::Error<<"Fatal error: "<<e.d_reason<<endl;
  }
  L<<Logger::Error<<"PowerSMTP ended"<<endl;
  return 0;
}
