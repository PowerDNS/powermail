#include "serversel.hh"
#include "megatalker.hh"
#include "logger.hh"
#include "argsettings.hh"
#include "common.hh"
#include <algorithm>
extern Logger L;


/** Use this to select the server that provides the most redundancy and has the highest priority. 
    Each server has a group and a priority. Servers within the same group do not provide mutual redundancy.

    Algorithm: make a list of groups with zero used servers and select a random group. 
    Within that group, randomize and stable sort, and take the first available server */

ServerSelect::ServerSelect(const vector<TargetData>&td)
{
  d_td=td;
  for(td_t::iterator i=d_td.begin();i!=d_td.end();++i) 
    i->used=false;
}

static bool byPrio(const TargetData *a, const TargetData *b)
{
  static bool first=true;
  static bool favorEmpty;
  static bool favorLoad;

  if(first) {
    first=false;
    favorEmpty=args().switchSet("spread-disk");
    favorLoad=args().switchSet("spread-load");
  }
  
  if(a->priority < b->priority)
    return true;
  if(a->priority==b->priority && favorEmpty && (b->kbfree < a->kbfree)) // favour emtpier disks
    return true; 
  if(a->priority==b->priority && favorLoad && (a->load < b->load)) // favour lower load
    return true; 
  return false;
}

const TargetData *ServerSelect::getServer()
{
  map<int,vector<TargetData *> > perGroup;

  map<int,bool> usedGroup;
  for(td_t::iterator i=d_td.begin();i!=d_td.end();++i) {
    if(i->used) {
      usedGroup[i->group]=true;
    }
  }

  for(td_t::iterator i=d_td.begin();i!=d_td.end();++i) {
    if(i->up && !i->readonly && usedGroup[i->group]==false)
      perGroup[i->group].push_back(&*i);
  }

  if(perGroup.empty()) {
    L<<"No redundant servers left"<<endl;
    return 0;
  }
    
  int cGroup=random()%perGroup.size();

  map<int,vector<TargetData *> >::const_iterator i;
  int n=0;
  for(i=perGroup.begin();
      i!=perGroup.end();++i) 
    if(n++==cGroup)
      break;

  vector<TargetData *> candidates=i->second;
  if(candidates.empty()) { // should not happen
    L<<"Bummer, no servers! [this should not happen]"<<endl;
    exit(0); // die
  }

  random_shuffle(candidates.begin(),candidates.end());
  stable_sort(candidates.begin(),candidates.end(),byPrio);
  candidates[0]->used=true;
  return candidates[0];
}


