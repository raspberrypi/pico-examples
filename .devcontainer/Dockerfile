FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    git \
    cmake \
    ninja-build \
    build-essential \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    python3 \
    && rm -rf /var/lib/apt/lists/*

