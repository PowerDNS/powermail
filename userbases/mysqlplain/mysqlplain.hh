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
#ifndef MYSQLUSERBASE_HH
#define MYSQLUSERBASE_HH
#include <string>
#include "lock.hh"
#include "userbase.hh"
#include "smysql.hh"

using namespace std;

class MySQLPlainUserBase : public UserBase
{
public:
  MySQLPlainUserBase(const string &database, const string &host="",  const string &msocket="", const string &user="", const string password="");
  ~MySQLPlainUserBase();
  static UserBase *maker();

  int mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect);  
 
  bool connected(); 

private:
  SSql *tryConnect();
  string d_database, d_host, d_socket, d_user, d_password;
  SMySQL *d_db;
};

#endif /* MYSQLUSERBASE_HH */

