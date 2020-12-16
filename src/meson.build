deps = [
  glib_dep,
  gtk_dep,
  x11_dep,
]

srcs = [
  'dialog.c',
  'err-throw.c',
  'fake_qof.c',
  'idle-timer.c',
  'myoaf.c',
  'proj.c',
  'util.c',
]

executable('gnotime', srcs,
           dependencies: deps,
           include_directories: [top_inc],
)
