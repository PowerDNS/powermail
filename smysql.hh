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
/* Copyright 2001 Netherlabs BV, bert.hubert@netherlabs.nl. See LICENSE 
   for more information.
   $Id: smysql.hh,v 1.2 2002-12-04 16:32:49 ahu Exp $  */
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
