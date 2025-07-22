cmake_minimum_required(VERSION 3.27)

# Required since CMake 3.4 otherwise symbols are not exported and
# XQF can't find some internal resources like compiled XPM images.
#
# See:
# - https://cmake.org/cmake/help/latest/release/3.4.html
# - https://cmake.org/cmake/help/v3.4/variable/CMAKE_ENABLE_EXPORTS.html
# - https://cmake.org/cmake/help/v3.27/variable/CMAKE_ENABLE_EXPORTS.html
# - https://cmake.org/cmake/help/v3.27/variable/CMAKE_EXECUTABLE_ENABLE_EXPORTS.html
# - https://cmake.org/cmake/help/v3.27/prop_tgt/ENABLE_EXPORTS.html
#
# > Normally an executable does not export any symbols because it is the final program.
# > It is possible for an executable to export symbols to be used by loadable modules.
# > When this property is set to true CMake will allow other targets to "link" to the executable.
#
# For some reasons we need that for the executable to find embedded
# data at run time.
set(CMAKE_EXECUTABLE_ENABLE_EXPORTS ON)

include(CheckCXXCompilerFlag)

# Project information
project(xqf LANGUAGES C CXX)

set(VERSION "1.0.6.2")

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
endif()

file(WRITE "${CMAKE_BINARY_DIR}/VERSION" ${VERSION})

set(DOMAIN "${PROJECT_NAME}")

set(LINGUAS ca da de es fi fr pl ru)

set(DEFAULT_QSTAT "qstat")
set(DEFAULT_GTK_TARGET 2)

# Macros
macro(set_c_flag FLAG)
	if (${ARGC} GREATER 1)
		set(CMAKE_C_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${FLAG}")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
	endif()
endmacro()

macro(set_cxx_flag FLAG)
	if (${ARGC} GREATER 1)
		set(CMAKE_CXX_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${FLAG}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
	endif()
endmacro()

macro(set_c_cxx_flag FLAG)
	set_c_flag(${FLAG})
	set_cxx_flag(${FLAG})
endmacro()

macro(set_linker_flag FLAG)
	if (${ARGC} GREATER 1)
		set(CMAKE_EXE_LINKER_FLAGS_${ARGV1} "${CMAKE_EXE_LINKER_FLAGS_${ARGV1}} ${FLAG}")
		set(CMAKE_SHARED_LINKER_FLAGS_${ARGV1} "${CMAKE_SHARED_LINKER_FLAGS_${ARGV1}} ${FLAG}")
		set(CMAKE_MODULE_LINKER_FLAGS_${ARGV1} "${CMAKE_MODULE_LINKER_FLAGS_${ARGV1}} ${FLAG}")
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAG}")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAG}")
	endif()
endmacro()

macro(try_c_flag FLAG PROP)
	check_C_compiler_flag(${FLAG} FLAG_${PROP})

	if (FLAG_${PROP})
		set_c_flag(${FLAG})
	endif()
endmacro()

macro(try_cxx_flag FLAG PROP)
	check_CXX_compiler_flag(${FLAG} FLAG_${PROP})

	if (FLAG_${PROP})
		set_cxx_flag(${FLAG})
	endif()
endmacro()

macro(try_c_cxx_flag FLAG PROP)
	# Only try the flag once on the C++ compiler.
	try_cxx_flag(${PROP} ${FLAG} ${ARGV2})
	if (FLAG_${PROP})
		set_c_flag(${FLAG} ${ARGV2})
	endif()
endmacro()

# Paths
set(PACKAGE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/xqf")
set(LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale")

if (NOT MAN_ENTRY_PATH)
	set(MAN_ENTRY_PATH "${CMAKE_INSTALL_PREFIX}/share/man")
endif()

if (NOT PIXMAPS_ENTRY_PATH)
	set(PIXMAPS_ENTRY_PATH "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor")
endif()

# Definitions
add_definitions(-DPACKAGE="${PROJECT_NAME}" -DPACKAGE_VERSION="${VERSION}" -DXQF_VERSION="${VERSION}" -DDOMAIN="${DOMAIN}" -DLOCALEDIR="${LOCALEDIR}" -DPACKAGE_DATA_DIR="${PACKAGE_DATA_DIR}")

# Options
option(DEPRECATED_DISABLE "Disable deprecated parts of GLib, GObject, GDK and GTK+" OFF)

option(USE_GEOIP "Depend on GeoIP library for geolocation" ON)
option(USE_GZIP "Enable gzip compressor support" ON)
option(USE_CRUNCH "Enable Crunch decompressor support (DDS, KTX, CRN)" ON)
option(RCON_STANDALONE "Build standalone RCON" OFF)

set(QSTAT_HELP "QStat binary name")
set(WITH_QSTAT "${DEFAULT_QSTAT}" CACHE STRING "${QSTAT_HELP}")

if (NOT WITH_QSTAT)
	set(WITH_QSTAT "qstat" CACHE STRING "${QSTAT_HELP}" FORCE)
endif()

add_definitions(-DQSTAT_EXEC="${WITH_QSTAT}")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE
		STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

if (CMAKE_BUILD_TYPE MATCHES "DEBUG")
	add_definitions(-DDEBUG)
endif()

set_c_cxx_flag("-g3" DEBUG)
set_c_cxx_flag("-g3" RELWITHDEBINFO)

set(GTK_TARGET_HELP "GTK target (2 or 3)")
set(GTK_TARGET "${DEFAULT_GTK_TARGET}" CACHE STRING "${GTK_TARGET_HELP}")

# Default GTK_TARGET setting
if (NOT (GTK_TARGET STREQUAL "2" OR GTK_TARGET STREQUAL "3"))
	set(GTK_TARGET "${DEFAULT_GTK_TARGET}" CACHE STRING "${GTK_TARGET_HELP}" FORCE)
endif()

# GUI definitions
if (GTK_TARGET STREQUAL "2")
	set(GUI_GTK2 ON)
elseif (GTK_TARGET STREQUAL "3")
	set(GUI_GTK3 ON)
endif()

add_definitions(-DGUI_GTK${GTK_TARGET})

# Other definitions
if (RCON_STANDALONE)
	add_definitions(-DRCON_STANDALONE)
endif()

if (USE_GEOIP)
	add_definitions(-DUSE_GEOIP)
endif()

if (USE_GZIP)
	add_definitions(-DUSE_GZIP)
endif()

# Compiler flags
# Only set -Wall on the C compiler for now.
set_c_flag("-Wall")

if (GUI_GTK2)
	set_c_cxx_flag("-Wno-deprecated-declarations")
endif()

option(USE_LTO "Enable link-time optimization" OFF)

# It should be done at the very end because it copies all compiler flags
# to the linker flags.
if (USE_LTO)
	try_cxx_flag("-flto=auto" FLTO_AUTO)

	if (NOT FLAG_FLTO_AUTO)
		try_cxx_flag("-flto" FLTO)
	endif()

	set_linker_flag("${CMAKE_CXX_FLAGS}" RELEASE)
	set_linker_flag("${CMAKE_CXX_FLAGS}" RELWITHDEBINFO)
	set_linker_flag("${CMAKE_CXX_FLAGS}" MINSIZEREL)
endif()

# Source list
set(xqf_HEADERS_DIR ${CMAKE_SOURCE_DIR}/src)

set(xqf_SOURCES
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
	 ${CMAKE_SOURCE_DIR}/src/crntotga.cpp

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
	 ${CMAKE_SOURCE_DIR}/src/crntotga.h
 )

set(flag_DATA
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

set(xpm_MAIN ${CMAKE_SOURCE_DIR}/pixmaps/xqf.xpm)

set(xpm_DATA
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

set(icon_SIZE 22 32 48 128)

include_directories(${xqf_HEADERS_DIR} ${CMAKE_BINARY_DIR})

if (DEPRECATED_DISABLE)
	add_definitions(-DGSEAL_ENABLE
	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED
	-DG_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES)
endif()

# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
find_package(Gettext REQUIRED)
pkg_check_modules(UNZIP REQUIRED minizip)
pkg_check_modules(XML REQUIRED libxml-2.0)

if (GUI_GTK2)
	pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
	include_directories(${GTK2_INCLUDE_DIRS})
	link_directories(${GTK2_LIBRARY_DIRS})
	add_definitions(${GTK2_CFLAGS_OTHER})
	set(GUI_SET ON)
elseif (GUI_GTK3)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	include_directories(${GTK3_INCLUDE_DIRS})
	link_directories(${GTK3_LIBRARY_DIRS})
	add_definitions(${GTK3_CFLAGS_OTHER})
	set(GUI_SET ON)
endif()

if (USE_GEOIP)
	pkg_check_modules(GEOIP REQUIRED geoip)
	include_directories(${GEOIP_INCLUDE_DIRS})
	link_directories(${GEOIP_LIBRARY_DIRS})
endif()

include_directories(${UNZIP_INCLUDE_DIRS})
link_directories(${UNZIP_LIBRARY_DIRS})

include_directories(${XML_INCLUDE_DIRS})
link_directories(${XML_LIBRARY_DIRS})

include_directories(${GETTEXT_INCLUDE_DIRS})
link_directories(${GETTEXT_LIBRARY_DIRS})

# Compile and link
add_executable (gamesxml2c ${CMAKE_SOURCE_DIR}/src/gamesxml2c.c)
target_link_libraries(gamesxml2c xml2)

# Custom build rule for converting games.xml to games.c
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/games.c
	COMMAND gamesxml2c ${CMAKE_SOURCE_DIR}/src/games.xml ${CMAKE_BINARY_DIR}/games.c ${CMAKE_BINARY_DIR}/games.h ${CMAKE_BINARY_DIR}/icons.c
	MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/games.xml
	DEPENDS gamesxml2c)

# Define games.c filename for including in src/game.c
add_definitions(-DGAMES_C_INCLUDE="${CMAKE_BINARY_DIR}/games.c")
add_definitions(-DGAMES_H_INCLUDE="${CMAKE_BINARY_DIR}/games.h")
add_definitions(-DICONS_C_INCLUDE="${CMAKE_BINARY_DIR}/icons.c")

# Make src/game.c depend on games.c so that src/game.c is rebuilt when games.c changes,
# this must be manually specified due to the include name being a define
set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/game.c
	APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/games.c)

# Compile and link
add_executable (xqf ${xqf_SOURCES})
target_link_libraries(xqf ${UNZIP_LIBRARIES} dl m)

if (GUI_GTK2)
	target_link_libraries(xqf ${GTK2_LIBRARIES})
elseif (GUI_GTK3)
	target_link_libraries(xqf ${GTK3_LIBRARIES})
endif()

if (USE_GEOIP)
	target_link_libraries(xqf GeoIP)
endif()

if (USE_CRUNCH)
	set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

	set(BUILD_CRUNCH OFF)
	set(BUILD_EXAMPLES OFF)
	set(BUILD_SHARED_LIBCRN OFF)
	set(BUILD_STATIC_LIBCRN ON)

	add_subdirectory(${CMAKE_SOURCE_DIR}/libs/crunch)

	target_include_directories(xqf PRIVATE ${CMAKE_SOURCE_DIR}/libs/crunch/crnlib)
	target_link_directories(xqf PRIVATE ${CMAKE_BINARY_DIR}/libs/crunch/crnlib)
	target_link_libraries(xqf crn)
endif()

if (RCON_STANDALONE)
	target_link_libraries(xqf -lreadline)
endif()

# i18n
add_custom_target(translation ALL)

foreach (LANG ${LINGUAS})
	set(_poFile ${CMAKE_SOURCE_DIR}/po/${LANG}.po)

	if (EXISTS ${_poFile})
		set(POT_FILE ${CMAKE_SOURCE_DIR}/po/${DOMAIN}.pot)
		set(PO_FILE_NEW ${CMAKE_BINARY_DIR}/${LANG}.po)
		set(GMO_FILE_NEW ${CMAKE_BINARY_DIR}/${LANG}.gmo)

		add_custom_command(TARGET translation
			COMMAND msgmerge ${_poFile} ${POT_FILE} -o ${PO_FILE_NEW}
			COMMAND msgfmt -c -o ${GMO_FILE_NEW} ${PO_FILE_NEW})
	endif()
endforeach()

add_custom_command(TARGET translation
	COMMAND echo ${LINGUAS} > ${CMAKE_BINARY_DIR}/LINGUAS
	COMMAND msgfmt --desktop -d ${CMAKE_BINARY_DIR} --template=${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.desktop.in -c -o ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.desktop)

# Installation
# Executable
install(TARGETS xqf
	DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# UI
if (GUI_GTK2)
	install(FILES ${CMAKE_SOURCE_DIR}/src/xqf-gtk2.ui
	DESTINATION ${PACKAGE_DATA_DIR}/ui)
elseif (GUI_GTK3)
	install(FILES ${CMAKE_SOURCE_DIR}/src/xqf-gtk3.ui
	DESTINATION ${PACKAGE_DATA_DIR}/ui)
endif()

# Icons
install(FILES ${xpm_DATA}
	DESTINATION ${PACKAGE_DATA_DIR}/xpm)

install(FILES ${xpm_MAIN}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps)

install(FILES ${flag_DATA}
	DESTINATION ${PACKAGE_DATA_DIR}/default/flags)

foreach (SIZE ${icon_SIZE})
	install(FILES ${CMAKE_SOURCE_DIR}/pixmaps/${SIZE}x${SIZE}/xqf.png
		DESTINATION ${PIXMAPS_ENTRY_PATH}/${SIZE}x${SIZE}/apps)
endforeach()

install(FILES ${CMAKE_SOURCE_DIR}/pixmaps/scalable/xqf.svg
	DESTINATION ${PIXMAPS_ENTRY_PATH}/scalable/apps)

# Config
install(FILES ${CMAKE_SOURCE_DIR}/src/qstat.cfg
	DESTINATION ${PACKAGE_DATA_DIR})

# Man pages
install(FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.6
	DESTINATION ${MAN_ENTRY_PATH}/man6)

# i18n
foreach (LANG ${LINGUAS})
	install(FILES ${CMAKE_BINARY_DIR}/${LANG}.gmo
		DESTINATION ${LOCALEDIR}/${LANG}/LC_MESSAGES
		RENAME ${DOMAIN}.mo)
endforeach()

install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.desktop
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
