#ifndef TEXTUSERBASE_HH
#define TEXTUSERBASE_HH
#include <string>
#include <time.h>
#include "lock.hh"
#include "userbase.hh"
#include "smysql.hh"
#include "textparser.hh"

using namespace std;

class TextUserBase : public UserBase
{
public:
  TextUserBase(const string &fname);
  ~TextUserBase();
  static UserBase *maker();

  int mboxData(const string &mbox, MboxData &md, const string &password, string &error, bool &exists, bool &pwcorrect);

  bool connected(); 

private:
  void tryParse();
  vector<TextBaseEntry> d_textbase; 
  string d_fname;
  bool d_parsed;
  static pthread_mutex_t d_launchlock;
  static bool d_first;
  time_t d_last_mtime;
};

#endif /* MYSQLUSERBASE_HH */

