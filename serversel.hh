#ifndef SERVERSEL_HH
#define SERVERSEL_HH
#include "megatalker.hh"

class ServerSelect
{
  typedef vector<TargetData> td_t;
public:
  ServerSelect(const td_t&td);
  const TargetData *getServer();
private:

  td_t d_td;
  map<int,int>d_pgUsed;
};
#endif
