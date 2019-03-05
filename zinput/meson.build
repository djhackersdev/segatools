shared_library(
    'zinput',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'zinput.def',
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    link_with : [
        util_lib,
    ],
    sources : [
        'dllmain.c',
    ],
)
