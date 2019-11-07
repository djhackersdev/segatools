# Segatools

Quick start on a Linux build host:

```
# Install Meson and a recent build of MinGW-w64, then:

$ meson --cross cross-build-32.txt _build32
$ ninja -C _build32
$ meson --cross cross-build-64.txt
$ ninja -C _build64
```

Building on MSYS2 is also possible; consult Meson documentation for details.

Additional documentation will be forthcoming.
