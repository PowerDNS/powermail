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
// $Id: mysqlpdns.cc,v 1.3 2003-02-04 12:35:26 ahu Exp $

#include "mysqlpdns.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "common.hh"
#include <stdlib.h>

extern Logger L;


UserBase *MySQLPDNSUserBase::maker()
{
  return new MySQLPDNSUserBase(args().paramString("mysql-database"),
			   args().paramString("mysql-host"),
			   args().paramString("mysql-socket"),
			   args().paramString("mysql-user"),
			   args().paramString("mysql-password"),
                           args().paramString("mysql-socket"));
}

MySQLPDNSUserBase::MySQLPDNSUserBase(const string &database, const string &host,  
			     const string &msocket, const string &user,
                             const string &password, const string &socket)
{
   d_database=database;
   d_host=host;
   d_socket=msocket;
   d_user=user;
   d_password=password;
   d_socket=socket;
   
   d_db=0;
   tryConnect();
}

MySQLPDNSUserBase::~MySQLPDNSUserBase()
{
   if(d_db) {
      delete d_db;
      d_db=0;
   }
}

SSql *MySQLPDNSUserBase::tryConnect()
{
   if(!d_db) {
      try {
         d_db=new SMySQL(d_database, d_host, d_socket, d_user, d_password);
         L<<Logger::Warning<<"MySQLPDNS: Connected to the database"<<endl;
      }
      catch(SSqlException &e) {
         L<<Logger::Error<<"Connecting to database: "<<e.txtReason()<<endl;
      }
   }
   
   return d_db;
}

bool MySQLPDNSUserBase::connected()
{
   return !!d_db;
}

/** returns -1 for a database error, 0 for ok */
int MySQLPDNSUserBase::mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect, const string &challenge)
{
   exists=pwcorrect=false;
   int ret = -1; // temporary failure by default
   error = "Temporary database error (not connected)";
   
   if (!d_db || !tryConnect()) 
     return -1;
     
   // Do we have a mail forward
   try
   {
      if (!exists) {
	 string query = "select m.Destination from MailForwards m,Zones z where m.ZoneId = z.Id and z.Active = 1" +
	    string(" and m.Name = lower('") + d_db->escape(mbox) + string("') and m.Active = 1");
	 
	 SSql::result_t result;
	 d_db->doQuery(query, result);
	 
	 // 'if 1 match, welcome!'
	 
	 if (result.size() != 0) {
	    ret = 0;
	    md.canonicalMbox      = mbox;
	    md.mbQuota   = 0;
	    md.isForward = true;
	    
	    md.fwdDest   = result[0][0];
	    exists=true;
	 }
      }
      
      // Do we have a mailbox?
      
      if (!exists) {
	 string query = "select lower(m.Name), m.Quota, m.Password from Mailboxes m,Zones z where m.ZoneId = z.Id and z.Active = 1" +
	    string(" and m.Name = lower('") + d_db->escape(mbox) + string("') and m.Active = 1");
	 
	 SSql::result_t result;
	 d_db->doQuery(query, result);
	 
	 // 'if 1 match, welcome!'
	 
	 if (result.size() != 0) {
	    ret = 0;
	    md.canonicalMbox      = result[0][0];
	    md.mbQuota   = atoi(result[0][1].c_str());
	    md.isForward = false;
	    md.fwdDest   = "";
	    if (challenge.empty())
  	      pwcorrect=pwMatch(pass,result[0][2]);
	    else 
	      pwcorrect=md5Match(challenge,pass,result[0][2]);
	    exists=true;
	 }
      }
      
      
      if (!exists)
      {
	 error = "No such account. If you think this is not correct then please mail to support@powerdns.com.";
	 ret = 0;
      }
   }
   
   catch (SSqlException &e)
   {
      L << Logger::Error << "Database error verifying user: " << e.txtReason() << endl;
      error = "Temporary error verifying user/password";
      ret = -1;
   }


   return ret;
}

class MysqlPdnsAutoLoaderClass
{
public:
  MysqlPdnsAutoLoaderClass()
  {
    UserBaseRepository()["mysqlpdns"]=&MySQLPDNSUserBase::maker;
  }
};

static MysqlPdnsAutoLoaderClass MysqlPdnsAutoLoader;
