#ifndef SMTPTALKER_HH
#define SMTPTALKER_HH

#include "talker.hh"
typedef TalkerException SmtpTalkerException;
class SmtpTalker : public Talker
{
public:
  SmtpTalker(const string &dest, u_int16_t port, const string &from, int timeout=60);
  ~SmtpTalker();

  int addRecipient(const string &address, const string &index);
  int msgAddLine(const string &line);
  int msgDone(); 
  int msgRollback(); 
  void startData();

  /******* Penguin problem ********/

  void blastMessageFD(const string &address, const string &index, size_t length, int fd, int limit=-1){no();}
  int delMsg(const string &address, const string &index){return no();}
  void ping(bool &readonly){no();}
  void stat(int *kbytes, int *inodes=0, double *load=0){no();}
  void getMsgs(const string &mbox, mboxInfo_t& mbi){no();}

private:
  int no() 
  {
    throw TalkerException("Tried to retrieve a message over SMTP!");
    return 0;
  }
  Session *d_session;
  bool d_inmsg;
};
#endif
