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
#ifndef LOCK_HH
#define LOCK_HH

#include <pthread.h>
#include <iostream>
#include <unistd.h>
using namespace std;


class RCMutex
{
public:
  class RCMutexImp
  {
    RCMutexImp() : count(1), d_mutex(new pthread_mutex_t)
    {

    }
    ~RCMutexImp()
    {
      delete d_mutex;
    }
  private:
    int count;
    pthread_mutex_t *d_mutex;

    friend class RCMutex;
  };

  RCMutex() : d_imp(new RCMutexImp)
  {
  }
  
  RCMutex(const RCMutex &orig)
  {
    d_imp=orig.d_imp;
    d_imp->count++;
  }

  RCMutex &operator=(const RCMutex& rhs)
  {
    if(rhs.d_imp!=d_imp) {
      if(!--d_imp->count)
	delete d_imp;
      
      d_imp=rhs.d_imp;
      ++d_imp->count;
    }
    return *this;
  }
  ~RCMutex()
  {
    if(!--d_imp->count)
      delete d_imp;
  }
  
  operator pthread_mutex_t*()
  {
    return d_imp->d_mutex;
  }

private:
  RCMutexImp *d_imp;
};

class Lock
{
  pthread_mutex_t *d_lock;
public:

  Lock(pthread_mutex_t *lock) : d_lock(lock)
  {
    pthread_mutex_lock(d_lock);
    
  }
  ~Lock()
  {
    pthread_mutex_unlock(d_lock);
  }

  void un()
  {
    pthread_mutex_unlock(d_lock);
  }

  void re()
  {
    pthread_mutex_lock(d_lock);
  }
};
#endif
