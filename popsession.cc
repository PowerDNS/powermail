#include "popsession.hh"
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

void PopSession::strip(string &line)
{
  unsigned int pos=line.find_first_of("\r\n");
  if(pos!=string::npos) {
    line.resize(pos);
  }

}

PopSession::~PopSession()
{
  if(d_megatalker)
    delete d_megatalker;
}

void PopSession::firstWordUC(string &line)
{
  for(string::iterator i=line.begin();i!=line.end();++i) {
    if(*i==' ')
      return;
    *i=toupper(*i);
  }
}

int PopSession::delDeleted(string &response)
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

bool PopSession::topCommands(const string &line, string &response, quit_t &quit)
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

bool PopSession::loginCommands(const string &line, string &response, quit_t &quit)
{
  if(!line.find("USER ")) {
    if(d_login) {
      response="-ERR Already logged in";
      return true;
    }

    d_user=line.substr(5);
    response="+OK ";
    return 1;
  }

  if(!line.find("PASS ")) {
    if(d_login) {
      response="-ERR Already logged in";
      return true;
    }
    if(d_user.empty()) {
      response="-ERR USER first";
      return true;
    }
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
      res=ubh.d_thing->mboxData(*dest,md,line.substr(5), response, exists, pwcorrect); // sets response!
      if(res<0) {
	response="-ERR "+response;
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

    response="-ERR Invalid credentials (wrong username or password?)";
    if(exists && pwcorrect) {
      L<<Logger::Warning<<"Mailbox '"<<d_user<<"' logged in"<<endl;
      d_login=true;

      
      try {
	d_megatalker=0;
	d_megatalker=new MegaTalker;
      }
      catch(MegaTalkerException &e) {
	L<<"Fatal error connecting to MegaTalker: "<<e.getReason()<<endl;
	response="-ERR Temporary error connecting to backends";
	return 1;
      }
      if(scanDir(response))
	return false;

      response=makeStatResponse();
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

const string PopSession::makeStatResponse()
{
  size_t tot=0;
  int num=0;
  for(msgData_t::const_iterator i=d_msgData.begin();i!=d_msgData.end();++i) {
    if(!i->d_deleted) {
      tot+=i->d_size;
      num++;
    }
  }
  ostringstream o;
  o<<"+OK "<<num<<" "<<tot;
  return o.str();
}

bool PopSession::restCommands(const string &line, string &response, quit_t &quit, int &fd)
{
  fd=0;
  if(!d_login) {
      return 0;
  }

  if(!d_scanned) {
    if(scanDir(response))
      return true;
  }

  vector<string>arguments;
  stringtok(arguments,line);
  if(!arguments.size())
    return 0;

  int numArgs=arguments.size();

  string &command=arguments[0];

  if(command=="UIDL") {
    if(numArgs==1) {
      response="+OK\r\n";
      
      for(msgData_t::const_iterator i=d_msgData.begin();i!=d_msgData.end();++i) {
	if(i->d_deleted)
	  continue;
	
	ostringstream o;
	o<<1+i-d_msgData.begin()<<" "<<i->d_fname<<"\r\n";
	response+=o.str();
      }
      response+=".";
      return 1;
    }
    
    unsigned int num=atoi(arguments[1].c_str())-1;
    if(num>=d_msgData.size()) {
      response="-ERR Number out of range";
      return 1;
    }
    
    ostringstream o;
    o<<"+OK "<<num+1<<" "<<d_msgData[num].d_fname;
    response=o.str();
    return 1;
  }



  if(command=="RETR" && numArgs==2) {
    response="+OK";
    unsigned int num=atoi(arguments[1].c_str())-1;
    if(num>=d_msgData.size()) {
      response="-ERR Number out of range";
      return 1;
    }
    try {
      d_megatalker->blastMessageFD(d_msgData[num].d_fname,d_msgData[num].d_size,d_clisock);
      d_retrieved++;
      response="";
    }
    catch(MegaTalkerException &e) {
      response="-ERR Retrieving message, "+e.getReason();
    }

    return 1;
  }

  if(command=="TOP" && numArgs>1 && numArgs<=3) {
    if(numArgs<2) {
      response="-ERR Missing parameter";
      return true;
    }
      
    response="+OK";
    unsigned int num=atoi(arguments[1].c_str())-1;

    if(num>=d_msgData.size()) {
      response="-ERR Number out of range";
      return 1;
    }

    int limit=-1;
    if(arguments.size()>2)
      limit=atoi(arguments[2].c_str());

    d_megatalker->blastMessageFD(d_msgData[num].d_fname,0,d_clisock,limit);
    response="";
    return 1;
  }


  if(command=="DELE" && numArgs==2) {
    if(!d_scanned) {
      if(scanDir(response))
	return true;
    }

    unsigned int num=atoi(arguments[1].c_str())-1;
    if(num>=d_msgData.size()) {
      response="-ERR Number out of range";
      return 1;
    }
    if(!d_msgData[num].d_deleted) {
      d_msgData[num].d_deleted=true;
      response="+OK";
    }
    else {
      response="-ERR Already deleted";
    }

    return 1;
  }

  if(command=="STAT" && numArgs==1) {
    response=makeStatResponse();

    return 1;
  }
  
  if(command=="LIST" && numArgs==2) {
    if(!d_scanned) {
      if(scanDir(response))
	return true;
    }

    unsigned int num=atoi(arguments[1].c_str())-1;
    if(num>=d_msgData.size()) {
      response="-ERR Number out of range";
      return 1;
    }

    if(d_msgData[num].d_deleted) {
      response="-ERR was deleted";
      return 1;
    }

    ostringstream o;
    o<<"+OK "<<num+1<<" "<<d_msgData[num].d_size;
    response=o.str();
    return 1;
  }


  if(command=="LIST" && numArgs==1) { // FIXME, should also accept a parameter
    response="+OK\r\n";
    ostringstream o;
    for(msgData_t::const_iterator i=d_msgData.begin();i!=d_msgData.end();++i) {
      if(i->d_deleted)
	continue;


      o<<1+i-d_msgData.begin()<<" "<<i->d_size<<"\r\n";

    }
    response+=o.str();
    response+=".";
    return 1;
  }

  if(!line.find("TOP")) {
    response="+OK";
    return 1;
  }
  return 0;
}

int PopSession::scanDir(string &response)
{
  MegaTalker::mboxInfo_t data;
  try {
    d_megatalker->getMsgs(d_user, data);
  }
  catch(MegaTalkerException &e) {
    response="-ERR "+e.getReason();
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

bool PopSession::controlCommands(const string &line, string &response, quit_t &quit)
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


bool PopSession::giveLine(string &line, string &response, quit_t &quit, int &fd)
{
  quit=noQuit;
  fd=0;
  strip(line);

  firstWordUC(line);

  if(controlCommands(line,response,quit))
    return true;

  if(topCommands(line, response, quit)) // RSET, HELP, QUIT, NOOP
    return true;

  if(loginCommands(line,response,quit))
    return true;

  if(restCommands(line,response,quit,fd))
    return true;
  
  response="-ERR Not Implemented or Not Authorized Yet";
  L<<Logger::Error<<"Unable to parse or process line '"<<line<<"'"<<endl;
  return true;
}

string PopSession::getBanner()
{
  return "+OK "+getHostname()+"\r\n";
}
