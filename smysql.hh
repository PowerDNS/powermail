/* Copyright 2001 Netherlabs BV, bert.hubert@netherlabs.nl. See LICENSE 
   for more information.
   $Id: smysql.hh,v 1.1 2002-12-04 13:46:38 ahu Exp $  */
#ifndef SMYSQL_HH
#define SMYSQL_HH

#include <mysql/mysql.h>
#include "ssql.hh"

class SMySQL : public SSql
{
public:
  SMySQL(const string &database, const string &host="", 
	 const string &msocket="",const string &user="", 
	 const string &password="");

  ~SMySQL();
  
  SSqlException sPerrorException(const string &reason);
  int doQuery(const string &query, result_t &result);
  string escape(const string &str);    
private:
  MYSQL d_db;
};
      
#endif /* SSMYSQL_HH */
