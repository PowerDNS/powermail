#include "powerldap.hh"
#include <errno.h>
#include <iostream>

#include <map>
#include <vector>
#include <crypt.h>
#include <exception>
#include <stdexcept>
#include <string>

// test
 
PowerLDAP::PowerLDAP(const string &host, u_int16_t port) : d_host(host), d_port(port), d_timeout(1)
{
  d_bound=false;
  if (( d_ld = ldap_init(d_host.c_str(), d_port )) == NULL ) {
    throw LDAPException("Error initializing LDAP connection: "+string(strerror(errno)));
  }
}
  
void PowerLDAP::bind(const string &ldapbinddn, const string& ldapsecret)
{
  int msgid;
  if((msgid=ldap_simple_bind(d_ld, ldapbinddn.c_str(), ldapsecret.c_str())) == -1)
    throw LDAPException("Error binding to LDAP server: "+getError());
  
  int rc;
  if((rc=waitResult(msgid))!=LDAP_RES_BIND)
    throw LDAPException("Binding to LDAP server returned: "+getError(rc));
  d_bound=true;
}

/** Function waits for a result, returns its type and optionally stores the result
    in retresult. If returned via retresult, the caller is responsible for freeing
    it with ldap_msgfree! */
int PowerLDAP::waitResult(int msgid,LDAPMessage **retresult) 
{
  struct timeval tv;
  tv.tv_sec=d_timeout;
  tv.tv_usec=0;
  LDAPMessage *result;
  
  int rc=ldap_result(d_ld,msgid,0,&tv,&result);
  if(rc==-1)
    throw LDAPException("Error waiting for LDAP result: "+getError());
  if(!rc)
    throw LDAPTimeout();
  
  if(retresult)
    *retresult=result;
  
  if(rc==LDAP_RES_SEARCH_ENTRY || LDAP_RES_SEARCH_RESULT) // no error in that case
    return rc;
  
  int err;
  if((err=ldap_result2error(d_ld, result,0))!=LDAP_SUCCESS) {
    ldap_msgfree(result);
    throw LDAPException("LDAP Server reported error: "+getError(err));
  }
  
  if(!retresult)
    ldap_msgfree(result);
  
  return rc;
}


void PowerLDAP::search(const string& base, const string& filter, const char **attr)
{
  if(ldap_search( d_ld, base.c_str(), LDAP_SCOPE_SUBTREE, filter.c_str(),const_cast<char **>(attr),0)== -1)
    throw LDAPException("Starting LDAP search: "+getError());
  d_searchresult=0;
}

bool PowerLDAP::getSearchEntry(sentry_t &entry) 
{
  entry.clear();
  if(!d_searchresult) {
    int rc=waitResult(LDAP_RES_ANY,&d_searchresult);
    if(rc==LDAP_RES_SEARCH_RESULT) {
      ldap_msgfree(d_searchresult);
      return false;
    }
    if(rc!=LDAP_RES_SEARCH_ENTRY)
      throw LDAPException("Search returned non-answer result");
    d_searchentry=ldap_first_entry(d_ld, d_searchresult);
  }
  
  // we now have an entry in d_searchentry
  
  BerElement *ber;
  
  for(char *attr = ldap_first_attribute( d_ld, d_searchresult, &ber ); attr ; attr=ldap_next_attribute(d_ld, d_searchresult, ber)) {
    struct berval **bvals=ldap_get_values_len(d_ld,d_searchentry,attr);
    vector<string> rvalues;
    if(bvals)
      for(struct berval** bval=bvals;*bval;++bval) 
	rvalues.push_back((*bval)->bv_val);
    entry[attr]=rvalues;
    ldap_value_free_len(bvals);
    ldap_memfree(attr);
  }
  ber_free(ber,0);
  
  d_searchentry=ldap_next_entry(d_ld,d_searchentry);
  if(!d_searchentry) { // exhausted this result
    ldap_msgfree(d_searchresult);
    d_searchresult=0;
  }
  return true;
}

void PowerLDAP::getSearchResults(sresult_t &result)
{
  result.clear();
  sentry_t entry;
  while(getSearchEntry(entry))
    result.push_back(entry);
}
    
PowerLDAP::~PowerLDAP()
{
  if(d_bound)
    ldap_unbind(d_ld); 
}

const string PowerLDAP::getError(int rc)
{
  int ld_errno=rc;
  if(ld_errno==-1)
    ldap_get_option(d_ld, LDAP_OPT_ERROR_NUMBER, &ld_errno);
  return ldap_err2string(ld_errno);
}

const string PowerLDAP::escape(const string &name)
{
  string a;

  for(string::const_iterator i=name.begin();i!=name.end();++i) {
    if(*i=='*' || *i=='\\')
      a+='\\';
    a+=*i;
  }
  return a;
}


#ifdef TESTDRIVER
int main(int argc, char **argv)
try
{
  for(int k=0;k<2;++k) {
    PowerLDAP ldap;
    ldap.bind("uid=ahu,ou=people,dc=snapcount","wuhwuh"); // anon
    
    for(int n=0;n<3;n++) {
      PowerLDAP::sresult_t ret;
      const char *attr[]={"uid","userPassword",0};
      ldap.search("ou=people,dc=snapcount","uid=ahu",attr);
      
      ldap.getSearchResults(ret);
      cout<<ret.size()<<" records"<<endl;
      
      for(PowerLDAP::sresult_t::const_iterator h=ret.begin();h!=ret.end();++h) {
	for(PowerLDAP::sentry_t::const_iterator i=h->begin();i!=h->end();++i) {
	  cout<<"attr: "<<i->first<<endl;
	  for(vector<string>::const_iterator j=i->second.begin();j!=i->second.end();++j)
	    cout<<"\t"<<*j<<endl;
	}
	cout<<endl;
      }
    }
  }
}
catch(exception &e)
{
  cerr<<"Fatal: "<<e.what()<<endl;
}
#endif
