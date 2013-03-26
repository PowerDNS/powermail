dnl intro
AC_INIT(pplistener.cc)
AM_INIT_AUTOMAKE(powermail, 1.9)
AC_PREFIX_DEFAULT(/usr/local)
dnl AC_SYSCONFDIR_DEFAULT(/etc/powermail)
if test -z "$CXXFLAGS"
then
	CXXFLAGS="-Wall -pthread -O2"
fi

AM_CONFIG_HEADER(config.h pplistenerdefaults.h)
AC_PROG_CC
AC_PROG_CXX
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl Checks for header files.
AC_HEADER_STDC

AC_CHECK_HEADERS( sys/types.h sys/fcntl.h limits.h string.h errno.h db.h \
sys/types.h stdlib.h unistd.h pwd.h ctype.h sys/types.h time.h unistd.h)


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
# AC_CHECK_FUNCS(gethostname gettimeofday mkdir mktime select socket strerror)

#
# Static Binary Support
#

AC_MSG_CHECKING(whether we should build static binaries)
AC_ARG_ENABLE(static-binaries,
 [  --enable-static-binaries    Build static binaries],enable_static_binaries=yes ,enable_static_binaries=no)

if test $enable_static_binaries = yes;
then
  LDFLAGS="-all-static $LD_FLAGS"
fi
AC_MSG_RESULT($enable_static_binaries)


userbases="mysqlpdns mysqlplain mysqlsimplepdns text"
AC_ARG_WITH(userbases, [  --with-userbases Which userbases to compile with ], 
[
	userbases="$withval"  
])



#
# Location of MySQL installation
#


if echo $userbases | grep mysql > /dev/null
then 
AM_CONDITIONAL(MYSQL, true)
AC_ARG_WITH(mysql,
    [  --with-mysql=<path>     root directory path of MySQL installation],
    [MYSQL_lib_check="$withval/lib/mysql $with_mysql/lib"
MYSQL_inc_check="$withval/include/mysql"],
    [MYSQL_lib_check="/usr/local/mysql/lib/mysql /usr/local/lib/mysql /opt/mysql/lib/mysql /usr/lib/mysql /usr/local/mysql/lib /usr/local/lib /opt/mysql/lib /usr/lib"
MYSQL_inc_check="/usr/local/mysql/include /usr/include/mysql /usr/include /opt/mysql/include"])

AC_ARG_WITH(mysql-lib,
    [  --with-mysql-lib=<path> directory path of MySQL library installation],
    [MYSQL_lib_check="$withval/lib/mysql $withval/mysql $withval"])

AC_ARG_WITH(mysql-includes,
    [  --with-mysql-includes=<path>
                          directory path of MySQL header installation],
    [MYSQL_inc_check="$withval/include $withval/mysql $withval"])

AC_MSG_CHECKING([for MySQL library directory])
MYSQL_libdir=
for m in $MYSQL_lib_check; do
        if test -d "$m" && \
	   (test -f "$m/libmysqlclient.so" || test -f "$m/libmysqlclient.a")
        then
                MYSQL_libdir=$m
                break
        fi
done

if test -z "$MYSQL_libdir"; then
        AC_MSG_ERROR([Didn't find the mysql library dir in '$MYSQL_lib_check'])
fi

case "$MYSQL_libdir" in
  /* ) ;;
  * )  AC_MSG_ERROR([The MySQL library directory ($MYSQL_libdir) must be an absolute path.]) ;;
esac

AC_MSG_RESULT([$MYSQL_libdir])

case "$MYSQL_libdir" in
  /usr/lib) ;;
  *) LDFLAGS="$LDFLAGS -L${MYSQL_libdir}" ;;
esac

AC_MSG_CHECKING([for MySQL include directory])
MYSQL_incdir=
for m in $MYSQL_inc_check; do
        if test -f "$m/mysql.h"
        then
                MYSQL_incdir="$m"
                break
        fi
done

if test -z "$MYSQL_incdir"; then
        AC_MSG_ERROR([Didn't find the mysql include dir in '$MYSQL_inc_check'])
fi

case "$MYSQL_incdir" in
  /* ) ;;
  
  * )  AC_MSG_ERROR([The MySQL include directory ($MYSQL_incdir) must be an absolute path.]) ;;
esac

AC_MSG_RESULT([$MYSQL_incdir])

CPPFLAGS="$CPPFLAGS -I${MYSQL_incdir}"
else
	AM_CONDITIONAL(MYSQL, false)
fi

if echo $userbases | grep postgresql > /dev/null
then
	AC_ARG_WITH(pgsql,
	    [  --with-pgsql=<path>     root directory path of PgSQL installation],
	    [PGSQL_lib_check="$withval/lib/pgsql $with_pgsql/lib"
	PGSQL_inc_check="$withval/include/pgsql"],
	    [PGSQL_lib_check="/usr/local/pgsql/lib/pgsql /usr/local/lib/pgsql /opt/pgsql/lib/pgsql /opt/postgresql/lib/ /usr/lib/pgsql /usr/local/pgsql/lib /usr/local/lib /opt/pgsql/lib /usr/lib"
	PGSQL_inc_check="/usr/local/pgsql/include/pgsql /usr/local/include/postgresql/ /usr/local/include /opt/postgresql/include/ /opt/pgsql/include/pgsql /opt/pgsql/include /usr/include/pgsql/ /usr/include /usr/include/postgresql"])
		AC_ARG_WITH(pgsql-lib,
	    [  --with-pgsql-lib=<path> directory path of PgSQL library installation],
	    [PGSQL_lib_check="$withval/lib/pgsql $withval/pgsql $withval"])
		AC_ARG_WITH(pgsql-includes,
	    [  --with-pgsql-includes=<path>
                         directory path of PgSQL header installation],
	    [PGSQL_inc_check="$withval/include/pgsql $withval/pgsql $withval"])
		AC_MSG_CHECKING([for PgSQL library directory])
	PGSQL_libdir=
	for m in $PGSQL_lib_check; do
	        if test -d "$m" && \
		   (test -f "$m/libpq++.so" || test -f "$m/libpq++.a")
	        then
	                PGSQL_libdir=$m
	                break
	        fi
	done
		if test -z "$PGSQL_libdir"; then
	        AC_MSG_ERROR([Didn't find the pgsql library dir in '$PGSQL_lib_check'])
	fi
	case "$PGSQL_libdir" in
           /usr/lib ) PGSQL_lib="" ;;
	  /* ) PGSQL_lib="-L$PGSQL_libdir -Wl,-rpath,$PGSQL_libdir" 
	       LDFLAGS="$PGSQL_lib $LDFLAGS"
               ;;
	  * )  AC_MSG_ERROR([The PgSQL library directory ($PGSQL_libdir) must be an absolute path.]) ;;
	esac

	AC_SUBST(PGSQL_lib)
	AC_MSG_RESULT([$PGSQL_libdir])

	AC_MSG_CHECKING([for PgSQL include directory])
	PGSQL_incdir=
	for m in $PGSQL_inc_check; do
	        if test -d "$m" && test -f "$m/libpq++.h"
	        then
	                PGSQL_incdir=$m
	                break
	        fi
	done
		if test -z "$PGSQL_incdir"; then
	        AC_MSG_ERROR([Didn't find the PgSQL include dir in '$PGSQL_inc_check'])
	fi
	case "$PGSQL_incdir" in
          /usr/include ) 
		PGSQL_incinstr="";
		;;
	  /* ) PGSQL_incinstr=-I$PGSQL_incdir ;;
	  * )  AC_MSG_ERROR([The PgSQL include directory ($PGSQL_incdir) must be an absolute path.]) ;;
	esac
	AC_SUBST(PGSQL_incinstr)
	AC_MSG_RESULT([$PGSQL_incdir])
fi

# Userbases

AC_SUBST(userbasesdirs)
AC_SUBST(userbasesfiles)
AC_SUBST(userbaseslibs)

tmp=""
for a in $userbases
do
	tmp="$tmp ./$a/Makefile"
	userbasesdirs="$userbasesdirs $a"
	userbasesfiles="$userbasesfiles $(cat userbases/$a/OBJECTFILES)"
	userbaseslibs="$userbaseslibs $(cat userbases/$a/OBJECTLIBS)"
done
export userbasesdirs userbasesfiles userbaseslibs

AC_OUTPUT(userbases/text/Makefile \
userbases/Makefile 
userbases/mysqlplain/Makefile \
userbases/mysqlpdns/Makefile \
userbases/mysqlsimplepdns/Makefile \
Makefile codedocs/Makefile \

pplistener.init.d powersmtp.init.d powerpop.init.d )

dnl userbases/postgresql/Makefile 
dnl userbases/oracle/Makefile 
dnl userbases/ldap/Makefile 
