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
#include "ldap.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "common.hh"
#include "misc.hh"

extern Logger L;


UserBase *LDAPUserBase::maker()
{
  return new LDAPUserBase(args().paramString("ldap-host"));
}

LDAPUserBase::LDAPUserBase(const string &host)
{
  d_host=host;
  d_attribute=args().paramString("ldap-search-attribute");

  vector<string> parts;
  stringtok(parts,args().paramString("ldap-domain-map")," \t\n");
  for(vector<string>::const_iterator i=parts.begin();i!=parts.end();++i) {
    vector<string>dommap;

    stringtok(dommap,*i,":");
    if(dommap.size()==2)
      d_map[dommap[0]]=dommap[1];
    else
      d_map[""]=dommap[0];
  }

  try {
    d_db=0;
    d_db=new PowerLDAP(d_host);
    d_db->bind();
  }
  catch(LDAPException &le) {
    L<<Logger::Error<<"Unable to launch LDAP connection with '"<<host<<"' ("<<le.what()<<"), will try again later"<<endl;
    throw AhuException(le.what());
  }
}

LDAPUserBase::~LDAPUserBase()
{
  delete d_db;
}

/** returns -1 for a database error, 0 for 'exists' and 1 for 'does not exist */
int LDAPUserBase::mboxData(const string &mbox, MboxData &md, const string &pass, string &error, bool &exists, bool &pwcorrect)
{
  exists=pwcorrect=false;

  string localpart, domain;
  vector<string> parts;
  stringtok(parts,mbox,"@");
  localpart=parts[0];
  if(parts.size()==2)
    domain=parts[1];

  string base;
  if(d_map.count(domain))
    base=d_map[domain];
  else
    base=d_map[""];

  L<<Logger::Notice<<"Base for mailbox '"<<mbox<<"' is '"<<base<<"'"<<endl;

  if(!pass.empty()) { // if a bind succeeds, we are happy
    try {
      d_db->bind("uid="+PowerLDAP::escape(mbox)+","+base,pass);
      pwcorrect=true;
    }
    catch(LDAPException &le) {
      L<<Logger::Error<<"LDAP error performing user bind: "<<le.what()<<endl;
      pwcorrect=false;
      exists=true;
      return 0;
    }

    exists=true;
    md.canonicalMbox=mbox;
    md.isForward=false;
    md.mbQuota=0;
    return 0;
  }
  
  d_db->search(base,d_attribute+"="+PowerLDAP::escape(mbox));
  PowerLDAP::sresult_t res;
  d_db->getSearchResults(res);
  if(res.empty()) {
    error="No such user";
    return 1;
  }
  else {
    exists=true;
    md.canonicalMbox=mbox;
    md.isForward=false;
    md.mbQuota=0;
    return 0;
  }
  
  return -1;
}

bool LDAPUserBase::connected()
{
  return !!d_db;
}

class LdapAutoLoaderClass
{
public:
  LdapAutoLoaderClass()
  {
    UserBaseRepository()["ldap"]=&LDAPUserBase::maker;
    args().addParameter("ldap-host","LDAP server(s) to connect to","127.0.0.1");
    args().addParameter("ldap-domain-map","How to map a domain to a search-base","");
    args().addParameter("ldap-search-attribute","Attribute to use for ldap searches and binds","uid");
  }
};

static LdapAutoLoaderClass LdapAutoLoader;
