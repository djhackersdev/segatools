jvs_lib = static_library(
    'jvs',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'jvs-bus.c',
        'jvs-bus.h',
        'jvs-cmd.h',
        'jvs-frame.c',
        'jvs-frame.h',
        'jvs-util.c',
        'jvs-util.h',
    ],
)
