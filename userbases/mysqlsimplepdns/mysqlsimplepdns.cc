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
// $Id: mysqlsimplepdns.cc,v 1.2 2002-12-04 16:32:50 ahu Exp $

#include "mysqlsimplepdns.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "common.hh"

extern Logger L;


UserBase *MySQLSimplePDNSUserBase::maker()
{
  return new MySQLSimplePDNSUserBase(args().paramString("mysql-database"),
				     args().paramString("mysqlsimplepdns-table"),
				     args().paramString("mysql-host"),
				     args().paramString("mysql-socket"),
				     args().paramString("mysql-user"),
				     args().paramString("mysql-password"),
				     args().paramString("mysql-socket"));
}

MySQLSimplePDNSUserBase::MySQLSimplePDNSUserBase(const string &database, const string &table, const string &host,  
			     const string &msocket, const string &user,
                             const string &password, const string &socket)
{
   d_database=database;
   d_host=host;
   d_socket=msocket;
   d_user=user;
   d_password=password;
   d_socket=socket;
   d_table=table;
   
   d_db=0;
   tryConnect();
}

MySQLSimplePDNSUserBase::~MySQLSimplePDNSUserBase()
{
   if(d_db) {
      delete d_db;
      d_db=0;
   }
}

SSql *MySQLSimplePDNSUserBase::tryConnect()
{
   if(!d_db) {
      try {
         d_db=new SMySQL(d_database, d_host, d_socket, d_user, d_password);
         L<<Logger::Info<<"Connected to the database"<<endl;
      }
      catch(SSqlException &e) {
         L<<Logger::Error<<"Connecting to database: "<<e.txtReason()<<endl;
      }
   }
   
   return d_db;
}

bool MySQLSimplePDNSUserBase::connected()
{
   return !!d_db;
}

/** returns -1 for a database error, 0 for ok */
int MySQLSimplePDNSUserBase::mboxData(const string &mbox, MboxData &md, const string &password, string &error, bool &exists, bool &pwcorrect)
{
   exists=pwcorrect=false;   

   int ret = -1; // temporary failure by default
   error = "Temporary database error (not connected)";
   
   if (d_db || tryConnect())
   {
     string query = "select content from "+d_table+" where name='"+d_db->escape(mbox)+"'";
     //     L<<Logger::Error<<"Query: '"<<query<<"'"<<endl;
     try {
       SSql::result_t result;
       d_db->doQuery(query, result);
	
       // 'if 1 match, welcome!'
	
       if (result.size()) {
	 ret = 0;
	 md.canonicalMbox      = mbox;
	 md.mbQuota   = 0;
	 md.isForward = true;
	 md.fwdDest   = result[0][0];
	 exists=true;
	 return 0;
       }
       else {
	 error = "No such account. If you think this is not correct then please mail to support@powerdns.com.";
	 exists=false;
	 return 0;
       }
     }
     catch(SSqlException &e) {
       L << Logger::Error << "Database error verifying user: " << e.txtReason() << endl;
       error = "Temporary error verifying user/password";
       return -1;
     }
   }
   
   return -1;
}

class MysqlSimplePDNSAutoLoaderClass
{
public:
  MysqlSimplePDNSAutoLoaderClass()
  {
    args().addParameter("mysqlsimplepdns-table","MySQL table to search","records");
    UserBaseRepository()["mysqlsimplepdns"]=&MySQLSimplePDNSUserBase::maker;
  }
};

static MysqlSimplePDNSAutoLoaderClass MysqlSimplePDNSAutoLoader __attribute__ ((init_priority(65535)));
