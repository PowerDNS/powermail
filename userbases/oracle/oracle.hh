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
// $Id: oracle.hh,v 1.2 2003-02-04 13:32:55 ahu Exp $

#ifndef ORACLEUSERBASE_HH
#define ORACLEUSERBASE_HH

#include <string>

#include "lock.hh"
#include "userbase.hh"

#include <oci.h>

using namespace std;

class OracleException
{
   public:
      
      OracleException()
      {
	 mReason = "Unspecified";
      }
      
      OracleException(string theReason)
      {
	 mReason = theReason;
      }

      OracleException(OCIError *theErrorHandle)
      {
	 mReason = "ORA-UNKNOWN";

	 if (theErrorHandle != NULL)
	 {
	    text  msg[512];
	    sb4   errcode = 0;
   
	    memset((void *) msg, (int)'\0', (size_t)512);
   
	    OCIErrorGet((dvoid *) theErrorHandle,1, NULL, &errcode, msg, sizeof(msg), OCI_HTYPE_ERROR);
	    if (errcode)
            {
	      char *p = (char*) msg;
	       while (*p++ != 0x00) {
		  if (*p == '\n' || *p == '\r') {
		    *p = ';';
		  }
		}
		
		mReason = (char*) msg;
	    }
	 }
      }

      string Reason()
      {
	 return mReason;
      }
      
      string mReason;
};

class OracleUserBase : public UserBase
{
   public:
  OracleUserBase(const string &home, const string &sid, const string &database, const string &username,
		 const string &password, const string &query);
  ~OracleUserBase();
  
  static UserBase *maker();
  
  int mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect, const string &challege);
private:
  
  bool connect();
  void cleanup();
  bool connected(); 
  
private:
  
  OCIEnv    *mEnvironmentHandle;
  OCIError  *mErrorHandle;
  OCISvcCtx *mServiceContextHandle;
  OCIStmt   *mStatementHandle;
  
  char      mQueryName[256];
  char      mResultDestination[256];
  sb2       mResultDestinationIndicator;
  
  string d_home, d_sid, d_database, d_username, d_password, d_query;
  pthread_mutex_t d_dblock;
};

#endif /* ORACLEUSERBASE_HH */
