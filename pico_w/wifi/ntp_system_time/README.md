# Overview

Creates a time of day clock that periodically synchronises itself to Internet time servers using simple NTP (see [RFC 4330](https://datatracker.ietf.org/doc/html/rfc4330)).

The example connects to Wi-Fi and displays the local time in the UK or another timezone that you specify, synchronised once an hour to one of the servers from [pool.ntp.org](https://www.ntppool.org/en/). 

Uses the SNTP application provided by lwIP and the Pico 'always-on timer' _(RTC on Pico/RP2040, powman timer on Pico-2/RP2350)_.

# Running the example

Provide the SSID and password of your Wi-Fi network by editing `CMakeLists.txt` or on the command line; then build and run the example as usual.

You should see something like this:

```
Connecting to Wi-Fi...
connect status: joining
connect status: link up
Connected
IP address 192.168.0.100
system time not yet initialised
-> initialised system time from NTP
GMT: Sun Oct 26 10:41:07 2025
GMT: Sun Oct 26 10:41:12 2025
...
```

### To use it in your own code
Configure the lwIP callbacks and connect to the network as shown in the example. You can then initialise the background NTP synchronisation like this:

```
sntp_setoperatingmode(SNTP_OPMODE_POLL);
sntp_init();
```

Your code can now call 

```
void get_time_utc(struct timespec *)
```

whenever it wants the current UTC time. 
 
 You can also use the [pico_aon_timer API](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_aon_timer) to read the time directly or to set alarms. _Note however that with a direct read it is theoretically possible (although very unlikely) to get an erroneous result if NTP was in the process of updating the timer in the background._

 To reduce the logging level change the `SNTP_DEBUG` option in **lwipopts.h** to `LWIP_DBG_OFF` and/or remove the informational messages from `sntp_set_system_time_us()` in **ntp_system_time.c**.


# Further details

The example uses:

1. the [SNTP application](https://www.nongnu.org/lwip/2_0_x/group__sntp.html) provided by the lwIP network stack
2. the Pico SDK high level "always on timer" abstraction: [pico_aon_timer](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_aon_timer)
3. an optional [POSIX timezone](https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_node/libc_431.html) to convert UTC to local time

### lwIP SNTP

The lwIP SNTP app provides a straightforward way to obtain and process timestamps from a pool of NTP servers without the complexity of a full NTP implementation. The lwIP documentation covers the [configuration options](https://www.nongnu.org/lwip/2_0_x/group__sntp.html) but the comments in the [source code](https://github.com/lwip-tcpip/lwip/blob/master/src/apps/sntp/sntp.c) are also very helpful.

> Note that on RP2040 platforms you should **not** enable `SNTP_COMP_ROUNDTRIP` if you are keeping system time with the `aon_timer`, because on that platform it has a resolution of only one second. The example looks after this with a test in `CMakeLists.txt`.

SNTP uses the **macros** `SNTP_GET_SYSTEM_TIME(sec, us)` and `SNTP_SET_SYSTEM_TIME_US(sec, us)` to call user-provided functions for accessing the system clock. The example defines the macros in `lwipopts.h` and the callbacks themselves are near the top of `ntp_system_time.c`.

Note that the example runs lwIP/SNTP from `pico_cyw43_arch` in _threadsafe background mode_ as described in [SDK Networking](https://www.raspberrypi.com/documentation/pico-sdk/networking.html#group_pico_cyw43_arch).
If you reconfigure it to use _polling mode_ then your user code should periodically call `cyw43_arch_poll()`.

### Always on timer

The SDK provides the high level [pico_aon_timer](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_aon_timer) API to provide the same always-on timer functions on Pico and Pico-2 despite their hardware differences.

On the original Pico (RP2040) these functions use the real time clock (RTC) and on the Pico-2 (RP2350) the POWMAN timer.

For further details refer to the [SDK documentation](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_aon_timer).

### POSIX timezone

NTP timestamps always refer to universal coordinated time (UTC) in seconds past the epoch. In contrast users and user applications often require **local time**, which varies from region to region and at different times of the year (daylight-saving time or DST).

Converting from UTC to local time often requires inconvenient rules, but fortunately the Pico SDK time-conversion functions like `ctime()` and `pico_localtime_r()` can do it automatically if you define a **POSIX timezone (TZ)**.

The example shows a suitable definition for the Europe/London timezone:
```
setenv("TZ", "BST0GMT,M3.5.0/1,M10.5.0/2", 1);
```

which means 
```
Normal time ("GMT") is UTC +0. Daylight-saving time ("BST") runs from 1am on the last Sunday in March to 2am on the last Sunday in October.
```

The format to define your own POSIX timezone is pretty straightforward and can be found [here](https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_node/libc_431.html); or you can simply choose a pre-defined one from an online resource such as [this](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv).

_Note that it is entirely optional to create a local timezone: without one the Pico SDK time-conversion functions will simply use UTC._