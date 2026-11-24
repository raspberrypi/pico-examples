# Pico W FreeRTOS examples

These examples build on top of the FreeRTOS SMP port for RP-series microcontrollers.
They are **skipped** by the top-level build unless a FreeRTOS kernel checkout is
available.

## Getting the FreeRTOS kernel

Clone the FreeRTOS kernel alongside this repository:

```sh
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git
```

Use a release that contains the RP2040/RP2350 port (for example the
`V11.1.0` tag). The examples expect the port under
`portable/ThirdParty/GCC/RP2040` (or the Community-Supported-Ports location for
newer layouts).

## Building

Point `FREERTOS_KERNEL_PATH` at the kernel checkout, either on the cmake command
line or in the environment:

```sh
cmake -B build -DPICO_BOARD=pico_w \
  -DFREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel
cmake --build build --target picow_freertos_ping
```

Or via the environment:

```sh
export FREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel
cmake -B build -DPICO_BOARD=pico_w
cmake --build build
```

The correct port is selected automatically from `PICO_PLATFORM`:

- `rp2040` — `portable/ThirdParty/GCC/RP2040`
- `rp2350-arm` — `portable/ThirdParty/GCC/RP2350_ARM_NTZ`
- `rp2350-riscv` — `portable/ThirdParty/GCC/RP2350_RISC-V`

## Available examples

- `http_client` — HTTP client
- `httpd` — HTTP server with mDNS
- `iperf` — network throughput test
- `ntp_client_socket` — NTP client using sockets
- `ping` — ICMP ping

Each example uses `FreeRTOSConfig_examples_common.h` in this directory for the
shared kernel configuration.
