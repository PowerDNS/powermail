#ifndef POSTGRESQLUSERBASE_HH
#define POSTGRESQLUSERBASE_HH
#include <string>
#include "lock.hh"
#include "userbase.hh"
#include <libpq++.h>

using namespace std;

class PostgreSQLUserBase : public UserBase
{
public:
  PostgreSQLUserBase(const string &database, const string &host,  
		     const string &msocket, const string &user, 
		     const string &password, const string &query);
  ~PostgreSQLUserBase();
  static UserBase *maker();

  int mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect);  
 
  bool connected(); 

private:
  string sqlEscape(const string &name);
  bool tryConnect();
  PgDatabase *d_db;
  string d_database, d_host, d_socket, d_user, d_password, d_query;
};

#endif /* POSTGRESQLUSERBASE_HH */

