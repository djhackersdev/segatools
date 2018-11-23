project('capnhook', 'c', version: '0.1.0')

add_project_arguments(
    '-Wall',
    '-DCOBJMACROS',
    '-ffunction-sections',
    '-fdata-sections',
    language: 'c',
)

add_project_link_arguments(
    '-Wl,--exclude-all-symbols',
    '-Wl,--gc-sections',
    '-static-libgcc',
    language: 'c',
)

inc = include_directories('.')
capnhook = subproject('capnhook')

subdir('board')
subdir('jvs')
subdir('nu')
subdir('util')

subdir('cardhook')
subdir('chunihook')
subdir('minihook')
