Buildroot: /tmp/powermail
Name: powermail
Version: 1.4
Release: 1
Summary: extremely powerful and versatile mail receiver
Copyright: see /usr/doc/pdns/copyright
Distribution: Neutral
Vendor: PowerDNS.COM BV
Group: System/MAIL
AutoReqProv: no

%define _rpmdir ../
%define _rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm

%description
PowerMail is a wonderful mailserver. Use it.
This RPM is statically compiled and should work on all Linux distributions.

%files
%defattr(-,root,root)
"/usr/sbin/powersmtp"
"/usr/sbin/powerpop"
"/usr/bin/pptool"
"/usr/sbin/pplistener"
#%dir "/usr/lib/"
#%dir "/usr/lib/powerdns/"
#"/usr/lib/powerdns/libbindbackend.so"
#"/usr/lib/powerdns/libgpgsqlbackend.so"
#"/usr/lib/powerdns/libmysqlbackend.so"
#"/usr/lib/powerdns/libpipebackend.so"
#%dir "/usr/share/doc/pdns/"
%dir "/etc/powermail/"
%config "/etc/powermail/power.conf"
%config "/etc/powermail/powersmtp.conf"
%config "/etc/powermail/powerpop.conf"
%config "/etc/powermail/pplistener.conf"
%config "/etc/powermail/mailboxes"
%config "/etc/init.d/powersmtp"
%config "/etc/init.d/powerpop"
%config "/etc/init.d/pplistener"
%dir "/var/powermail/"
%post
#echo Remember to create a 'pdns' user before starting pdns
