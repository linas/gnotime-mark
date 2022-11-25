# This CMakeLists.txt file is not able to build this project (yet) but is mainly
# intended to enable the usage of modern IDEs.

cmake_minimum_required(VERSION 3.13.4)

project(gnotime
    VERSION 2.99.0
    DESCRIPTION "A to-do list organizer, diary and billing system"
    HOMEPAGE_URL "https://gttr.sourceforge.net"
    LANGUAGES C)

find_package(PkgConfig)

pkg_check_modules(DBUS_GLIB REQUIRED dbus-glib-1>=0.110)
pkg_check_modules(GLADE2 REQUIRED libglade-2.0>=2.6.4)
pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.58.3)
pkg_check_modules(GTK2 REQUIRED gtk+-2.0>=2.24.32)
pkg_check_modules(GUILE2 REQUIRED guile-2.2>=2.2.4)
pkg_check_modules(X11 REQUIRED x11>=1.6.7)
pkg_check_modules(XSCRNSAVER REQUIRED xscrnsaver>=1.2.3)

add_subdirectory(src)
