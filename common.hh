#ifndef COMMON_HH
#define COMMON_HH
#include "argsettings.hh"

void daemonize(void);
void UserBaseArguments();
void startUserBase();
ArgSettings &args();
#endif
