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
#include "postgresql.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "common.hh"

extern Logger L;


UserBase *PostgreSQLUserBase::maker()
{
  return new PostgreSQLUserBase(args().paramString("postgresql-database"),
				args().paramString("postgresql-host"),
				args().paramString("postgresql-socket"),
				args().paramString("postgresql-user"),
				args().paramString("postgresql-password"),
				args().paramString("postgresql-query"));
}

PostgreSQLUserBase::PostgreSQLUserBase(const string &database, const string &host, const string &msocket, 
				       const string &user, const string &password, const string &query)
{
  d_database=database;
  d_host=host;
  d_socket=msocket;
  d_user=user;
  d_password=password;
  d_query=query;
  d_db=0;
  tryConnect();
}

PostgreSQLUserBase::~PostgreSQLUserBase()
{
  if(d_db) {
    delete d_db;
    d_db=0;
  }
}


bool PostgreSQLUserBase::tryConnect()
{
  if(!d_db) {
    string connectstr;
    connectstr="dbname=";
    connectstr+=d_database;
    if(!d_user.empty()) {
      connectstr+=" user=";
      connectstr+=d_user;
    }
    
    if(!d_host.empty())
      connectstr+=" host="+d_host;
    
    if(!d_password.empty())
      connectstr+=" password="+d_password;

    L<<Logger::Warning<<"Connecting to database with connect string '"<<connectstr<<"'"<<endl;
    
    d_db=new PgDatabase(connectstr.c_str());
    
    // Check to see that the backend connection was successfully made
    if (d_db->ConnectionBad() ) {
      L<<Logger::Error << "Connection to database failed, connect string was '" <<connectstr<<"': "
       << d_db->ErrorMessage() << endl;
      delete d_db;
      d_db=0;
    }
  }
  return d_db;
}
  
bool PostgreSQLUserBase::connected()
{
  return !!d_db;
}

string PostgreSQLUserBase::sqlEscape(const string &name)
{
  string a;

  for(string::const_iterator i=name.begin();i!=name.end();++i)

    if(*i=='\'' || *i=='\\'){
      a+='\\';
      a+=*i;
    }
    else
      a+=*i;
  return a;
}


/** returns -1 for a database error, 0 for 'exists' and 1 for 'does not exist */
int PostgreSQLUserBase::mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect)
{
  exists=pwcorrect=false;
  error="Temporary database error (not connected)";

  if(d_db || tryConnect()) { // connected, or reconnect helped
    char format[1024];
    snprintf(format,1023,d_query.c_str(),mbox.c_str());

    if(!d_db->ExecTuplesOk(format)) {
      L<<Logger::Error<<"command failed: '" << d_db->ErrorMessage() << "'" << endl;
      return -1;
    }
    if(!d_db->Tuples()) {
      error="No such mail address known";
      return 1;
    }
    
    md.mbQuota=atoi(d_db->GetValue(0,0)); // 0,1 etc
    md.canonicalMbox=mbox;
    md.isForward=(d_db->GetValue(0,1)=="1");
    if(md.isForward)
      md.fwdDest=d_db->GetValue(0,2);
    else {
      md.fwdDest="";
      pwcorrect=pwMatch(pass,d_db->GetValue(0,3));
    }
    exists=true;
  }
  if(exists)
    return 0;
  return -1;
}


class PostgreSQLAutoLoaderClass
{
public:
  PostgreSQLAutoLoaderClass()
  {
    UserBaseRepository()["postgresql"]=&PostgreSQLUserBase::maker;
    args().addParameter("postgresql-database","PostgreSQL database to connect to","powermail");
    args().addParameter("postgresql-host","PostgreSQL host to connect to","127.0.0.1");
    args().addParameter("postgresql-user","PostgreSQL user to log on as","");
    args().addParameter("postgresql-password","PostgreSQL password","");
    args().addParameter("postgresql-query",
			"PostgreSQL query",
			"select quotaMB,isForward,fwdDest,password from mboxes where mbox='%s'");
  }
};

static PostgreSQLAutoLoaderClass PostgreSQLAutoLoader;
