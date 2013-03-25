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
/* Copyright 2001 Netherlabs BV, bert.hubert@netherlabs.nl. See LICENSE 
   for more information.
   $Id: mboxdir.cc,v 1.3 2009-05-28 20:12:27 ahu Exp $  */
#include <stdio.h>
#include <stdlib.h>
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
  
  
