#ifndef ARGUMENTS_HH
#define ARGUMENTS_HH

#include <map>
#include <string>
#include <vector>
#include "ahuexception.hh"

class ArgumentException
{
public:
  ArgumentException(const string &str)
  {
    d_reason=str;
  }

  string txtReason()
  {
    return d_reason;
  }
private:
  string d_reason;
};


using namespace std;
class Argument;



class ArgSettings
{
public:
  void preparseArgs(int argc, char **argv,const string &preparg);
  void parseArgs(int argc, char **argv); //!< use this to parse from argc and argv
  bool parseFiles(string &response, const char *first, ...);
  bool parseFile(const char *fname); //!< Parses a file with parameters

  void getRest(int argc, char **argv,vector<string>&commands); //!< return all non-option parts
  bool commandGiven(const string &name); //!< returns true if the command was given
  bool switchSet(const string &name); //!< returns true if a switch was set
  int paramAsNum(const string &var); //!< return a variable value as a number
  double paramAsDouble(const string &var); //!< return a variable value as a number
  const string &paramString(const string &var);

  void addParameter(const string &name, const string &help, const string &value); //!< Does the same but also allows to specify a help message
  void ignore(const string &name);
  void addCommand(const string &name, const string &help); //!< Add a command flag
  void addSwitch(const string &, const string &help, bool isset); //!< Add a switch which can have a value
  string makeHelp(); //!< generates the --help
  string makeConfig(); //!< generates the --mkconfig

  typedef enum {Parameter,Switch,Command,Ignore} argtype_t;
  
private:
  map<string,Argument> d_arguments;
  void parseOne(const string &piece, Argument *arg);
  void tupleToParts(vector<string>&parts,const string& tuple);
  void setParam(const vector<string> &parts, bool lax=false);
  string makeLeft(const string &name, const Argument &arg, bool config=false);
  string wordWrap(const string &input, int leftskip);
};

class Argument 
{
public:
  Argument(){}
  Argument(const string &help, const string &value, ArgSettings::argtype_t type);
  Argument(const string &help, ArgSettings::argtype_t type);

  const string &getHelp() const
  {
    return d_help;
  }
  
  const string &parameterValue() const
  {
    return d_value;
  }
  bool switchIsSet() const;
  bool commandGiven() const;

  void switchSet(bool val);
  void commandGive();
  void parameterSet(const string &val); 

  ArgSettings::argtype_t getType() const
  {
    return d_type;
  }

private:
  string d_value;
  string d_help;
  ArgSettings::argtype_t d_type;

};


#endif /* ARGUMENTS_HH */
