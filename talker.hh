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
