/* Copyright 2001 Netherlabs BV, bert.hubert@netherlabs.nl. See LICENSE 
   for more information.
   $Id: smysql.cc,v 1.1.1.1 2002-12-04 13:46:38 ahu Exp $  */
#include "smysql.hh"
#include <string>
#include <iostream>

using namespace std;


SMySQL::SMySQL(const string &database, const string &host, const string &msocket, const string &user, 
	       const string &password)
{
  mysql_init(&d_db);
  if (!mysql_real_connect(&d_db, host.empty() ? 0 : host.c_str(), 
			  user.empty() ? 0 : user.c_str(), 
			  password.empty() ? 0 : password.c_str(),
			  database.c_str(), 0,
			  msocket.empty() ? 0 : msocket.c_str(),
			  0)) {
    throw sPerrorException("Unable to connect to database");
  }

}

SMySQL::~SMySQL()
{
  mysql_close(&d_db);
}

SSqlException SMySQL::sPerrorException(const string &reason)
{
  return SSqlException(reason+string(": ")+mysql_error(&d_db));
}

int SMySQL::doQuery(const string &query, result_t &result)
{
  result.clear();
  if(mysql_query(&d_db,query.c_str())) 
    throw sPerrorException("Failed to execute mysql_query");

  MYSQL_RES *rres;
    
  if(!(rres = mysql_use_result(&d_db)))
    throw sPerrorException("Failed on mysql_use_result");

  MYSQL_ROW rrow;

  int num=0;
  row_t row;

  while((rrow = mysql_fetch_row(rres))) {
    num++;
    row.clear();
    for(unsigned int i=0;i<mysql_num_fields(rres);i++)
      row.push_back(rrow[i] ?: "");
    
    result.push_back(row);
  }
  mysql_free_result(rres);
  return num;
}

string SMySQL::escape(const string &name)
{
  string a;

  for(string::const_iterator i=name.begin();i!=name.end();++i) {
    if(*i=='\'' || *i=='\\')
      a+='\\';
    a+=*i;
  }
  return a;
}


#if 0
int main()
{
  try {
    SMySQL s("kkfnetmail","127.0.0.1","readonly");
    SSql::result_t juh;
    
    int num=s.doQuery("select *, from mboxes", juh);
    cout<<num<<" responses"<<endl;
    
    for(int i=0;i<num;i++) {
      const SSql::row_t &row=juh[i];

      for(SSql::row_t::const_iterator j=row.begin();j!=row.end();++j)
	cout <<"'"<< *j<<"', ";
      cout<<endl;
    }
  }
  catch(SSqlException &e) {
    cerr<<e.txtReason()<<endl;
  }
}


#endif
