cmake_minimum_required(VERSION 2.8.7)

# Project information
project(xqf)

set (VERSION "1.0.6.2")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    find_package(Git)
    if (GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --long --match xqf-* --dirty=+dirty
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

if (GIT_VERSION)
    string(REGEX REPLACE "^xqf-" "" GIT_VERSION "${GIT_VERSION}")
    set(VERSION "${GIT_VERSION}")
endif ()

file(WRITE "${CMAKE_BINARY_DIR}/VERSION" ${VERSION})

set (DOMAIN "${PROJECT_NAME}")

set (LINGUAS ca da de es fi fr pl ru)
set (DEFAULT_GUI GTK2)

# Paths
set (PACKAGE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/xqf")
set (LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale")

if (NOT MAN_ENTRY_PATH)
    set (MAN_ENTRY_PATH "${CMAKE_INSTALL_PREFIX}/share/man")
endif (NOT MAN_ENTRY_PATH)

if (NOT PIXMAPS_ENTRY_PATH)
    set (PIXMAPS_ENTRY_PATH "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor")
endif (NOT PIXMAPS_ENTRY_PATH)

if (NOT CMAKE_INSTALL_PREFIX)
    set (CMAKE_INSTALL_PREFIX "/usr")
endif (NOT CMAKE_INSTALL_PREFIX)

# Definitions
add_definitions (-DPACKAGE="${PROJECT_NAME}" -DPACKAGE_VERSION="${VERSION}" -DXQF_VERSION="${VERSION}" -DDOMAIN="${DOMAIN}" -DLOCALEDIR="${LOCALEDIR}" -DPACKAGE_DATA_DIR="${PACKAGE_DATA_DIR}")

# Options
option(DEPRECATED_DISABLE "Disable deprecated parts of GLib, GObject, GDK and GTK+" OFF)

option(USE_GEOIP "Depend on GeoIP library for geolocation" ON)
option(USE_GZIP "Enable gzip compressor support" ON)
option(RCON_STANDALONE "Build standalone RCON" OFF)

if (NOT WITH_QSTAT)
    set (WITH_QSTAT "qstat")
endif (NOT WITH_QSTAT)

add_definitions (-DQSTAT_EXEC="${WITH_QSTAT}")

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    add_definitions (-DDEBUG)
    set (CMAKE_C_FLAGS "-g -ggdb -O0")
endif (CMAKE_BUILD_TYPE MATCHES DEBUG)

# Default GUI setting
if (NOT (GUI MATCHES GTK2 OR GUI MATCHES GTK3))
    set (GUI ${DEFAULT_GUI})
endif (NOT (GUI MATCHES GTK2 OR GUI MATCHES GTK3))

# GUI definitions
if (GUI MATCHES GTK2)
    add_definitions (-DGUI_GTK2)
endif (GUI MATCHES GTK2)

if (GUI MATCHES GTK3)
    add_definitions (-DGUI_GTK3)
endif (GUI MATCHES GTK3)

# Other definitions
if (RCON_STANDALONE)
    add_definitions (-DRCON_STANDALONE)
endif (RCON_STANDALONE)

if (USE_GEOIP)
    add_definitions (-DUSE_GEOIP)
endif (USE_GEOIP)

if (USE_GZIP)
    add_definitions (-DUSE_GZIP)
endif (USE_GZIP)

# Compiler flags
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Source list
set (xqf_HEADERS_DIR ${CMAKE_SOURCE_DIR}/src)

set (xqf_SOURCES

     ${CMAKE_SOURCE_DIR}/src/addmaster.c
     ${CMAKE_SOURCE_DIR}/src/addserver.c
     ${CMAKE_SOURCE_DIR}/src/callbacks.c
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
     ${CMAKE_SOURCE_DIR}/src/pixmaps.c
     ${CMAKE_SOURCE_DIR}/src/pref.c
     ${CMAKE_SOURCE_DIR}/src/psearch.c
     ${CMAKE_SOURCE_DIR}/src/rc.c
     ${CMAKE_SOURCE_DIR}/src/rcon.c
     ${CMAKE_SOURCE_DIR}/src/huffman.c
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
     ${CMAKE_SOURCE_DIR}/src/memtopixmap.c

     ${CMAKE_SOURCE_DIR}/src/addmaster.h
     ${CMAKE_SOURCE_DIR}/src/addserver.h
     ${CMAKE_SOURCE_DIR}/src/callbacks.h
     ${CMAKE_SOURCE_DIR}/src/config.h
     ${CMAKE_SOURCE_DIR}/src/country-filter.h
     ${CMAKE_SOURCE_DIR}/src/debug.h
     ${CMAKE_SOURCE_DIR}/src/defs.h
     ${CMAKE_SOURCE_DIR}/src/dialogs.h
     ${CMAKE_SOURCE_DIR}/src/dns.h
     ${CMAKE_SOURCE_DIR}/src/filter.h
     ${CMAKE_SOURCE_DIR}/src/flt-player.h
     ${CMAKE_SOURCE_DIR}/src/game.h
     ${CMAKE_SOURCE_DIR}/src/history.h
     ${CMAKE_SOURCE_DIR}/src/host.h
     ${CMAKE_SOURCE_DIR}/src/launch.h
     ${CMAKE_SOURCE_DIR}/src/pixmaps.h
     ${CMAKE_SOURCE_DIR}/src/pref.h
     ${CMAKE_SOURCE_DIR}/src/psearch.h
     ${CMAKE_SOURCE_DIR}/src/quake2_pal.h
     ${CMAKE_SOURCE_DIR}/src/quake_pal.h
     ${CMAKE_SOURCE_DIR}/src/rc.h
     ${CMAKE_SOURCE_DIR}/src/rcon.h
     ${CMAKE_SOURCE_DIR}/src/huffman.h
     ${CMAKE_SOURCE_DIR}/src/hufffreq.h
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
     ${CMAKE_SOURCE_DIR}/src/memtopixmap.h
     )

set (flag_DATA
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/lan.png

    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ad.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ae.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/af.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ag.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ai.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/al.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/am.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/an.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ao.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ar.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/as.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/at.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/au.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/aw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ax.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/az.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ba.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bb.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bd.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/be.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bf.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bi.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bj.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bo.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/br.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bs.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/by.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/bz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ca.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cd.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cf.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ch.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ci.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ck.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/co.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cs.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cx.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cy.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/cz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/de.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/dj.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/dk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/dm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/do.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/dz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ec.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ee.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/eg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/eh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/er.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/es.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/et.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/fi.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/fj.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/fk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/fm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/fo.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/fr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ga.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gb.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gd.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ge.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gf.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gi.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gp.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gq.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gs.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/gy.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/hk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/hm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/hn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/hr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ht.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/hu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/id.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ie.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/il.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/in.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/io.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/iq.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ir.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/is.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/it.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/jm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/jo.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/jp.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ke.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/kg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/kh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ki.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/km.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/kn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/kp.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/kr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/kw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ky.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/kz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/la.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/lb.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/lc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/li.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/lk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/lr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ls.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/lt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/lu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/lv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ly.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ma.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/md.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/me.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ml.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mo.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mp.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mq.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ms.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mx.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/my.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/mz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/na.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/nc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ne.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/nf.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ng.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ni.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/nl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/no.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/np.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/nr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/nu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/nz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/om.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pa.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pe.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pf.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ph.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ps.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/pw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/py.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/qa.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/re.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ro.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/rs.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ru.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/rw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sa.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sb.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sd.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/se.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sh.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/si.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sj.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/so.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/st.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sy.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/sz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/td.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tf.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/th.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tj.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tk.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tl.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/to.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tr.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tv.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tw.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/tz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ua.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ug.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/um.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/us.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/uy.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/uz.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/va.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/vc.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ve.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/vg.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/vi.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/vn.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/vu.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/wf.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ws.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/ye.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/yt.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/za.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/zm.png
    ${CMAKE_SOURCE_DIR}/pixmaps/flags/country/zw.png
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

if (GUI MATCHES GTK2)
    pkg_check_modules (GTK2 REQUIRED gtk+-2.0)
    include_directories (${GTK2_INCLUDE_DIRS})
    link_directories (${GTK2_LIBRARY_DIRS})
    add_definitions (${GTK2_CFLAGS_OTHER})
    set (GUI_SET ON)
endif (GUI MATCHES GTK2)

if (GUI MATCHES GTK3)
    pkg_check_modules (GTK3 REQUIRED gtk+-3.0)
    include_directories (${GTK3_INCLUDE_DIRS})
    link_directories (${GTK3_LIBRARY_DIRS})
    add_definitions (${GTK3_CFLAGS_OTHER})
    set (GUI_SET ON)
endif (GUI MATCHES GTK3)

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

# Custom build rule for converting games.xml to games.c
add_custom_command (
    OUTPUT ${CMAKE_BINARY_DIR}/games.c
    COMMAND gamesxml2c ${CMAKE_SOURCE_DIR}/src/games.xml ${CMAKE_BINARY_DIR}/games.c ${CMAKE_BINARY_DIR}/games.h ${CMAKE_BINARY_DIR}/icons.c
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/games.xml
    DEPENDS gamesxml2c)

# Define games.c filename for including in src/game.c
add_definitions (-DGAMES_C_INCLUDE="${CMAKE_BINARY_DIR}/games.c")
add_definitions (-DGAMES_H_INCLUDE="${CMAKE_BINARY_DIR}/games.h")
add_definitions (-DICONS_C_INCLUDE="${CMAKE_BINARY_DIR}/icons.c")

# Make src/game.c depend on games.c so that src/game.c is rebuilt when games.c changes,
# this must be manually specified due to the include name being a define
set_property (SOURCE ${CMAKE_SOURCE_DIR}/src/game.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/games.c)

# Compile and link
add_executable (xqf ${xqf_SOURCES})
target_link_libraries (xqf ${UNZIP_LIBRARIES} dl)

if (GUI MATCHES GTK2)
    target_link_libraries (xqf ${GTK2_LIBRARIES})
endif (GUI MATCHES GTK2)

if (GUI MATCHES GTK3)
    target_link_libraries (xqf ${GTK3_LIBRARIES})
endif (GUI MATCHES GTK3)

if (USE_GEOIP)
    target_link_libraries (xqf GeoIP)
endif (USE_GEOIP)

if (RCON_STANDALONE)
    target_link_libraries (xqf -lreadline)
endif (RCON_STANDALONE)

# i18n
add_custom_target (translation ALL)

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

# UI
if (GUI MATCHES GTK2)
    install (FILES ${CMAKE_SOURCE_DIR}/src/xqf-gtk2.ui DESTINATION ${PACKAGE_DATA_DIR}/ui)
endif (GUI MATCHES GTK2)

if (GUI MATCHES GTK3)
    install (FILES ${CMAKE_SOURCE_DIR}/src/xqf-gtk3.ui DESTINATION ${PACKAGE_DATA_DIR}/ui)
endif (GUI MATCHES GTK3)

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
install (FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.6 DESTINATION ${MAN_ENTRY_PATH}/man6)

# i18n
foreach (LANG ${LINGUAS})
    install (FILES ${CMAKE_BINARY_DIR}/${LANG}.gmo DESTINATION ${LOCALEDIR}/${LANG}/LC_MESSAGES RENAME ${DOMAIN}.mo )
endforeach (LANG ${LINGUAS})

install (FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
