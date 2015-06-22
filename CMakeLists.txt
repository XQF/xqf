cmake_minimum_required(VERSION 2.8.7)

# Project information
project(xqf)

set (VERSION "1.0.6.2")
set (DOMAIN ${PROJECT_NAME})

set (LINGUAS ca da de es fi fr pl ru)

# Paths
set (PACKAGE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/xqf")
set (LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale")
set (PIXMAPS_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor)

if (NOT CMAKE_INSTALL_PREFIX)
	set (CMAKE_INSTALL_PREFIX "/usr")
endif (NOT CMAKE_INSTALL_PREFIX)

# Definitions
add_definitions (-DPACKAGE="${PROJECT_NAME}" -DPACKAGE_VERSION="${VERSION}" -DXQF_VERSION="${VERSION}" -DDOMAIN="${DOMAIN}" -DLOCALEDIR="${LOCALEDIR}" -DPACKAGE_DATA_DIR="${PACKAGE_DATA_DIR}")

# Options
option(USE_GTK3 "Use GTK+ toolkit version 3" OFF)
option(DEPRECATED_DISABLE "Disable deprecated parts of GLib, GObject, GDK and GTK+" OFF)

option(USE_GEOIP "Depend on GeoIP library for geolocation" ON)
option(USE_GZIP "Enable gzip compressor support" ON)
option(RCON_STANDALONE "Build standalone RCON" OFF)

if (NOT WITH_QSTAT)
	set (WITH_QSTAT "/usr/bin/qstat")
endif (NOT WITH_QSTAT)

add_definitions (-DQSTAT_EXEC="${WITH_QSTAT}")

if (CMAKE_BUILD_TYPE EQUAL DEBUG)
	add_definitions (-DDEBUG)
endif (CMAKE_BUILD_TYPE EQUAL DEBUG)

if (RCON_STANDALONE)
	add_definitions (-DRCON_STANDALONE)
endif (RCON_STANDALONE)

if (USE_GEOIP)
	add_definitions (-DUSE_GEOIP)
endif (USE_GEOIP)

if (USE_GZIP)
	add_definitions (-DUSE_GZIP)
endif (USE_GZIP)

# Source list
set (xqf_HEADERS_DIR ${CMAKE_SOURCE_DIR}/src)

set (xqf_SOURCES

     ${CMAKE_SOURCE_DIR}/src/xqf.ui

     ${CMAKE_SOURCE_DIR}/src/addmaster.c
     ${CMAKE_SOURCE_DIR}/src/addserver.c
     ${CMAKE_SOURCE_DIR}/src/config.c
     ${CMAKE_SOURCE_DIR}/src/country-filter.c
     ${CMAKE_SOURCE_DIR}/src/debug.c
     ${CMAKE_SOURCE_DIR}/src/dialogs.c
     ${CMAKE_SOURCE_DIR}/src/dns.c
     ${CMAKE_SOURCE_DIR}/src/filter.c
     ${CMAKE_SOURCE_DIR}/src/flt-player.c
     ${CMAKE_SOURCE_DIR}/src/game.c
     ${CMAKE_SOURCE_DIR}/src/history.c
     ${CMAKE_SOURCE_DIR}/src/host.c
     ${CMAKE_SOURCE_DIR}/src/launch.c
     ${CMAKE_SOURCE_DIR}/src/menus.c
     ${CMAKE_SOURCE_DIR}/src/pixmaps.c
     ${CMAKE_SOURCE_DIR}/src/pref.c
     ${CMAKE_SOURCE_DIR}/src/psearch.c
     ${CMAKE_SOURCE_DIR}/src/rc.c
     ${CMAKE_SOURCE_DIR}/src/rcon.c
     ${CMAKE_SOURCE_DIR}/src/server.c
     ${CMAKE_SOURCE_DIR}/src/skin.c
     ${CMAKE_SOURCE_DIR}/src/skin_pcx.c
     ${CMAKE_SOURCE_DIR}/src/sort.c
     ${CMAKE_SOURCE_DIR}/src/source.c
     ${CMAKE_SOURCE_DIR}/src/srv-info.c
     ${CMAKE_SOURCE_DIR}/src/srv-list.c
     ${CMAKE_SOURCE_DIR}/src/srv-prop.c
     ${CMAKE_SOURCE_DIR}/src/stat.c
     ${CMAKE_SOURCE_DIR}/src/statistics.c
     ${CMAKE_SOURCE_DIR}/src/utils.c
     ${CMAKE_SOURCE_DIR}/src/xqf.c
     ${CMAKE_SOURCE_DIR}/src/xqf-ui.c
     ${CMAKE_SOURCE_DIR}/src/zipped.c
     ${CMAKE_SOURCE_DIR}/src/redial.c
     ${CMAKE_SOURCE_DIR}/src/q3maps.c
     ${CMAKE_SOURCE_DIR}/src/utmaps.c
     ${CMAKE_SOURCE_DIR}/src/loadpixmap.c
     ${CMAKE_SOURCE_DIR}/src/scripts.c
     ${CMAKE_SOURCE_DIR}/src/tga/memtopixmap.c
     ${CMAKE_SOURCE_DIR}/src/tga/tga.c

     ${CMAKE_SOURCE_DIR}/src/addmaster.h
     ${CMAKE_SOURCE_DIR}/src/addserver.h
     ${CMAKE_SOURCE_DIR}/src/config.h
     ${CMAKE_SOURCE_DIR}/src/country-filter.h
     ${CMAKE_SOURCE_DIR}/src/debug.h
     ${CMAKE_SOURCE_DIR}/src/dialogs.h
     ${CMAKE_SOURCE_DIR}/src/dns.h
     ${CMAKE_SOURCE_DIR}/src/filter.h
     ${CMAKE_SOURCE_DIR}/src/flt-player.h
     ${CMAKE_SOURCE_DIR}/src/game.h
     ${CMAKE_SOURCE_DIR}/src/history.h
     ${CMAKE_SOURCE_DIR}/src/host.h
     ${CMAKE_SOURCE_DIR}/src/launch.h
     ${CMAKE_SOURCE_DIR}/src/menus.h
     ${CMAKE_SOURCE_DIR}/src/pixmaps.h
     ${CMAKE_SOURCE_DIR}/src/pref.h
     ${CMAKE_SOURCE_DIR}/src/psearch.h
     ${CMAKE_SOURCE_DIR}/src/quake2_pal.h
     ${CMAKE_SOURCE_DIR}/src/quake_pal.h
     ${CMAKE_SOURCE_DIR}/src/rc.h
     ${CMAKE_SOURCE_DIR}/src/rcon.h
     ${CMAKE_SOURCE_DIR}/src/server.h
     ${CMAKE_SOURCE_DIR}/src/skin.h
     ${CMAKE_SOURCE_DIR}/src/skin_pcx.h
     ${CMAKE_SOURCE_DIR}/src/sort.h
     ${CMAKE_SOURCE_DIR}/src/source.h
     ${CMAKE_SOURCE_DIR}/src/srv-info.h
     ${CMAKE_SOURCE_DIR}/src/srv-list.h
     ${CMAKE_SOURCE_DIR}/src/srv-prop.h
     ${CMAKE_SOURCE_DIR}/src/stat.h
     ${CMAKE_SOURCE_DIR}/src/statistics.h
     ${CMAKE_SOURCE_DIR}/src/utils.h
     ${CMAKE_SOURCE_DIR}/src/xqf-ui.h
     ${CMAKE_SOURCE_DIR}/src/xqf.h
     ${CMAKE_SOURCE_DIR}/src/zipped.h
     ${CMAKE_SOURCE_DIR}/src/redial.h
     ${CMAKE_SOURCE_DIR}/src/q3maps.h
     ${CMAKE_SOURCE_DIR}/src/utmaps.h
     ${CMAKE_SOURCE_DIR}/src/loadpixmap.h
     ${CMAKE_SOURCE_DIR}/src/scripts.h
     ${CMAKE_SOURCE_DIR}/src/tga/memtopixmap.h
     ${CMAKE_SOURCE_DIR}/src/tga/tga.h
     )

set (flag_DATA
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/lan.png

    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ae.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ag.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ar.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/at.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/au.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/az.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ba.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/bb.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/bd.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/be.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/bg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/bh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/bo.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/br.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/by.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ca.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ch.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/cl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/cn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/co.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/cr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/cu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/cz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/de.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/dj.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/dk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/do.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/dz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ec.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ee.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/eg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/es.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/fi.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/fr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/gb.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/gd.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/gh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/gr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/gt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/hn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/hr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/hu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/id.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ie.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/il.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/in.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/iq.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ir.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/is.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/it.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/jm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/jo.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/jp.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/kn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/kp.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/kr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/kw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/lb.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/lc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/lt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/lu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/lv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ly.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ma.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/mk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/mt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/mx.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ni.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/nl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/no.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/nz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/om.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/pa.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/pe.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/pl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ps.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/pt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/py.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/qa.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ro.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ru.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/sa.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/sd.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/se.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/si.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/sk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/so.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/sr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/sv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/sy.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/th.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/tn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/tr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/tt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/tw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ua.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/us.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/uy.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/vc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ve.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/vn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/ye.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/za.png
    )


set (xpm_MAIN ${CMAKE_SOURCE_DIR}/pixmaps/xqf.xpm)

set (xpm_DATA

     ${CMAKE_SOURCE_DIR}/src/xpm/update.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/refresh.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/refrsel.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/stop.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/connect.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/observe.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/record.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/sfilter.xpm
     ${CMAKE_SOURCE_DIR}/src/xpm/pfilter.xpm
     )

set (icon_SIZE 22 32 48 128)

include_directories (${xqf_HEADERS_DIR} ${CMAKE_BINARY_DIR})

if (DEPRECATED_DISABLE)
	add_definitions (-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED
	-DG_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES)
endif (DEPRECATED_DISABLE)

# Use the package PkgConfig to detect GTK+ headers/library files
find_package (PkgConfig REQUIRED)
find_package (Gettext REQUIRED)
pkg_check_modules (UNZIP REQUIRED minizip)
pkg_check_modules (XML REQUIRED libxml-2.0)

if (USE_GTK3)
	pkg_check_modules (GTK3 REQUIRED gtk+-3.0)
	include_directories (${GTK3_INCLUDE_DIRS})
	link_directories (${GTK3_LIBRARY_DIRS})
	add_definitions (${GTK3_CFLAGS_OTHER})
else (USE_GTK3)
	pkg_check_modules (GTK2 REQUIRED gtk+-2.0)
	include_directories (${GTK2_INCLUDE_DIRS})
	link_directories (${GTK2_LIBRARY_DIRS})
	add_definitions (${GTK2_CFLAGS_OTHER})
endif (USE_GTK3)

if (USE_GEOIP)
	pkg_check_modules (GEOIP REQUIRED geoip)
	include_directories (${GEOIP_INCLUDE_DIRS})
	link_directories (${GEOIP_LIBRARY_DIRS})
endif (USE_GEOIP)

include_directories (${UNZIP_INCLUDE_DIRS})
link_directories (${UNZIP_LIBRARY_DIRS})

include_directories (${XML_INCLUDE_DIRS})
link_directories (${XML_LIBRARY_DIRS})

include_directories (${GETTEXT_INCLUDE_DIRS})
link_directories (${GETTEXT_LIBRARY_DIRS})

# Compile and link
add_executable (gamesxml2c ${CMAKE_SOURCE_DIR}/src/gamesxml2c.c)
target_link_libraries (gamesxml2c xml2)
add_custom_command (TARGET gamesxml2c COMMAND gamesxml2c ${CMAKE_SOURCE_DIR}/src/games.xml > ${CMAKE_BINARY_DIR}/games.c DEPENDS gamesxml2c)
add_definitions (-DGAMES_INCLUDE="${CMAKE_BINARY_DIR}/games.c")

# Compile and link
add_executable (xqf ${xqf_SOURCES})
target_link_libraries (xqf ${UNZIP_LIBRARIES} dl)

if (USE_GTK3)
	target_link_libraries (xqf ${GTK3_LIBRARIES})
else (USE_GTK3)
	target_link_libraries (xqf ${GTK2_LIBRARIES})
endif (USE_GTK3)

if (USE_GEOIP)
	target_link_libraries (xqf GeoIP)
endif (USE_GEOIP)

if (RCON_STANDALONE)
	target_link_libraries (xqf -lreadline)
endif (RCON_STANDALONE)

# i18n
add_custom_target (translation ALL DEPENDS)

foreach (LANG ${LINGUAS})
	set (_poFile ${CMAKE_SOURCE_DIR}/po/${LANG}.po)

	if (EXISTS ${_poFile})

		set (POT_FILE ${CMAKE_SOURCE_DIR}/po/${DOMAIN}.pot)
		set (PO_FILE_NEW ${CMAKE_BINARY_DIR}/${LANG}.po)
		set (GMO_FILE_NEW ${CMAKE_BINARY_DIR}/${LANG}.gmo)

		add_custom_command (TARGET translation

		                    COMMAND msgmerge ${_poFile} ${POT_FILE} -o ${PO_FILE_NEW}
		                    COMMAND msgfmt -c -o ${GMO_FILE_NEW} ${PO_FILE_NEW})

	endif (EXISTS ${_poFile})
endforeach (LANG ${LINGUAS})

add_custom_command (TARGET translation

                    COMMAND echo ${LINGUAS} > ${CMAKE_BINARY_DIR}/LINGUAS
                    COMMAND msgfmt --desktop -d ${CMAKE_BINARY_DIR} --template=${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.desktop.in -c -o ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.desktop
                    )

# Installation
# Executable
install (TARGETS xqf DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# Query script
install (PROGRAMS ${CMAKE_SOURCE_DIR}/src/script/qstat_savage.sh DESTINATION "${PACKAGE_DATA_DIR}/script")

# UI
install (FILES ${CMAKE_SOURCE_DIR}/src/xqf.ui DESTINATION ${PACKAGE_DATA_DIR}/ui)

# Icons
install (FILES ${xpm_DATA} DESTINATION ${PACKAGE_DATA_DIR}/xpm)

install (FILES ${xpm_MAIN} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps)

install (FILES ${flag_DATA} DESTINATION ${PACKAGE_DATA_DIR}/default/flags)

foreach (SIZE ${icon_SIZE})
	install (FILES ${CMAKE_SOURCE_DIR}/pixmaps/${SIZE}x${SIZE}/xqf.png
	         DESTINATION ${PIXMAPS_ENTRY_PATH}/${SIZE}x${SIZE}/apps)
endforeach (SIZE ${icon_SIZE})

install (FILES ${CMAKE_SOURCE_DIR}/pixmaps/scalable/xqf.svg DESTINATION ${PIXMAPS_ENTRY_PATH}/scalable/apps)

# Config
install (FILES ${CMAKE_SOURCE_DIR}/src/qstat.cfg DESTINATION ${PACKAGE_DATA_DIR})

# Man pages
install (FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.6 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man6)

# i18n
foreach (LANG ${LINGUAS})
	install (FILES ${CMAKE_BINARY_DIR}/${LANG}.gmo DESTINATION ${LOCALEDIR}/${LANG}/LC_MESSAGES RENAME ${DOMAIN}.mo )
endforeach (LANG ${LINGUAS})

install (FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
