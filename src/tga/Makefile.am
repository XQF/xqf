INCLUDES = -I$(top_srcdir)/intl \
	-I$(top_srcdir)/src \
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

libtga_a_SOURCES=memtopixmap.c memtopixmap.h tga.c tga.h
noinst_LIBRARIES=libtga.a

EXTRA_PROGRAMS=memtopixmap

memtopixmap_CFLAGS = -DSTANDALONE $(INCLUDES)
memtopixmap_SOURCES = $(libtga_a_SOURCES)
memtopixmap_LDADD = @INTLLIBS@ $(GTK_LIBS) $(GDK_PIXBUF_LIBS) @PACKAGE_LIBS@
