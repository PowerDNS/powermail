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

	restart)
	$bindir/powerpop stop
	$bindir/powerpop start
	;;
	
	*)
	echo ahudns [start\|stop\|restart\|status]
	;;
esac

