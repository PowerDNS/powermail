//
// File    : mysqlpdns.hh
// Version : $Id: mysqlpdns.hh,v 1.1 2002-12-04 13:47:01 ahu Exp $
//

#ifndef MYSQLUSERBASE_HH
#define MYSQLUSERBASE_HH

#include <string>
#include "lock.hh"
#include "userbase.hh"
#include "smysql.hh"

using namespace std;

class MySQLPDNSUserBase : public UserBase
{
   public:

      MySQLPDNSUserBase(const string &database, const string &host="",  
		    const string &msocket="",
		    const string &user="", const string &password="",
                    const string &socket="");
  static UserBase* maker();
  ~MySQLPDNSUserBase();

  int mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect);
  bool connected(); 
  
private:
  
  SSql *tryConnect();
  string d_database, d_host, d_user, d_password, d_socket;
  SMySQL *d_db;
};

#endif /* MYSQLUSERBASE_HH */

