# Segatools

Quick start on a Linux build host:

```
# Install Meson and a recent build of MinGW-w64, then:

$ meson --cross cross-mingw-32.txt _build32
$ ninja -C _build32
$ meson --cross cross-mingw-64.txt _build64
$ ninja -C _build64
```

Building on MSYS2 is also possible; consult Meson documentation for details.

Additional documentation will be forthcoming.
