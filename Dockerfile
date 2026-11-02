FROM --platform=linux/amd64 ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-multilib \
    nasm \
    binutils \
    grub-common \
    grub-pc-bin \
    xorriso \
    qemu-system-x86 \
    make \
    file \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /kfs