SUBDIRS = xpm

localedir = $(datadir)/locale
INCLUDES = -I../intl -DLOCALEDIR=\"$(localedir)\" -I@includedir@ @GTK_CFLAGS@
CFLAGS = @CFLAGS@ @QSTAT23@ @DEBUG@ @COMPRESSION@

bin_PROGRAMS = xqf

xqf_LDADD = @INTLLIBS@ @GTK_LIBS@

xqf_SOURCES =  \
addmaster.c \
addserver.c \
config.c \
debug.c \
dialogs.c \
dns.c \
filter.c \
flt-player.c \
game.c \
history.c \
host.c \
launch.c \
menus.c \
pixmaps.c \
pref.c \
psearch.c \
rc.c \
rcon.c \
server.c \
skin.c \
skin_pcx.c \
sort.c \
source.c \
srv-info.c \
srv-list.c \
srv-prop.c \
stat.c \
statistics.c \
utils.c \
xqf.c \
xqf-ui.c \
xutils.c \
zipped.c \
addmaster.h \
addserver.h \
config.h \
debug.h \
dialogs.h \
dns.h \
filter.h \
flt-player.h \
game.h \
history.h \
host.h \
launch.h \
menus.h \
pixmaps.h \
pref.h \
psearch.h \
quake2_pal.h \
quake_pal.h \
rc.h \
rcon.h \
server.h \
skin.h \
skin_pcx.h \
sort.h \
source.h \
srv-info.h \
srv-list.h \
srv-prop.h \
stat.h \
statistics.h \
utils.h \
xqf-ui.h \
xqf.h \
xutils.h \
zipped.h
