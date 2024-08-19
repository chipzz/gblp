cmake_minimum_required(VERSION 3.5.0)
project(gblp VERSION 0 LANGUAGES C CXX)

find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)

# squish
# vcpkg port
find_package(unofficial-libsquish CONFIG)
if(unofficial-libsquish_DIR)
	add_library(libsquish::libsquish ALIAS unofficial::libsquish::squish)
else()
	# from pkgconfig, libsquish-dev package on Debian
	pkg_check_modules(libsquish REQUIRED IMPORTED_TARGET libsquish)
	add_library(libsquish::libsquish ALIAS PkgConfig::libsquish)
endif()

pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk4)
pkg_get_variable(GLIB_COMPILE_RESOURCES "gio-2.0" glib_compile_resources)
if (MSVC)
message(CHECK_START "Updating pixbuf loader cache for rsvg pixbuf loader...")

pkg_get_variable(BINDIR "gdk-pixbuf-2.0" bindir)
pkg_get_variable(GDK_PIXBUF_QUERY_LOADERS "gdk-pixbuf-2.0" gdk_pixbuf_query_loaders)
pkg_get_variable(GDK_PIXBUF_MODULEDIR "gdk-pixbuf-2.0" gdk_pixbuf_moduledir)
#message("GDK_PIXBUF_MODULEDIR=${GDK_PIXBUF_MODULEDIR}")
#string(REPLACE "/" "\\" GDK_PIXBUF_MODULEDIR ${GDK_PIXBUF_MODULEDIR})
pkg_get_variable(GDK_PIXBUF_CACHE_FILE "gdk-pixbuf-2.0" gdk_pixbuf_cache_file)

set(ENV{GDK_PIXBUF_MODULEDIR} ${GDK_PIXBUF_MODULEDIR})
set(ENV{GDK_PIXBUF_MODULE_FILE} ${GDK_PIXBUF_CACHE_FILE})
execute_process(COMMAND "${GDK_PIXBUF_QUERY_LOADERS}" --update-cache
	WORKING_DIRECTORY ${BINDIR}
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if(error)
	message(CHECK_FAIL "FAILED")

	message("gdk-pixbuf-query-loaders environment:")
	message("BINDIR=${BINDIR}")
	message("GDK_PIXBUF_QUERY_LOADERS=${GDK_PIXBUF_QUERY_LOADERS}")
	message("GDK_PIXBUF_MODULEDIR=${GDK_PIXBUF_MODULEDIR}")
	message("GDK_PIXBUF_MODULE_FILE=${GDK_PIXBUF_CACHE_FILE}")
	message("")

	message("gdk-pixbuf-query-loaders error:")
	message(${error})
	message("gdk-pixbuf-query-loaders output:")
	message(${output})
else()
	message(CHECK_PASS "SUCCESS")
endif()
endif()

pkg_check_modules(Epoxy REQUIRED IMPORTED_TARGET epoxy)

file(GLOB PALBMP_SRC "palbmp/*.c")

# add_custom_target(gblp.gresource.depend
# COMMAND ${GLIB_COMPILE_RESOURCES}
# "${CMAKE_SOURCE_DIR}/gtk/gblp.gresource.xml"
# --sourcedir "${CMAKE_SOURCE_DIR}/gtk"
# --generate-dependencies
# --dependency-file gblp.gresource.depend
# VERBATIM
# DEPENDS gtk/gblp.gresource.xml
# )

add_custom_command(OUTPUT gblp.gresource.c
COMMAND ${GLIB_COMPILE_RESOURCES}
"${CMAKE_SOURCE_DIR}/gtk/gblp.gresource.xml"
--sourcedir "${CMAKE_SOURCE_DIR}/gtk"
--generate-source
--target gblp.gresource.c
VERBATIM
MAIN_DEPENDENCY gtk/gblp.gresource.xml
DEPENDS gtk/gblp-app-menu.ui gtk/gblp-app-window.ui
# DEPFILE gblp.gresource.depend
)

add_executable(gblp
	"gtk/gblp.c"
	"gtk/gblp-app-window.cc"
	"gblp.gresource.c"
	"blp.cc"
	"openglblp.cc"
)

# add_dependencies(gblp gblp.gresource.depend)

target_include_directories(gblp PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(gblp PRIVATE
	PkgConfig::GTK
	PkgConfig::Epoxy
	libsquish::libsquish
)

if(MSVC)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_MBCS)
add_definitions(-D_VC80_UPGRADE=0x0600)
endif()
