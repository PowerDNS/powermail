AM_CXXFLAGS=-DSYSCONFDIR=\"@sysconfdir@\" -DBINDIR=\"@bindir@\" -DLOCALSTATEDIR=\"@localstatedir@\" -D_GNU_SOURCE
bin_PROGRAMS=powermail

EXTRA_DIST=make.init.d.in docs powersmtp.init.d powerpop.init.d pplistener.init.d \
mailboxes.dist README COPYING

powermail_SOURCES=powerpop.cc popsession.cc popsession.hh \
	session.cc session.hh lock.hh ssql.hh smysql.cc smysql.hh \
	userbase.hh logger.cc logger.hh common.cc mboxdir.cc\
	pptalker.cc talker.hh pptalker.hh misc.cc misc.hh argsettings.cc argsettings.hh \
	megatalker.cc megatalker.hh serversel.cc serversel.hh smtptalker.cc \
	smtptalker.hh  userbase.cc  powersmtp.cc smtpsession.cc smtpsession.hh \
	pptool.cc delivery.cc delivery.hh pplistener.cc ppsession.cc ppsession.hh \
	powerimap.cc imapsession.cc imapsession.hh ahuexception.hh common.hh \
	pool.hh


powermail_LDADD= -lmysqlclient_r -lz  -ldl -lcrypt 
# -L/opt/oracle/lib -ldl -lclient8  -lclntst8 -ldl
powermail_LDFLAGS=-pthread @userbasesfiles@

SUBDIRS=codedocs userbases 

install-exec-local: 
	-mkdir -p $(DESTDIR)/etc/init.d/
	$(INSTALL) powersmtp.init.d $(DESTDIR)/etc/init.d/powersmtp
	$(INSTALL) powerpop.init.d $(DESTDIR)/etc/init.d/powerpop
	$(INSTALL) pplistener.init.d $(DESTDIR)/etc/init.d/pplistener
	strip $(DESTDIR)/usr/bin/powermail
	-mkdir -p $(DESTDIR)/usr/sbin/
	ln -f $(DESTDIR)/usr/bin/powermail $(DESTDIR)/usr/sbin/powersmtp
	ln -f $(DESTDIR)/usr/bin/powermail $(DESTDIR)/usr/sbin/powerpop
	ln -f $(DESTDIR)/usr/bin/powermail $(DESTDIR)/usr/sbin/powerimap
	ln -f $(DESTDIR)/usr/bin/powermail $(DESTDIR)/usr/sbin/pplistener
	ln -f $(DESTDIR)/usr/bin/powermail $(DESTDIR)/usr/bin/mboxdir
	ln -f $(DESTDIR)/usr/bin/powermail $(DESTDIR)/usr/bin/pptool


install-data-local:
	-mkdir -p $(DESTDIR)$(sysconfdir)
	@(if [ -e $(DESTDIR)$(sysconfdir)/powersmtp.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/powersmtp.conf file;else $(DESTDIR)/usr/sbin/powersmtp --make-config > $(DESTDIR)$(sysconfdir)/powersmtp.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/powerpop.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/powerpop.conf file;else $(DESTDIR)/usr/sbin//powerpop --make-config > $(DESTDIR)$(sysconfdir)/powerpop.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/pplistener.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/pplistener.conf file;else $(DESTDIR)/usr/sbin//pplistener --make-config > $(DESTDIR)$(sysconfdir)/pplistener.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/power.conf ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/power.conf file;else echo > $(DESTDIR)$(sysconfdir)/power.conf;fi )
	@(if [ -e $(DESTDIR)$(sysconfdir)/mailboxes ] ;then echo Not overwriting existing $(DESTDIR)$(sysconfdir)/mailboxes file;else cat mailboxes.dist > $(DESTDIR)$(sysconfdir)/mailboxes;fi )
	-mkdir -p $(DESTDIR)$(localstatedir)
	@(if [ -e $(DESTDIR)$(localstatedir)/messages ] ; then echo "Not changing existing $(DESTDIR)$(localstatedir)/messages directory" ; else mkdir $(DESTDIR)$(localstatedir)/messages ; fi)
	@(if [ -e $(DESTDIR)$(localstatedir)/state ] ; then echo "Not changing existing $(DESTDIR)$(localstatedir)/state directory" ; else mkdir $(DESTDIR)$(localstatedir)/state ; fi)

