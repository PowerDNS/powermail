AM_CXXFLAGS=-DSYSCONFDIR=\"@sysconfdir@\" -DBINDIR=\"@bindir@\" -DLOCALSTATEDIR=\"@localstatedir@\" -D_GNU_SOURCE
bin_PROGRAMS=powermail 

EXTRA_DIST=make.init.d.in docs powersmtp.init.d powerpop.init.d pplistener.init.d \
mailboxes.dist README COPYING

if !MYSQL
powermail_SOURCES=powerpop.cc popsession.cc popsession.hh \
	session.cc session.hh lock.hh ssql.hh  \
	userbase.hh logger.cc logger.hh common.cc mboxdir.cc\
	pptalker.cc talker.hh pptalker.hh misc.cc misc.hh argsettings.cc argsettings.hh \
	megatalker.cc megatalker.hh serversel.cc serversel.hh smtptalker.cc \
	smtptalker.hh  userbase.cc  powersmtp.cc smtpsession.cc smtpsession.hh \
	pptool.cc delivery.cc delivery.hh pplistener.cc ppsession.cc ppsession.hh \
	powerimap.cc imapsession.cc imapsession.hh ahuexception.hh common.hh \
	pool.hh 
else 
powermail_SOURCES=powerpop.cc popsession.cc popsession.hh \
	session.cc session.hh lock.hh ssql.hh  \
	userbase.hh logger.cc logger.hh common.cc mboxdir.cc\
	pptalker.cc talker.hh pptalker.hh misc.cc misc.hh argsettings.cc argsettings.hh \
	megatalker.cc megatalker.hh serversel.cc serversel.hh smtptalker.cc \
	smtptalker.hh  userbase.cc  powersmtp.cc smtpsession.cc smtpsession.hh \
	pptool.cc delivery.cc delivery.hh pplistener.cc ppsession.cc ppsession.hh \
	powerimap.cc imapsession.cc imapsession.hh ahuexception.hh common.hh \
	pool.hh smysql.cc
endif 

powermail_LDADD= @userbasesfiles@ @userbaseslibs@ 
powermail_LDFLAGS=-pthread  

SUBDIRS=codedocs userbases 

install-exec-local: 
	-mkdir -p $(DESTDIR)/etc/init.d/
	$(INSTALL) powersmtp.init.d $(DESTDIR)/etc/init.d/powersmtp
	$(INSTALL) powerpop.init.d $(DESTDIR)/etc/init.d/powerpop
	$(INSTALL) pplistener.init.d $(DESTDIR)/etc/init.d/pplistener
	strip $(DESTDIR)/$(bindir)/powermail
	-mkdir -p $(DESTDIR)/$(sbindir)
	ln -f $(DESTDIR)/$(bindir)/powermail $(DESTDIR)/$(sbindir)/powersmtp
	ln -f $(DESTDIR)/$(bindir)/powermail $(DESTDIR)/$(sbindir)/powerpop
	ln -f $(DESTDIR)/$(bindir)/powermail $(DESTDIR)/$(sbindir)/powerimap
	ln -f $(DESTDIR)/$(bindir)/powermail $(DESTDIR)/$(sbindir)/pplistener
	ln -f $(DESTDIR)/$(bindir)/powermail $(DESTDIR)/$(bindir)/mboxdir
	ln -f $(DESTDIR)/$(bindir)/powermail $(DESTDIR)/$(bindir)/pptool


install-data-local:
	-mkdir -p $(DESTDIR)$(sysconfdir)
	@(if [ -e $(DESTDIR)$(sysconfdir)/powersmtp.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/powersmtp.conf file;else $(DESTDIR)/$(sbindir)/powersmtp --make-config > $(DESTDIR)$(sysconfdir)/powersmtp.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/powerpop.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/powerpop.conf file;else $(DESTDIR)/$(sbindir)//powerpop --make-config > $(DESTDIR)$(sysconfdir)/powerpop.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/pplistener.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/pplistener.conf file;else $(DESTDIR)/$(sbindir)/pplistener --make-config > $(DESTDIR)$(sysconfdir)/pplistener.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/power.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/power.conf file;else echo > $(DESTDIR)$(sysconfdir)/power.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/mailboxes ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/mailboxes file;else cat mailboxes.dist > $(DESTDIR)$(sysconfdir)/mailboxes;fi )
	-mkdir -p $(DESTDIR)$(localstatedir)
	@(if [ -e $(DESTDIR)$(localstatedir)/messages ] ; then echo "Not changing existing $(DESTDIR)$(localstatedir)/messages directory" ; else mkdir -p $(DESTDIR)$(localstatedir)/messages ; fi)
	@(if [ -e $(DESTDIR)$(localstatedir)/state ] ; then echo "Not changing existing $(DESTDIR)$(localstatedir)/state directory" ; else mkdir -p $(DESTDIR)$(localstatedir)/state ; fi)

