#ifndef POWERLDAP_HH
#define POWERLDAP_HH
#include <map>
#include <vector>
#include <crypt.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <lber.h>
#include <ldap.h>

using namespace std;


class LDAPException : public runtime_error
{
public:
  explicit LDAPException(const string &str) : runtime_error(str){}
};

class LDAPTimeout : public LDAPException
{
public:
  explicit LDAPTimeout() : LDAPException("Timeout"){}
};

class PowerLDAP
{
public:
  typedef map<string,vector<string> > sentry_t;
  typedef vector<sentry_t> sresult_t;

  PowerLDAP(const string &host="127.0.0.1", u_int16_t port=389);
  void bind(const string &ldapbinddn="", const string& ldapsecret="");
  int waitResult(int msgid=LDAP_RES_ANY,LDAPMessage **retresult=0) ;
  void search(const string& base, const string& filter, const char **attr=0);
  bool getSearchEntry(sentry_t &entry);
  void getSearchResults(sresult_t &result);
  ~PowerLDAP();
  static const string escape(const string &tobe);
private:
  const string getError(int rc=-1);
  LDAP *d_ld;
  string d_host;
  u_int16_t d_port;
  int d_timeout;
  bool d_bound;
  LDAPMessage *d_searchresult;
  LDAPMessage *d_searchentry;
};

#endif
