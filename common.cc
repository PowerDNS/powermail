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
#include <iostream>
#include "argsettings.hh"
#include "logger.hh"
#include "userbase.hh"
#include "pool.hh"

using namespace std;
extern int PowerSmtpMain(int argc, char **argv);
extern int PowerPopMain(int argc, char **argv);
extern int PowerImapMain(int argc, char **argv);
extern int PPListenerMain(int argc, char **argv);
extern int PPToolMain(int argc, char **argv);
extern int MboxDirMain(int argc, char **argv);

ArgSettings &args()
{
  static ArgSettings args;
  return args;
}

Logger L("powersmtp",LOG_MAIL);
PoolClass<UserBase>* UBP;

void startUserBase()
{
  string userbase=args().paramString("userbase");
  if(UserBaseRepository()[userbase]) {
    UBP=new PoolClass<UserBase>(UserBaseRepository()[userbase]);
    
    if(userbase=="text") { // singlethreaded userbases
      UBP->setMax(1);
      UBP->setMaxSpares(1);
    }
    else {
      UBP->setMax(args().paramAsNum("max-userbase-connections"));
      UBP->setMaxSpares(args().paramAsNum("max-userbase-spares"));
      UBP->stock(args().paramAsNum("max-userbase-spares"));
    }
  }
  else {
    L<<Logger::Error<<"Unknown userbase '"<<args().paramString("userbase")<<"'"<<endl;
    exit(1);
  }
}

void UserBaseArguments()
{
  args().addParameter("userbase","Which userbase to use","text");

  args().addParameter("max-userbase-connections","Maximum number of userbase connections to make","10");
  args().addParameter("max-userbase-spares","Maximum number of userbase connections to keep on hand unused","2");



}

int main(int argc, char **argv)
{
  if(strstr(*argv,"powersmtp"))
    return PowerSmtpMain(argc, argv);
  if(strstr(*argv,"powerpop"))
    return PowerPopMain(argc, argv);
  if(strstr(*argv,"powerimap"))
    return PowerImapMain(argc, argv);
  if(strstr(*argv,"pplistener"))
    return PPListenerMain(argc, argv);
  if(strstr(*argv,"mboxdir"))
    return MboxDirMain(argc, argv);

  return PPToolMain(argc, argv);
}
