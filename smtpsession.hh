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
#ifndef SMTPSESSION_HH
#define SMTPSESSION_HH
#include "megatalker.hh"
#include "userbase.hh"
#include <string>
#include <vector>
using namespace std;

/** This is the class that is fed SMTP commands which are probably received over tcp/ip. It 
    consults with the UserBase over what accounts exist, and what should happen with mail for 
    them. Actual deliveries are handed to the MegaTalker */
class SmtpSession
{
public:
  SmtpSession(const string &remote); //!< Commands we will be receiving are from the remote
  ~SmtpSession();
  enum quit_t {noQuit, quitNow,pleaseQuit,pleaseDie}; //!< giveLine can set these quit commands
  bool giveLine(string &line, string &response, quit_t &quit); //!< Feed us a line from somewhere and parse it, give a response, possibly indicate a quit
  string getBanner(); 

private:
  void sendReceivedLine();
  string makeIndex();
  bool rsetCommand(string &response);
  void rsetSession();
  bool eatDataLine(const string &line,string &response);
  bool controlCommands(const string &line, string &response, quit_t &quit);
  bool topCommands(const string &line, string &response, quit_t &quit);
  bool eatFrom(const string &line, string &response);
  bool eatTo(const string &line, string &response);
  bool eatData(const string &line, string &response);
  void strip(string &line); //!< Strip cr/lf from a line
  void firstWordUC(string &line);
  string extractAddress(const string &line);
  int determineKbUsage(const string &mbox, size_t *usage, string &response);

  bool d_quotaneeded;
  int d_kbroom; // things get complicated if this is unsigned
  size_t d_databytes;

  bool d_fromset;
  string d_from;
  string d_index;
  vector<MboxData>d_recipients;
  vector<string> d_addressees;
  string d_subject;
  bool d_indata;
  bool d_inheaders;
  bool d_multipart;
  string d_boundary;
  MegaTalker *d_megatalker;
  static int s_counter;
  static pthread_mutex_t s_counterlock;
  string d_remote;
  size_t d_esmtpkbsize;
};

#endif /* SMTPSESSION_HH */
