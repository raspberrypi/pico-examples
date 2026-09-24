# rpi-connect-ota-demo — OTA integration example

A complete, working example of integrating **Raspberry Pi Connect** over-the-air
(OTA) firmware updates into a Pico application. It is intended to be read and
copied: the code in `rpi_connect_ota_demo.c` and `main_pico.c` shows the full
lifecycle — sign in, register for OTA, listen for deployments, download to the
inactive flash slot, verify, and reboot into the new image.

If you are adding OTA to your own application, start with `rpi_connect_ota_demo.c` —
it is the smallest piece of code that drives the `pico_rpi_connect_ota` API end
to end.

---

## What it demonstrates

* Obtaining a Connect **access token** at runtime by signing a *device-identity
  exchange* with a P-256 private key held in **OTP** — so no token or key need
  be carried in the firmware image (baking a token in is only ever done for
  debug).
* Caching the token (and WiFi credentials) in **FFS** (flash file storage) so
  they survive reboots and are never compiled into the binary.
* Registering the **OTA capability** and listening for deployment events over a
  persistent SSE connection.
* Streaming a firmware image straight into the **inactive A/B flash partition**,
  verifying its SHA-256 checksum, and rebooting into it via the RP2350 bootrom.
* **Resuming** a deployment that was interrupted by a reboot, and reporting
  success/failure back to the server.

## Hardware and host requirements

* **Board:** a **Pico 2 W (RP2350 with WiFi)**. OTA is RP2350-only — it relies
  on the bootrom flash-update mechanism, A/B partitions and OTP. WiFi is needed
  for connectivity.
* **Host tools:** [`picotool`](https://github.com/raspberrypi/picotool),
  `openssl`, a C compiler, `python3`, the libcurl and OpenSSL development
  packages (for the host build in Step 2), and the Pico SDK with its
  submodules initialised. The SDK's `lib/lwip` submodule points at the
  Raspberry Pi lwIP fork, which carries the HTTP client changes Pi Connect
  relies on; `lwipopts.h` in this directory also configures that client for
  Connect (`HTTPC_SEND_ACCEPT_HEADER 0`, `HTTPC_SEND_CONNECTION_CLOSE 0`, and a
  raised `HTTPC_POLL_TIMEOUT` for the long-lived requests).
* A Raspberry Pi Connect account with an **organisation** you can register
  devices against, and an **organisation token** for that org.

## How authentication works (the security model)

Under normal operation **no secret is ever compiled into the firmware**:

| Secret                | Where it lives        | Provisioned by                                   |
|-----------------------|-----------------------|--------------------------------------------------|
| Device identity key   | OTP (row `0xc0`)      | `pico_rpi_connect_program_identity_key_otp.sh`   |
| WiFi credentials      | FFS partition         | `partition_pico2_for_ffs.sh`                     |
| Connect access token  | FFS (id `0x1`)        | derived at runtime, then cached                  |

At boot the application reads the P-256 private key from OTP, regenerates the
matching public key, and performs a **device-identity exchange** with the
Connect API. The request is signed with the private key, so the device proves
its identity without ever transmitting the key. The returned access token is
cached in FFS so subsequent boots skip the exchange until it expires.

The device's **public** key must have been registered against your organisation
beforehand (Step 2 below) — that is what links this physical device to your org.

Holding the identity key in OTP is what makes this robust. The credible
alternative — pre-provisioning a Connect token (or the key) directly into flash
— is fragile: flash can be erased or overwritten (not least by an OTA update
itself), taking the credential with it. OTP survives a full flash erase, so the
device can always re-derive a fresh token. Tokens are never carried in the
firmware image; for development a literal token can be provisioned into FFS
with `partition_pico2_for_ffs.sh --connect-token`.

> **Debug shortcuts.** For bring-up you can bypass OTP with a build-time PEM
> identity key via `debug_options.cmake`. This exists only for development —
> production builds provision via OTP as described below. WiFi credentials
> and auth tokens cannot be compiled in at all: they only ever enter the
> device via FFS provisioning (or, for tokens, the device-identity exchange),
> and the serial number always comes from the Pico's OTP unique board ID.

---

## Step-by-step walkthrough

### Step 1 — Generate a device identity key pair

Each device needs its own P-256 (`prime256v1`) ECDSA key pair. The private key
is burned into the device's OTP; the public key is registered with your org.

```sh
openssl ecparam -name prime256v1 -genkey -noout -out device-priv-key.pem
openssl ec -in device-priv-key.pem -pubout -out device-pub-key.pem
```

> **Store the private key safely — and do not lose it.** It is the device's
> identity: anyone who has it can impersonate the device, and OTP is
> one-time-programmable so the key written there can never be changed. Treat it
> as a per-device secret, keep it under restricted access (ideally in a
> dedicated key store / HSM for production), and back it up. The matching
> public key is what you register with your org, so losing the private key means
> you can no longer prove ownership of that identity.

### Step 2 — Write the private key to OTP

Burn the 32-byte raw private-key scalar into the device's OTP. **OTP is
one-time-programmable - once written it can never be changed or erased**.
The key takes up 16 OTP rows (by default rows `0xc0 - 0xcf`).

To write the private key as ECC, use `picotool`:
```sh
picotool otp load -s 0xc0 device-priv-key.pem
```

This writes the key starting at OTP row `0xc0` (page 3), matching
`RPI_CONNECT_IDENTITY_OTP_ROW` in the library. Change the `-s 0xc0`
argument to use a different row.

### Step 3 — Register the device identity with your organisation

#### Option A - Host built binary

Registration runs on the **host** (Linux/macOS) using the host build of this
example (`main.c` in this directory), which links the `pico_rpi_connect`
library against host OpenSSL and curl.

Build the host tool with the SDK host platform. Use a separate build
directory and select the `none` board explicitly: a device `PICO_BOARD`
exported in your environment (e.g. `pico2_w`) would otherwise be picked up and
conflict with the host platform. `rpi-connect-ota-host-build.sh` in this
directory runs exactly these two commands:

```sh
cmake -S $PICO_EXAMPLES_PATH -B $PICO_EXAMPLES_PATH/build-host -DPICO_PLATFORM=host -DPICO_BOARD=none
cmake --build $PICO_EXAMPLES_PATH/build-host --target rpi_connect_ota_demo
alias rpi-connect-test=$PICO_EXAMPLES_PATH/build-host/rpi-connect/rpi-connect-ota-demo/rpi_connect_ota_demo
```

The host build needs the libcurl and OpenSSL development packages installed;
without them the SDK skips `pico_rpi_connect` and the target does not exist.

Register the public key against your org. The request is itself signed with the
private key (`--device-privkey`) to prove ownership of the pair:

```sh
export RPI_CONNECT_ORG_TOKEN="<your-organisation-token>"

rpi-connect-test --create-device-identity \
    --device-privkey device-priv-key.pem \
    --device-pubkey device-pub-key.pem \
    --description   "Pico 2 W OTA demo" \
    --device-name   "pico-ota-01"
```

The device now exists in your organisation. (You can sanity-check the whole
auth chain before touching OTP by running
`rpi-connect-test --device-identity-exchange --serial <serial>
--device-privkey device-priv-key.pem --device-pubkey device-pub-key.pem`, which
prints the `RPI_CONNECT_TOKEN` the device would obtain.)

#### Option B - On-Device binary

If you don't have a host toolchain or the necessary dependencies, you can compile
a provisioning binary to run on device:

```sh
cd <pico-examples-root>
cmake -S . -B build -DPICO_BOARD=pico2_w
cmake --build build --target rpi_connect_create_identity
```

This requires the `WIFI_SSID`/`WIFI_PASSWORD` CMake variables to be defined, and
requires the private-key to already be in OTP.

You can drag & drop the `rpi_connect_create_identity.uf2` file, or load it using
`picotool`:

```sh
picotool load -x build/rpi-connect/rpi-connect-ota-demo/rpi_connect_create_identity.uf2
```

Once it has connected to WiFi it will prompt for UART or USB input to
provide the organisation token:

```sh
Connecting to WiFi XXXXXXXX
Organisation token: <enter-your-organisation-token-then-press-enter>
```

### Step 4 — Build the application firmware

The example is built as part of pico-examples. Select the board and platform
as usual and build the `rpi_connect_ota_demos` target (which builds all the
targets in this directory):

```sh
cd <pico-examples-root>
cmake -S . -B build -DPICO_BOARD=pico2_w
cmake --build build --target rpi_connect_ota_demos
```

This produces UF2s for each demo. The default build (see `CMakeLists.txt`)
enables INFO-level logging for the app and OTA library so you can watch the flow
over UART/USB serial. Logging verbosity and the debug overrides in
`debug_options.cmake` are all set there.

> **No secrets in the firmware UF2s.** WiFi credentials and auth tokens are only
> written to FFS UF2s, so they never appear in the firmware images.

### Step 5 — Partition for FFS, provision credentials, and flash

The RP2350 must be partitioned with an A/B layout (two main slots for OTA) plus
a small **FFS** data partition for credentials and the cached token. The build
produces a `rpi_connect_ota_demo_combined.uf2` file which flashes this partition
table, and flashes `rpi_connect_ota_demo.uf2` in the A partition, and an FFS
blob in the FFS data partition. If the `WIFI_SSID`/`WIFI_PASSWORD` CMake
variables are defined, that blob will contain them, otherwise it will just have
empty files for the SSID and password.

### Step 6 — Run it and watch the log

Open the serial console (USB or UART) at the SDK default baud and reset the
board. With the default INFO logging you will see, in order:

```
rpi_connect_ota_demo version: 1.0.0 (image v1.<build>)
WiFi SSID: MyNetwork
Connecting to WiFi MyNetwork (client_id=96BE5607-... serial=<pico unique board id>)
Connected
No pending deployment
Starting event listener
```

At this point the device has signed in via the OTP key, cached its token in FFS,
registered the OTA capability, checked for a deployment assigned while it was
offline, and is waiting for deployments.

### Step 7 — Deploy an update

This project builds separate `update` UF2s for each demo, which have the
try-before-you-buy bit set. This means a bad update will only be tried once, and
if it fails to reach `rpi_connect_ota_boot_sync` the device will reboot and use
the old version again.

From the Raspberry Pi Connect dashboard (or API) for your organisation, create a
deployment targeting this device with a new `rpi_connect_ota_demo_update.uf2`.
The image should ideally carry a strictly higher picobin version than the one
running, otherwise the bootrom will delete the old image when the new image is
bought: the minor version defaults to the git commit count of the examples
checkout (see `RPI_CONNECT_BUILD_NUMBER` in `CMakeLists.txt`), so commit or pass
`-DRPI_CONNECT_BUILD_NUMBER=<n>` to bump it. The device
will log the deployment, stream the image into the inactive slot with a live
progress percentage, verify the SHA-256, and reboot into the new firmware:

```
Starting deployment ID=...
Download progress: 65536 / 524288 bytes (12%)
...
Download succeeded for deployment ID=... (524288 bytes sha256=...)
Rebooting to apply update...
```

> **Artefact host verification.** The download is fetched over TLS and the
> artefact host's certificate is verified against the Pi Connect root CA by
> default. If your artefacts are hosted on a server that does not chain to
> that root (e.g. third-party object storage), build with
> `RPI_CONNECT_OTA_DEMO_UNVERIFIED_DOWNLOAD=1` (see `CMakeLists.txt`): the
> download is then encrypted but the host is unauthenticated, and integrity
> rests entirely on the deployment's SHA-256 checksum, which is always
> required and verified. This applies to the download only — API requests
> are always verified against the Pi Connect root CA.

Success is **not** reported to the server before the reboot — only the
`APPLYING` status is persisted to FFS. After the reboot the new image signs in
and registers its OTA capability, and only then is success reported and the
bootrom *buy* issued to commit it (both from `rpi_connect_ota_boot_sync()`). So the deployment is
reported successful only once the new firmware has actually booted and reached
the API. If the new image can't sign in or reach the server (or crashes
earlier), neither the success report nor the buy happens, and the bootrom rolls
back to the previous image on the next reset. The old image then finds the
leftover `APPLYING` status on a normal (non-flash-update) boot and reports the
deployment **failed**, so the server knows to re-deploy. If a download is
interrupted by a reboot mid-way, the resume path picks it up and re-downloads
automatically.

---

## Code map

| File                         | Role                                                                 |
|------------------------------|----------------------------------------------------------------------|
| `main_pico.c`                | Device entry point: network/FFS setup, WiFi from FFS, sign-in, main loop. |
| `rpi_connect_ota_demo.c`     | **The OTA integration itself** — the part to copy into your app.     |
| `debug_options.cmake`        | Build-time overrides for OTP/FFS/token — development only.           |
| `partition_tables/`          | Partition tables used to produce the combined UF2s      |
| `main.c`                     | Host entry point and registration/debug CLI (Step 3, option A).                |
| `main_create_identity_pico.c`| Device entry point for registration only (Step 3, option B). |

## API sequence (see `pico/rpi_connect_ota.h`)

The example drives the `pico_rpi_connect` / `pico_rpi_connect_ota` APIs in a
fixed order. The library owns the deployment state machine; `main_pico.c` does
the bring-up and `rpi_connect_ota_demo.c` shows how an application main loop (which
in a real product also does non-OTA work) integrates it. End to end:

**1. Storage — `ffs_initialise()`**
Bring up FFS first. It backs the cached access token (file id `0x1`), the WiFi
credentials (`0x10`/`0x11`), and the in-progress deployment state (`0x2`–`0x5`)
that lets a download be resumed across a reboot.

**2. Sign in — `rpi_connect_ota_init()`**
Obtain an access token, in priority order: a token in FFS (cached from a
previous exchange, or provisioned with `--connect-token`) → a build-time PEM
identity key (debug) → the **OTP identity key**, via a signed device-identity
exchange. On success the token is cached back to FFS and retrieved with
`rpi_connect_ota_get_auth_token()`.

**3. Boot sync — `rpi_connect_ota_boot_sync()`**
One call, made once after sign-in, owns the boot-time ordering. The deployment
lifecycle is tracked in FFS (`RPI_CONNECT_FFS_DEPLOYMENT_STATUS`): `IDLE`
(empty) → `DOWNLOADING` → `APPLYING` → `IDLE`. Boot sync:

* registers the OTA capability (`rpi_connect_ota_register_ota_capability()`) —
  the first guaranteed authenticated round-trip to the server, which gates
  everything below;
* if the persisted status is `APPLYING` and the bootrom reports a
  **flash-update boot** (`rpi_connect_ota_boot_is_flash_update()`), the device
  has just rebooted into a freshly downloaded image *and* has now signed in and
  registered its capability — i.e. the new firmware demonstrably works. Only now
  is success reported to the server with `rpi_connect_ota_complete_deployment()`
  (retried a few times), which clears the status back to `IDLE`. If the report
  cannot be delivered, the buy is skipped and boot sync returns an error, so the
  bootrom rolls back on the next reset — a deployment is never left
  half-reported;
* if the status is `APPLYING` but this is a **normal boot**, the update is not
  running: the flash-update reboot never happened, or the new image failed and
  the bootrom rolled back to this (old) image. `rpi_connect_ota_fail_deployment()`
  reports failure so the server can re-deploy instead of recording a rolled-back
  update as successful;
* finally `rpi_connect_ota_handle_boot()` issues the bootrom **explicit-buy** to
  commit the new image permanently (the RP2350 runs a freshly flashed image
  *try-before-you-buy* until bought).

Deferring both the success report *and* the buy until the connection is proven
(sign-in *and* an authenticated round-trip) is deliberate: an image that can't
reach the network or authenticate is never reported as successful and never
committed, so the bootrom rolls the device back to the previous image on the
next reset. This is the anti-brick safety net, and it means the server only ever
sees a deployment marked successful once the new firmware has actually run and
reached the API.

**4. Resume check (polled) — `rpi_connect_ota_get_resumable_deployment_id()` + `rpi_connect_ota_resume_deployment()`**
Before waiting for anything new, check FFS for a deployment that was in flight
(a stored deployment id with any status other than `APPLYING`) when the device
last rebooted; if found, re-fetch its URI/checksum and pick the download back
up. A status of `APPLYING` is *not* resumable — that image is already downloaded
and is handled by boot sync in step 3. If nothing is being resumed, a one-shot
`rpi_connect_get_pending_deployment()` call then asks the server for a
deployment assigned while the device was offline (the event stream does not
replay those); if there is one it is started exactly like a pushed deployment.

**5. Subscribe for notifications (optional) — `rpi_connect_ota_event_listen()`**
Open a persistent SSE connection so the deploy callback fires when the server
assigns a new deployment. This push path is optional — a device could rely
solely on the boot-time pending-deployment check in step 4. On data-limited or
metered connections, build with `RPI_CONNECT_OTA_DEMO_POLL_ONLY=1` (see
`CMakeLists.txt`) to skip the persistent connection entirely: new deployments
are then picked up by the pending check on the next boot — applying an update
reboots the device anyway.

**6. Fetch + install — `rpi_connect_ota_start_deployment()` → `rpi_connect_ota_install_update_start()`**
`start_deployment` returns the firmware URI and expected checksum, and sets the
status to `DOWNLOADING`. `install_update_start` streams the download straight
into the inactive A/B flash slot, computing SHA-256 as it goes. The main loop
drives it with `async_context_poll()` followed by
`rpi_connect_ota_install_update_poll()`, reporting progress from
`rpi_connect_ota_install_update_bytes_so_far()`.

**7. Apply + reboot — `rpi_connect_ota_set_deployment_status(APPLYING)` → `rpi_connect_ota_try_booting_to_flash_update()`**
On a verified download the image is already written to flash. The device marks
the deployment `APPLYING` (persisted in FFS) and reboots into the new slot (a
bootrom flash-update reboot) — it does **not** report success here. Success is
reported only after the reboot, in step 3, once the new image proves it works.
On a failed download, `rpi_connect_ota_fail_deployment()` reports the reason and
clears the status to `IDLE` instead. After the reboot the cycle returns to step
2; the new image must sign in and complete boot sync before, in step 3, its
success is reported and it is bought — otherwise the bootrom rolls back.

---

## Debugging

### Serial console

All output goes to the Pico SDK's standard I/O. Open a terminal on the
appropriate interface — UART or USB CDC, depending on how `stdio` is enabled for
your board build (`pico_enable_stdio_uart` / `pico_enable_stdio_usb`) — at the
SDK default of **115200 baud, 8N1**, then reset the board. `INFO` and `ERROR`
output is on by default and is enough to follow the whole flow; the milestones
in Step 6/7 above are what to watch for.

### Log level configuration

The example deliberately separates the **application** log namespace from the
**library** namespaces so the flow of control stays readable. Each level is a
compile-time switch set in the logging block of `CMakeLists.txt`:

| Flag                                   | Enables                                            |
|----------------------------------------|----------------------------------------------------|
| `RPI_CONNECT_ERROR_ENABLE`             | Core `rpi_connect` ERROR (on by default)           |
| `RPI_CONNECT_OTA_ERROR_ENABLE`         | OTA library ERROR (on by default)                  |
| `RPI_CONNECT_INFO_ENABLE`              | Core `rpi_connect` INFO (on by default)            |
| `RPI_CONNECT_OTA_INFO_ENABLE`          | OTA library INFO (on by default)                   |
| `RPI_CONNECT_OTA_DEMO_DEBUG_ENABLE`    | Demo-app DEBUG — per-chunk download progress       |
| `RPI_CONNECT_DEBUG_ENABLE`             | Core `rpi_connect` DEBUG (incl. HTTP request layer) |
| `RPI_CONNECT_VERBOSE_DEBUG_ENABLE`     | ...plus full header/body dumps                      |
| `RPI_CONNECT_OTA_DEBUG_ENABLE`         | OTA library DEBUG                                   |
| `HTTPC_DEBUG=LWIP_DBG_ON`              | lwIP HTTP-client DEBUG                              |
| `RPI_CONNECT_MBEDTLS_DEBUG_LEVEL`      | mbedTLS verbosity, `0` (off) – `4` (most verbose)  |

The demo app's own `ERROR` and `INFO` output is always compiled in; the
libraries log nothing unless the flags above are set, and the default build
enables their ERROR and INFO levels. `ERROR`/`INFO` go to `stderr`/`stdout`
respectively (see `rpi_connect_ota_demo.h`), so they can be filtered separately if
your console distinguishes the streams.

Start with the defaults; reach for `RPI_CONNECT_OTA_DEBUG_ENABLE` and
`RPI_CONNECT_DEBUG_ENABLE` when a deployment misbehaves, and raise
`RPI_CONNECT_MBEDTLS_DEBUG_LEVEL` only when chasing a TLS handshake failure (it
is extremely verbose).

### Overriding the device identity

`debug_options.cmake` lets you bypass the OTP key during bring-up by
hard-coding a PEM key pair at build time. It can be passed as `-D<NAME>=`
on the CMake command line **or** as an environment variable of the same name.
**This is for development only** — a production build provisions via OTP
(Steps 2–3) so nothing secret lands in the UF2.

| Override                                                   | Effect                                                                 |
|------------------------------------------------------------|------------------------------------------------------------------------|
| `RPI_CONNECT_IDENTITY_PRIVKEY_PEM` + `..._PUBKEY_PEM`      | Use a build-time PEM key pair instead of the OTP key for the exchange. |

For example, to drive the identity exchange from the Step 1 key pair without
writing it to OTP:

```sh
cmake -S . -B build -DPICO_BOARD=pico2_w \
    -DRPI_CONNECT_IDENTITY_PRIVKEY_PEM=$PWD/rpi-connect/rpi-connect-ota-demo/device-priv-key.pem \
    -DRPI_CONNECT_IDENTITY_PUBKEY_PEM=$PWD/rpi-connect/rpi-connect-ota-demo/device-pub-key.pem
```

The sign-in order is as documented on `rpi_connect_ota_init()`: token in FFS
(cached or provisioned) → build-time PEM key → OTP key.

### Debugging the auth chain on the host

The host build of this example (see Step 2) is the quickest way
to isolate an authentication problem from the embedded networking stack. It
runs the *same* `pico_rpi_connect` library against host OpenSSL/curl, with
`-v` for request verbosity:

```sh
# Verify a key pair signs correctly
rpi-connect-test --ec-key device-priv-key.pem

# Confirm the registered identity can obtain a token (prints RPI_CONNECT_TOKEN=...)
rpi-connect-test -v --device-identity-exchange --serial <serial> \
    --device-privkey device-priv-key.pem --device-pubkey device-pub-key.pem

# Run the full OTA loop against a token
RPI_CONNECT_TOKEN=<token> rpi-connect-test --ota --serial <serial>
```

If the host path works but the device does not, the problem is in
provisioning (OTP/FFS) or connectivity rather than the API exchange itself.
