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

