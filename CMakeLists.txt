cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

project(gnotime
    VERSION 3.0.0
    LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0>=2.66.3)

add_subdirectory(src)
