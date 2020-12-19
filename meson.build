project('gnotime', 'c')

glib_req_version = '>= 2.66.3'
gtk_req_version = '>= 2.24.32'
popt_req_version = '>= 1.18'
x11_req_version = '>= 1.6.2'
xss_req_version = '>= 1.2.3'

glib_dep = dependency('glib-2.0', version: glib_req_version)
gtk_dep = dependency('gtk+-2.0', version: gtk_req_version)
popt_dep = dependency('popt', version: popt_req_version)
x11_dep = dependency('x11', version: x11_req_version)
xss_dep = dependency('xscrnsaver', version: xss_req_version)

conf_data = configuration_data()
conf_data.set('gttgladedir', '/usr/local/share/gnotime')
configure_file(configuration: conf_data,
               input: 'meson_config.h.in',
               output: 'config.h')

top_inc = include_directories('.')

subdir('src')
