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
#ifndef PPSESSION_HH
#define PPSESSION_HH
#include "delivery.hh"
#include <string>
#include <vector>
using namespace std;

class PPState
{
public:
  PPState();
  bool isReadonly();
  void setReadonly();
  void unsetReadonly();
  void setFull();
  void unsetFull();
private:
  string d_stateDir;
  bool d_full;
};


class PPListenSession
{
public:
  PPListenSession(string remote) : d_remote(remote),d_indata(false), d_failed(false), d_mode(unAuth) {}
  enum quit_t {noQuit, quitNow,pleaseQuit, pleaseDie};
  bool controlCommands(const string &line, string &response, quit_t &quit);

  bool giveLine(string &line, string &response, quit_t &quit, int &fd, bool &more);
  bool listResponse(const string &mbox, string &response);
  bool putCommand(string &response);
  bool pingCommand(string &response);
  bool statCommand(string &response);
  bool quitCommand(string &response, quit_t &quit);
  bool passwordCommand(const string &password, string &response);
  bool putLine(string &line, string &response);
  bool rollBackCommand(string &response);
  bool commitCommand(string &response);
  bool rcptCommand(const string &mbox, const string &index, string &response);
  bool getCommand(const string &mbox, const string &index, int *fd, string &response);
  bool delCommand(const string &mbox, const string &index, string &response);
  bool dirCommand(string &response, bool &more) ;
  bool moreDir(string &response, bool &more) ;
  bool nukeCommand(const string &mbox, string &response);
  string getBanner(); 

private:
  string d_remote;
  bool d_indata;
  bool d_failed;
  string d_reason;
  void strip(string &line);
  void lowerCase(string &line);
  int scanDir();
  enum {blank, unAuth, inLimbo, preLimbo, inData, Dir} d_mode;
  Delivery d_delivery;
  string d_filename;
  int d_fd;
  PPState d_pps;
};

#endif /* PPSESSION_HH */
