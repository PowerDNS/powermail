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

	restart)
	$bindir/powersmtp stop
	$bindir/powersmtp start
	;;
	
	*)
	echo ahudns [start\|stop\|restart\|status]
	;;
esac

