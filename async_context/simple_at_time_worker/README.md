# Simple at-time worker

A simple example demonstrating how to add an *at-time* worker to an `async_context`.

The program creates an `async_context_threadsafe_background` and uses an *at-time* worker to flash the LED on a Pico or Pico-W board.

## note
An `async_context_threadsafe_background` is generally used to ensure that a non-reentrant library such as lwIP can be used successfully in a multi tasking application; so a practical networking application would typically use the `async_context` provided by `cyw43_arch` rather than creating its own.

More details about `async_context` can be found under [High Level APIs](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_async_context) in the SDK documentation.