#!/bin/sh
rm -f \
Makefile \
config.status \
config.cache \
config.log \
configure \
Makefile.in \
aclocal.m4 \
config.guess \
config.sub \
install-sh \
mkinstalldirs \
missing \
depcomp \
*/Makefile \
src/Makefile.in \
ltconfig \
ltmain.sh \
libtool \
compile \
config.h \
intl/libintl.h \
src/gnuconfig.h \
src/gnuconfig.h.in \
src/xpm/Makefile.in \
src/xpm/Makefile \
pixmaps/Makefile.in \
pixmaps/flags/Makefile.in \
po/Makefile.in \
po/Makefile.in.in \
po/POTFILES \
po/*.gmo \
INSTALL \
COPYING

# gettextize
rm -rf \
ABOUT-NLS \
config.rpath \
mkinstalldirs \
m4 \
po/Makefile.in \
po/Makevars.template \
po/Rules-quot \
po/boldquot.sed \
po/en@boldquot.header \
po/en@quot.header \
po/insert-header.sin \
po/quot.sed \
po/remove-potcdate.sin \
intl/*.* \
intl/ChangeLog \
intl/VERSION

#intltool
rm -rf \
intltool-*.in \
po/remove-potcdate.sed \
po/stamp-po

rm -rf \
pixmaps/flags/*.png
