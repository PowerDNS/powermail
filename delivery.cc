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
#include "delivery.hh"
#include <dirent.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <iomanip>
#include "argsettings.hh"
#include "common.hh"
#include "logger.hh"

extern Logger L;


Delivery::Delivery()
{
  d_mailroot=args().paramString("mail-root")+"/";
}

Delivery::~Delivery()
{
  if(d_fd>0) {
    ::close(d_fd);
    d_fd=-1;
  }
  unlinkAll();
}

void Delivery::reset()
{
  if(d_fd>0) {
    ::close(d_fd); 
    d_fd=-1;
  }
    
  unlinkAll();
  d_filenames.clear();
  d_destinations.clear();
}

int Delivery::setupMaildir(const char *dir, string *response)
{
   if(mkdir(dir,0700)==-1) 
   {
      if(errno!=EEXIST)  { /* mailbox doesn't exist and can't be made */
	*response="-ERR Error creating Maildir: "+string(strerror(errno));
	return -1;
      }
      /* it exists, so go there */
      if(chdir(dir)==-1)
      {
	*response="-ERR Error doing chdir() to "+string(dir)+string(strerror(errno));
	return -1;
      }
   }
   else
   {
      if(chdir(dir)==-1)
      {
	*response="-ERR Error doing chdir() to "+string(dir)+string(strerror(errno));
	return -1;

      }
      
      
      if(mkdir("cur",0700)==-1 || mkdir("tmp",0700))
      {
	*response="-ERR Error doing mkdir() in "+string(dir)+string(strerror(errno));
	return -1;
      }
   }
   return 0;
   /* we now have a mailbox, and we are in it */
}

string Delivery::dohash(const string &mbox, int *x, int *y)
{
  char hashdir[9];
  int len=mbox.length();
  const char *s=mbox.c_str();
  unsigned long h;
  char ch;
        
  h = 5381;
  while (len > 0) {
    ch = *s++ - 'A';
    if (ch <= 'Z' - 'A') ch += 'a' - 'A';
    h = ((h << 5) + h) ^ ch;
    --len;
  }
  /* hash now in h */
  snprintf(hashdir,8,"/%02d/%02d/",(int)(h%100),(int)((h%10000)/100));
  if(x)
    *x=(int)(h%100);
  if(y)
    *y=(int)((h%10000)/100);

  return hashdir;
}

int Delivery::makeHashDirs(const string &mbox, string *response)
{
  int x,y;
  string hash=dohash(mbox,&x,&y);
  
  if(mkdir(d_mailroot.c_str(),0700)<0 && errno!=EEXIST) {
    *response="Unable to make d_mailroot "+d_mailroot+": "+string(strerror(errno));
    return -1;
  }
  chdir(d_mailroot.c_str());
  char tmp[8];
  snprintf(tmp,3,"%02d",x);

  if(mkdir(tmp,0700)<0 && errno!=EEXIST) {
    *response="Unable to make mail root level 1 hash directory "+string(d_mailroot)+"/"+string(tmp)+": "+string(strerror(errno));
    return -1;
  }

  snprintf(tmp,7,"%02d/%02d",x,y);

  if(mkdir(tmp,0700)<0 && errno!=EEXIST) {
    *response="Unable to make mail root level 2 hash directory "+string(d_mailroot)+"/"+string(tmp)+": "+string(strerror(errno));
    return -1;
  }
  return 0;
}

string Delivery::calcMaildir(const string &mbox)
{
  return string(d_mailroot)+dohash(mbox)+mbox;
}


int Delivery::addFile(const string &address, const string &index, string *response)
{
  *response="-ERR Unspecified error";

  string filename;

  if(d_destinations.empty()) {
    if((d_fd=makeRealMessage(address, index, &filename, response))<0)
      return -1;
    d_filename=filename;
  } 
  else {
    if(makeLinkMessage(address, index, d_filename, &filename, response)<0)
      return -1;
  }

  d_filenames.push_back(filename);
  d_destinations.push_back(address);

  *response="+OK";
  return 0;
}

int Delivery::makeMessageLoc(const string &address, const string &index, string *filename, string *response)
{
  if(makeHashDirs(address, response)<0) {
    return -1;
  }
  string dir=calcMaildir(address);

  setupMaildir(dir.c_str(), response);

  ostringstream fstream;
  fstream<<dir<<"/tmp/"<<index;
  *filename=fstream.str();
  return 0;
}

int Delivery::makeRealMessage(const string &address, const string &index, string *filename, string *response)
{
  if(makeMessageLoc(address,index, filename,response))
    return -1;

  int fd;
  fd=open(filename->c_str(),O_CREAT|O_WRONLY,0600);
  if(fd<0) {
    syslog(LOG_ERR,"Error opening '%s': %m",
	   filename->c_str());
    *response="-ERR Temporary error opening file";
  }

  return fd;
}

int Delivery::makeLinkMessage(const string &address, const string &index, const string &first, string *filename, string *response)
{
  if(makeMessageLoc(address,index, filename,response))
    return -1;

  unlink(filename->c_str()); // move out of the way!
  if(link(first.c_str(),filename->c_str())) {
    syslog(LOG_ERR,"Error linking '%s' to '%s': %m",
	   first.c_str(),filename->c_str());

    *response="450 Temporary error linking file";
    return -1;
  }

  return 0;
}


int Delivery::numRecipients()
{
  return d_destinations.size();
}


int Delivery::getMessageFD(const string &mbox, const string &index, string &response)
{
  ostringstream o;
  o<<calcMaildir(mbox)<<"/cur/"<<index;

  string dir=o.str();
  int fd=open(dir.c_str(),O_RDONLY);
  if(fd<0) {
    response="-ERR "+string(strerror(errno));
    return -1;
  }
  response="+OK";
  return fd;
}

int Delivery::delMessage(const string &mbox, const string &index, string &response)
{
  ostringstream o;
  o<<calcMaildir(mbox)<<"/cur/"<<index;

  string dir=o.str();
  
  if(unlink(dir.c_str())<0 && errno!=ENOENT) {
    response="-ERR File: '"+dir+"' "+string(strerror(errno));
    return -1;
  }
  response="+OK";
  return 0;
}


void Delivery::listMbox(const string &mbox, string &response)
{
  string dir=calcMaildir(mbox)+"/cur/";

  DIR *dirp=opendir(dir.c_str());

  if(access(d_mailroot.c_str(),W_OK)) {
    response="-ERR: "+string(strerror(errno));
    return;
  }

  if(!dirp) {
    if(errno!=ENOENT) {
      response="-ERR ";
      response+=strerror(errno);

      syslog(LOG_ERR,"Unable to scanDir '%s': %m", dir.c_str());
      return;
    }
    else {
      response="+OK\n.";
      return; // have no mailbox like this
    }
  }
  
  response="+OK\n";
  string file;
  struct dirent *dp;
  struct stat filest;
  ostringstream o;
  while ((dp = readdir(dirp)) != 0) {
    file=dir;
    file+=string("/")+dp->d_name;
    if (stat(file.c_str(), &filest) == 0) {
      if (S_ISREG(filest.st_mode)) {
	o<<dp->d_name<<" "<<filest.st_size<<"\n";
      }
      
    }
  }
  response+=o.str();
  response+=".";
  closedir(dirp);
  return;
}

int Delivery::giveLine(const string &line)
{
  return write(d_fd,line.c_str(),line.size());
}


int Delivery::moveAll()
{
  string dest;
  unsigned int pos;
  for(vector<string>::iterator i=d_filenames.begin();
      i!=d_filenames.end();
      ++i) {
    pos=i->find("/tmp/");
    if(pos==string::npos)
      continue;

    dest=i->substr(0,pos);
    dest+="/cur/";
    dest+=i->substr(pos+4);

    if(rename(i->c_str(),dest.c_str())<0) {
      syslog(LOG_ERR,"Error renaming '%s' to '%s': %m",
	     i->c_str(), dest.c_str());
      
      return -1;
    }
    *i=dest; // log the rename
  }
  return 0;
}


void Delivery::unlinkAll()
{
  for(vector<string>::const_iterator i=d_filenames.begin();
      i!=d_filenames.end();
      ++i) {
    unlink(i->c_str());
  }
}



void Delivery::commit()
{
  if(::close(d_fd)<0) { // NFS dorks
    int tmp=errno;
    unlinkAll();
    throw DeliveryException(string("Close error: ")+strerror(tmp));
  }
  
  if(moveAll()) {
    int tmp=errno;
    unlinkAll();
    throw DeliveryException(string("Error moving message into mailbox: ")+strerror(tmp));
  }
  else
    d_filenames.clear();

}

void Delivery::rollBack(string &response)
{
  reset();
  unlinkAll();
  response="+OK";
}

void Delivery::startList()
{
  d_listYvect.clear(); 
  d_listXvect.clear();

  DIR *dir=opendir(d_mailroot.c_str());
  if(!dir) {
    if(errno!=ENOENT) 
      throw DeliveryException("Unable to open dir '"+d_mailroot+"': "+strerror(errno));
    return;
  }
  struct dirent *entry;
  while((entry=readdir(dir)))
    if(isdigit(entry->d_name[0]))
      d_listXvect.push_back(atoi(entry->d_name));
  closedir(dir);

  d_listX=d_listXvect.begin();
  d_listY=d_listYvect.begin(); // haha, is end() too! (geeky me)
}

bool Delivery::getListNext(string &response)
{
 back:;
  if(d_listX==d_listXvect.end())
    return false; 

  if(d_listY==d_listYvect.end()) { 
    d_listYvect.clear();
    ostringstream dirname;
    dirname<<d_mailroot<<setw(2)<<setfill('0')<<*(d_listX++)<<"/";
    
    DIR *dir=opendir(dirname.str().c_str());
    if(!dir) {
      if(errno!=ENOENT) 
	L<<Logger::Error<<"Unable to open dir '"<<dirname.str()<<"': "<<strerror(errno)<<endl;
      goto back; // phear me
    }
    struct dirent *entry;
    while((entry=readdir(dir))) 
      if(isdigit(entry->d_name[0]))
	d_listYvect.push_back(atoi(entry->d_name));
    
    closedir(dir);
    d_listY=d_listYvect.begin();
  }


  ostringstream dirname;
  dirname<<d_mailroot;
  dirname<<setw(2)<<setfill('0');
  dirname<<*(d_listX-1)<<"/"; // nasty wart
  dirname<<setw(2)<<setfill('0');
  dirname<<*d_listY;

  DIR *dir=opendir(dirname.str().c_str());
  if(!dir) {
    if(errno!=ENOENT) {
      L<<Logger::Error<<"Unable to open dir '"<<dirname.str()<<"': "<<strerror(errno)<<endl;
    }
    d_listY++;
    goto back;

  }
  struct dirent *entry;
  while((entry=readdir(dir))) {
    if(!strchr(entry->d_name,'@'))
      continue;
      
    response.append(entry->d_name);
    response.append("\r\n");
  }
  closedir(dir);
      
  d_listY++;
  return true;
}

void Delivery::nuke(const string &mbox, string &response)
{
  string dir=calcMaildir(mbox);
  try {
    nukeDir(dir+"/cur");
    nukeDir(dir+"/tmp");
    nukeDir(dir+"/");
    response="+OK";
  }
  catch(DeliveryException &e) {
    response="-ERR "+e.getReason();
  }
  return;
}

void Delivery::nukeDir(const string &dir)
{
  DIR *dirp=opendir(dir.c_str());

  if(!dirp) {
    if(errno!=ENOENT) 
      throw DeliveryException("Unable to open '"+dir+"': "+strerror(errno));
    else 
      return; // have no mailbox like this
  }
  
  string file;
  struct dirent *dp;
  struct stat filest;
  ostringstream o;
  while ((dp = readdir(dirp)) != 0) {
    file=dir;
    file+=string("/")+dp->d_name;
    if (stat(file.c_str(), &filest) == 0) {
      if (S_ISREG(filest.st_mode)) {
	if(unlink(file.c_str())<0 && errno!=ENOENT)
	  throw DeliveryException("Deleting file '"+file+"': "+strerror(errno));
      }
    }
  }
  if(rmdir(dir.c_str())<0 && errno!=ENOENT)
    throw DeliveryException("Removing directory '"+dir+"': "+strerror(errno));
  return;
}


