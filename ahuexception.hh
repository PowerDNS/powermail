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
#ifndef AHUEXCEPTION_HH
#define AHUEXCEPTION_HH

#include<string>

using namespace std;

//! Generic Exception thrown 
class AhuException
{
public:
  AhuException(){d_reason="Unspecified";};
  AhuException(string r){d_reason=r;};
  
  string d_reason; //! Print this to tell the user what went wrong
};

#endif
