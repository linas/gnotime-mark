cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

project(gnotime
    LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GTK2 REQUIRED IMPORTED_TARGET gtk+-2.0)

add_subdirectory(src)
