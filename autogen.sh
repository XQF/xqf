#!/bin/bash
die()
{
	echo "$1 failed, are you sure you have the right version of $2 installed?"
	exit 1
}

#In case people have both autoconf 2.5 and 2.1 installed,
#  we will try to select 2.5 -baa
export WANT_AUTOCONF_2_5=1


libtoolize --force || die libtoolize libtool
aclocal || die aclocal automake
autoheader || die autoheader autoconf
automake -a || die automake automake
autoconf || die autoconf autoconf
