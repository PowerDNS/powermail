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
