idzio_dll = shared_library(
    'idzio',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'idzio.def',
    c_pch : '../precompiled.h',
    dependencies : [
        xinput_lib,
    ],
    sources : [
        'idzio.c',
        'idzio.h',
    ],
)
