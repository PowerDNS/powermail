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
#ifndef TEXTUSERBASE_HH
#define TEXTUSERBASE_HH
#include <string>
#include <time.h>
#include <vector>
#include "lock.hh"
#include "userbase.hh"
#include "textparser.hh"

using namespace std;

class TextUserBase : public UserBase
{
public:
  TextUserBase(const string &fname);
  ~TextUserBase();
  static UserBase *maker();

  int mboxData(const string &mbox, MboxData &md, const string &password, string &error, bool &exists, bool &pwcorrect);

  bool connected(); 

private:
  void tryParse();
  vector<TextBaseEntry> d_textbase; 
  string d_fname;
  bool d_parsed;
  static pthread_mutex_t d_launchlock;
  time_t d_last_mtime;
};

#endif /* MYSQLUSERBASE_HH */

