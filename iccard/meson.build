iccard_lib = static_library(
    'iccard',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'aime.c',
        'aime.h',
        'felica.c',
        'felica.h',
        'mifare.h',
    ],
)
