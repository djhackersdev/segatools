aimeio_dll = shared_library(
    'aimeio',
    name_prefix : '',
    include_directories: inc,
    implicit_include_directories : false,
    vs_module_defs : 'aimeio.def',
    c_pch : '../precompiled.h',
    link_with : [
        util_lib,
    ],
    sources : [
        'aimeio.c',
    ],
)
