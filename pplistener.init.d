#!/bin/sh

# chkconfig: - 70 75
# description: PowerSMTP pplistener manages access to disk storage,
#              making it available to the PowerSMTP and PowerPOP services.

prefix=/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/sbin

case "$1" in
	start | stop | status)
	$bindir/pplistener $1
	;;

	force-reload | restart)
	$bindir/pplistener stop
	$bindir/pplistener start
	;;
	
	*)
	echo pplistener [start\|stop\|restart\|force-reload\|status]
	;;
esac

