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
#ifndef SERVERSEL_HH
#define SERVERSEL_HH
#include "megatalker.hh"

class ServerSelect
{
  typedef vector<TargetData> td_t;
public:
  ServerSelect(const td_t&td);
  const TargetData *getServer();
private:

  td_t d_td;
  map<int,int>d_pgUsed;
};
#endif
