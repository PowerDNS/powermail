#include "logger.hh"

using namespace std;

void Logger::log(const string &msg, Urgency u)
{

  struct tm tmb;
  time_t t;
  time(&t);
  tmb=*localtime(&t);



  if(u<=consoleUrgency) // Sep 14 06:52:09
    {
      char buffer[512];
      strftime(buffer,511,"%b %d %H:%M:%S ", &tmb);
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
  pthread_mutex_lock(&lock);

  d_outputurgencies[pthread_self()]=u;

  pthread_mutex_unlock(&lock);
  return *this;
}

Logger& Logger::operator<<(const string &s)
{
  pthread_mutex_lock(&lock);

  if(!d_outputurgencies.count(pthread_self())) // default urgency
    d_outputurgencies[pthread_self()]=Info;

  if(d_outputurgencies.count(pthread_self())<=(unsigned int)consoleUrgency) // prevent building strings we won't ever print
      d_strings[pthread_self()]+=s;

  pthread_mutex_unlock(&lock);
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
  pthread_mutex_lock(&lock);

  log(d_strings[pthread_self()], d_outputurgencies[pthread_self()]);
  d_strings.erase(pthread_self());  
  d_outputurgencies.erase(pthread_self());

  pthread_mutex_unlock(&lock);
  return *this;
}
