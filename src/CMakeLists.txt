cmake_minimum_required(VERSION 3.18.4)

add_executable(${PROJECT_NAME}
    gtt_activation_dialog.c
    gtt_application_window.c
    gtt_date_edit.c
    gtt_dbus.c
    gtt_entry.c
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
    gtt_plug_in_edit.c
    gtt_preferences.c
    gtt_project.c
    gtt_project_queries.c
    gtt_projects_tree.c
    gtt_props_dlg_interval.c
    gtt_props_dlg_project.c
    gtt_props_dlg_task.c
    gtt_queries.c
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
    PRIVATE ${GLIB_INCLUDE_DIRS}
    PRIVATE ${GTK_INCLUDE_DIRS}
    PRIVATE ${GUILE_INCLUDE_DIRS}
    PRIVATE ${LIBGLADE_INCLUDE_DIRS}
    PRIVATE ${LIBXML_INCLUDE_DIRS}
    PRIVATE ${X11_INCLUDE_DIRS}
    PRIVATE ${XSCRNSAVER_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
    PRIVATE ${DBUS_GLIB_LINK_LIBRARIES}
    PRIVATE ${GLIB_LINK_LIBRARIES}
    PRIVATE ${GTK_LINK_LIBRARIES}
    PRIVATE ${GUILE_LINK_LIBRARIES}
    PRIVATE ${LIBGLADE_LINK_LIBRARIES}
    PRIVATE ${LIBXML_LINK_LIBRARIES}
    PRIVATE ${X11_LINK_LIBRARIES}
    PRIVATE ${XSCRNSAVER_LINK_LIBRARIES})
