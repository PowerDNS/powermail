#include "ppsession.hh"
#include "misc.hh"
#include "delivery.hh"
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sstream>
#include <ctype.h>
#include <string>

#ifdef __FreeBSD__
# include <sys/param.h>
# include <sys/mount.h>
#else
# include <sys/vfs.h>
#endif

#include "argsettings.hh"
#include "logger.hh"
#include "common.hh"

extern Logger L;

PPState::PPState()
{
  d_stateDir=args().paramString("mail-root");
  if(mkdir(d_stateDir.c_str(),0700)<0 && errno!=EEXIST)
    throw AhuException("Unable to make mail-root in "+d_stateDir+
		       ": "+stringerror());

  d_full=false;

  if(isReadonly()) 
    L<<Logger::Error<<"pplistener is readonly!"<<endl;
}

bool PPState::isReadonly()
{
  return d_full || !access((d_stateDir+"/readonly").c_str(),F_OK);
} 

void PPState::setReadonly()
{
  if(isReadonly())
    return;
  
  L<<Logger::Error<<"State change to readonly"<<endl;
  int fd=open((d_stateDir+"/readonly").c_str(),O_CREAT|O_RDONLY);
  if(!fd<0)
    close(fd);
}

void PPState::setFull()
{
 d_full=true;
  if(isReadonly())
    return;
  
  L<<Logger::Error<<"State change to Full"<<endl;
}

void PPState::unsetFull()
{
  if(!isReadonly())
    return;
  d_full=false;
  L<<Logger::Error<<"State no longer Full"<<endl;
}



void PPState::unsetReadonly()
{
  if(!isReadonly())
    return;

  if(unlink((d_stateDir+"/readonly").c_str())<0)
    throw AhuException("Unable to make mail-root in "+d_stateDir);
  d_full=false;
  L<<Logger::Error<<"State change to readonly"<<endl;
}

void PPListenSession::strip(string &line)
{
  unsigned int pos=line.find_first_of("\r\n");
  if(pos!=string::npos) {
    line.resize(pos);
  }
}

void PPListenSession::lowerCase(string &line)
{
  for(string::iterator i=line.begin();i!=line.end();++i) {
    *i=toupper(*i);
  }
}

bool PPListenSession::statCommand(string &response)
{
  struct statfs buf;
  if(statfs((args().paramString("mail-root")+"/").c_str(),&buf)<0)
    response="-ERR "+stringerror();
  else {
    ostringstream o;
    unsigned int kbytes=buf.f_bavail;
    kbytes*=(buf.f_bsize>>10); // force sequence point

    o<<"+OK "<<kbytes<<" kbytes "<<buf.f_ffree<<" inodes "<<getLoad()<<" load";
    response=o.str();

    if(kbytes>(unsigned int)args().paramAsNum("min-kbytes-free"))
      d_pps.unsetFull();
  }
  return true;
}

bool PPListenSession::pingCommand(string &response)
{
  response="+OK";

  if(d_pps.isReadonly()) 
    response+=" READONLY";

  return true;
}

bool PPListenSession::quitCommand(string &response, quit_t &quit)
{
  quit=pleaseQuit;
  response="+OK";
  return true;
}


bool PPListenSession::passwordCommand(const string &password, string &response)
{

  if((password.empty() && args().paramString("password").empty() && d_remote.find("127.0.0.1:")) || // no password & localhost
     (password==args().paramString("password"))) {
    response="+OK";
    d_mode=blank;
  }
  else {
    response="-ERR Incorrect credentials";
    L<<Logger::Error<<"Incorrect password tried"<<endl;
    if(args().paramString("password").empty())
      L<<Logger::Error<<"No password set either!"<<endl;
  }

  return true;
}

bool PPListenSession::controlCommands(const string &line, string &response, quit_t &quit)
{

  response="-ERR Unknown error processing control command";
  vector<string> words;
  stringtok(words,line);
  if(words.empty())
    return true;

  if(d_remote.find("127.0.0.1:") && d_remote.find(args().paramString("listen-address")+":")) {
    L<<Logger::Error<<"Remote "<<d_remote<<" tried a control command: "<<line<<endl;
    return true;
  }
    
  if(words[0]!="control")
    return true;

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

bool PPListenSession::giveLine(string &line, string &response, quit_t &quit, int &fd, bool &more)
{
  more=false;
  if(d_mode==inData) {
    return putLine(line, response);
  }

  if(d_mode==Dir)
    return moreDir(response,more);

  response="-ERR Not Implemented or Not Authorized Yet";

  quit=noQuit;
  fd=-1;
  strip(line);

  vector<string> arguments;
  stringtok(arguments,line);

  const int numArgs=arguments.size();
  if(!numArgs) 
    return true;

  const string &command=arguments[0];

  if(command=="quit")
    return quitCommand(response, quit);


  if(d_mode==unAuth) {
    if(command=="pass")
      return passwordCommand(numArgs==2 ? arguments[1] : "", response);

    response="-ERR Not Implemented or Not Authorized Yet";
    return true;
  }
  if(command=="control")
    return controlCommands(line,response,quit);

  if(d_mode==inLimbo) {
    if(line=="commit") 
      return commitCommand(response);
    
    if(line=="rollback") 
      return rollBackCommand(response);
  }
  else if(d_mode==preLimbo) {
    if(command=="rcpt" && numArgs==3)
      return rcptCommand(arguments[1],arguments[2], response);

    if(line=="put") 
      return putCommand(response);
  }
  else {
    if(command=="rcpt" && numArgs==3)
      return rcptCommand(arguments[1],arguments[2], response);
    
    if(numArgs==2 && command=="ls")
      return listResponse(arguments[1],response);

    if(numArgs==2 && command=="nuke")
      return nukeCommand(arguments[1],response);

    if(numArgs==3 && command=="del")
      return delCommand(arguments[1],arguments[2],response);


    if(numArgs==3 && command=="get")
      return getCommand(arguments[1], arguments[2], &fd, response);

    if(command=="ping")
      return pingCommand(response);

    if(command=="stat")
      return statCommand(response);

    if(command=="dir")
      return dirCommand(response,more);

    
    try {
      if(command=="readonly") {
	response="+OK";
	d_pps.setReadonly();
	return true;
      }
      if(command=="readwrite") {
	response="+OK";
	d_pps.unsetReadonly();
	return true;
      }
    }
    catch(AhuException &e) {
      response="-ERR "+e.d_reason;
      return true;
    }
  }
  return true;
}

bool PPListenSession::moreDir(string &response, bool &more)
{
  int n=0;
  response="";
  more=true;
  try {
    while(d_delivery.getListNext(response))
      if(n==1000)
	return true;
  }
  catch(DeliveryException &e) {
    response="-ERR Error listing mailboxes: "+e.getReason();
    more=false;
    d_mode=blank;
    return true;
  }

  response.append(".");
  more=false;
  d_mode=blank;
  return true;
}


bool PPListenSession::dirCommand(string &response, bool &more) 
{
  more=true;
  d_delivery.startList();
  response="+OK List of mailboxes follows";
  d_mode=Dir;
  return true;
}

bool PPListenSession::nukeCommand(const string &mbox, string &response) 
{
  d_delivery.nuke(mbox,response);
  return true;
}


// rcpt ahu@ds9a.nl 25
bool PPListenSession::getCommand(const string &mbox, const string &index, int *fd, string &response)
{
  return (*fd=d_delivery.getMessageFD(mbox,index,response));
}

bool PPListenSession::delCommand(const string &mbox, const string &index, string &response)
{
  d_delivery.delMessage(mbox,index,response);
  return true;
}



// rcpt ahu@ds9a.nl 25
bool PPListenSession::rcptCommand(const string &mbox, const string &index, string &response)
{

  if(d_delivery.addFile(mbox, index, &response)) {
    response="-ERR "+response;
    return true;
  }

  d_mode=preLimbo;

  response="+OK";
  return true;

}

bool PPListenSession::putCommand(string &response)
{
  d_failed=false;
  d_mode=inData;
  response="+OK Incoming";
  return true;
}

bool PPListenSession::putLine(string &line, string &response)
{
  if(line[0]=='.' && (line[1]=='\r' || line[1]=='\n')){
    response="+OK";
    d_mode=inLimbo;
    return true;
  }
    
  if(!d_failed && d_delivery.giveLine(line)<0) {
    int err=errno;
    if(err==ENOSPC) {
      d_pps.setFull(); // now what? when do we reset this?
    }
      
    d_reason=strerror(err);
    d_failed=true;
  }

  return false;
}

bool PPListenSession::commitCommand(string &response)
{
  d_mode=blank;
  if(d_failed) {
    response="-ERR failed during reception of message and writing to disk: "+d_reason;
    d_failed=false;
    return true;
  }
  else {
    try {
      d_delivery.commit();
      response="+OK";
    }
    catch(DeliveryException &e) {
      response="-ERR "+e.getReason();
    }
  }
  d_failed=false;
  d_delivery.reset();
  return true;
}

bool PPListenSession::rollBackCommand(string &response)
{
  d_mode=blank;
  d_delivery.rollBack(response);
  d_delivery.reset();
  return true;
}



bool PPListenSession::listResponse(const string &mbox, string &response)
{
  d_delivery.listMbox(mbox, response);
  return true;
}
  

string PPListenSession::getBanner()
{
  return "+OK "+getHostname()+"\r\n";
}
