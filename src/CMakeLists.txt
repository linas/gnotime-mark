cmake_minimum_required(VERSION 3.13.4)

add_executable(${PROJECT_NAME}
    gtt_activation_dialog.c
    gtt_application_window.c
    gtt_dbus.c
    gtt_err_throw.c
    gtt_export.c
    gtt_file_io.c
    gtt_ghtml.c
    gtt_ghtml_deprecated.c
    gtt_gsettings_gnomeui.c
    gtt_gsettings_io.c
    gtt_gsettings_io_p.c
    gtt_help_popup.c
    gtt_idle_dialog.c
    gtt_idle_timer.c
    gtt_journal.c
    gtt_log.c
    gtt_menu_commands.c
    gtt_menus.c
    gtt_myoaf.c
    gtt_notes_area.c
    gtt_plug_in.c
    gtt_plug_in_editor.c
    gtt_preferences.c
    gtt_project.c
    gtt_project_queries.c
    gtt_projects_tree.c
    gtt_props_dlg_interval.c
    gtt_props_dlg_project.c
    gtt_props_dlg_task.c
    gtt_query.c
    gtt_signal_handlers.c
    gtt_status_icon.c
    gtt_timer.c
    gtt_toolbar.c
    gtt_util.c
    gtt_xml_read.c
    gtt_xml_write.c
    main.c)
target_include_directories(${PROJECT_NAME} SYSTEM
    PRIVATE ${DBUS_GLIB_INCLUDE_DIRS}
    PRIVATE ${GLADE2_INCLUDE_DIRS}
    PRIVATE ${GLIB2_INCLUDE_DIRS}
    PRIVATE ${GTK2_INCLUDE_DIRS}
    PRIVATE ${GUILE2_INCLUDE_DIRS}
    PRIVATE ${X11_INCLUDE_DIRS}
    PRIVATE ${XSCRNSAVER_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
    ${DBUS_GLIB_LINK_LIBRARIES}
    ${GLADE2_LINK_LIBRARIES}
    ${GLIB2_LINK_LIBRARIES}
    ${GTK2_LINK_LIBRARIES}
    ${GUILE2_LINK_LIBRARIES}
    ${X11_LINK_LIBRARIES}
    ${XSCRNSAVER_LINK_LIBRARIES})
