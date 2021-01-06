cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

configure_file(../cmake_config.h.in config.h @ONLY)

add_executable(${PROJECT_NAME}
    active-dialog.c
    app.c
    dbus.c
    dialog.c
    err.c
    err-throw.c
    export.c
    file-io.c
    gconf-gnomeui.c
    gconf-io.c
    ghtml.c
    ghtml-deprecated.c
    idle-dialog.c
    idle-timer.c
    journal.c
    log.c
    main.c
    menucmd.c
    menus.c
    myoaf.c
    notes-area.c
    plug-edit.c
    plug-in.c
    prefs.c
    proj.c
    projects-tree.c
    proj-query.c
    props-invl.c
    props-proj.c
    props-task.c
    query.c
    status-icon.c
    timer.c
    toolbar.c
    util.c
    vfs-io.c
    xml-read.c
    xml-write.c)
target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
