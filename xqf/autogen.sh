#!/bin/bash
die()
{
	echo "$1 failed, are you sure you have the right version of $2 installed?"
	exit 1
}

libtoolize --force || die libtoolize libtool
aclocal || die aclocal automake
autoheader || die autoheader autoconf
automake -a || die automake automake
autoconf || die autoconf autoconf
