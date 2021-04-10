# Segatools common configuration settings

This file describes configuration settings for Segatools that are common to
all games.

Keyboard binding settings use
[Virtual-Key Codes](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes).

## `[aime]`

Controls emulation of the Aime card reader assembly.

### `enable`

Default: `1`

Enable Aime card reader assembly emulation. Disable to use a real SEGA Aime
reader (COM port number varies by game).

### `aimePath`

Default: `DEVICE\aime.txt`

Path to a text file containing a classic Aime IC card ID. **This does not
currently work**.

### `felicaPath`

Default: `DEVICE\felica.txt`

Path to a text file containing a FeliCa e-cash card IDm serial number.

### `felicaGen`

Default: `1`

Whether to generate a random FeliCa ID if the file at `felicaPath` does not
exist.

### `scan`

Default: `0x0D` (`VK_RETURN`)

Virtual-key code. If this button is **held** then the emulated IC card reader
emulates an IC card in its proximity. A variety of different IC cards can be
emulated; the exact choice of card that is emulated depends on the presence or
absence of the configured card ID files.

## `[amvideo]`

Controls the `amvideo.dll` stub built into Segatools. This is a DLL that is
normally present on the SEGA operating system image which is responsible for
changing screen resolution and orientation.

### `enable`

Default: `1`

Enable stub `amvideo.dll`. Disable to use a real `amvideo.dll` build. Note that
you must have the correct registry settings installed and you must use the
version of `amvideo.dll` that matches your GPU vendor (since these DLLs make
use of vendor-specific APIs).

## `[clock]`

Controls hooks for Windows time-of-day APIs.

### `timezone`

Default: `1`

Make the system time zone appear to be JST. SEGA games malfunction in strange
ways if the system time zone is not JST. There should not be any reason to
disable this hook other than possible implementation bugs, but the option is
provided if you need it.

### `timewarp`

Default: `0`

Experimental time-of-day warping hook that skips over the hardcoded server
maintenance period. Causes an incorrect in-game time-of-day to be reported.
Better solutions for this problem exist and this feature will probably be
removed soon.

### `writeable`

Default: `0`

Allow game to adjust system clock and time zone settings. This should normally
be left at `0`, but the option is provided if you need it.

## `[dns]`

Controls redirection of network server hostname lookups

### `default`

Default: `localhost`

Controls hostname of all of the common network services servers, unless
overriden by a specific setting below. Most users will only need to change this
setting. Also, loopback addresses are specifically checked for and rejected by
the games themselves; this needs to be a LAN or WAN IP (or a hostname that
resolves to one).

### `router`

Default: Empty string (i.e. use value from `default` setting)

Overrides the target of the `tenporouter.loc` and `bbrouter.loc` hostname
lookups.

### `startup`

Default: Empty string (i.e. use value from `default` setting)

Overrides the target of the `naominet.jp` host lookup.

### `billing`

Default: Empty string (i.e. use value from `default` setting)

Overrides the target of the `ib.naominet.jp` host lookup.

### `aimedb`

Default: Empty string (i.e. use value from `default` setting)

Overrides the target of the `aime.naominet.jp` host lookup.

## `[ds]`

Controls emulation of the "DS (Dallas Semiconductor) EEPROM" chip on the AMEX
PCIe board. This is a small (32 byte) EEPROM that contains serial number and
region code information. It is not normally written to outside of inital
factory provisioning of a Sega Nu.

### `enable`

Default: `1`

Enable DS EEPROM emulation. Disable to use the DS EEPROM chip on a real AMEX.

### `region`

Default: `1`

AMEX Board region code. This appears to be a bit mask?

- `1`: Japan
- `2`: USA? (Dead code, not used)
- `4`: Export
- `8`: China

### `serialNo`

Default `AAVE-01A99999999`

"MAIN ID" serial number. First three characters are hardware series:

- `AAV`: Nu-series
- `AAW`: NuSX-series
- `ACA`: ALLS-series

## `[eeprom]`

Controls emulation of the bulk EEPROM on the AMEX PCIe board. This chip stores
status and configuration information.

### `enable`

Default: `1`

Enable bulk EEPROM emulation. Disable to use the bulk EEPROM chip on a real
AMEX.

### `path`

Default: `DEVICE\eeprom.bin`

Path to the storage file for EEPROM emulation. This file is automatically
created and initialized with a suitable number of zero bytes if it does not
already exist.

## `[gpio]`

Configure emulation of the AMEX PCIe GPIO (General Purpose Input Output)
controller.

### `enable`

Default: `1`

Enable GPIO emulation. Disable to use the GPIO controller on a real AMEX.

### `sw1`

Default `0x70` (`VK_F1`)

Keyboard binding for Nu chassis SW1 button (alternative Test)

### `sw2`

Default `0x71` (`VK_F2`)

Keyboard binding for Nu chassis SW2 button (alternative Service)

### `dipsw1` .. `dipsw8`

Defaults: `1`, `0`, `0`, `0`, `0`, `0`, `0`, `0`

Nu chassis DIP switch settings:

- Switch 1: Game-specific, but usually controls the "distribution server"
    setting. Exactly one arcade machine on a cabinet router must be set to the
    Server setting.
    - `0`: Client
    - `1`: Server
- Switch 2,3: Game-specific.
    - Used by Mario&Sonic to configure cabinet ID, possibly other games.
- Switch 4: Screen orientation. Only used by the Nu system startup program.
    - `0`: YOKO/Horizontal
    - `1`: TATE/Vertical
- Switch 5,6,7: Screen resolution. Only used by the Nu system startup program.
    - `000`: No change
    - `100`: 640x480
    - `010`: 1024x600
    - `110`: 1024x768
    - `001`: 1280x720
    - `101`: 1280x1024
    - `110`: 1360x768
    - `111`: 1920x1080
- Switch 8: Game-specific. Not used in any shipping game.

## `[hwmon]`

Configure stub implementation of the platform hardware monitor driver. The
real implementation of this driver monitors CPU temperatures by reading from
Intel Model Specific Registers, which is an action that is only permitted from
kernel mode.

### `enable`

Default `1`

Enable hwmon emulation. Disable to use the real hwmon driver.

## `[jvs]`

Configure emulation of the AMEX PCIe JVS *controller* (not IO board!)

### `enable`

Default `1`

Enable JVS port emulation. Disable to use the JVS port on a real AMEX.

## `[keychip]`

Configure keychip emulation.

### `enable`

Enable keychip emulation. Disable to use a real keychip.

### `id`

Default: `A69E-01A88888888`

Keychip serial number. Keychip serials observed in the wild follow this
pattern: `A6xE-01Ayyyyyyyy`.

### `gameId`

Default: (Varies depending on game)

Override the game's four-character model code. Changing this from the game's
expected value will probably just cause a system error.

### `platformId`

Default: (Varies depending on game)

Override the game's four-character platform code (e.g. `AAV2` for Nu 2). This
is actually supposed to be a separate three-character `platformId` and
integer `modelType` setting, but they are combined here for convenience. Valid
values include:

- `AAV0`: Nu 1 (Project DIVA)
- `AAV1`: Nu 1.1 (Chunithm)
- `AAV2`: Nu 2 (Initial D Zero)
- `AAW0`: NuSX 1
- `AAW1`: NuSX 1.1
- `ACA0`: ALLS UX
- `ACA1`: ALLS HX
- `ACA2`: ALLS UX (without dedicated GPU)
- `ACA4`: ALLS MX

### `region`

Default: `1`

Override the keychip's region code. Most games seem to pay attention to the
DS EEPROM region code and not the keychip region code, and this seems to be
a bit mask that controls which Nu PCB region codes this keychip is authorized
for. So it probably only affects the system software and not the game software.
Bit values are:

- 1: JPN: Japan
- 2: USA (unused)
- 3: EXP: Export (for Asian markets)
- 4: CHS: China (Simplified Chinese?)

### `systemFlag`

Default: `0x64`

An 8-bit bitfield of unclear meaning. The least significant bit indicates a
developer dongle, I think? Changing this doesn't seem to have any effect on
anything other than Project DIVA.

Other values observed in the wild:

- `0x04`: SDCH, SDCA
- `0x20`: SDCA

### `subnet`

Default `192.168.100.0`

The LAN IP range that the game will expect. The prefix length is hardcoded into
the game program: for some games this is `/24`, for others it is `/20`.

## `[netenv]`

Configure network environment virtualization. This module helps bypass various
restrictions placed upon the game's LAN environment.

### `enable`

Default `1`

Enable network environment virtualization. You may need to disable this if
you want to do any head-to-head play on your LAN.

Note: The virtualized LAN IP range is taken from the emulated keychip's
`subnet` setting.

### `addrSuffix`

Default: `11`

The final octet of the local host's IP address on the virtualized subnet (so,
if the keychip subnet is `192.168.32.0` and this value is set to `11`, then the
local host's virtualized LAN IP is `192.168.32.11`).

### `routerSuffix`

Default: `1`

The final octet of the default gateway's IP address on the virtualized subnet.

### `macAddr`

Default: `01:02:03:04:05:06`

The MAC address of the virtualized Ethernet adapter. The exact value shouldn't
ever matter.

## `[pcbid]`

Configure Windows host name virtualization. The ALLS-series platform no longer
has an AMEX board, so the MAIN ID serial number is stored in the Windows
hostname.

### `enable`

Default: `1`

Enable Windows host name virtualization. This is only needed for ALLS-platform
games (since the ALLS lacks an AMEX and therefore has no DS EEPROM, so it needs
another way to store the PCB serial), but it does no harm on games that run on
earlier hardware.

### `serialNo`

Default: `ACAE01A99999999`

Set the Windows host name. This should be an ALLS MAIN ID, without the
hyphen (which is not a valid character in a Windows host name).

## `[sram]`

Configure emulation of the AMEX PCIe battery-backed SRAM. This stores
bookkeeping state and settings. This file is automatically created and
initialized with a suitable number of zero bytes if it does not already exist.

### `enable`

Default `1`

Enable SRAM emulation. Disable to use the SRAM on a real AMEX.

### `path`

Default `DEVICE\sram.bin`

Path to the storage file for SRAM emulation.

## `[vfs]`

Configure Windows path redirection hooks.

### `enable`

Default: `1`

Enable path redirection.

### `amfs`

Default: Empty string (causes a startup error)

Configure the location of the SEGA AMFS volume. Stored on the `E` partition on
real hardware.

### `appdata`

Default: Empty string (causes a startup error)

Configure the location of the SEGA "APPDATA" volume (nothing to do with the
Windows user's `%APPDATA%` directory). Stored on the `Y` partition on real
hardware.

### `option`

Default: Empty string

Configure the location of the "Option" data mount point. This mount point is
optional (hence the name, probably) and contains directories which contain
minor over-the-air content updates.
