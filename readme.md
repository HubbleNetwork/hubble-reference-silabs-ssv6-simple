# Hubble Network SDK — Silicon Labs Reference Application

This is a reference application demonstrating how to integrate the [Hubble Network SDK](https://github.com/HubbleNetwork/hubble-device-sdk) on a Silicon Labs EFR32BG22 development kit. It shows the minimal setup needed to broadcast Hubble BLE advertisements from a bare-metal (no RTOS) application built with Simplicity Studio v6.

## What This Application Does

The application initializes the Hubble SDK, configures BLE advertising with a non-resolvable random address, and broadcasts Hubble-encrypted advertisement packets. A periodic timer rotates the advertisement every hour to keep the ephemeral ID and encryption key current.

Key integration points:

- **`app.c`** — Hubble SDK initialization (`hubble_init`), advertisement construction (`hubble_ble_advertise_get`), BLE stack event handling, and periodic ad rotation
- **`hubble_port.c`** — Platform abstraction layer implementing `hubble_uptime_get` and `hubble_log` for the Silicon Labs platform
- **`cmake_gcc/CMakeLists.txt`** — Build configuration linking the Hubble SDK sources and setting required compile definitions

## Hardware

- **SoC:** EFR32BG22C224F512IM40
- **Board:** BRD4108A (Thunderboard BG22)
- **SDK:** Simplicity SDK 2025.6.2

## Prerequisites

- [Simplicity Studio v6](https://www.silabs.com/developers/simplicity-studio) with the Bluetooth SDK installed
- ARM GCC toolchain (installed automatically by Simplicity Studio)
- Simplicity Commander (installed automatically by Simplicity Studio)

## Getting Started

Clone the repository with the Hubble SDK submodule:

```bash
git clone --recurse-submodules https://github.com/HubbleNetwork/hubble-reference-silabs-ssv6-simple.git
cd hubble-reference-silabs-ssv6-simple
```

If you've already cloned without submodules:

```bash
git submodule update --init
```

## Building

### VS Code (Silicon Labs Extension)

Open the project in VS Code with the [Silicon Labs extension](https://marketplace.visualstudio.com/items?itemName=SiliconLabs.silabs-vscode) installed. The extension auto-detects the `.slcp` project file and provides build/flash commands through its UI.

### CLI

From the repository root:

```bash
cd cmake_gcc
cmake --preset project
cmake --build build --config base
```

Build artifacts are output to `cmake_gcc/build/base/`:

| File | Format |
|------|--------|
| `hubble-sample.out` | ELF executable |
| `hubble-sample.s37` | Simplicity Commander (S37) |
| `hubble-sample.hex` | Intel HEX |
| `hubble-sample.bin` | Raw binary |

## Flashing

Connect the BRD4108A dev kit via USB. The on-board J-Link debugger will be detected automatically.

### VS Code (Silicon Labs Extension)

Use the Silicon Labs extension's flash command from the command palette or toolbar.

### CLI (Simplicity Commander)

Flashing from the command line uses `commander-cli`, which is included with [Simplicity Commander](https://www.silabs.com/developers/mcu-programming-options). Locate the `commander-cli` binary in your Simplicity Commander installation and add it to your PATH.

Flash the built firmware:

```bash
commander-cli flash cmake_gcc/build/base/hubble-sample.s37
```

To mass-erase the device before flashing:

```bash
commander-cli flash cmake_gcc/build/base/hubble-sample.s37 --masserase
```

To reset the device after flashing (if not done automatically):

```bash
commander-cli device reset
```

Other useful commands:

```bash
commander-cli device info          # Show connected device info
commander-cli device masserase     # Erase the entire main flash
```

## Project Structure

```
.
├── app.c                  # Hubble SDK integration and BLE event handling
├── app.h                  # Application interface
├── app_bm.c               # Bare-metal compatibility layer (semaphore, mutex stubs)
├── hubble_port.c           # Hubble SDK platform abstraction (uptime, logging)
├── main.c                  # Entry point and super-loop
├── hubble-sdk/             # Hubble Device SDK (git submodule)
├── cmake_gcc/
│   ├── CMakeLists.txt      # Build configuration
│   ├── toolchain.cmake     # ARM GCC cross-compilation toolchain
│   └── build/              # Build output (generated)
├── config/                 # Silicon Labs component configuration headers
├── autogen/                # SLC-generated code (do not edit manually)
├── hubble-sample.slcp      # Simplicity Studio project file
└── readme.md
```
