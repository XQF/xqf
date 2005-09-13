SUBDIRS = xpm zip tga

localedir = $(datadir)/locale
INCLUDES = -I$(top_srcdir)/intl \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
	-DPIXMAPSDIR=\"$(datadir)/pixmaps\" \
	$(GTK_CFLAGS) \
	$(QSTAT23) \
	$(DEBUG) \
	$(COMPRESSION) \
	$(GDK_PIXBUF_CFLAGS) \
	$(PACKAGE_CFLAGS) \
	$(OLD_GTK_SUPPORT)

qstatcfgdir = $(pkgdatadir)
qstatcfg_DATA = qstat.cfg

EXTRA_DIST = qstat.cfg games.xml games.c $(noinst_SCRIPTS)

bin_PROGRAMS = xqf
if BUILD_XQFRCON
bin_PROGRAMS += xqf-rcon
endif

EXTRA_PROGRAMS = gamesxml2c

xqf_LDADD = $(INTLLIBS) \
	    $(top_builddir)/src/zip/libunzip.a \
	    $(top_builddir)/src/tga/libtga.a \
	    $(GTK_LIBS) \
	    $(GEOIP_LIB) \
	    $(PACKAGE_LIBS) \
	    $(GDK_PIXBUF_LIBS) \
	    -lz

xqf_LDFLAGS = -Wl,-rpath,$(pkglibdir)

# we need to specify all deps manually due to dummy libs
xqf_DEPENDENCIES = \
	$(top_builddir)/src/zip/libunzip.a \
	$(top_builddir)/src/tga/libtga.a

xqf_rcon_LDADD = @INTLLIBS@ $(GLIB_LIBS) $(XQFRCON_LIBS) @PACKAGE_LIBS@
xqf_rcon_SOURCES = \
    rcon.c rcon.h \
    debug.c debug.h \
    utils.c utils.h
xqf_rcon_CFLAGS = -DRCON_STANDALONE $(GLIB_CFLAGS) $(OLD_GTK_SUPPORT)

xqf_SOURCES =  \
addmaster.c \
addserver.c \
config.c \
country-filter.c \
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
trayicon.c \
utils.c \
xqf.c \
xqf-ui.c \
xutils.c \
zipped.c \
redial.c \
q3maps.c \
utmaps.c \
loadpixmap.c \
splash.c \
scripts.c \
addmaster.h \
addserver.h \
config.h \
country-filter.h \
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
trayicon.h \
utils.h \
xqf-ui.h \
xqf.h \
xutils.h \
zipped.h \
redial.h \
q3maps.h \
utmaps.h \
i18n.h \
loadpixmap.h \
splash.h \
scripts.h

noinst_SCRIPTS = gensyms.pl
noinst_PROGRAMS = test_utils
test_utils_SOURCES = test_utils.c \
		     utils.c utils.h \
		     debug.c debug.h
test_utils_LDADD = $(xqf_LDADD)

# it's only temporary, so ...
gamesxml2c_LDFLAGS = $(shell xml2-config --libs)
gamesxml2c_CFLAGS = $(shell xml2-config --cflags) 

pkglib_PROGRAMS =

if DUMMY_LIBGEOIP
pkglib_PROGRAMS += libxqf_dummy_GeoIP.so.0
xqf_DEPENDENCIES += libxqf_dummy_GeoIP.so
endif
if DUMMY_LIBGDKPIXBUF
pkglib_PROGRAMS += libxqf_dummy_gdk_pixbuf.so.0
xqf_DEPENDENCIES += libxqf_dummy_gdk_pixbuf.so
endif

libxqf_dummy_gdk_pixbuf.c:
	$(top_srcdir)/src/gensyms.pl -x gdk_pixbuf_new_from_file,gdk_pixbuf_render_pixmap_and_mask,gdk_pixbuf_unref $(DUMMY_LIBGDKPIXBUF_ORIG) -o $(top_srcdir)/src/libxqf_dummy_gdk_pixbuf.c || rm $(top_srcdir)/src/libxqf_dummy_gdk_pixbuf.c
	test -s $(top_srcdir)/src/libxqf_dummy_gdk_pixbuf.c

nodist_libxqf_dummy_gdk_pixbuf_so_0_SOURCES = libxqf_dummy_gdk_pixbuf.c
libxqf_dummy_gdk_pixbuf_so_0_SOURCES = libxqf_dummy_gdk_pixbuf_stubs.c
libxqf_dummy_gdk_pixbuf_so_0_LDFLAGS = -nostdlib -shared -Wl,-f,$(DUMMY_LIBGDKPIXBUF_SONAME) -Wl,-soname,libxqf_dummy_gdk_pixbuf.so.0
libxqf_dummy_gdk_pixbuf_so_0_CFLAGS = -fPIC -DPIC

libxqf_dummy_gdk_pixbuf.so: libxqf_dummy_gdk_pixbuf.so.0
	rm -f $@
	$(LN_S) $< $@

libxqf_dummy_GeoIP.c:
	$(top_srcdir)/src/gensyms.pl -x GeoIP_new,GeoIP_open $(DUMMY_LIBGEOIP_ORIG) -o $(top_srcdir)/src/libxqf_dummy_GeoIP.c || rm $(top_srcdir)/src/libxqf_dummy_GeoIP.c
	test -s $(top_srcdir)/src/libxqf_dummy_GeoIP.c

nodist_libxqf_dummy_GeoIP_so_0_SOURCES = libxqf_dummy_GeoIP.c
libxqf_dummy_GeoIP_so_0_SOURCES = libxqf_dummy_GeoIP_stubs.c
libxqf_dummy_GeoIP_so_0_LDFLAGS = -nostdlib -shared -Wl,-f,$(DUMMY_LIBGEOIP_SONAME) -Wl,-soname,libxqf_dummy_GeoIP.so.0
libxqf_dummy_GeoIP_so_0_CFLAGS = -fPIC -DPIC

libxqf_dummy_GeoIP.so: libxqf_dummy_GeoIP.so.0
	rm -f $@
	$(LN_S) $< $@

CLEANFILES = \
	$(nodist_libxqf_dummy_gdk_pixbuf_so_0_SOURCES) \
	$(nodist_libxqf_dummy_GeoIP_so_0_SOURCES) \
	libxqf_dummy_GeoIP.so \
	libxqf_dummy_gdk_pixbuf.so

gamelist: gamesxml2c
	$(top_builddir)/src/gamesxml2c $(top_srcdir)/src/games.xml > $(top_srcdir)/src/games.c

.PHONY: gamelist
