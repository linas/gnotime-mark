cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

add_executable(${PROJECT_NAME}
    err-throw.c
    fake_qof.c
    gnome-dialog.c
    gnome-propertybox.c
    prefs.c
    proj.c
    projects-tree.c
    query.c)
target_link_libraries(${PROJECT_NAME}
    PkgConfig::GLIB2
    PkgConfig::GTK2
    PkgConfig::POPT)
