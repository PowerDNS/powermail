#include <stdio.h>
#include <unistd.h>
#include <iomanip>
#include <set>
#include <algorithm>

#if defined(__Linux__)
# include <crypt.h>
#endif

#include "delivery.hh"
#include "misc.hh"
#include "megatalker.hh"
#include "userbase.hh"
#include "argsettings.hh"
#include "logger.hh"
#include "common.hh"

bool degraded(false);
unsigned char maxBackendLen;

extern Logger L;

static UserBase *UB;
bool g_simple_output = true;

struct BackendData
{
  string address;
  int port;
};
vector<BackendData> backends;

void printUsage()
{
  cerr<<"Parameters and default values:"<<endl;
  cerr<<args().makeHelp()<<endl;
  cerr<<endl;
  cerr<<"Commands: "<<endl;
  cerr<<setw(30)<<ios::left<<"block backend1 [backends..]"<<"Set backends to readonly"<<endl;
  cerr<<setw(30)<<ios::left<<"unblock backend1 [backends..]"<<"Set backends to read/write"<<endl;
  cerr<<setw(30)<<ios::left<<"dir"<<"List all mailboxes on all backends"<<endl;
  cerr<<setw(30)<<ios::left<<"dir backend1 [backendse]"<<"List all mailboxes on specified backends"<<endl;
  cerr<<setw(30)<<ios::left<<"orphans"<<"List all mailboxes that do not appear in the database"<<endl;
  cerr<<setw(30)<<ios::left<<"purge mbox1 [mboxen..]"<<"Purge a mailbox on all backends"<<endl;
  cerr<<setw(30)<<ios::left<<"purge-orphans"<<"Purge all mailboxes that do not appear in the database"<<endl;
  cerr<<setw(30)<<ios::left<<"list mbox"<<"List a mailbox and redundancy info"<<endl;
  cerr<<setw(30)<<ios::left<<"status"<<"See status of all backends"<<endl;
  cerr<<setw(30)<<ios::left<<"usage mbox"<<"See usage of a mailbox"<<endl;
}

void die(const string &s)
{
  cerr<<"Exiting: "<<s<<endl;
  exit(1);
}

void statCommand(const vector<string>&words)
{
  long totalKB=0;
  for(vector<BackendData>::const_iterator i=backends.begin();i!=backends.end();++i) {
    if(i->port!=1101)
      cout<<setw(maxBackendLen)<<ios::left<<(i->address+":"+itoa(i->port));
    else
      cout<<setw(maxBackendLen)<<ios::left<<i->address;

    try {
      PPTalker pp(i->address,i->port,args().paramString("pplistener-password"));
      int kb, inodes;
      double load=-1;
      bool readonly;
      pp.stat(&kb, &inodes, &load);
      pp.ping(readonly);
      if(kb>10000)
	cout<<kb/1000<<" mb, "<<inodes<<" inodes, load: "<<load<<", ";
      else
	cout<<kb<<" kb, "<<inodes<<" inodes, load: "<<load<<", ";

      if(readonly)
	cout<<"readonly access"<<endl;
      else {
	totalKB+=kb;
	cout<<"read/write access"<<endl;
      }
    }
    catch(TalkerException &e) {
      cout<<e.getReason()<<endl;
    }
    
  }
  cout<<setw(maxBackendLen)<<ios::left<<""<<totalKB/1000<<" mb available for writing"<<endl;
}

void nukeCommand(const vector<string>&words)
{
  for(vector<BackendData>::const_iterator i=backends.begin();i!=backends.end();++i)
  {
     
     if (g_simple_output == false) {
        if(i->port!=1101)
           cout<<setw(maxBackendLen)<<ios::left<<(i->address+":"+itoa(i->port));
        else
           cout<<setw(maxBackendLen)<<ios::left<<i->address;
     }

    try {
      PPTalker pp(i->address,i->port,args().paramString("pplistener-password"));
      vector<string>::const_iterator i=words.begin();
      ++i;
      if (g_simple_output == false)
         cout<<"Purging ";

      for(;i!=words.end();++i) {

         if (g_simple_output == false)
            cout<<"'"<<*i<<"'..";
         
	pp.nukeMbox(*i);

        if (g_simple_output == false)
           cout<<" done. ";
      }
      
      if (g_simple_output == false)
         cout<<endl;
    }
    
    catch(TalkerException &e) {
       if (g_simple_output == true) {
          cout << "1:" << e.getReason()<<endl;
       } else {
          cout<<e.getReason()<<endl;
          degraded=true;
       }
    }
  }

  if (g_simple_output) {
     cout << "0:OK" << endl;
  }
}

bool byIndex(const Talker::msgInfo_t &a, const Talker::msgInfo_t &b)
{
  if(a.index<b.index)
    return true;
  if(a.index==b.index && a.ppname<b.ppname)
    return true;
  return false;
}

void dirMboxesName(const string &address, unsigned int port,vector<string> &mboxes,map<string,int>&all)
{
  if(port!=1101)
    cout<<setw(maxBackendLen)<<ios::left<<(address+":"+itoa(port));
  else
    cout<<setw(maxBackendLen)<<ios::left<<address;

  try {
    PPTalker pp(address,port,args().paramString("pplistener-password"));
    pp.listMboxes(mboxes);
    cout<<mboxes.size()<<" mailboxes"<<endl;
    for(vector<string>::const_iterator j=mboxes.begin();j!=mboxes.end();++j)
      all[*j]++;
  }
  catch(TalkerException &e) {
    cout<<e.getReason()<<endl;
  }
}

void dbLookup(const string &mbox)
{
  MboxData md;
  string response;
  bool exists=false, pwcorrect;
  int res=UB->mboxData(mbox,md,"",response,exists,pwcorrect);
  if(res<0)
    cout<<"(Database error: "<<response<<")";
  else if(!res && exists) {
    cout<<"Exists, quota: ";
    if(md.mbQuota)
      cout<<md.mbQuota<<" mb";
    else
      cout<<"none";
  }
  else
    cout<<"Not in the database";
}

void findOrphans(vector<string> &orphans)
{
  map<string,int>all;

  for(vector<BackendData>::const_iterator i=backends.begin();i!=backends.end();++i) {
    vector<string> mboxes;
    try {
      PPTalker pp(i->address,i->port,args().paramString("pplistener-password"));
      pp.listMboxes(mboxes);
      for(vector<string>::const_iterator j=mboxes.begin();j!=mboxes.end();++j)
	all[*j]++;
    }
    catch(TalkerException &e) {
      die(i->address+":"+itoa(i->port)+":"+e.getReason());
    }
  }

  for(map<string,int>::const_iterator i=all.begin();i!=all.end();++i) {
    MboxData md;
    string response;
    bool exists=false,pwcorrect;
    if(UB->mboxData(i->first,md,"",response,exists,pwcorrect)==0 && !exists)
      orphans.push_back(i->first);
  }
}  

void listOrphans()
{
  vector<string> orphans;
  findOrphans(orphans);
  if(orphans.empty())
    cerr<<"No orphans!"<<endl;
  else
    for(vector<string>::const_iterator i=orphans.begin();i!=orphans.end();++i)
      cout<<*i<<endl;
}

void nukeOrphans()
{
  vector<string> orphans;
  orphans.push_back("dummy");
  findOrphans(orphans);
  if(orphans.size()==1)
    cerr<<"No orphans!"<<endl;
  else
    nukeCommand(orphans);
}

void dirMboxes(bool lookup=false)
{
  map<string,int>all;

  for(vector<BackendData>::const_iterator i=backends.begin();i!=backends.end();++i) {
    vector<string> mboxes;
    dirMboxesName(i->address,i->port,mboxes,all);
  }

  unsigned int maxlen=0;
  for(map<string,int>::const_iterator i=all.begin();i!=all.end();++i) 
    maxlen=max(maxlen,i->first.length());

  cout<<endl<<setw(maxlen+2)<<ios::left<<"Mailbox"<<"# backends"<<endl;
  for(map<string,int>::const_iterator i=all.begin();i!=all.end();++i) {
    cout<<setw(maxlen+2)<<ios::left<<i->first<<setw(10)<<ios::left<<i->second<<"  ";
    if(lookup) 
      dbLookup(i->first);

    cout<<endl;
  }
}


void dirMboxes(const vector<string> &bends, bool lookup=false)
{
  map<string,int>all;
  
  for(vector<string>::const_iterator i=bends.begin()+1;i!=bends.end();++i) {
    vector<string>parts;
    stringtok(parts,*i,":");
    
    vector<string> mboxes;
    dirMboxesName(parts[0],parts.size()>1 ? atoi(parts[1].c_str()) : 1101,mboxes,all);
  }

  unsigned int maxlen=0;
  for(map<string,int>::const_iterator i=all.begin();i!=all.end();++i) 
    maxlen=max(maxlen,i->first.length());

  cout<<endl<<setw(maxlen+2)<<ios::left<<"Mailbox"<<"# backends"<<endl;
  for(map<string,int>::const_iterator i=all.begin();i!=all.end();++i) {
    cout<<setw(maxlen+2)<<ios::left<<i->first<<setw(10)<<ios::left<<i->second<<"  ";
    if(lookup) 
      dbLookup(i->first);
    cout<<endl;
  }
}


void blockunblockCommand(const vector<string>&backends, bool dir)
{
  for(vector<string>::const_iterator i=backends.begin()+1;i!=backends.end();++i) {
    vector<string>parts;
    int port=1101;

    stringtok(parts,*i,":");
    if(parts.size()<1)
      die("Invalid backend '"+*i+"' lacks fields");

    string address=parts[0];
    if(parts.size()>1)
      port=atoi(parts[1].c_str());

    if(port!=1101)
      cout<<setw(maxBackendLen)<<ios::left<<(address+":"+itoa(port));
    else
      cout<<setw(maxBackendLen)<<ios::left<<address;


    try {
      PPTalker pp(address,port,args().paramString("pplistener-password"));
      pp.setReadonly(dir);

      cout<<(dir ? "blocked" : "unblocked")<<endl;
    }
    catch(TalkerException &e) {
      cout<<e.getReason()<<endl;
    }
    
  }  
}

void blockCommand(const vector<string>&backends) 
{
  blockunblockCommand(backends,true);
}

void unblockCommand(const vector<string>&backends) 
{
  blockunblockCommand(backends,false);
}


void listCommand(const string &mbox)
{
  Talker::mboxInfo_t mbi;
  int lastmsgs=0;
  for(vector<BackendData>::const_iterator i=backends.begin();i!=backends.end();++i) {
    if(i->port!=1101)
      cout<<setw(maxBackendLen)<<ios::left<<(i->address+":"+itoa(i->port));
    else
      cout<<setw(maxBackendLen)<<ios::left<<i->address;

    try {
      PPTalker pp(i->address,i->port,args().paramString("pplistener-password"));
      pp.getMsgs(mbox, mbi);

      cout << "done listing '"<<mbox<<"', "<<mbi.size()-lastmsgs<<" messages" << endl;
      lastmsgs=mbi.size();
    }
    catch(TalkerException &e) {
      cout<<e.getReason()<<endl;
      degraded=true;
    }
  }    
  cout<<endl;
  sort(mbi.begin(),mbi.end(),byIndex);

  string lastindex;
  int count=0;
  int msgs=0, nonredundant=0;
  int lastsize=-1;
  long long totalsize=0;
  for(Talker::mboxInfo_t::const_iterator i=mbi.begin();i!=mbi.end();++i) {
    if(i->index!=lastindex) {
      if(!lastindex.empty() && count<2) {
	cout<<"\t * NOT REDUNDANT!"<<endl;
	nonredundant++;
      }
      totalsize+=i->size;
      msgs++;
      cout<<i->index<<endl;
      count=0;
    }
    else if(lastsize>0 && (unsigned int)lastsize!=i->size)
      cout<<"SIZE!";

    count++;
    cout<<"\t"<<setw(maxBackendLen)<<ios::left<<i->ppname<<"\t"<<i->size<<endl;
    lastindex=i->index;
    lastsize=i->size;
  }

  if(!lastindex.empty() && count<2) {
    cout<<"\t * NOT REDUNDANT!"<<endl;
    nonredundant++;
  }

  cout<<endl;
  if(msgs) {
    cout<<msgs<<" messages, "<<(int)(nonredundant*100.0/msgs)<<"% non-redundant"<<endl;
    cout<<totalsize/1024<<" kilobytes net"<<endl;
  }
}

void usageCommand(const string &mbox)
{
  Talker::mboxInfo_t mbi;

  //
  // Loop through all the backends. Ask the first one available for the mailbox
  // size. If none are available then we return an error.
  //

  size_t usage = 0;
  bool ok = false;

  for (vector<BackendData>::const_iterator i=backends.begin();i!=backends.end();++i)
  {     
     try {
       PPTalker pp(i->address,i->port, args().paramString("pplistener-password"));
       pp.getMsgs(mbox, mbi);
       
       for (Talker::mboxInfo_t::const_iterator i=mbi.begin(); i!=mbi.end(); ++i) {
          usage += max((unsigned int)4,(i->size/1024)); // penalize small files
       }
  
       ok = true;
   
       break;
    }
    
    catch(TalkerException &e) {
       if (g_simple_output == false) {
          cerr<<"Unable to talk to backend." << endl;
       }
    }
  }

  if (g_simple_output) {  
     if (ok == false) {
        cout << "1:Server Failure" << endl;
     } else {
        cout << "0:" << usage << endl;
     }
  } else {
     if (ok == false) {
        cout << "None of the backends responded" << endl;
     } else {
        cout << "Mailbox " << mbox << " is using " << usage << "KB" << endl;
     }
  }
}
static char salts[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
void doCrypt(const vector<string>&words)
{
  if(words.size()!=2) {
    cerr<<"crypt needs exactly one parameter - the password to crypt"<<endl;
    exit(1);
  }
  char salt[3];
  srandom(time(0));

  salt[0]=salts[random()%sizeof(salts)];
  salt[1]=salts[random()%sizeof(salts)];
  salt[2]=0;

  cout<<"{crypt}"<<crypt(words[1].c_str(),salt)<<endl;
}

void doMD5(const vector<string>&words)
{
  if(words.size()!=2) {
    cerr<<"crypt needs exactly one parameter - the password to crypt"<<endl;
    exit(1);
  }
  char salt[12];
  memset(salt,0,12);
  srandom(time(0));
  strcpy(salt,"$1$");
  int length=(1+random())%8;
  for(int n=0;n<length;++n)
    salt[3+n]=salts[random()%sizeof(salts)];

  cout<<"{md5}"<<crypt(words[1].c_str(),salt)<<endl;
}


int PPToolMain(int argc, char **argv)
{
  L.setName("pptool");
  args().addParameter("config-dir","Location to read configuration files from","/etc/powermail");
  args().ignore("config-name");
  args().addParameter("listen-port","Port on which to listen for new connections","110");
  args().addParameter("listen-address","Address on which to listen for new connections","0.0.0.0");

  UserBaseArguments();

  args().addParameter("mysql-password","MySQL password","");
  args().addParameter("pplistener-password","Password for the pptalker to connect to the pplisteners","");
  args().addParameter("backends","Comma or space separated list of PPListener backends","1:1:127.0.0.1");
  args().ignore("run-as-uid");
  args().ignore("run-as-gid");
  args().ignore("daemon");
  args().addCommand("help","Display this helpful message");

  L.toConsole(true);
  vector<string>words;

  try {
    string response;

    args().preparseArgs(argc, argv,"help");
    if(argc==1 || args().commandGiven("help")) {
      printUsage();
      exit(0);
    }
    args().preparseArgs(argc, argv,"config-dir");
    if(args().parseFiles(response, (args().paramString("config-dir")+"/power.conf").c_str(),0))
      L<<Logger::Warning<<"Warning: unable to open common configuration file '"<<
	args().paramString("config-dir")+"/power.conf"<<"'"<<endl;

    args().parseArgs(argc, argv);
    args().getRest(argc,argv,words);
  }
  catch(ArgumentException &e) {
    cerr<<"Unable to parse argument:"<<endl<<"\t"<<e.txtReason()<<endl;
    exit(0);
  }

  vector<string>backendargs;
  stringtok(backendargs, args().paramString("backends"),", ");


  for(vector<string>::const_iterator i=backendargs.begin();i!=backendargs.end();++i) {
    vector<string>parts;
    BackendData bd;
    int port=1101;

    stringtok(parts,*i,":");
    if(parts.size()<3)
      die("Invalid backend '"+*i+"' lacks fields");

    //bd.group=atoi(parts[0].c_str());
    //bd.priority=atoi(parts[1].c_str());
    bd.address=parts[2];
    if(parts.size()>3)
      port=atoi(parts[3].c_str());
    bd.port=port;
    //td.password=password;
    ostringstream s;
    s<<bd.address;
    if(bd.port!=1101)
      s<<":"<<bd.port;
    maxBackendLen=max((unsigned char)(s.str().size()+3),maxBackendLen);

    backends.push_back(bd);
  }

  if(words.empty()) {
    printUsage();
    exit(0);
  }
  const string &command=words[0];

  L.toConsole(false);
  try {

    if(UserBaseRepository()[args().paramString("userbase")])
      UB=(*UserBaseRepository()["mysqlpdns"])();
    else {
      L<<Logger::Error<<"Unknown userbase '"<<args().paramString("userbase")<<"'"<<endl;
      exit(1);
    }


    if(!UB->connected()) 
      L<<Logger::Error<<"No connection to the database! Will not be able to get user data."<<endl;

    if(command=="status" || command=="stat")
      statCommand(words);
    else if(command=="purge" && words.size()>1)
      nukeCommand(words);
    else if(command=="list") {
      if(words.size()!=2) 
	die("list accepts only 1 mailbox as argument");
      listCommand(words[1]);
    }
    else if(command=="usage") {
       if (words.size() != 2)
          die("usage accepts only 1 mailbox as argument");
       usageCommand(words[1]);
    }
    else if(command=="block") {
      if(words.size()<2) 
	die("block needs parameter(s)");
      blockCommand(words);
    }
    else if(command=="unblock") {
      if(words.size()<2) 
	die("unblock needs parameter(s)");
      unblockCommand(words);
    }
    else if(command=="dir" && words.size()==1) {
      dirMboxes();
    }
    else if(command=="dir" && words.size()>1) {
      dirMboxes(words);
    }
    else if(command=="ldir" && words.size()==1) {
      dirMboxes(true);
    }
    else if(command=="ldir" && words.size()>1) {
      dirMboxes(words,true);
    }
    else if(command=="orphans" && words.size()==1) {
      listOrphans();
    }
    else if(command=="purge-orphans" && words.size()==1) {
      nukeOrphans();
    }
    else if(command=="crypt") {
      doCrypt(words);
    }
    else if(command=="md5") {
      doMD5(words);
    }
    else {
      printUsage();
      exit(1);
    }
    exit(degraded);
  }
  catch(Exception &e) {
    L<<Logger::Error<<"Fatal error in pptool main: "<<e.reason<<endl;
  }
  catch(TalkerException &e) {
    L<<Logger::Error<<"Fatal error talking to backend: "<<e.getReason()<<endl;
  }
  exit(1);
}
