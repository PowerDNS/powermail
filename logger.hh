#ifndef LOGGER_HH
#define LOGGER_HH
#include <string>
#include <map>
#include <ctime>
#include <iostream>
#include <sstream>
#include <pthread.h>

#include <syslog.h>

using namespace std;

//! The Logger class can be used to log messages in various ways.
class Logger
{
public:
  Logger(const string &, int facility=LOG_DAEMON); //!< pass the identification you wish to appear in the log

  //! The urgency of a log message
  enum Urgency {All=99999,Alert=LOG_ALERT, Critical=LOG_CRIT, Error=LOG_ERR, Warning=LOG_WARNING,
		Notice=LOG_NOTICE,Info=LOG_INFO, Debug=LOG_DEBUG, None=-1};

  /** Log a message. 
      \param msg Message you wish to log 
      \param Urgency Urgency of the message you wish to log
  */
  void log(const string &msg, Urgency u=Notice); 

  void setFacility(int f){d_facility=f;open();} //!< Choose logging facility
  void setName(const string &n); //!< Change the name we log under
  void setFlag(int f){flags|=f;open();} //!< set a syslog flag
  //! determine of messages go only to syslog, or appear on the screen as well
  void toConsole(bool b);

  //! set lower limit of urgency needed for console display. Messages of this urgency, and higher, will be displayed
  void toConsole(Urgency);

  void resetFlags(){flags=0;open();} //!< zero the flags
  /** Use this to stream to your log, like this:
      \code
      L<<"This is an informational message"<<endl; // logged at default loglevel (Info)
      L<<Logger::Warning<<"Out of diskspace"<<endl; // Logged as a warning 
      L<<"This is an informational message"<<endl; // logged AGAIN at default loglevel (Info)
      \endcode
  */
  Logger& operator<<(const string &s);   //!< log a string
  Logger& operator<<(int);   //!< log an int
  Logger& operator<<(Urgency);    //!< set the urgency, << style
  Logger& operator<<(ostream & (&)(ostream &)); //!< this is to recognise the endl, and to commit the log
private:
  map<pthread_t,string>d_strings;
  map<pthread_t,Urgency> d_outputurgencies;
  void open();
  string name;
  int flags;
  int d_facility;
  bool opened;
  Urgency consoleUrgency;
  pthread_mutex_t lock;
};

#ifdef VERBOSELOG
#define DLOG(x) x
#else
#define DLOG(x)
#endif


#endif
