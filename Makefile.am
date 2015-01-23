SUBDIRS = po src docs pixmaps

man_MANS = xqf.6

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES = xqf.desktop.in
DESKTOP_FILES = $(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir= $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

EXTRA_DIST =			\
	BUGS			\
	autogen.sh		\
	clean.sh		\
	$(man_MANS)		\
	xqf.spec		\
	$(DESKTOP_IN_FILES)	\
	intltool-merge.in	\
	intltool-update.in	\
	intltool-extract.in

DISTCLEANFILES =			\
	po/.intltool-merge-cache	\
	intltool-extract		\
	intltool-merge			\
	intltool-update			\
	$(DESKTOP_FILES)

