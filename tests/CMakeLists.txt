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
