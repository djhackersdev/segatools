project('segatools', 'c', version: '0.1.0')

add_project_arguments(
    '-DCOBJMACROS',
    '-DWIN32_LEAN_AND_MEAN',
    '-D_WIN32_WINNT=_WIN32_WINNT_WIN7',
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

cc = meson.get_compiler('c')
xinput_lib = cc.find_library('xinput')

inc = include_directories('.')
capnhook = subproject('capnhook')

subdir('amex')
subdir('board')
subdir('jvs')
subdir('platform')
subdir('util')

subdir('aimeio')
subdir('cardhook')
subdir('chunihook')
subdir('divahook')
subdir('idzhook')
subdir('minihook')
subdir('zinput')
