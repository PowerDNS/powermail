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
// $Id: oracle.cc,v 1.2 2003-02-04 12:35:26 ahu Exp $

#include "logger.hh"
#include "argsettings.hh"
#include "common.hh"
#include "oracle.hh"

extern Logger L;

UserBase *OracleUserBase::maker()
{
   return new OracleUserBase
     (
       args().paramString("oracle-home"),
       args().paramString("oracle-sid"),       
       args().paramString("oracle-database"),
       args().paramString("oracle-username"),
       args().paramString("oracle-password"),
       args().paramString("oracle-query")
     );
}

OracleUserBase::OracleUserBase(const string &home, const string &sid, const string &database, const string &username,
   const string &password, const string &query)
{
   d_home = home;
   d_sid = sid;   
   d_database = database;
   d_username = username;
   d_password = password;
   d_query = query;
//   cout<<"Query: '"<<d_query<<"'"<<endl;
   mEnvironmentHandle = NULL;
   mErrorHandle = NULL;
   mServiceContextHandle = NULL;
   mStatementHandle = NULL;
   
   if (this->connect() == false) {
      // XXX Wat nu?
     
      // nou, niet zoveel, gewoon bij iedere query opnieuw proberen - ahu.
   }
}

OracleUserBase::~OracleUserBase()
{
   this->cleanup();
}

/** returns -1 for a database error, 0 for ok */
int OracleUserBase::mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect, const string &challenge)
{
   int theResult = -1;
   error="Temporary database error (unspecified)";
   exists = pwcorrect = false;

   try
   {
      //
      // Execute the statement
      //
      string mboxUPPER(mbox);
      if(args().switchSet("oracle-uppercase")) {
	for(string::iterator i=mboxUPPER.begin();i!=mboxUPPER.end();++i) {
	  *i=toupper(*i);
	}
      }

      strncpy(mQueryName, mboxUPPER.c_str(), sizeof(mQueryName));
      
      dsword theQueryResult = OCIStmtExecute(mServiceContextHandle, mStatementHandle, mErrorHandle, 1, 0,
         (OCISnapshot *)NULL, (OCISnapshot*) NULL, OCI_DEFAULT);
   
      if (theQueryResult != OCI_SUCCESS && theQueryResult != OCI_SUCCESS_WITH_INFO && theQueryResult != OCI_NO_DATA) {
         throw OracleException(mErrorHandle);
      }
   
      //
      // Fetch the result
      //
#if 0      
      theQueryResult = OCIStmtFetch(mStatementHandle, mErrorHandle, 1, 0, 0);
      if (theQueryResult != OCI_SUCCESS && theQueryResult != OCI_SUCCESS_WITH_INFO && theQueryResult != OCI_NO_DATA) {
	 throw new OracleException(mErrorHandle);  // ? - ahu
      }
#endif
      theResult=0;
      if (theQueryResult != OCI_NO_DATA)
      {         
         md.canonicalMbox = mboxUPPER;
         md.isForward = true;
         md.fwdDest = mResultDestination;
         
         exists = true;
      }
      else
	error="No such mailbox found";
   }
   
   catch (OracleException &theException)
   {
      L << Logger::Error << "[OracleUserBase] Fetch failed: " << theException.Reason() << endl;
      error="Oracle Backend failure";
      theResult = -1;
   }

   return theResult;
}

bool OracleUserBase::connect()
{
   sword err;
   bool ok = true;

   try
   {
      //
      // Set ORACLE_HOME & ORACLE_SID
      //

      if (setenv("ORACLE_HOME", d_home.c_str(), 1) == -1) {
         throw OracleException("Cannot set ORACLE_HOME");
      }

      if (setenv("ORACLE_SID", d_sid.c_str(), 1) == -1) {
         throw OracleException("Cannot set ORACLE_SID");
      }

      //
      // Initialize and create the environment
      //

      L << Logger::Warning << "[OracleUserBase] ORACLE_HOME = '" << d_home << "' ORACLE_SID = '"
        << d_sid << "'" << endl;
  
      err = OCIInitialize(OCI_THREADED, 0,  NULL, NULL, NULL);
      if (err) {
	 throw OracleException("OCIInitialize");
      }

      err = OCIEnvInit(&mEnvironmentHandle, OCI_DEFAULT, 0, 0);
      if (err) {
	 throw OracleException("OCIEnvInit");
      }
  
      //
      // Allocate an error handle
      //
      
      err = OCIHandleAlloc(mEnvironmentHandle, (dvoid**) &mErrorHandle, OCI_HTYPE_ERROR, 0, NULL);
      if (err) {
	 throw OracleException("OCIHandleAlloc");
      }
  
      //
      // Logon to the database
      //
      
      const char *username = d_username.c_str();
      const char *password = d_password.c_str();
      const char *database = d_database.c_str();
      const char *query    = d_query.c_str();

      L << Logger::Warning << "[OracleUserBase] Connecting to database '" << d_database << "' as "
        << d_username << "/" << d_password << endl;

      err = OCILogon(mEnvironmentHandle, mErrorHandle, &mServiceContextHandle, (OraText*) username, strlen(username),
        (OraText*) password,  strlen(password), (OraText*) database, strlen(database));
      
      if (err) {
	 throw OracleException(mErrorHandle);
      }

      //
      // Allocate the statement handles, and prepare the statements
      //

      err = OCIHandleAlloc(mEnvironmentHandle, (dvoid **) &mStatementHandle, OCI_HTYPE_STMT, 0, NULL);
	 
      if (err) {
         throw OracleException(mErrorHandle);
      }

      err = OCIStmtPrepare(mStatementHandle, mErrorHandle, (text*) query, strlen(query),
         OCI_NTV_SYNTAX, OCI_DEFAULT);

      if (err) {
         throw OracleException(mErrorHandle);
      }

      //
      // Bind query arguments. Query is:
      //
      //  select Destination from MailForwards where Name = :name
      //
      
      OCIBind *theBindHandle = NULL;
      
      err = OCIBindByName(mStatementHandle, &theBindHandle, mErrorHandle, (OraText*) ":name", strlen(":name"),
         (ub1*) mQueryName, sizeof(mQueryName), SQLT_STR, NULL, NULL, 0, 0, NULL, OCI_DEFAULT);

      if (err) {
         throw OracleException(mErrorHandle);
      }

      //
      // Bind query result
      //

      OCIDefine *theDefineHandle;

      mResultDestinationIndicator = 0;

      theDefineHandle = NULL; 
      err = OCIDefineByPos(mStatementHandle, &theDefineHandle, mErrorHandle, 1, mResultDestination,
         sizeof(mResultDestination) - 1, SQLT_STR, (dvoid*) &mResultDestinationIndicator, NULL, NULL, OCI_DEFAULT);
      
      if (err) {
         throw OracleException(mErrorHandle);
      }      

      L << Logger::Notice << "[OracleUserBase] Connected" << endl;
   }
   
   catch (OracleException &theException)
   {
      L << Logger::Error << "[OracleUserBase] Connection to database failed: " << theException.Reason() << endl;
      this->cleanup();
      ok = false;
   }
   
   return ok;
}

bool OracleUserBase::connected()
{
   return true; // XXX Fixme. Oracle has a ping method.
}

void OracleUserBase::cleanup()
{
   if (mStatementHandle != NULL) {
      OCIHandleFree(mStatementHandle, OCI_HTYPE_STMT);
      mStatementHandle = NULL;
   }
   
   if (mServiceContextHandle != NULL) {
      sword error = OCILogoff(mServiceContextHandle, mErrorHandle);
      if (error != 0) {
         L << Logger::Error << "[OracleUserBase] OCILogoff returned a error (" << error << ")" << endl;
      }
   }

   if (mErrorHandle != NULL) {
      OCIHandleFree(mErrorHandle, OCI_HTYPE_ERROR);
      mErrorHandle = NULL;
   }
   
   if (mEnvironmentHandle != NULL) {
      OCIHandleFree(mEnvironmentHandle, OCI_HTYPE_ENV);
      mEnvironmentHandle = NULL;
   }   
   L << Logger::Error << "[OracleUserBase] Disconnected" << endl;
}

class OracleAutoLoaderClass
{
   public:
      
      OracleAutoLoaderClass()
      {
         UserBaseRepository()["oracle"] = &OracleUserBase::maker;
	 args().addParameter("oracle-home","ORACLE_HOME","/opt/oracle");
	 args().addSwitch("oracle-uppercase","UPPERCASE mailboxes before searching",true);
	 args().addParameter("oracle-sid","Oracle SID","");
	 args().addParameter("oracle-database","Oracle database","");
	 args().addParameter("oracle-username","Oracle username","");
	 args().addParameter("oracle-password","Oracle password","");
	 args().addParameter("oracle-query","Oracle Query",       "select Destination from MailForwards where Name = :name and Active = 1" );
	 /*
	 args().addParameter("oracle-query","Oracle Query", "select fldcontent from tbldnshost,tbldnshosttype,tbldomaindnshost,tbldomain
where tbldnshost.flddnshostnr=tbldomaindnshost.flddnshostnr
and tbldomain.flddomainnr=tbldomaindnshost.flddomainnr
and tbldnshost.flddnshosttypenr=tbldnshosttype.flddnshosttypenr and flddnshosttype='MBOXFW'
and fldname = :name");
	 */

      }
};

static OracleAutoLoaderClass OracleAutoLoader;
