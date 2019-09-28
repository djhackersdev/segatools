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
        'backend.h',
        'dllmain.c',
        'idzio.h',
        'shifter.c',
        'shifter.h',
        'wnd.c',
        'wnd.h',
        'xi.c',
        'xi.h',
    ],
)
