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
#include "userbase.hh"
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

/** pwmatch needs to check if the plaintext supplied password matches something 
    in the database. The database can contain:
    plaintext
    {crypt}boring-unix-crypt
    {md5}MD5-crypt
*/
bool UserBase::pwMatch(const string &supplied, const string &db)
{
  if(db.empty())
    return false;
  if(db[0]!='{' || (db[0]=='{' && db.find('}')==string::npos))   // database does not start with {, or does but there is no }
    return supplied==db;
  
  if(!db.find("{plain}"))
    return supplied==db.substr(db.find('}')+1);

  if(
     (!db.find("{crypt}") || !db.find("{md5}")) 
     && 
     db.length()>12) {

    char *match=strdup(crypt(supplied.c_str(), db.substr(db.find('}')+1).c_str()));
    
    bool matched=(match==db.substr(db.find('}')+1));
    free(match);
    return matched;
  }
  return false;
}
