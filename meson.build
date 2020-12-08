project('gnotime', 'c')

glib_req_version = '>= 2.66.3'
gtk_req_version = '>= 2.24.32'
x11_req_version = '>= 1.6.2'

glib_dep = dependency('glib-2.0', version: glib_req_version)
gtk_dep = dependency('gtk+-2.0', version: gtk_req_version)
x11_dep = dependency('x11', version: x11_req_version)

subdir('src')
