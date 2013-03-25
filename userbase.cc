/*
    PowerMail versatile mail receiver
    Copyright (C) 2002 - 2009 PowerDNS.COM BV

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
#include <string.h>
#include "userbase.hh"
#include "misc.hh"
#include "md5.hh"
#include <iostream>

#ifdef __FreeBSD__
# include <unistd.h>
#else
# include <crypt.h>
#endif

map<string,UserBase*(*)()>& UserBaseRepository()
{
  static map<string,UserBase*(*)()> repo;
  return repo;
}

/** md5Match
 * This is the password matcher for a APOP request.
*/ 
bool UserBase::md5Match(const string &challenge,const string &supplied, const string &db) 
{
  // check if there actually IS a password in the database.
  if (db.empty())
    return false;
  
  string our_digit=challenge;
  
  // check if it's a normal password...
  if (db.find("{md5}") == string::npos && db.find("{crypt}") == string::npos) {
    if (db.find("{plain}") == string::npos) 
      our_digit=our_digit+db;
    else
      our_digit=our_digit+db.substr(7);
    
    our_digit=md5calc((unsigned char *)our_digit.c_str(),our_digit.size());
    return our_digit==supplied;
  }
  return false;  
}


/** pwmatch needs to check if the plaintext supplied password matches something 
    in the database. The database can contain:
    plaintext
    {crypt}boring-unix-crypt
    {md5}MD5-crypt
*/
bool UserBase::pwMatch(const string &supplied, const string &db)
{
  // check if there actually IS a password in the database.
  if (db.empty())
    return false;
  
  // check if it's a normal password...
  if (db.find("{md5}") == string::npos && db.find("{crypt}") == string::npos) {
    if (db.find("{plain}") == string::npos) {
      return supplied==db;
    } else {
      return supplied==(db.substr(7));
    }
  }
 
  string tmp;
  if (db.find("{md5}") != string::npos) {
    tmp = md5calc((unsigned char *)supplied.c_str(),supplied.size());
    return (strncmp(tmp.c_str(),db.substr(5).c_str(),strlen(db.substr(5).c_str())) == 0);
  } else {
    tmp=db.substr(7);
    char *match=strdup(crypt(supplied.c_str(), tmp.c_str()));
    bool matched=(match==db.substr(7));
    free(match);
    return matched;
  }
}
