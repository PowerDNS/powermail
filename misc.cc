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
#include "misc.hh"
#include "logger.hh"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <errno.h>
#include <cstring>


void cleanSlashes(string &str)
{
  string::const_iterator i;
  string out;
  for(i=str.begin();i!=str.end();++i) {
    if(*i=='/' && i!=str.begin() && *(i-1)=='/')
      continue;
    out.append(1,*i);
  }
  str=out;
}


string getLoad()
{
  string load="unknown";
  ifstream ifs("/proc/loadavg");
  if(ifs)
    ifs>>load;
  return load;
}

string getHostname()
{
  char tmp[513];
  if(gethostname(tmp, 512))
    return "UNKNOWN";

  return tmp;
}

string itoa(int i)
{
  ostringstream o;
  o<<i;
  return o.str();
}

string stringerror()
{
  return strerror(errno);
}

void stripLine(string &line)
{
  unsigned int pos=line.find_first_of("\r\n");
  if(pos!=string::npos) {
    line.resize(pos);
  }
}

void dropPrivs(int uid, int gid)
{
  extern Logger L;
  if(gid) {
    if(setgid(gid)<0) {
      L<<Logger::Error<<"Unable to set effective group id to "<<gid<<": "<<stringerror()<<endl;
      exit(1);
    }
  }
  if(uid) {
    if(setuid(uid)<0) {
      L<<Logger::Error<<"Unable to set effective user id to "<<uid<<": "<<stringerror()<<endl;
      exit(1);
    }
  }
}

