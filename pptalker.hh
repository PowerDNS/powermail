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
#ifndef PPTALKER_HH
#define PPTALKER_HH
#include <string>
#include <vector>
#include "session.hh"
#include "talker.hh"
using namespace std;

typedef TalkerException PPTalkerException;
class PPTalker : public Talker
{
public:
  PPTalker(const string &dest, u_int16_t port, const string &password, int timeout=60);
  ~PPTalker();

  int addRecipient(const string &address, const string &index);
  int msgAddLine(const string &line);
  void blastMessageFD(const string &address, const string &index, size_t size, int fd, int limit=-1);

  int delMsg(const string &address, const string &index);
  int msgDone(); 
  int msgRollback(); 
  int nukeMbox(const string &mbox);
  void ping(bool &readonly);
  void stat(int *kbytes, int *inodes=0,double *load=0);
  void startData();
  void getMsgs(const string &mbox, mboxInfo_t& mbi);
  void setReadonly(bool flag);
  void listMboxes(vector<string> &mboxes);
private:
  Session *d_session;
  bool d_inmsg;
  string d_ppname;
  int d_usage;
};
#endif
