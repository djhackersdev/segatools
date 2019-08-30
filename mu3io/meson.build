mu3io_dll = shared_library(
    'mu3io',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'mu3io.def',
    c_pch : '../precompiled.h',
    dependencies : [
        xinput_lib,
    ],
    sources : [
        'mu3io.c',
        'mu3io.h',
    ],
)
