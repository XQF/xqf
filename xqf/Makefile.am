SUBDIRS = po src docs pixmaps

man_MANS = xqf.6

Applicationsdir = $(datadir)/applications
Applications_in_files = xqf.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =			\
	BUGS			\
	autogen.sh		\
	clean.sh		\
	$(man_MANS)		\
	xqf.spec		\
	$(Applications_DATA)	\
	intltool-merge.in	\
	intltool-update.in	\
	intltool-extract.in

DISTCLEANFILES =			\
	po/.intltool-merge-cache	\
	intltool-extract		\
	intltool-merge			\
	intltool-update			\
	$(Applications_DATA)

