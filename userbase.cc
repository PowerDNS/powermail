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
