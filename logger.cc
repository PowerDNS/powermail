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
#include "logger.hh"
#include "lock.hh"
using namespace std;

void Logger::log(const string &msg, Urgency u)
{

  struct tm tmb;
  time_t t;
  time(&t);
  tmb=*localtime(&t);

  if(u<=consoleUrgency) // Sep 14 06:52:09
    {
      char buffer[51];
      strftime(buffer,sizeof(buffer)-1,"%b %d %H:%M:%S ", &tmb);
      clog<<buffer;
      clog << msg <<endl;
    }
  syslog(u,"%s",msg.c_str());
}

void Logger::toConsole(bool b)
{
  consoleUrgency=b ? All : None;
}

void Logger::toConsole(Urgency u)
{
  consoleUrgency=u;
}

void Logger::open()
{
  if(opened)
    closelog();
  openlog(name.c_str(),flags,d_facility);
  opened=true;
}

Logger::Logger(const string &n, int facility)
{
  cout<<"logger launched "<<(void *)&lock<<endl;
  opened=false;
  flags=LOG_PID|LOG_NDELAY;
  d_facility=facility;
  consoleUrgency=Error;
  name=n;
  pthread_mutex_init(&lock,0);
  open();
}

void Logger::setName(const string &n)
{
  closelog();
  name=n;
  openlog(name.c_str(),flags,d_facility);
}

Logger& Logger::operator<<(Urgency u)
{
  Lock l(&lock);

  d_outputurgencies[pthread_self()]=u;

  return *this;
}

Logger& Logger::operator<<(const string &s)
{
  Lock l(&lock);

  if(!d_outputurgencies.count(pthread_self())) // default urgency
    d_outputurgencies[pthread_self()]=Info;

  if(d_outputurgencies.count(pthread_self())<=(unsigned int)consoleUrgency) // prevent building strings we won't ever print
      d_strings[pthread_self()].append(s);


  return *this;
}

Logger& Logger::operator<<(int i)
{
  ostringstream tmp;
  tmp<<i;

  *this<<tmp.str(); // this locks for us

  return *this;
}

Logger& Logger::operator<<(ostream & (&)(ostream &))
{
  // *this<<" ("<<(int)d_outputurgencies[pthread_self()]<<", "<<(int)consoleUrgency<<")";
  cout<<"lock: "<<(void *)&lock<<endl;

  Lock l(&lock);

  log(d_strings[pthread_self()], d_outputurgencies[pthread_self()]);
  d_strings.erase(pthread_self());  // ??
  d_outputurgencies.erase(pthread_self());


  return *this;
}
