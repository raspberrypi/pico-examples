# Sample configuration files for developing with VSCode and the SWD port

This example provides files that illustrate how to use Microsoft(R) Visual Studio Code (*VSCode*) with a target connected via its Serial Wire Debug (*SWD*) port.

Many configurations are possible. VSCode might be running on:

* a Raspberry Pi wired directly to a target's SWD pins
* a PC or Mac connected to a target via a Pico running [picoprobe](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html#debugging-using-another-raspberry-pi-pico)
* a machine with an already running instance of `OpenOCD`

[Getting Started with the Raspberry Pi Pico](https://rptl.io/pico-get-started) describes a basic setup for VSCode and how to build picoprobe and `OpenOCD`, a tool that connects to the GNU Debugger [GDB](https://www.sourceware.org/gdb/).

**Configuring VSCode to use the SWD port lets it upload code transparently and gives you step-through debugging from within the IDE.**

---

## Assumptions

The example assumes:

1. A functioning development environment with [pico-sdk](https://github.com/raspberrypi/pico-sdk), ARM Toolchain, VSCode and `OpenOCD` all built and installed according to the instructions in the [Getting Started Guide](https://rptl.io/pico-get-started).
2. A target connected via the SWD port (see above).
3. Reasonable familiarity with [pico-sdk](https://github.com/raspberrypi/pico-sdk), [CMake](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) and [VSCode workspaces](https://code.visualstudio.com/docs/editor/workspaces).

> Note: The provided files illustrate working configurations but you will almost certainly want to customise them further to meet your needs.


---

## Using the Example

1. Open a fresh VScode workspace, add a simple standalone SDK project (removing any existing `build` directory) and create a `.vscode` folder at its top level.

2. Choose the `launch-*-swd.json` file closest to your setup (see below), rename it to `launch.json` and copy it to the `.vscode` folder you created.

    Start with:
    * `launch-probe-swd.json` if the target is connected via an SWD probe (e.g. picoprobe)
    * `launch-raspberrypi-swd.json` if the target is directly connected to a Raspberry Pi GPIO 
    * `launch-remote-openocd.json` if VSCode should connect to an already running instance of `OpenOCD`.

> Be sure to review the selected file and make any changes needed for your environment.

3. Copy the `settings.json` file into the `.vscode` folder. This illustrates how to configure the *CMake* plugin so that you debug using *cortex-debug* instead of trying to launch the executable on the host.

Lauching a debug session in the workspace (e.g. with *f5*) should now build the project and if successful upload it to the target, open a debug session and pause at the start of `main()`.

> VSCode has some background work to do the first time through. If appears to have stalled then leave it alone for at least a minute.

> If `OpenOCD` reports that GDB won't launch or quits unexpectedly, you may need to adjust the *gdbPath* and/or *configFiles* settings in `launch.json`. See the comments in the example files and check the latest [Getting Started](https://rptl.io/pico-get-started).
