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
#include <stdlib.h>
#include "mysqlplain.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "common.hh"

extern Logger L;


UserBase *MySQLPlainUserBase::maker()
{
  return new MySQLPlainUserBase(args().paramString("mysql-database"),
			   args().paramString("mysql-host"),
			   args().paramString("mysql-socket"),
			   args().paramString("mysql-user"),
			   args().paramString("mysql-password"));
}

MySQLPlainUserBase::MySQLPlainUserBase(const string &database, const string &host, 
				       const string &msocket,  const string &user, 
				       const string password)
{
  d_database=database;
  d_host=host;
  d_socket=msocket;
  d_user=user;
  d_password=password;

  d_db=0;
  tryConnect();
}

MySQLPlainUserBase::~MySQLPlainUserBase()
{
  if(d_db) {
    delete d_db;
    d_db=0;
  }
}


SSql *MySQLPlainUserBase::tryConnect()
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

bool MySQLPlainUserBase::connected()
{
  return !!d_db;
}


/** returns -1 for a database error, 0 for 'exists' and 1 for 'does not exist */
int MySQLPlainUserBase::mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect, const string &challenge)
{
  int ret=-1; // temporary failure by default
  exists=pwcorrect=false;
  error="Temporary database error (not connected)";

  if(d_db || tryConnect()) { // connected, or reconnect helped
    string query="select quotaMB,isForward,fwdDest,password from mboxes where mbox='"+d_db->escape(mbox)+"'";
    
    try {
      SSql::result_t result;
      d_db->doQuery(query, result);
      if(result.size()==1) {// 'if 1 match, welcome!'
	ret=0;
	md.canonicalMbox=mbox;
	md.mbQuota=atoi(result[0][0].c_str());
	md.isForward=(result[0][1]=="1");
	if(md.isForward)
	  md.fwdDest=result[0][2];
	else {
	  md.fwdDest="";
	  if (challenge.empty())
            pwcorrect=pwMatch(pass,result[0][3]);
	  else 
	    pwcorrect=md5Match(challenge,pass,result[0][3]);
	}
	exists=true;
      }
      else {
	error="No such mailbox known";
      }
      return 0;
    }
    catch(SSqlException &e) {
      L<<Logger::Error<<"Database error verifying user: "<<e.txtReason()<<endl;
      error="Temporary error verifying user/password";
      ret=-1;
    }
  }
  return -1;
}


class MysqlPlainAutoLoaderClass
{
public:
  MysqlPlainAutoLoaderClass()
  {
    UserBaseRepository()["mysqlplain"]=&MySQLPlainUserBase::maker;
    args().addParameter("mysql-database","MySQL database to connect to","powermail");
    args().addParameter("mysql-host","MySQL host to connect to","127.0.0.1");
    args().addParameter("mysql-user","MySQL user to log on as","");
    args().addParameter("mysql-password","MySQL password","");
    args().addParameter("mysql-socket","MySQL socket","");
  }
};

static MysqlPlainAutoLoaderClass MysqlPlainAutoLoader;
