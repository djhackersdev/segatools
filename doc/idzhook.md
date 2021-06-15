# idzhook

## Supported games

- Initial D Arcade Stage Zero Version 1
- Initial D Arcade Stage Zero Version 2

## General remarks

- The minimum version of Windows that this game supports is Windows 8
- Most of the game is in japanese, even in EXP region mode.
- This game is hard to set up. You may encounter weird errors that are hard to
  diagnose

## Known issues

- The game will run in a semi windowed mode, even when set to fullscreen.
- No DirectInput force feedback (future Segatools enhancement)
- Felica.txt cards do not work with Version 2 of the game

## Data and game setup

1. Get the data
1. Ensure the game files are not marked read-only
1. Start up your favorite ALLNET server implementation in the background.
   Without it you won't be able to save your progress whatsoever.
1. The data releases have the following structure:

- `package/`: Game data
- `opt/`: Addon data
- `amfs/`: Metadata

4. Unpack segatools to the `package` directory
5. Create an `appdata` folder (this isn't Windows APPDATA) in the data release
   next to `package/`, `opt/` and `amfs/`
6. In the `[vfs]` section of `package/segatools.ini` set the paths for the
   folders:

```text
[vfs]
amfs=../../amfs
appdata=../../appdata
option=../../opt
```

1. In the `[dns]` section, set `default=` to your computer's hostname or LAN IP.
   Do not put `127.0.0.1` here, the game specifically checks for and rejects
   loopback addresses. This setting controls the address of the network services
   server
1. If you want to change the language to english, you can set `region=` in the
   `[ds]` section to `4`.
1. This will change the region to `EXP` which comes with a rudimentary english
   translation.

- Note for Version 2 of the game: You'll have to manually create a card ID and
  set it up. To do so, change your `package/segatools.ini` so that the `[aime]`
  section looks like this:

```
aimePath=DEVICE\aime.txt
felicaGen=0
```

- Then, go into the `package\DEVICE` folder, create a text file called aime.txt
  and fill it with 20 digits, for example `01234567891234567890`. Make sure to
  save the file.

1. Right click `start.bat` in `package` and run it as Administrator. After the
   first run you may be able to run the game as a normal user.
1. Once you're at the title screen, press 2 or 3 a few times to add some
   credits, then _hold_ the Enter key for a few seconds to scan a card and start
   a credit.

- In some cases it might be necessary to run the game from the `C:\` drive. Try
  copying the game there if for some reason it won't boot on a different drive.

## Segatools configuration

Configurable settings are exposed in the `segatools.ini` file. For a detailed
description, please refer to [this document](doc/config/common.md).

## Initial D Arcade Stage Zero specific configuration

For configuring IDZ specific features, e.g. IO, please refer to
[this document](doc/config/initiald.md).

## FAQ

### Where is the Free Play setting?

In the SEGA Nu system supervisor program, which is not included in this release.
A command-line tool to change this setting will be provided in a future release
of Segatools.

Do note however that Free Play is not advised for this game, as it will prohibit
you from purchasing additional cars.

### How do I get extra music?

Play through the story and also rack up your mileage.

### How do I unlock extra gauges?

Some will unlock in the story but most were time-limited online events. Try
editing the "idz_unlocks" table in the MiniMe database to force-unlock them on
your profile.

### Why does my story progress keep resetting?

If you are using minime, this is a known issue and will be fixed in a future
release.
