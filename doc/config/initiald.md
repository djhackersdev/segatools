# Initial D Arcade Stage Zero configuration settings

This file describes configuration settings specific to Initial D Arcade Stage
Zero.

Keyboard binding settings use
[Virtual-Key Codes](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes).

## `[io3]`

Cabinet specific inputs.

### `test`

Default `0x31` (`1 Key`)

Key-binding for cabinet test button.

### `service`

Default `0x32` (`2 Key`)

Key-binding for cabinet service button.

### `coin`

Default `0x33` (`3 Key`)

Key-binding for cabinet coin switch.

### `mode`

Choose whether you want to use a xinput (Xbox controllers, etc) or a dinput (PS
controllers, most wheels) type controller.

### `restrict`

Default `97`

This setting scales the steering wheel input so that the maximum positive and
minimum negative steering inputs reported in the operator menu's input test
screen do not exceed the value below. The maximum possible value is 128, and the
value that matches the input range of a real cabinet is 97.

NOTE: This is not the same thing as DirectInput steering wheel movement range!
Segatools cannot control the maximum angle of your physical steering wheel
controller, this setting is vendor-specific and can only be adjusted in the
Control Panel.

### `autoNeutral`

Default `1`

Automatically reset the simulated shifter to Neutral when XInput Start is
pressed (e.g. when navigating menus between races).

### `singleStickSteering`

Default `0`

By default if you use a xinput controller, segatools will use both sticks for
steering. While this is recommended for precision, it may be confusing to some
people. Set this to `1` to use a more traditional control scheme where only the
left stick is used to steer.

## `[dinput]`

Configuration for dinput controllers.

### `deviceName`

Default ` `

Name of the DirectInput wheel or controller to use (or any text that occurs in
its name) Example: TMX, Wireless Controller, T300, etc

If this is left blank then the first DirectInput device will be used. If you
don't know the name of your input device, you can find it in the windows
controller panel. The quickest way to access it is to press Win+R, then type in
`joy.cpl` and look at the list it will display.

### `shifterName`

Default ` `

Name of the positional shifter to use (or any subset thereof). Leave blank if
you do not have a positional shifter; a positional shifter will be simulated
using the configured Shift Down and Shift Up buttons in this case. If you don't
know the name of your input device, you can find it in the windows controller
panel. The quickest way to access it is to press Win+R, then type in `joy.cpl`
and look at the list it will display.

### Note on pedal mappings

Pedal mappings. Valid axis names are:

`X, Y, Z, RX, RY, RZ, U, V`

(U and V are old names for Slider 1 and Slider 2). The default values below are
valid for a Thrustmaster TMX.

### `brakeAxis`

Default `RZ`

Which axis to use for the brake pedal.

### `accelAxis`

Default `Y`

Which axis to use for the accelerator pedal.

### Note on button mappings

These are dinput button numbers. Note that buttons are numbered from 1, while
some software numbers buttons from 0. The default windows controller panel
should show the correct ones, if it hasn't been replaced by the controller
driver.

### `start`

Default `2`

Button mapping for the `Start` button.

### `viewChg`

Default `3`

Button mapping for the `View Change` button.

### `shiftDn`

Default `5`

Button mapping for shifting down with the emulated shifter. (If no `shifterName`
was specified)

### `shiftUp`

Default `6`

Button mapping for shifting up with the emulated shifter. (If no `shifterName`
was specified)

### `gear1`, `gear2`, `gear3`, `gear4`, `gear5`, `gear6`

Default `1, 2, 3, 4, 5, 6`

Button mappings for the physical shifter, if one was specified via the
`shifterName` setting.

### `reverseAccelAxis`

Default `0`

Inverts the accelerator axis if set to ‘1’. This is needed for certain
controllers like the DS4 or Dualsense.

### `reverseBrakeAxis`

Default `0`

Inverts the brake axis if set to ‘1’. This is needed for certain controllers
like the DS4 or Dualsense.
