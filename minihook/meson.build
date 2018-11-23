shared_library(
    'minihook',
    name_prefix : '',
    include_directories: inc,
    implicit_include_directories : false,
    c_pch : [
        '../precompiled.c',
        '../precompiled.h',
    ],
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    link_with : [
        nu_lib,
        util_lib,
    ],
    sources : [
        'dllmain.c',
    ],
)
