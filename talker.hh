#ifndef TALKER_HH
#define TALKER_HH
#include <string>
#include <vector>
#include "session.hh"
using namespace std;


class TalkerException
{
public:
  TalkerException(const string &reason) : d_reason(reason) {}
  const string &getReason() const
  {
    return d_reason;
  }
    
private:
  const string d_reason;
};

class Talker 
{
public:
  virtual ~Talker(){}

  virtual int addRecipient(const string &address, const string &index)=0;
  virtual int msgAddLine(const string &line)=0;
  virtual void blastMessageFD(const string &address, const string &index, size_t length, int fd, int limit=-1)=0;
  virtual int delMsg(const string &address, const string &index)=0;
  virtual int msgDone()=0;
  virtual int msgRollback()=0;
  virtual void ping(bool &readonly)=0;
  virtual void stat(int *kbytes, int *inodes=0, double *load=0)=0;
  virtual void startData()=0;

  typedef struct msgInfo_t 
  {
    string index;
    size_t size;
    string ppname;
  };

  typedef vector<msgInfo_t> mboxInfo_t;

  virtual void getMsgs(const string &mbox, mboxInfo_t& mbi)=0;
};

#endif /* TALKER.HH */
