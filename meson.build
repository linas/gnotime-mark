project('gnotime', ['c'],
        license: 'GPL-2.0-or-later',
        version: '3.0.0',
)

conf_data = configuration_data()
configure_file(configuration: conf_data,
               input: 'meson_config.h.in',
               output: 'config.h')

top_inc = include_directories('.')

subdir('src')
