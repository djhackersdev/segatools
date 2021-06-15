# chunihook

## Supported games

* Chunithm (Plus)
* Chunithm Air (Plus)
* Chunithm Star (Plus)
* Chunithm Amazon (Plus)
* Chunithm Crystal (Plus)

## General remarks

* The minimum version of Windows that this game supports is Windows 8
* The entire user interface, including the operator menu, is in Japanese
* This game is hard to set up. You may encounter weird errors that are hard to diagnose

## Known issues

* JST LOCKOUT: Game is not playable between 1:30 AM and 7:00ish AM JST.
* Only on-board audio works
* Cross-shaped graphical artifacts during gameplay

## Data and game setup

1. Get the data
1. Ensure the game files are not marked read-only
1. Start up your favorite ALLNET server implementation in the background. Whether or not you plan to
save your scores the game must be "blessed" by a server at least once, otherwise there will be a
spinner on the title screen forever and you will be unable to start a credit
1. The data releases have the following structure:
  * `app/`: Game data
  * `option/`: Addon data
  * `amfs/`: Metadata
1. Unpack segatools to the `app/bin` directory
1. Create an `appdata` foder (this isn't Windows APPDATA) in the data release next to `app/`,
`option/` and `amfs/`
1. In the `[vfs]` section of `app/bin/segatools.ini` set the paths for the folders:
```text
[vfs]
amfs=../../amfs
appdata=../../appdata
option=../../option
```
1. In the `[dns]` section, set `default=` to your computer's hostname or LAN IP. Do not put
`127.0.0.1` here, the game specifically checks for and rejects loopback addresses. This setting
controls the address of the network services server
1. Right click `start.bat` in `app/bin` and run it as Administrator. I think you need to run it as
admin at least once, but once you have done that you can run the game as a regular user
1. A sequence of several start-up screens will be displayed. You should also see a bunch of debug
output in a command line window; if you're seeing hex dumps here then that's a good sign. There
will eventually be a screen with a red error message about LEDs. This is being displayed because
Segatools does not currently emulate the cabinet's RGB LED strips
1. Press 1 to go to the operator menu. Use either Test/Service or the touch bar to navigate the
options
  1. Select ゲーム設定 (Game settings)
  1. Select 配信サーバー設定 (Distribution server setting)
  1. Set this setting to サーバー　(Server)
  1. Select 筐体グループ設定 (Cabinet group setting)
  1. Set this setting to OFF
  1. If desired, you can also set 音楽選択時間設定 (Music selection time setting) to 99 or whatever
  1. Select 終了 (Finish)
  1. There will be a centered prompt notifying you that a restart is necessary to put your new
  settings into effect. Confirm both prompts. The game will now exit

This setting has nothing to do with the game's central network services, it describes the role that
the cabinet has on the shop LAN. To be exact, every networked group of SEGA cabinets (one cabinet is
still a group) must have exactly one cabinet designated as the "Distribution Server" and all the
others configured as "Clients". The clients will search for a distribution server on their
inter-cabinet LAN: they will not finish starting up until they find one.

Normally the client/server setting is controlled by DIP switch 1 on the Nu PCB chassis. However, for
some reason this is controlled from the operator menu in Chunithm. And the default setting after
NVRAM reset is Client mode.

* Start the game up again, then dismiss the LED error message using the touch strip. You should see
a title screen with a red glow along the bottom (or, if you were less lucky, a red banner or a
spinner that doesn't go away).
* Press 2 or 3 a few times to add some credits, then *hold* the Enter key for a few seconds to scan
a card and start a credit. A random card ID will be written to `DEVICE\felica.txt` the first time
you do this.

## Segatools configuration

Configurable settings are exposed in the `segatools.ini` file. For a detailed description, please
refer to [this document](doc/config/common.md).

## Chunithm specific configuration

For configuring chuinthm specific features, e.g. IO, please refer to
[this document](doc/config/chunithm.md).

## FAQ

### Where is the Free Play setting?

In the SEGA Nu system supervisor program, which is not included in this release. A command-line
tool to change this setting will be provided in a future release of Segatools.

### The network test screen shows a bunch of BAD checks and (either a red RTC error screen or an infinite spinner on the title screen) appears which I can't get past

This means that the game's ALLNET client software failed to start up. The ISP domain squatting
thing is a common cause for this problem, but unfortunately there are a large number of other
possibilities. Try uncommenting the `chuniApp.exe=chunispike.txt` line in `segatools.ini` to enable
the internal ALLNET debug log; this may possibly yield some clues.

### What does the red "本日のプレイ受付は終了しました" text on the title screen mean?

"Play acceptance has ended for today". Basically it means that it's outside legal operating hours
for Japanese arcades right now. Hopefully somebody can come up with a hex edit to bypass that
message but for now you'll need to change your PC's clock or something.

### How do I play courses?

This requires additional server support which might not be implemented currently.

### How do I unlock a character other than the penguin?

Pick any map other than the default. You will see characters available as potential rewards for
those maps.
