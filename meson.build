project('gnotime', ['c'],
        license: 'GPL-2.0-or-later',
        version: '3.0.0',
)
add_project_link_arguments('-lm', language: ['c'])

glib_req_version = '>= 2.66.4'
gtk_req_version = '>= 2.24.32'
guile_req_version = '>= 2.2.7'
libxml_req_version = '>= 2.9.10'
popt_req_version = '>= 1.18'
x11_req_version = '>= 1.6.2'
xss_req_version = '>= 1.2.3'

glib_dep = dependency('glib-2.0', version: glib_req_version)
gtk_dep = dependency('gtk+-2.0', version: gtk_req_version)
guile_dep = dependency('guile-2.2', version: guile_req_version)
libxml_dep = dependency('libxml-2.0', version: libxml_req_version)
popt_dep = dependency('popt', version: popt_req_version)
x11_dep = dependency('x11', version: x11_req_version)
xss_dep = dependency('xscrnsaver', version: xss_req_version)

conf_data = configuration_data()
conf_data.set('gettext_package', 'gnotime-2.0')
conf_data.set('gttdatadir', '/usr/local/share/gnotime')
conf_data.set('gttgladedir', '/usr/local/share/gnotime')
conf_data.set('package', meson.project_name())
conf_data.set('version', meson.project_version())
configure_file(configuration: conf_data,
               input: 'meson_config.h.in',
               output: 'config.h')

top_inc = include_directories('.')

subdir('src')
