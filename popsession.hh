#ifndef POPSESSION_HH
#define POPSESSION_HH
#include "delivery.hh"
#include "pptalker.hh"
#include "megatalker.hh"
#include <string>
#include <vector>
using namespace std;

class PopSession
{
public:
  PopSession(int clisock,string remote) : d_remote(remote),d_login(false),
					  d_scanned(false), d_megatalker(0), 
					  d_clisock(clisock), d_retrieved(0), 
					  d_deleted(0) {}
  ~PopSession();
  enum quit_t {noQuit, quitNow,pleaseQuit, pleaseDie};
  bool giveLine(string &line, string &response, quit_t &quit, int &fd);
  string getBanner(); 

private:
  bool topCommands(const string &line, string &response, quit_t &quit);
  bool loginCommands(const string &line, string &response, quit_t &quit);
  bool controlCommands(const string &line, string &response, quit_t &quit);
  bool restCommands(const string &line, string &response, quit_t &quit, int &fd);
  void strip(string &line);
  void firstWordUC(string &line);
  int delDeleted(string &response);
  int scanDir(string &response);
  const string PopSession::makeStatResponse();

  string d_remote;
  string d_user;
  bool d_login;
  string d_dir;
  
  struct MsgDatum {
    string d_fname;
    bool d_deleted;
    size_t d_size;
  };

  typedef vector<MsgDatum> msgData_t;
  msgData_t d_msgData;
  bool d_scanned;
  MegaTalker *d_megatalker;

  int d_clisock;
  int d_retrieved;
  int d_deleted;

};

#endif /* SMTPSESSION_HH */
