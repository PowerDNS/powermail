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
#include "megatalker.hh"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "argsettings.hh"
#include "lock.hh"
#include "logger.hh"
#include "misc.hh"
#include "serversel.hh"
#include "smtptalker.hh"
#include "common.hh"
using namespace std;

extern Logger L;

vector<TargetData>MegaTalker::s_targets;
unsigned int MegaTalker::s_redundancy;
pthread_t MegaTalker::s_statethreadtid;
pthread_mutex_t MegaTalker::s_statelock=PTHREAD_MUTEX_INITIALIZER;

string MegaTalker::s_statedir;

void MegaTalker::setStateDir(const string &statedir)
{
  s_statedir=statedir;
  mkdir(s_statedir.c_str(),0700);
  if(access(s_statedir.c_str(),W_OK))
    throw MegaTalkerException("State dir '"+s_statedir+"' is unwriteable: "+strerror(errno));
}


void MegaTalker::setRedundancy(unsigned int red)
{
  s_redundancy=red;
}

void MegaTalker::setBackends(const vector<string>&backends, const string &password)
{
  for(vector<string>::const_iterator i=backends.begin();i!=backends.end();++i) {
    TargetData td;

    int port=1101;
    vector<string>parts;

    // syntax is: group:priority:server-address:port

    stringtok(parts,*i,":");
    if(parts.size()<3)
      throw MegaTalkerException("Invalid backend '"+*i+"' lacks fields");

    td.group=atoi(parts[0].c_str());
    td.priority=atoi(parts[1].c_str());
    td.address=parts[2];
    if(parts.size()>3)
      port=atoi(parts[3].c_str());
    td.port=port;
    td.password=password;
    td.up=false;
    pthread_mutex_init(td.lock,0);
    s_targets.push_back(td);
  }
}

MegaTalker::~MegaTalker()
{
  for(vector<TalkerData>::iterator i=d_pptalkers.begin();
      i!=d_pptalkers.end();++i) {
    if(i->talker) {
      delete i->talker;
      i->talker=0;
    }
  }
}

void MegaTalker::startStateThread()
{
  pthread_mutex_lock(&s_statelock); // prevent connections from being made till the statethread is ready
  pthread_create(&s_statethreadtid, 0, doStateThread,0); 
}

void *MegaTalker::doStateThread(void *)
{
  int first=1;
  int round=-1;
  long long kbfreetotal=0; // YEAH!
  for(;;) {
    round++;
    for(vector<TargetData>::iterator i=s_targets.begin();i!=s_targets.end();++i) {
      Lock l(i->lock);
      if(i->up && i->pp_talker) {
	try {
	  bool wasreadonly=i->readonly;
	  i->pp_talker->ping(i->readonly); 
	  if(!wasreadonly && i->readonly) 
	    L<<Logger::Error<<"Backend "<<i->address<<":"<<i->port<<" now readonly"<<endl;
	  if(wasreadonly && !i->readonly)
	    L<<Logger::Error<<"Backend "<<i->address<<":"<<i->port<<" now available for write access again"<<endl;

	  if(s_statedir.empty() && !(round%60)) {
  	    i->pp_talker->stat(&i->kbfree, &i->inodes, &i->load);
	    kbfreetotal+=i->kbfree;
	    //	    L<<Logger::Error<<"Backend "<<i->address<<":"<<i->port<<" has "<<i->kbfree<<" kbytes available"<<endl;
	  }
	}
	catch(TalkerException &e) {// error
	  if(i->pp_talker) 
	    delete i->pp_talker;
	  i->pp_talker=0;
	  i->up=false;
	  L<<Logger::Error<<"State change, storage backend "<<i->address<<":"<<i->port<<" down: "<<e.getReason()<<endl;

	  continue;
	}
      }
      else { // currently down
	if(i->pp_talker) {
	  delete i->pp_talker;
	  i->pp_talker=0;
	}
	try {
	  i->pp_talker=new PPTalker(i->address,i->port,i->password,10);
	  i->pp_talker->ping(i->readonly);
	  
	  L<<Logger::Error<<"State change, storage backend "<<i->address<<":"<<i->port<<" up!"<<endl;
	  i->up=true;
	  i->error="";
	  if(s_statedir.empty() && !(round%60)) {
  	    i->pp_talker->stat(&i->kbfree, &i->inodes, &i->load);
	    L<<Logger::Error<<"Backend "<<i->address<<":"<<i->port<<" has "<<i->kbfree<<" kbytes available"<<endl;
	    kbfreetotal+=i->kbfree;
	  }
	  
	  if(!s_statedir.empty()) // not in SMTP!
	    playbackLog(i->address, i->port,i->pp_talker);

	}
	catch(PPTalkerException &e){
	  if(first || e.getReason()!=i->error)
	    L<<Logger::Error<<"Backend connection failed for "<<i->address<<":"<<i->port<<": "<<e.getReason()<<endl;
	  i->error=e.getReason();

	} // still down
      }
    }
    if(first) {
      pthread_mutex_unlock(&s_statelock); // we're done, rest of MegaTalker can start to function
      first=0;
    }
    if(s_statedir.empty() && !(round%60)) {
      L<<Logger::Error<<"Backends have "<<kbfreetotal<<" kbytes available"<<endl;
      kbfreetotal=0;
    }
    sleep(10);
  }
  return 0;
}

void MegaTalker::addRemote(const string &from, const string &to, const string &index)
{
  TalkerData td;
  td.talker=0;
  try {
    td.talker=new SmtpTalker(args().paramString("smtp-sender-address"),args().paramAsNum("smtp-sender-port"),from,60);
    td.talker->addRecipient(to,"");
    td.ok=true;
    td.address=args().paramString("smtp-sender-address");
    td.port=args().paramAsNum("smtp-sender-port");
    d_pptalkers.push_back(td);
    d_index=index;
  }
  catch(SmtpTalkerException &e) {
    delete td.talker;
    throw MegaTalkerException("Announcing remote <"+to+"> to sender failed: "+e.getReason());
  }
}

//!< Choose s_redundancy backends for storage
void MegaTalker::openStorage()
{
  int res;

  if((res=pthread_mutex_trylock(&s_statelock))) {
    L<<Logger::Warning<<"Waiting for backend states to be assessed: "<<strerror(res)<<endl;
    {
      Lock l(&s_statelock);
      L<<Logger::Warning<<"Done, backend states have been assessed"<<endl;
    }
  }
  else
    pthread_mutex_unlock(&s_statelock);

  // select s_redundancy targets

  ServerSelect ss(s_targets); // danger?
  for(unsigned int n=0;n<s_redundancy;n++) {
    const TargetData *tdp=ss.getServer();
    if(!tdp)
      break;
    try {
      TalkerData td;
      td.talker=new PPTalker(tdp->address,tdp->port,tdp->password,60);
      td.ok=true;
      td.address=tdp->address;
      td.port=tdp->port;

      d_pptalkers.push_back(td);
      L<<Logger::Warning<<"Connected successfully to backend "<<tdp->address<<":"<<tdp->port<<endl;
    }
    catch (PPTalkerException &e) {
      L<<Logger::Warning<<"Unable to connect to "<<tdp->address<<", trying another backend:"<<tdp->port<<": "<<e.getReason()<<endl;
    }
  }
  if(d_pptalkers.empty())
    throw(MegaTalkerException("No backends available, unable to function"));

  if(d_pptalkers.size()<s_redundancy) {
    L<<Logger::Warning<<"Did not meet target of "<<s_redundancy<<" backends, operating in degraded mode"<<endl;
    d_degraded=true;
  }
}

MegaTalker::MegaTalker()
{
  d_degraded=false;
  d_msgnum=0;
}

bool MegaTalker::isDegraded()
{
  return d_degraded;
}

int MegaTalker::addRecipient(const string &recipient, const string &index)
{
  int n=0;
  d_msgnum++;
  for(vector<TalkerData>::iterator i=d_pptalkers.begin();
      i!=d_pptalkers.end();++i) {
    if(!i->ok)
      continue;
    

    if(!dynamic_cast<PPTalker *>(i->talker)) // only announce to PPtalkers! 
	continue;

    Talker *pp=i->talker;
    try {
      pp->addRecipient(recipient,index+"."+itoa(d_msgnum));
      L<<Logger::Warning<<"Announced message "<<recipient<<"/"<<index<<" to backend "<<i->address<<":"<<i->port<<endl;
      n++;
    }
    catch (PPTalkerException &e) {
      L<<Logger::Error<<"Fatal error adding recipient "<<recipient<<" to "<<i->address<<":"<<i->port<<": "<<e.getReason()<<endl;
      i->ok=false;
      d_degraded=true;
    }
  }
  if(!n)
    throw MegaTalkerException("Not a single backend accepted recipient");
  d_index=index;
  return 0;
}

void MegaTalker::startData()
{
  int success=0;
  for(vector<TalkerData>::iterator i=d_pptalkers.begin();
      i!=d_pptalkers.end();++i) {
    if(!i->ok)
      continue;

    Talker *pp=i->talker;
    try {
      pp->startData();
      success++;
    }
    catch (PPTalkerException &e) {
      L<<Logger::Error<<"Backend "<<i->address<<":"<<i->port<<" signals error starting data: "<<e.getReason()<<endl;
      i->ok=false;
      d_degraded=true;
    }
  }
  if(!success)
    throw MegaTalkerException("When sending start data command, not a single backend responded ok");
}


int MegaTalker::msgAddLine(const string &line)
{
  for(vector<TalkerData>::iterator i=d_pptalkers.begin();
      i!=d_pptalkers.end();++i) {
    if(!i->ok)
      continue;

    Talker *pp=i->talker;
    try {
      pp->msgAddLine(line);
    }
    catch (PPTalkerException &e) {
      L<<Logger::Error<<"Backend signals error adding line: "<<e.getReason()<<endl;
      i->ok=false;
      d_degraded=true;
    }
  }
  return 0;
}

//! Make a list of all messages known in this mailbox, and on which host they reside
void MegaTalker::getMsgs(const string &mbox, mboxInfo_t &data)
{
  d_mbox=mbox;
  d_megaData.clear();
  for(vector<TargetData>::iterator i=s_targets.begin();i!=s_targets.end();++i) {
    Lock l(i->lock);
    if(!i->up)
      continue;

    PPTalker *pp=i->pp_talker;
    try {
      PPTalker::mboxInfo_t ppData;
      pp->getMsgs(mbox, ppData); 
      d_megaData[&*i]=ppData;
    }
    catch (PPTalkerException &e) {
      L<<Logger::Error<<"Backend signals error listing mailbox: "<<e.getReason()<<endl;
      throw MegaTalkerException("Unable to list mailbox: "+e.getReason());
    }
  }

  map<string,int>msgs;

  for(map<TargetData *,PPTalker::mboxInfo_t>::const_iterator i=d_megaData.begin();
      i!=d_megaData.end();++i) {
    for(PPTalker::mboxInfo_t::const_iterator j=i->second.begin();j!=i->second.end();++j) {
      msgs[j->index]=j->size;
    }
  }

  data.clear();

  for(map<string,int>::const_iterator i=msgs.begin();i!=msgs.end();++i) {
    msgInfo_t mbi;
    mbi.index=i->first;
    mbi.size=i->second;
    data.push_back(mbi);
  }

}

int MegaTalker::msgDone()
{
  unsigned int success=0;
  for(vector<TalkerData>::iterator i=d_pptalkers.begin();
      i!=d_pptalkers.end();++i) {
    if(!i->ok)
      continue;

    Talker *pp=i->talker;
    try {
      pp->msgDone();
      L<<Logger::Warning<<"Backend "<<i->address<<":"<<i->port<<" committed "<<d_index<<" successfully"<<endl;
      success++;
    }
    catch (PPTalkerException &e) {
      L<<Logger::Error<<"Backend failed when committing message: "<<e.getReason()<<endl;
      i->ok=false;
      d_degraded=true;
    }
  }
  
  if(!success)
    throw MegaTalkerException("Did not deliver a single instance of message <"+d_index+">");

  if(d_degraded) { // only error if *storing* mail
    L<<Logger::Warning<<"Did not reach redundancy target ("<<s_redundancy<<") for <"<<d_index<<">"<<endl;

  }

  return 0;
}

int MegaTalker::msgRollback()
{
  for(vector<TalkerData>::iterator i=d_pptalkers.begin();
      i!=d_pptalkers.end();++i) {
    if(!i->ok)
      continue;

    Talker *pp=i->talker;
    try {
      pp->msgRollback();
      L<<Logger::Warning<<"Backend "<<i->address<<":"<<i->port<<" roled back "<<d_index<<" successfully"<<endl;
    }
    catch (PPTalkerException &e) {
      L<<Logger::Error<<"Backend failed rollback of message: "<<e.getReason()<<endl;
      // bummer (not a lot we can do)
    }
  }
  return 0;
}



void MegaTalker::storeFailure(const string &backendName, const string &message)
{
  string fname=s_statedir+"/"+backendName;
  ofstream ofs(fname.c_str());

  if(!ofs) { // luck is against us today, isn't it
    L<<Logger::Error<<"Failed to log failure action '"<<message<<"' for "<<backendName<<": "<<strerror(errno)<<endl;
  }
  else {
    ofs<<message<<endl; // backendName is the filename
  }
}

void MegaTalker::playbackLog(const string &address, unsigned int port, PPTalker *ppt)
{
  ostringstream fname;
  fname<<s_statedir+"/"+address<<":"<<port;

  fstream ifs(fname.str().c_str(),ios::in | ios::out);
  if(!ifs) {
    L<<Logger::Error<<"No failure backlog for "<<address<<":"<<port<<" ("<<fname.str()<<")"<<endl;
    return;
  }
  L<<"Starting failure log playback for "<<address<<":"<<port<<endl;
  string line;
  vector<string> words;
  streampos sp;
  bool keeplog=false;
  while(sp=ifs.tellg(),getline(ifs,line)) {
    if(line[0]!='?')  // no reason to keep the log 
      continue;

    // line looks like: '? del ahu@ds9a.nl 12343'
    stringtok(words,line);
    if(words.size()!=4) {
      stripLine(line);
      L<<"Unable to parse logline in file "<<fname.str()<<": '"<<line<<"'"<<endl;
      keeplog=true;
      continue;
    }
    if(words[1]!="del") {
      L<<"Unable to replay command '"<<words[1]<<"' in file "<<fname.str()<<endl;
      keeplog=true;
      continue;
    }
    try {
      ppt->delMsg(words[2],words[3]);
      streampos nlp=ifs.tellg();
      ifs.seekg(sp);
      ifs<<"!";  // mark as done!
      ifs.seekg(nlp); 
      L<<"Successfully replayed delete of "<<words[2]<<" "<<words[3]<<endl;
    }
    catch(PPTalkerException &e) {
      L<<"Tried to replay delete of "<<words[2]<<" "<<words[3]<<" but failed: "<<e.getReason()<<endl;
      keeplog=true;
      continue;
    }
  }
  if(!keeplog) {
    if(unlink(fname.str().c_str())<0)
      L<<Logger::Error<<"Unable to delete processed failure log '"<<fname.str()<<"': "<<strerror(errno)<<endl;
    else
      L<<Logger::Error<<"All commands in failure log '"<<fname.str()<<"' processed, deleted file"<<endl;
  }
  else
    L<<Logger::Error<<"Some unprocessed items remain in log '"<<fname.str()<<"'"<<endl;
}


// a delete gets broadcast to *all* backends
void MegaTalker::delMsg(const string &index)
{
  for(vector<TargetData>::iterator i=s_targets.begin();
      i!=s_targets.end();++i) {
    Lock l(i->lock);
    if(!i->up) {
      storeFailure(i->address+":"+itoa(i->port),"? del "+d_mbox+" "+index);
      continue;
    }
    try {
      i->pp_talker->delMsg(d_mbox,index);
    }
    catch(PPTalkerException &e) {
      storeFailure(i->address+":"+itoa(i->port),"? del "+d_mbox+" "+index);
      L<<Logger::Error<<"Error deleting message '"<<index<<"' from mailbox '"<<d_mbox<<"' on "<<
	i->address<<":"<<i->port<<": "<<e.getReason()<<endl;
    }
  }
}

void MegaTalker::blastMessageFD(const string &index, size_t length, int fd, int limit)
{
  vector<TargetData *>pptalkers;

  for(map<TargetData *,PPTalker::mboxInfo_t>::const_iterator i=d_megaData.begin();
      i!=d_megaData.end();++i) {

    for(PPTalker::mboxInfo_t::const_iterator j=i->second.begin();j!=i->second.end();++j) {
      if(index==j->index)
	pptalkers.push_back(i->first);
    }
  }
  
  //  L<<Logger::Error<<pptalkers.size()<<" candidates"<<endl;
  if(pptalkers.empty())
    throw MegaTalkerException("Index unavailable");

  // randomize here
  random_shuffle(pptalkers.begin(),pptalkers.end());
  try {
    PPTalker *pptp=new PPTalker(pptalkers[0]->address,pptalkers[0]->port,pptalkers[0]->password,60);
    pptp->blastMessageFD(d_mbox,index,length,fd,limit);
    delete pptp;
  }
  catch(PPTalkerException &e) {
    throw MegaTalkerException("While retrieving message: "+e.getReason());
  }
}
#ifdef DRIVER

int main(int argc, char **argv)
{
  L.toConsole(true);

  vector<string> backends;
  backends.push_back("127.0.0.1");
  backends.push_back("130.161.252.29:80");
  backends.push_back("1.2.3.4:80");
  backends.push_back("213.244.168.241");

  try {
    MegaTalker::setBackends(backends);
    MegaTalker::setRedundancy(2);
    MegaTalker::startStateThread();
    MegaTalker MT;
    

    MT.addRecipient("ahu@ds9a.nl",itoa(time(0)));
    MT.addRecipient("dave@powerdns.com",itoa(time(0)*2));

    MT.msgAddLine("From: ahu@ds9a.nl\r\n");
    MT.msgAddLine("To: ahu@ds9a.nl\r\n");
    MT.msgAddLine("\r\n");
    MT.msgAddLine("Hallo, hallo!\r\n");
    MT.msgAddLine("Regel 2\r\n");
    MT.msgDone();
    
    MegaTalker::mboxInfo_t data;
    MT.getMsgs("ahu@ds9a.nl", data);
    MegaTalker::mboxInfo_t::const_iterator i;
    for(i=data.begin();
	i!=data.end();
	++i)
	cout<<"'"<<i->index<<"', "<<i->size<<endl;

    --i;--i;
    MT.blastMessageFD(i->index,0);

    MT.delMsg(i->index);

  }
  catch(MegaTalkerException &e) {
    L<<Logger::Error<<"Fatal error: "<<e.getReason()<<endl;
  }
  
  L<<"Success!"<<endl;

}
#endif /* DRIVER */
