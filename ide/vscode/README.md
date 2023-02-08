# Sample configuration files for developing with VSCode and an SWD Probe

This example provides files that illustrate how to use Microsoft(R) Visual Studio Code (*VSCode*) with a Serial Wire Debug (*SWD*) probe such as another Pico running running [picoprobe](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html#debugging-using-another-raspberry-pi-pico).

[Getting Started with the Raspberry Pi Pico](https://rptl.io/pico-get-started) describes a basic setup for VSCode and how to build picoprobe and `OpenOCD`, a tool that connects it to the GNU Debugger [GDB](https://www.sourceware.org/gdb/).

Configuring VSCode to use these tools enables it to upload code transparently and provide step-through debugging from within the IDE.

---

## Assumptions

The example assumes:

1. A functioning development environment with [pico-sdk](https://github.com/raspberrypi/pico-sdk), ARM Toolchain, VSCode and `OpenOCD` all built and installed according to the instructions in the [Getting Started Guide](https://rptl.io/pico-get-started).
2. A target connected via another Pico running [picoprobe](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html#debugging-using-another-raspberry-pi-pico).
3. A reasonable familiarity with the `pico-sdk`, CMake and VSCode workspaces (see the [VSCode documentation](https://code.visualstudio.com/docs/editor/workspaces)).

> Note: The provided files just illustrate working configurations but you will almost certainly want to further customise them to meet your needs.


---

## Using the Example

1. Open a fresh VScode workspace, add a simple standalone SDK project and create a `.vscode` folder at its top level.

2. Choose the `launch-*-swd.json` file closest to your setup (see below), rename it to `launch.json` and copy it to the `.vscode` folder you created.

    Start with:
    * `launch-non_raspberrypi-swd.json` if you are running VSCode on something other than a Raspberry Pi;
    * `launch-raspberrypi-swd.json` if you ARE running VSCode on a Raspberry Pi;
    * `launch-remote-openocd.json` if VSCode will connect to `GDB` remotely.

    Be sure to review the selected file and make any changes needed for your environment.

3. Copy the `settings.json` file into the `.vscode` folder. This illustrates how to configure the *CMake* plugin so that you debug using *cortex-debug* instead of trying to launch the executable on the host.
