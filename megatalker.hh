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
#ifndef MEGATALKER_HH
#define MEGATALKER_HH
#include <string>
#include <vector>
#include <map>
#include <pthread.h>
#include "pptalker.hh"
#include "logger.hh"
#include "lock.hh"
using namespace std;


class MegaTalkerException
{
public:
  MegaTalkerException(const string &reason) : d_reason(reason) {}
  const string &getReason() const
  {
    return d_reason;
  }
    
private:
  const string d_reason;
};

class TargetData
{
public:
  TargetData() : up(false),pp_talker(0)
  {
  }

  string address;
  unsigned int port;
  int group;
  int priority;
  bool up;
  bool readonly;
  string password;

  bool used;
  int kbfree;
  int inodes;
  double load;
  string error;
  PPTalker *pp_talker;
  RCMutex lock;
};

/** Class used to talk to PPTalker backends. It contains a static vector of available backends, whose state is 
    determined by a thread you must start with the startStateThread static method. On construction, n backends will be 
    connected to to deliver messages to. 

    In order to list a mailbox, *all* backends must be queried. 
*/
class MegaTalker
{
public:
  MegaTalker(); 
  void openStorage(); //!< connects to n backends, as specified by setRedundancy. 
  ~MegaTalker();
  
  /** Use this to specify which backends are available */
  static void setBackends(const vector<string>&backends, const string &password); 
  /** Sets the number of backends that will receive a copy of any message you submit. Must be set before creation of first MegaTalker! */
  static void setRedundancy(unsigned int red);
  /** When a message cannot be delivered, or deleted, this must be logged. This logged is later replayed when backends become available again */
  static void setStateDir(const string &statedir);
  /** Call this once for each recipient of a message. Will be announced on receipt of 'DATA' command */
  int addRecipient(const string &address, const string &index);

  void addRemote(const string &from, const string &to, const string &index);

  /** This delivers a message from any available backend to the specified filedescriptor. Up to limit lines are sent, where 0 stands for infinite */
  void blastMessageFD(const string &index, size_t length, int fd, int limit=-1);
  /** Retrieve all messages for a certain mbox, from all backends. Also sets the mbox, so later commands know which mbox you are refering to */

  /** For message status */
  struct msgInfo_t 
  {
    string index;
    size_t size;
  };
  /** A vector containing mailbox status for the current mailbox */
  typedef vector<msgInfo_t> mboxInfo_t;


  /** Lists a mailbox, gathers data from all available backends */
  void getMsgs(const string &mbox, mboxInfo_t& mbi);
  /** Delete a message with a certain index, from the mbox specified with getMsgs()  */
  void delMsg(const string &index);
  /** Initiate transmission of data. Throws a MegaTalker exception if it didn't work out */
  void startData();
  /** Add a line to a message. You must first make at lease one call to addRecipient! */
  int msgAddLine(const string &line);
  /** Terminate a message, and commit to all backends */
  int msgDone(); 
  /** Terminate a message, and discard on all backends */
  int msgRollback(); 
  /** Call this to figure out if we are in full redundance */
  bool isDegraded();


  /** This thread must be started before the first MegaTalker can be constructed. It connects to all backends 
      specified with setBackends, to determine their state. */
  static void startStateThread();
private:
  int d_msgnum;
  static void* doStateThread(void *);
  static void playbackLog(const string &address, unsigned int port, PPTalker *ppt);
  Session *d_session;
  struct TalkerData 
  {
    Talker *talker;
    string address;
    unsigned int port;
    bool ok;
  };
  string d_index;
  vector<TalkerData> d_pptalkers;
  static vector<TargetData> s_targets;
  map<TargetData *,PPTalker::mboxInfo_t> d_megaData;
  bool d_inmsg;
  bool d_degraded;
  string d_mbox;
  void storeFailure(const string &backendName, const string &message);

  static unsigned int s_redundancy;
  static pthread_t s_statethreadtid;
  static pthread_mutex_t s_statelock;
  static string s_statedir;
};

#endif /* MEGATALKER_HH */
