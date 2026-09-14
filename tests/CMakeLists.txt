# Unit tests, built independently of BUILD_XQF/BUILD_RCON so `ctest` works
# regardless of which optional targets are enabled.
pkg_check_modules(GTK REQUIRED gtk4>=4.0)

include_directories(${GTK_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS})
add_definitions(${GTK_CFLAGS_OTHER})

add_executable(test_config
	test_config.c
	${CMAKE_SOURCE_DIR}/src/config.c
	${CMAKE_SOURCE_DIR}/src/utils.c
	${CMAKE_SOURCE_DIR}/src/debug.c
)
target_link_libraries(test_config ${GTK_LIBRARIES} m)

add_test(NAME test_config COMMAND test_config)

add_executable(test_host
	test_host.c
	${CMAKE_SOURCE_DIR}/src/host.c
	${CMAKE_SOURCE_DIR}/src/zipped.c
	${CMAKE_SOURCE_DIR}/src/utils.c
	${CMAKE_SOURCE_DIR}/src/debug.c
)
target_link_libraries(test_host ${GTK_LIBRARIES} m)
# host.c pulls in defs.h (via xqf.h), which needs games.h generated first.
if (TARGET generate_games)
	add_dependencies(test_host generate_games)
endif()

add_test(NAME test_host COMMAND test_host)

add_executable(test_filter
	test_filter.c
	${CMAKE_SOURCE_DIR}/src/filter-eval.c
	${CMAKE_SOURCE_DIR}/src/utils.c
	${CMAKE_SOURCE_DIR}/src/debug.c
)
target_link_libraries(test_filter ${GTK_LIBRARIES} m)
if (USE_GEOIP)
	target_compile_definitions(test_filter PRIVATE USE_GEOIP)
endif()
# filter-eval.c pulls in defs.h (via filter.h -> xqf.h), needs games.h first.
if (TARGET generate_games)
	add_dependencies(test_filter generate_games)
endif()

add_test(NAME test_filter COMMAND test_filter)

add_executable(test_player_filter
	test_player_filter.c
	${CMAKE_SOURCE_DIR}/src/player-filter-eval.c
	${CMAKE_SOURCE_DIR}/src/utils.c
	${CMAKE_SOURCE_DIR}/src/debug.c
)
target_link_libraries(test_player_filter ${GTK_LIBRARIES} m)
# player-filter-eval.c pulls in defs.h (via flt-player.h -> xqf.h), needs games.h first.
if (TARGET generate_games)
	add_dependencies(test_player_filter generate_games)
endif()

add_test(NAME test_player_filter COMMAND test_player_filter)

add_executable(test_addr_parse
	test_addr_parse.c
	${CMAKE_SOURCE_DIR}/src/addr-parse.c
	${CMAKE_SOURCE_DIR}/src/utils.c
	${CMAKE_SOURCE_DIR}/src/debug.c
)
target_link_libraries(test_addr_parse ${GTK_LIBRARIES} m)
# addr-parse.c pulls in defs.h (via server.h -> xqf.h), needs games.h first.
if (TARGET generate_games)
	add_dependencies(test_addr_parse generate_games)
endif()

add_test(NAME test_addr_parse COMMAND test_addr_parse)

add_executable(test_rcon_msg
	test_rcon_msg.c
	${CMAKE_SOURCE_DIR}/src/rcon-msg.c
)
target_link_libraries(test_rcon_msg ${GTK_LIBRARIES} m)

add_test(NAME test_rcon_msg COMMAND test_rcon_msg)

add_executable(test_huffman
	test_huffman.c
	${CMAKE_SOURCE_DIR}/src/huffman.c
)
target_link_libraries(test_huffman ${GTK_LIBRARIES} m)

add_test(NAME test_huffman COMMAND test_huffman)
