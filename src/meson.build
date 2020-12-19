deps = [
  glib_dep,
  gtk_dep,
  popt_dep,
  x11_dep,
  xss_dep,
]

srcs = [
  'active-dialog.c',
  'app.c',
  'dialog.c',
  'err-throw.c',
  'fake_qof.c',
  'gnome-dialog.c',
  'gnome-propertybox.c',
  'idle-dialog.c',
  'log.c',
  'notes-area.c',
  'prefs.c',
  'proj.c',
  'proj-query.c',
  'projects-tree.c',
  'props-invl.c',
  'props-proj.c',
  'props-task.c',
  'query.c',
  'status-icon.c',
  'timer.c',
  'util.c',
]

executable('gnotime', srcs,
           dependencies: deps,
           include_directories: [top_inc],
)
