deps = [
  glib_dep,
  gtk_dep,
  x11_dep,
]

srcs = [
  'dialog.c',
  'err-throw.c',
  'util.c',
]

executable('gnotime', srcs,
           dependencies: deps,
           include_directories: [top_inc],
)
