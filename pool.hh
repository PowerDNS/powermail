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
#include <string>
#include <iostream>
#include <queue>
#include <semaphore.h>
#include <signal.h>
#include "lock.hh"
using namespace std;

/** Generic pool, very useful for sharing database connections. 

    Has features to keep a few connections precreated handy,
    will cleanup unused connections and can be configured to create only a certain amount of connections. Uses a handle with 
    reference counting to make sure that connections are returned after the pool at all times */
template<class Thing> class PoolClass
{
public:
  typedef Thing *genFunc();
  /** Handle is returned by the PoolClass::get() method. On destruction of the last copy of it it will return
      the Thing to the pool. The content of this Handle is in d_thing. */
  friend class Handle;
  class Handle
  {
  public:
    Handle(Thing *thing, PoolClass *parent)
    {
      // cout<<(void *)this<<": handle created for thing "<<(void *)thing<<endl;
      d_thing=thing;
      d_count=1;

      d_parent=parent;
    }

    Handle(const Handle &handle)
    {
      // cout<<(void *)this<<": Copy constructor called with argument "<<(void *)&handle<<endl;
      d_thing=handle.d_thing;
      d_parent=handle.d_parent;
      d_count=handle.d_count++;
      // cout<<(void *)this<<": our count is now: "<<d_count<<endl;
    }

    ~Handle()
    {
      if(!--d_count && d_thing) {
	// cout<<(void *)this<<": giving back to the pool"<<endl;
	d_parent->giveBack(d_thing);
      }
      else
	; // cout<<(void *)this<<": Not giving back to the pool (d_count="<<d_count+1<<")"<<endl;
    }
    /** Only public method. This can be called to notify the pool that our Thing died, and should not
	be returned to the pool */
    void invalidate()
    {
      //      cout<<"notifying pool the thing died"<<endl;
      d_parent->died();
      delete d_thing;
      d_thing=0;
    }

    Thing *d_thing;
  private:
    PoolClass *d_parent;
    mutable int d_count;
  };

  /** Constructor of the PoolClass. gfp is a pointer to a function that can generate new things */
  PoolClass(genFunc *gfp)
  {
    pthread_mutex_init(&d_lock, 0);
    sem_init(&d_sem,0,0);
    d_gfp=gfp;
    d_created=0;
    d_max=0;
    d_maxspare=5;
  }
  
  //! Maximum number of Things to be alive at any one time
  void setMax(int newmax)
  {
    d_max=newmax;
  }

  //! Maximum number of Things to have on hand unused. 
  void setMaxSpares(int newmax)
  {
    d_maxspare=newmax;
  }


  ~PoolClass()
  {
    clean();
  }

  /** stock a number of things beforehand */
  void stock(unsigned int number)
  {
    while(number--) {
      {
	Lock l(&d_lock);
	d_created++;
	// cout<<"Stocked one"<<endl;
	d_available.push_front((*d_gfp)());
      }
      sem_post(&d_sem);
    }
  }

  /** request a thing for our exclusive use */
  Handle get()
  {
    {
      Lock l(&d_lock);

      if(d_available.empty()) {
	if(!d_max || d_created<d_max) {
	  d_created++;
	  d_available.push_front((*d_gfp)());

	  sem_post(&d_sem);
	}
	else
	  ;
      }
    }

    sem_wait(&d_sem);

    Thing *thing;
    {

      Lock l(&d_lock);

      thing=d_available.front();
      d_available.pop_front();



    }
    return Handle(thing,this);
  }

  typedef Handle handle;
private:

  /** Used by the handle to report that a backend died or was unhappy and that it deleted it. So we generate one to replace it */
  void died()
  {
    stock(1);
  }

  /** Handle calls this to return a thing. If we have enough spares already we'll just delete it and 
      not add it back */
  void giveBack(Thing *thing)
  {
    Lock l(&d_lock);
    if(d_available.size()<d_maxspare) {
      d_available.push_back(thing);
      sem_post(&d_sem);

    }
    else {
      delete thing;
      d_created--;
    }
  }  
  void clean()
  {
    Lock l(&d_lock);
    // cout<<"Cleaning the pool"<<endl;
    for(typename deque<Thing *>::iterator i=d_available.begin();i!=d_available.end();++i)
      delete *i;
    d_available.clean();
    // cout<<"Pool clean"<<endl;
  }

  genFunc *d_gfp;
  deque<Thing *> d_available;
  pthread_mutex_t d_lock;
  sem_t d_sem;
  int d_created;
  int d_max;
  unsigned int d_maxspare;
};


#ifdef TESTING

class Connection
{
public:
  Connection()
  {
    cout<<(void *)this<<": Connection made!"<<endl;
  }
  ~Connection()
  {
    cout<<(void *)this<<": Connection destroyed!"<<endl;
  }
  void doSomething()
  {
    cout<<(void *)this<<": Connection doing something"<<endl;
  }
};

Connection *maker()
{
  return new Connection;
}

PoolClass<Connection>::handle *g_handle;

void handler(int)
{
  cout<<"Called"<<endl;
  g_handle->invalidate();
}



int main(int argc, char **argv)
{
  
  PoolClass<Connection> pool(&maker);

  {
    PoolClass<Connection>::handle handle=pool.get();
    handle.d_thing->doSomething();
  }
  cout<<"----"<<endl;
  {
    PoolClass<Connection>::handle handle=pool.get();
    handle.d_thing->doSomething();
  }
  cout<<"----"<<endl;
  {
    PoolClass<Connection>::handle handle=pool.get();
    PoolClass<Connection>::handle handle2=pool.get();
    handle.d_thing->doSomething();
    handle2.d_thing->doSomething();
  }
  cout<<"----"<<endl;
  {
    PoolClass<Connection>::handle handle=pool.get();
    handle.d_thing->doSomething();
  }
  cout<<"----"<<endl;
  {
    PoolClass<Connection>::handle handle=pool.get();
    handle.d_thing->doSomething();
  }
  pool.setMax(3);
  cout<<"----"<<endl;
  {
    PoolClass<Connection>::handle handle=pool.get();
    PoolClass<Connection>::handle handle2=pool.get();
    PoolClass<Connection>::handle handle3=pool.get();
    g_handle=&handle;
    signal(SIGALRM, handler);
    alarm(1);
    PoolClass<Connection>::handle handle4=pool.get();

    handle.d_thing->doSomething();
    handle2.d_thing->doSomething();
  }
  cout<<"Exiting"<<endl;
}

#endif
