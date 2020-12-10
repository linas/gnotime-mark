project('gnotime', 'c')

glib_req_version = '>= 2.66.3'
gtk_req_version = '>= 2.24.32'
x11_req_version = '>= 1.6.2'

glib_dep = dependency('glib-2.0', version: glib_req_version)
gtk_dep = dependency('gtk+-2.0', version: gtk_req_version)
x11_dep = dependency('x11', version: x11_req_version)

conf_data = configuration_data()
conf_data.set('gttgladedir', '/usr/local/share/gnotime')
conf_data.set('package', meson.project_name())
configure_file(configuration: conf_data,
               input: 'meson_config.h.in',
               output: 'config.h')

top_inc = include_directories('.')

subdir('src')
