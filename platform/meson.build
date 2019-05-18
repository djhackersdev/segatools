platform_lib = static_library(
    'platform',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'amvideo.c',
        'amvideo.h',
        'config.c',
        'config.h',
        'hwmon.c',
        'hwmon.h',
        'pcbid.c',
        'pcbid.h',
        'nusec.c',
        'nusec.h',
    ],
)
