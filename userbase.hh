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
#ifndef USERBASE_HH
#define USERBASE_HH
#include <string>
#include <map>
using namespace std;

struct MboxData
{
  string canonicalMbox;
  int mbQuota;
  bool isForward;
  string fwdDest;
};

class UserBase
{
public:
  static UserBase *maker();
  virtual int mboxData(const string &label, MboxData &md, const string &password, string &error, bool &exists, bool &pwcorrect)=0;
  virtual ~UserBase(){};
  virtual bool connected()=0;
protected:
  bool pwMatch(const string &supplied, const string &db);
};
map<string,UserBase*(*)()>& UserBaseRepository();
#endif /* USERBASE_HH */
