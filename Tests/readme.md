# How to execute Tests

## Requirements

Only Linux OS is supported for now

## Kronos installation

Kronos can be downloaded from https://github.com/FCare/Kronos,
either use the Ubuntu binary delivered with each release, or build it from source.

## Kronos configuration

A few basic things are required for Kronos to run:
- Disable Wayland
- In Setting/General/BIOS Settings: a BIOS is required
- In Setting/Cart-Memory/Cartridge: Select Development Extension

Close Kronos, everything shall be setup at that point.

## Build the tests

Execute `make all` within the `SaturnRingLib/Tests` directory from a terminal.

You can override makefile defaults at build time. For example, to change the log output target:

```bash
make all SRL_LOG_OUTPUT=DEV_CART
```

Other valid values: `EMULATOR`, `DEV_CART`, `NONE`.

## Run the tests

Execute `run_tests.bat` within the `SaturnRingLib/Tests` directory from a terminal, tests results will be output in both the terminal and `SaturnRingLib/Tests/uts.log`, such as:

````
INFO : ***UT_START***
INFO : ****UT_ANGLE_ERROR(S)****
FATAL : angle_test_operator_addition_wrap_around failed:
	src/testsAngle.hpp:900: Addition wrap-around failed: 9 != 10
FATAL : angle_test_operator_subtraction_wrap_around failed:
	src/testsAngle.hpp:910: Subtraction wrap-around failed: 350 != 350
...
FATAL : memory_test_memory_leaks failed:
	src/testsMemory.hpp:844: Memory allocation failed
INFO : ****UT_bitmap_ERROR(S)****
FATAL : palette_test_initialization failed:
	src/testsBitmap.hpp:77: Palette colors not allocated correctly: 0
INFO : 219 tests, 327 assertions, 23 failures
INFO : Finished in f seconds (real) f seconds (proc)
INFO : ***UT_END***
````

Note : Changing SRL_LOG_LEVEL from INFO to TRACE in the makefile will make an output for every single tests.

## USBGamers (real hardware)

The `USBGamers` mode in `run_tests.bat` pushes the test binary to a USBGamers cartridge using `ftx`.

### Requirements

- USBGamers cartridge connected to the Saturn.
- `ftx` tool available in your PATH.

#### Linux Setup

`usbreset` tool shall be available in your PATH (used to reset the USB device).

#### Windows Driver Setup

On Microsoft Windows, the utility communicates with the Sega Saturn USB cartridge via `libusb` and `libftdi`. Since Windows installs the standard FTDI Serial Port (COM) driver by default, you must replace it with the generic **WinUSB** driver for the utility to detect and access the device.

To install the WinUSB driver:
1. Connect the USB cartridge to your Windows PC.
2. Download and run **Zadig** from [zadig.akeo.ie](https://zadig.akeo.ie/).
3. In Zadig, select **Options** -> **List All Devices** from the top menu.
4. In the main drop-down menu, select your cartridge device (typically shown as `FT245R USB FIFO` or similar).
5. Verify that the USB ID matches the target device (the default is VID `0403` and PID `6001`).
6. Select **WinUSB** as the target driver (to the right of the green arrow).
7. Click **Replace Driver** (or **Install Driver**) and wait for the process to finish.

*(Note: To revert to the standard FTDI virtual COM port drivers, you can uninstall the device from the Windows Device Manager and check the option to delete the driver software for the device, then replug it.)*

### Run

1. Build tests with `make all`.
2. Run `./run_tests.bat USBGamers`.

The script resets the USB device, uploads `cd/data/0.bin` to address `0x06004000`, and then starts the capture flow.
The output still goes to `uts.log` like the emulator runs.

