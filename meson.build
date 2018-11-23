project('segatools', 'c', version: '0.1.0')

add_project_arguments(
    '-DCOBJMACROS',
    '-DWIN32_LEAN_AND_MEAN',
    language: 'c',
)

# Use get_argument_syntax() instead once Meson 0.49.0 releases
if meson.get_compiler('c').get_id() != 'msvc'
    add_project_arguments(
        '-Wall',
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
endif

inc = include_directories('.')
capnhook = subproject('capnhook')

subdir('board')
subdir('jvs')
subdir('nu')
subdir('util')

subdir('cardhook')
subdir('chunihook')
subdir('minihook')
