/* Copyright 2001 Netherlabs BV, bert.hubert@netherlabs.nl. See LICENSE 
   for more information.
   $Id: mboxdir.cc,v 1.1 2002-12-04 13:46:38 ahu Exp $  */
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "delivery.hh"
#include "argsettings.hh"
#include "logger.hh"
#include "misc.hh"
#include "common.hh"

extern Logger L;


int MboxDirMain(int argc, char **argv)
{
  args().addParameter("mail-root",
		    "Location of all email messages. Must be readable, writable and "
		    "executable by the chosen UID & GID!",LOCALSTATEDIR"/messages");

  if(argc!=2) {
    fprintf(stderr,"Syntax: mboxdir mailbox\n");
    exit(1);
  }

  Delivery D;
  string dir=D.calcMaildir(argv[1]);
  cleanSlashes(dir);
  cout<<dir<<endl;
  return 0;
}
  
  
