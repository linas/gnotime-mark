deps = [
  glib_dep,
  gtk_dep,
]

srcs = [
  'app.c',
  'dialog.c',
  'err-throw.c',
  'fake_qof.c',
  'gnome-dialog.c',
  'gnome-propertybox.c',
  'prefs.c',
  'proj.c',
  'proj-query.c',
  'projects-tree.c',
  'query.c',
  'status-icon.c',
  'util.c',
]

executable('gnotime', srcs,
           dependencies: deps,
           include_directories: [top_inc],
)
