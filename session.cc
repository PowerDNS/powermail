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
#include "session.hh"
#include "ahuexception.hh"
#include "misc.hh"
#include <strings.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sstream>
#include <signal.h>

#ifdef __FreeBSD__
# include <sys/uio.h>
#else
# include <sys/sendfile.h>
#endif

void Session::init()
{
  d_bufsize=15049;

  d_verbose=false;
  rdoffset=0;
  wroffset=0;
}

void Session::beVerbose()
{
  d_verbose=true;
}

Session::Session(int s, struct sockaddr_in r)
{
  init();

  remote=r;
  clisock=s;
}

int Session::close()
{
  int rc=0;
  
  if(clisock>=0) {
    shutdown(clisock,SHUT_RDWR); // helps against CLOSE_WAIT problems
    rc=::close(clisock);
  }
  clisock=-1;
  return rc;
}

Session::~Session()
{
  /* NOT CLOSING AUTOMATICALLY ANYMORE!
    if(clisock>=0)
    ::close(clisock);
  */  
}

//! This function makes a deep copy of Session
Session::Session(const Session &s)
{
  d_bufsize=s.d_bufsize;

  init(); // needs d_bufsize, but will reset rdoffset & wroffset

  rdoffset=s.rdoffset;
  wroffset=s.wroffset;
  clisock=s.clisock;
  remote=s.remote;
}  

Session::Session(const string &dest, int port, int timeout)
{
  struct hostent *h;
  h=gethostbyname(dest.c_str());
  if(!h)
    throw SessionException("Unable to resolve target name");
  
  if(timeout)
    d_timeout=timeout;

  doConnect(*(int*)h->h_addr, port);
}

Session::Session(u_int32_t ip, int port, int timeout)
{
  if(timeout)
    d_timeout=timeout;

  doConnect(ip, port);
}

void Session::setTimeout(unsigned int seconds)
{
  d_timeout=seconds;
}

// gets called from constructor context, should clean up *everything*
void Session::doConnect(u_int32_t ip, int port)
{
  init();

  if((clisock=socket(AF_INET,SOCK_STREAM,0))<0) {
    throw SessionException("Unable to get a socket: "+stringerror());
  }
  
  bzero(&remote,sizeof(remote));
  remote.sin_family=AF_INET;
  remote.sin_port=htons(port);

  remote.sin_addr.s_addr=ip;

  int flags=fcntl(clisock,F_GETFL,0);
  fcntl(clisock, F_SETFL,flags|O_NONBLOCK);

  int err;
  if((err=connect(clisock,(struct sockaddr*)&remote,sizeof(remote)))<0 && errno!=EINPROGRESS)
    {
      ::close(clisock);
      throw SessionException("connect: "+stringerror());
    }

  if(!err)
    goto done;

  fd_set rset,wset;
  struct timeval tval;

  FD_ZERO(&rset);
  FD_SET(clisock, &rset);
  wset=rset;
  tval.tv_sec=d_timeout;
  tval.tv_usec=0;

  if(!select(clisock+1,&rset,&wset,0,tval.tv_sec ? &tval : 0))
    {
      ::close(clisock); // timeout
      clisock=-1;
      errno=ETIMEDOUT;
      throw SessionTimeoutException("Timeout connecting to server");
    }
  
  if(FD_ISSET(clisock, &rset) || FD_ISSET(clisock, &wset))
    {
      socklen_t len=sizeof(err);
      if(getsockopt(clisock, SOL_SOCKET,SO_ERROR,(char *)&err,&len)<0) {
        ::close(clisock);
	throw SessionException("Error connecting: "+stringerror()); // Solaris
      }

      if(err) {
        ::close(clisock);
	throw SessionException("Error connecting: "+string(strerror(err)));
      }

    }
  else {
    ::close(clisock);
    throw SessionException("nonblocking connect failed");
  }

 done:
  fcntl(clisock,F_SETFL,flags);
}

bool Session::putLine(const string &s)
{
  int length=s.length();
  int written=0;
  int err;

  while(written<length)
    {
      fd_set wset;
      FD_ZERO(&wset);
      FD_SET(clisock, &wset);
      struct timeval tval;
      tval.tv_sec=d_timeout;
      tval.tv_usec=0;
      
      if(!select(clisock+1,0,&wset,0,tval.tv_sec ? &tval : 0))
	throw SessionTimeoutException("timeout writing line");

      if(FD_ISSET(clisock, &wset))
	{
	  socklen_t len=sizeof(err);
	  if(getsockopt(clisock, SOL_SOCKET,SO_ERROR,(char *) &err,&len)<0)
	    throw SessionException(+strerror(err)); // Solaris..
	  
	  if(err)
	    throw SessionException(strerror(err));
	}
      else
	throw SessionException("nonblocking write failed: "+string(strerror(errno)));

      err=write(clisock,s.c_str()+written,length-written);

      if(err<0)
	throw SessionException("Error writing to remote: "+stringerror());
      
      written+=err;
    }

  return true;
}

char *strnchr(char *p, char c, int len)
{
  int n;
  for(n=0;n<len;n++)
    if(p[n]==c)
      return p+n;
  return 0;
}

int Session::timeoutRead(int s, char *buf, size_t len)
{
  fd_set rset;
  FD_ZERO(&rset);
  FD_SET(clisock, &rset);
  struct timeval tval;
  tval.tv_sec=d_timeout;
  tval.tv_usec=0;
  
  int err;
  if(!select(clisock+1,&rset,0,0,tval.tv_sec ? &tval : 0))
    throw SessionTimeoutException("timeout reading");
  
  if(FD_ISSET(clisock, &rset)) {
    socklen_t len=sizeof(err);
    if(getsockopt(clisock, SOL_SOCKET,SO_ERROR,(char *)&err,&len)<0)
      throw SessionException(strerror(errno)); // Solaris..
    
    if(err)
      throw SessionException(strerror(err));
  }
  else
    throw SessionException("nonblocking read failed"+string(strerror(errno)));
  
  return read(s,buf,len);
}

bool 
Session::haveLine()
{
  return (wroffset!=rdoffset && (strnchr(rdbuf+rdoffset,'\n',wroffset-rdoffset)!=NULL));
}
	

void Session::getLine(string &line)
{
  int bytes;
  char *p;

  int linelength;
  
  // read data into a buffer
  // find first \n, and return that as string, store how far we were

  for(;;) {
    if(wroffset==rdoffset)
      wroffset=rdoffset=0;


    if(wroffset!=rdoffset && (p=strnchr(rdbuf+rdoffset,'\n',wroffset-rdoffset))) { // we have a full line in store, return that 
      // from rdbuf+rdoffset to p should become the new line
      linelength=p-(rdbuf+rdoffset); 
      
      *p=0; // terminate
      
      line=rdbuf+rdoffset;
      line+="\n";
      
      rdoffset+=linelength+1;
      
      return;
    }
    // we need more data before we can return a line

    if(wroffset==d_bufsize) { // buffer is full, flush to left
      if(!rdoffset) { // line too long!
	throw SessionException("Line too long");
      }
      
      memmove(rdbuf,rdbuf+rdoffset,wroffset-rdoffset);
      wroffset-=rdoffset;
      rdoffset=0;
    }
    bytes=timeoutRead(clisock,rdbuf+wroffset,d_bufsize-wroffset);

    if(bytes<0)
      throw SessionException("error on read from socket: "+string(strerror(errno)));

    if(bytes==0)
      throw SessionException("Connection closed by foreign host");

    wroffset+=bytes;
  }
  // we never get here, but if we do, it's bad
  throw SessionException("Unexpected error");
}
  
int Session::getSocket()
{
  return clisock;
}

string Session::getRemote ()
{
  ostringstream o;
  u_int32_t rint=htonl(remote.sin_addr.s_addr);
  o<< (rint>>24 & 0xff)<<".";
  o<< (rint>>16 & 0xff)<<".";
  o<< (rint>>8  & 0xff)<<".";
  o<< (rint     & 0xff);
  o<<":"<<htons(remote.sin_port);

  return o.str();
}

u_int32_t Session::getRemoteAddr()
{
  return htonl(remote.sin_addr.s_addr);
}

string Session::getRemoteIP()
{
  ostringstream o;
  u_int32_t rint=htonl(remote.sin_addr.s_addr);
  o<< (rint>>24 & 0xff)<<".";
  o<< (rint>>16 & 0xff)<<".";
  o<< (rint>>8  & 0xff)<<".";
  o<< (rint     & 0xff);

  return o.str();
}
  

Session *Server::accept()
{
  struct sockaddr_in remote;
  socklen_t len=sizeof(remote);

  int clisock=-1;

  while((clisock=::accept(s,(struct sockaddr *)(&remote),&len))==-1) // repeat until we have a succesful connect
    {
      //      L<<Logger::Error<<"accept() returned: "<<strerror(errno)<<endl;
      if(errno==EMFILE) {
	throw SessionException("Out of file descriptors - won't recover from that");
      }

    }

  return new Session(clisock, remote);
}



Server::Server(int p, const string &p_localaddress)
{
  d_localaddress="0.0.0.0";
  string localaddress=p_localaddress;
  port=p;

  struct sockaddr_in local;
  s=socket(AF_INET,SOCK_STREAM,0);

  if(s<0)
    {
      throw Exception(string("socket: ")+strerror(errno));
    }
  
  bzero(&local,sizeof(local));
  
  local.sin_family=AF_INET;
  
  struct hostent *h;
  if(localaddress=="")
    localaddress=d_localaddress;


  h=gethostbyname(localaddress.c_str());

  if(!h)
    throw Exception(); 
  
  local.sin_addr.s_addr=*(int*)h->h_addr;

  local.sin_port=htons(port);
  
  int tmp=1;
  if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char*)&tmp,sizeof tmp)<0)
    throw SessionException(string("Setsockopt failed: ")+strerror(errno));


  if(bind(s, (sockaddr*)&local,sizeof(local))<0)
      throw SessionException("binding to port "+itoa(port)+string(": ")+strerror(errno));
  
  if(listen(s,128)<0)
      throw SessionException("listen: "+stringerror());

}

bool Session::sendFile(int fd)
{
#ifndef __FreeBSD__
  off_t offset=0;
#endif
  struct stat buf;
  fstat(fd, &buf);

#ifdef __FreeBSD__
  sendfile(fd, clisock, 0, buf.st_size, NULL, NULL, 0);
#else
  sendfile(clisock,fd, &offset, buf.st_size);
#endif

  return 0;
}
