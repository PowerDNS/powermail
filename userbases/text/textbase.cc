#include "textbase.hh"
#include "logger.hh"
#include "argsettings.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "common.hh"
#include "misc.hh"

extern Logger L;

static bool s_failed;
extern "C" {
  int yyparse();

  void errorwrapper(const char *problem, int linenumber) 
  {
    L<<Logger::Error<<("Error parsing textbase on line "+itoa(linenumber)+" at character '"+string(problem)+"'")<<endl;
    s_failed=1;
  }

}

vector<TextBaseEntry> s_newentries;

void textCallback(const TextBaseEntry &entry)
{
  s_newentries.push_back(entry);
}


UserBase *TextUserBase::maker()
{
  return new TextUserBase(args().paramString("text-base"));
}

TextUserBase::TextUserBase(const string &fname)
{
  d_fname=fname;
  d_parsed=false;
  L<<Logger::Error<<"Text userbase launched"<<endl;
  tryParse();
}

extern "C" void yyrestart(FILE *);
void TextUserBase::tryParse()
{
  extern FILE *yyin;

  yyin=fopen(d_fname.c_str(),"r");
  yyrestart(yyin);
  if(!yyin) {
    L<<Logger::Error<<"Unable to parse textual userbase in "<<d_fname<<": "<<stringerror()<<endl;
    return;
  }
  extern int linenumber;
  linenumber=1;
  s_failed=0;

  s_newentries.clear();
  try {
    yyparse();
  }
  catch(AhuException &ae) {
    L<<Logger::Error<<"Fatal error: "<<ae.d_reason<<endl;
    fclose(yyin);

    yyin=0;

    return;
  }
  fclose(yyin);

  if(!s_failed) {
    d_textbase=s_newentries; // and hop!
    d_parsed=true;

    struct stat buf;
    if(stat(d_fname.c_str(),&buf)) {
      L<<Logger::Error<<"Unable to stat user base file: "<<stringerror()<<endl;
    }
    d_last_mtime=buf.st_mtime;
  }

}

TextUserBase::~TextUserBase()
{
  L<<Logger::Error<<"Text userbase destroyed"<<endl;
}

bool TextUserBase::connected()
{
  struct stat buf;
  if(stat(d_fname.c_str(),&buf)) {
    L<<Logger::Error<<"Unable to stat user base file: "<<stringerror()<<endl;
    return false;
  }
  if(buf.st_mtime!=d_last_mtime) {
    L<<Logger::Warning<<"Reparse of text userbase started"<<endl;
    tryParse();
    L<<Logger::Warning<<"Reparse of text userbase finished"<<endl;
  }

  return d_parsed;
}

/** returns -1 for a database error, 0 for ok */
int TextUserBase::mboxData(const string &mbox, MboxData &md, const string &password, string &error, bool &exists, bool &pwcorrect)
{
  int ret=-1; // temporary failure by default
  exists=pwcorrect=false;
  error="Unknown error";
  if(!connected()) {
    error="Temporary database error (not connected)";
    return ret;
  }
  for(vector<TextBaseEntry>::const_iterator i=d_textbase.begin();i!=d_textbase.end();++i) {
    if(i->address==mbox) {
      md.canonicalMbox=i->address;
      md.mbQuota=i->quotaKB/1000;
      md.isForward=!i->forward.empty();
      md.fwdDest=i->forward;
      exists=true;
      pwcorrect=pwMatch(password,i->password);
      return 0;
    }
  }
  error="No such mailbox found";
  return 0;
}


class TextAutoLoaderClass
{
public:
  TextAutoLoaderClass()
  {
    UserBaseRepository()["text"]=&TextUserBase::maker;
    args().addParameter("text-base","File to read for the text-base","/etc/powermail/mailboxes");
  }
};

static TextAutoLoaderClass TextAutoLoader;

