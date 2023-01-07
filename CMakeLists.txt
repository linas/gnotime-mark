cmake_minimum_required(VERSION 3.18.4)

project(gnotime
    VERSION 2.99.0
    DESCRIPTION "A to-do list organizer, diary and billing system"
    HOMEPAGE_URL "https://gttr.sourceforge.net"
    LANGUAGES C)

find_package(PkgConfig)

pkg_check_modules(DBUS_GLIB REQUIRED dbus-glib-1>=0.110)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.66.8)
pkg_check_modules(GTK REQUIRED gtk+-2.0>=2.24.33)
pkg_check_modules(GUILE REQUIRED guile-2.2>=2.2.7)
pkg_check_modules(LIBGLADE REQUIRED libglade-2.0>=2.6.4)
pkg_check_modules(LIBXML REQUIRED libxml-2.0>=2.9.10)
pkg_check_modules(X11 REQUIRED x11>=1.7.2)
pkg_check_modules(XSCRNSAVER REQUIRED xscrnsaver>=1.2.3)

add_subdirectory(src)
