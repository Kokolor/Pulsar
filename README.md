# Pulsar

## Introduction

Pulsar is an EFI bootloader designed to work with the Orion kernel. This project aims to provide a flexible and efficient way to boot your custom kernel.

## Dependencies

- GCC or Clang
- GNU Make
- QEMU (for testing)

## Building Pulsar

To build Pulsar, you'll need to link it with [POSIX-EFI](https://gitlab.com/bztsrc/posix-uefi). Follow the steps below to set up the build environment:

### Linking with POSIX-EFI

Before building Pulsar, make sure to create a symbolic link to the POSIX-EFI library. Navigate to the root directory of Pulsar and run:

```bash
ln -s /path/to/posix-efi/uefi
