# Use Ubuntu as base image
FROM ubuntu:22.04

# Avoid timezone prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies 
RUN apt-get update && apt-get install -y \
    git \
    cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    build-essential \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Create workspace directory
WORKDIR /workspace

# Clone pico-sdk and update submodules
RUN git clone https://github.com/raspberrypi/pico-sdk.git
WORKDIR /workspace/pico-sdk
RUN git submodule update --init

# Set environment variable for SDK path
ENV PICO_SDK_PATH=/workspace/pico-sdk

# Copy pico-examples source
COPY . /workspace/pico-examples/

# Build examples
WORKDIR /workspace/pico-examples
RUN mkdir build
WORKDIR /workspace/pico-examples/build
RUN cmake .. -DPICO_BOARD=pico_w
RUN make -j$(nproc)

# Set default command to build
CMD ["make", "-j4"]