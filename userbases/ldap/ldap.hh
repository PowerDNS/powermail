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
#ifndef LDAPUSERBASE_HH
#define LDAPUSERBASE_HH
#include <string>
#include "lock.hh"
#include "userbase.hh"
#include "powerldap.hh"

using namespace std;

class LDAPUserBase : public UserBase
{
public:
  LDAPUserBase(const string &host);
  ~LDAPUserBase();
  static UserBase *maker();

  int mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect);  
 
  bool connected(); 

private:
  PowerLDAP *d_db;
  string d_host;
  map<string,string> d_map;
  string d_attribute;
  string d_alternate_attribute;
  string d_alternate_domain;
  string d_alternate_base;
  string d_forwarding_attribute;
};

#endif /* LDAPUSERBASE_HH */

