# v005

* Allow custom IO DLLs to be specified in INI files:
  * `[aimeio] path=` for aime reader drivers
  * `[chuniio] path=` for Chunithm input drivers
  * `[idzio] path=` for Initial D Zero input drivers
* Add INI documentation
* Build system and contribution workflow improvements (contributed by icex2)
* Add hook to hide DVD drives (contributed by BemaniWitch)
* Add option to disable Diva slider emulation (contributed by dogtopus)
* AMEX board accuracy fixes (contributed by seika1, Felix)
* Improve multi-monitor support (contributed by BemaniWitch)
* Various Ongeki fixes (contributed by Felix)
* Various Diva slider fixes (contributed by dogtopus)

# v004

* Add initial support for mounting DLC package dumps (contributed by Shiz)
* Fix configuration loading in aimeio.dll
* Build system fixes (contributed by Shiz)

# v003

* Add countermeasures for DNS-spamming ISPs (contributed by mon)
* Add option for single-stick steering in IDZ (contributed by BemaniWitch)
* Fix MSVC build (contributed by mariodon)
* Make all 32 Chunithm touch slider cells' keyboard bindings configurable (see
  INI)

# v002

* Ship correct inject.exe for IDZ
* Fix IDZ main EXE crash in GetIfTable() hook in platform/netenv.c

# v001

* Initial release
