#!/bin/bash
die()
{
	echo "$1 failed, are you sure you have the right version of $2 installed?"
	exit 1
}

#In case people have both autoconf 2.5 and 2.1 installed,
#  we will try to select 2.5 -baa
export WANT_AUTOCONF_2_5=1


# workaround for libtoolize putting files in ..
if [ ! -e install-sh ]; then
	removeinstallsh=1
	touch install-sh
fi

echo "running libtoolize ..."
libtoolize --force || die libtoolize libtool

[ -n "$removeinstallsh" ] && rm install-sh

echo "running aclocal ..."
aclocal || die aclocal automake
echo "running autoheader ..."
autoheader || die autoheader autoconf
echo "running automake ..."
automake -a || die automake automake
echo "running autoconf ..."
autoconf || die autoconf autoconf
