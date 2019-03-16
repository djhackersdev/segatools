platform_lib = static_library(
    'platform',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : [
        '../precompiled.c',
        '../precompiled.h',
    ],
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'hwmon.c',
        'hwmon.h',
        'pcbid.c',
        'pcbid.h',
        'nusec.c',
        'nusec.h',
    ],
)
