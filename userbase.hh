#ifndef USERBASE_HH
#define USERBASE_HH
#include <string>
#include <map>
using namespace std;

struct MboxData
{
  string canonicalMbox;
  int mbQuota;
  bool isForward;
  string fwdDest;
};

class UserBase
{
public:
  static UserBase *maker();
  virtual int mboxData(const string &label, MboxData &md, const string &password, string &error, bool &exists, bool &pwcorrect)=0;
  virtual ~UserBase(){};
  virtual bool connected()=0;
protected:
  bool pwMatch(const string &supplied, const string &db);
};
map<string,UserBase*(*)()>& UserBaseRepository();
#endif /* USERBASE_HH */
