SUBDIRS = po src docs pixmaps

man_MANS = xqf.6

DESKTOP_FILES = xqf.desktop

desktopdir= $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

EXTRA_DIST =			\
	autogen.sh		\
	clean.sh		\
	$(man_MANS)		\
	$(DESKTOP_FILES)	\
	intltool-merge.in	\
	intltool-update.in	\
	intltool-extract.in

DISTCLEANFILES =			\
	po/.intltool-merge-cache	\
	intltool-extract		\
	intltool-merge			\
	intltool-update
