cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

project(gnotime
    VERSION 3.0.0
    LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0>=2.66.3)
pkg_check_modules(GTK2 REQUIRED IMPORTED_TARGET gtk+-2.0>=2.24.32)
pkg_check_modules(POPT REQUIRED IMPORTED_TARGET popt>=1.18)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11>=1.6.2)
pkg_check_modules(XSS_EXTENSION REQUIRED IMPORTED_TARGET xscrnsaver>=1.2.3)

add_subdirectory(src)
