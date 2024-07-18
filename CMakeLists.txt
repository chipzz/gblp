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
