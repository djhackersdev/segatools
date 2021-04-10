# Chunithm configuration settings

This file describes configuration settings specific to Chunithm.

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

## `[slider]`

Key bindings for each of the 32 touch cells.

### `cellX`

Defaults to key mappings in left-to-right-order as follows:

```text
SSSSDDDDFFFFGGGGHHHHJJJJKKKKLLLL
```

Key binding for a single touch cell. Replace the `X` with a value from `1` to `32` to bind keys to
each cell, e.g. `cell32=0x53`, `cell1=0x53` etc.

Touch cells are numbered FROM RIGHT TO LEFT! starting from 1. This is in order to match the
numbering used in the operator menu and service manual.
