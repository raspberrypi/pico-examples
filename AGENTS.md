# Repository Guidelines

## Project Structure & Module Organization
This repository is a collection of Raspberry Pi Pico SDK examples. Top-level folders group examples by subsystem or feature area, such as `adc/`, `dma/`, `pio/`, `usb/`, and `pico_w/`. Each example usually lives in its own directory with source files plus a local `CMakeLists.txt`, for example `hello_world/serial/hello_serial.c` and `hello_world/serial/CMakeLists.txt`. Board- or feature-specific notes are stored beside examples as `README.md` or `README.adoc`.

## Build, Test, and Development Commands
Use an out-of-tree CMake build:

```sh
cmake -B build -S . -DPICO_BOARD=pico2
cmake --build build
```

Set `PICO_PLATFORM` explicitly when needed, for example `-DPICO_PLATFORM=rp2350`. Build a single example target during iteration:

```sh
cmake --build build --target hello_serial
```

When changing Pico W or FreeRTOS examples, configure any required SDK variables first, such as `PICO_BOARD=pico_w` or `FREERTOS_KERNEL_PATH`.

## Coding Style & Naming Conventions
Follow the existing C/C++ style from `CONTRIBUTING.md`:
- Use 4 spaces, not tabs.
- Keep opening braces on the same line.
- Use braces except for simple single-line `if` statements.

Match surrounding naming. Most examples use lowercase, underscore-separated target and file names such as `hello_uart`, `pwm_led_fade`, and `status_blink.c`.

## Testing Guidelines
There is no centralized unit-test suite in this repository. The expected validation is successful CMake configure plus a clean build of the affected targets. Prefer targeted builds while developing, then rebuild the broader tree if you changed shared code or CMake logic. Include the board or platform used for validation in your PR notes.

## Commit & Pull Request Guidelines
Recent commit subjects are short, imperative, and specific, for example `require SDK version 2.2.0` or `Update multi gcc (#689)`. Keep commits focused and descriptive.

Upstream contributions should open PRs against `develop`, not `master`; PRs against `master` are rejected by CI. In the PR description, summarize the example or subsystem changed, list build/board coverage, and link any related issue. Add screenshots or serial output only when they clarify user-visible behavior.
