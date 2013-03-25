/*
    PowerMail versatile mail receiver
    Copyright (C) 2002 - 2009  PowerDNS.COM BV

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
#include <cstring>
#include "argsettings.hh"
#include "misc.hh"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdarg.h>

Argument::Argument(const string &help, const string &value, ArgSettings::argtype_t type)
{
  d_help=help;
  d_value=value;
  d_type=type;
}


Argument::Argument(const string &help, ArgSettings::argtype_t type)
{
  d_help=help;
  d_value="";
  d_type=type;
}

void ArgSettings::ignore(const string &name)
{
  Argument arg("", "", Ignore);
  d_arguments.insert(make_pair(name,arg));

}
void ArgSettings::addParameter(const string &name, const string &help, const string &value)
{
  Argument arg(help, value, Parameter);
  d_arguments.insert(make_pair(name,arg));
}

void ArgSettings::addSwitch(const string &var, const string &help, bool isset)
{
  Argument arg(help,isset ? "true" : "false" ,Switch);
  d_arguments.insert(make_pair(var,arg));
}

void ArgSettings::addCommand(const string &var, const string &help)
{
  Argument arg(help,Command);
  d_arguments.insert(make_pair(var,arg));
}


void Argument::commandGive()
{
  d_value="given";
}

void Argument::switchSet(bool d_val)
{
  d_value=d_val ? "true" : "false";
}



void Argument::parameterSet(const string &value)
{
  d_value=value;
}

void ArgSettings::setParam(const vector<string> &parts, bool lax)
{
  if(parts.empty())
    return;
  const string &name=parts[0];

  map<string,Argument>::iterator i;
  if((i=d_arguments.find(name))==d_arguments.end())
    if(lax)
      return;
    else
      throw ArgumentException("Unknown argument passed: '"+name+"'");

  Argument &arg=i->second;
  if(arg.getType()==Ignore)
    return;

  if(arg.getType()==Command) {
    if(parts.size()>1)
      throw ArgumentException("Command '"+name+"' does not take an argument");

    arg.commandGive();
  }
  else if(arg.getType()==Switch) {
    if(parts.size()>2)
      throw ArgumentException("Switch '"+name+"' takes 0 or 1 arguments");
    
    if(parts.size()==1)
      arg.switchSet(true);
    else {
      const string &param=parts[1];
      if(param=="yes" || param=="true" || param=="set")
	arg.switchSet(true);
      else if(param=="no" || param=="false" || param=="unset")
	arg.switchSet(false);
      else
	throw ArgumentException("Switch '"+name+"' can only be set to yes/true/set/no/false/unset");
    }
  }
  else if(arg.getType()==Parameter) {
    if(parts.size()!=2)
      throw ArgumentException("Parameter '"+name+"' needs a single value");

    arg.parameterSet(parts[1]);
  }
}

void ArgSettings::preparseArgs(int argc, char **argv,const string &preparg)
{
  vector<string> arguments;
  for(int n=1;n<argc;n++)
    if(!strncmp(argv[n],"--",2))
      arguments.push_back(argv[n]+2);

  if(arguments.empty())
    return;

  for(vector<string>::const_iterator i=arguments.begin();i!=arguments.end();++i) {
    vector<string> parts;
    tupleToParts(parts,*i);
    if(parts.size()>0 && parts[0]==preparg) 
      setParam(parts);
  }
}

void ArgSettings::getRest(int argc, char **argv,vector<string> &commands)
{
  for(int n=1;n<argc;n++) 
    if(strncmp(argv[n],"--",2))
      commands.push_back(argv[n]);
}

void ArgSettings::parseArgs(int argc, char **argv)
{
  vector<string> arguments;
  for(int n=1;n<argc;n++)
    if(!strncmp(argv[n],"--",2))
      arguments.push_back(argv[n]+2);

  if(arguments.empty())
    return;

  for(vector<string>::const_iterator i=arguments.begin();i!=arguments.end();++i) {
    vector<string> parts;
    tupleToParts(parts,*i);
    setParam(parts);
  }
}



bool ArgSettings::parseFile(const char *fname)
{
  ifstream input(fname);
  if(!input)
    return false;

  string line;

  vector<string> arguments;
  string totLine;
  while(getline(input, line)) {
    string line2;
    bool equalsSeen=false;

    chomp(line," \r\n");
    if(line[line.size()-1]=='\\') {
      chomp(line,"\\");
      totLine+=line;
      continue;
    }
    totLine+=line;

    for(string::const_iterator i=totLine.begin();i!=totLine.end();++i) {
      if(*i=='#' || *i=='\n' || *i=='\r')
	break;
      
      if(*i=='=')
	equalsSeen=true;
      
      if(!equalsSeen && isspace(*i))
	continue;
      
      line2+=*i;
    }
    totLine="";
    arguments.push_back(line2);
  }

  if(arguments.empty())
    return true;

  for(vector<string>::const_iterator i=arguments.begin();i!=arguments.end();++i) {
    vector<string> parts;
    tupleToParts(parts,*i);
    if(i->find("=")!=string::npos && parts.size()==1)
      parts.push_back("");
    setParam(parts);
  }
  return true;
}


const string &ArgSettings::paramString(const string &var)
{
  const Argument &arg=d_arguments[var];
  return arg.parameterValue();
}


bool Argument::switchIsSet() const
{
  return d_value=="true";
}

bool Argument::commandGiven() const
{
  return d_value=="given";
}

bool ArgSettings::switchSet(const string &var)
{
  return d_arguments[var].switchIsSet();
}

bool ArgSettings::commandGiven(const string &var)
{
  return d_arguments[var].commandGiven();
}

string ArgSettings::makeLeft(const string &name, const Argument &arg, bool config)
{
  string say;
  if(!config || arg.getType()!=Command)
    say=name;
  
  if(config)
    if(arg.getType()==Command)
      say=arg.commandGiven() ? name : ("# "+name);

  switch(arg.getType()) {
  case Parameter:
    say+="="+arg.parameterValue(); //"=...";
    break;
  case Switch:
    if(!config) {
      if(arg.switchIsSet())
	say+="[=(TRUE|false)]";
      else
	say+="[=(true|FALSE)]";
    }
    else
      say+="="+arg.parameterValue();
    
    break;
  default:
    break;
  }
  return say;
}

bool ArgSettings::parseFiles(string &response, const char *first, ...)
{
  vector<string>files;

  files.push_back(first);

  va_list ap;
  va_start(ap, first);
  const char *p;
  while((p=va_arg(ap, const char *)))
    files.push_back(p);
 
  va_end(ap);
  for(vector<string>::const_iterator i=files.begin();i!=files.end();++i){
    if(parseFile(i->c_str())) {
      response=*i;
      return false;
    }
  }
  return true;
}

void ArgSettings::tupleToParts(vector<string>&parts,const string& tuple)
{
  parts.clear();
  if(tuple.empty())
    return;
  string::size_type pos;
  if((pos=tuple.find_first_of("="))==string::npos) {
    parts.push_back(tuple);
    return;
  }
  parts.push_back(tuple.substr(0,pos));
  parts.push_back(tuple.substr(pos+1));
}

string ArgSettings::wordWrap(const string &input, int leftskip)
{
  string output;
  vector<string> words;
  stringtok(words,input);
  
  unsigned int limit=80;
  unsigned int left=limit-leftskip;
  for(vector<string>::const_iterator i=words.begin();i!=words.end();++i) {
    if(left<i->length()) {
      output+="\n";
      output.append(leftskip,' ');
      left=limit-leftskip;
    }
    output+=*i+" ";
    left-=i->length()+1;
  }
  return output;
}

int ArgSettings::paramAsNum(const string &name)
{
  return atoi(d_arguments[name].parameterValue().c_str());
}

string ArgSettings::makeConfig()
{
  ostringstream help;
  help.setf(ios::uppercase);

  for(map<string,Argument>::const_iterator i=d_arguments.begin();
      i!=d_arguments.end();++i) {
    const Argument &arg=i->second;
    if(arg.getType()==Command)
      continue;
    //    help<<"### "<<i->first<<":"<<endl;
    help<<"# "<<arg.getHelp()<<endl;
    help<<"# "<<makeLeft(i->first, i->second,true)<<endl;
    help<<endl;

  }
  return help.str();
}


string ArgSettings::makeHelp()
{
  ostringstream help;
  help.setf(ios::uppercase);

  string::size_type maxLen=0;
  for(map<string,Argument>::const_iterator i=d_arguments.begin();
      i!=d_arguments.end();++i) {
    if(i->second.getType()!=Ignore)
      maxLen=max(maxLen,makeLeft(i->first,i->second).length());
  }

  int leftWidth=maxLen+2;

  for(map<string,Argument>::const_iterator i=d_arguments.begin();
      i!=d_arguments.end();++i) {

    const Argument &arg=i->second;
    if(arg.getType()!=Ignore) {
      help<<" --"<<makeLeft(i->first,i->second);
      for (int len = leftWidth-makeLeft(i->first,i->second).length();len>0;len--) { help << " ";}
      help << wordWrap(arg.getHelp(),leftWidth+3) << "\n";
    }
  }
  return help.str();
}

#ifdef TESTDRIVER

int main(int argc, char **argv)
{
  ArgSettings A;

  try {
    A.addParameter("hostname","configure our hostname that is going to be sent out everywhere, in pop and smtp",getHostname());
    A.addSwitch("quota-checking","if we do quota checking",true);
    A.addSwitch("reverse-checking","if we do reverse checking",false);
    A.addCommand("help","print this helpful message");
    A.parseArgs(argc, argv);
  }
  catch(ArgumentException &e) {
    cerr<<"Fatal error: "<<e.txtReason()<<endl;
  }

  cout<<"Our hostname is: "<<A.paramString("hostname")<<endl;
  cout<<"Are we doing quota checking: "<<A.switchSet("quota-checking")<<endl;
  cout<<"Are we doing reverse checking: "<<A.switchSet("reverse-checking")<<endl;
  if(A.commandGiven("help"))
    cout<<A.makeHelp()<<endl;

}
#endif  
