#!/bin/sh
# chkconfig: - 80 75
# description: PowerPOP makes email available using the pop3 protocol. 

prefix=/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/sbin

case "$1" in
	start | stop | status)
	$bindir/powerpop $1
	;;

	force-reload | restart)
	$bindir/powerpop stop
	$bindir/powerpop start
	;;
	
	*)
	echo powerpop [start\|stop\|restart\|force-reload\|status]
	;;
esac

