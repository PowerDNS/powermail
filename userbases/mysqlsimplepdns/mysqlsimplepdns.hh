//
// File    : mysqlpdns.hh
// Version : $Id: mysqlsimplepdns.hh,v 1.1 2002-12-04 13:47:01 ahu Exp $
// Project : Express Deux
//

#ifndef MYSQLUSERBASE_HH
#define MYSQLUSERBASE_HH

#include <string>
#include "lock.hh"
#include "userbase.hh"
#include "smysql.hh"

using namespace std;

class MySQLSimplePDNSUserBase : public UserBase
{
   public:

  MySQLSimplePDNSUserBase(const string &database, const string &table, const string &host="",  
			  const string &msocket="",
			  const string &user="", const string &password="",
			  const string &socket="");
  static UserBase *maker();
  ~MySQLSimplePDNSUserBase();

  int mboxData(const string &mbox, MboxData &md, const string &password, string &error, bool &exists, bool &pwcorrect);

  bool connected(); 
  
private:
  
  SSql *tryConnect();
  string d_database, d_host, d_user, d_password, d_socket, d_table;
  SSql *d_db;
};

#endif /* MYSQLUSERBASE_HH */

