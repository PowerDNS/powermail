#!/bin/sh

# chkconfig: - 80 75
# description: PowerSMTP receives incoming email

prefix=/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/sbin

case "$1" in
	start | stop | status)
	$bindir/powersmtp $1
	;;

	force-reload | restart)
	$bindir/powersmtp stop
	$bindir/powersmtp start
	;;
	
	*)
	echo powersmtp [start\|stop\|restart\|force-reload\|status]
	;;
esac

