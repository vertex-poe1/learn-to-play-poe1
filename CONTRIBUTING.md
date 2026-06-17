# Contributing to Learn to Play Path of Exile

Welcome! This document outlines the process for building and running this project locally on a Windows machine.

## Architecture

This project uses Rust to build a transparent, click-through overlay that floats above the Path of Exile game window.
Because of specific bugs related to how Windows handles transparent layers with mouse passthrough (`WS_EX_LAYERED | WS_EX_TRANSPARENT`), we use the `egui_overlay` crate. 
`egui_overlay` relies on a customized `glfw` implementation to correctly manage the desktop compositor and input hooks.

## Windows Build Requirements

Because our windowing layer requires building C/C++ code under the hood (specifically the `glfw-sys-passthrough` crate), a standard Rust installation is not enough. You must have `CMake` installed and available in your system's PATH.

### 1. Install Rust
If you haven't already, install Rust using `rustup`:
- Download and run `rustup-init.exe` from [rustup.rs](https://rustup.rs/).
- **Important:** During installation, ensure you install the **C++ Build Tools for Visual Studio** when prompted. CMake requires this MSVC toolchain to compile the GLFW C code.

### 2. Install CMake
You need CMake to build the `glfw-sys-passthrough` dependency. You can install it via the Windows Package Manager (`winget`):

```powershell
winget install cmake
```
*(Alternatively, you can download the installer directly from [cmake.org](https://cmake.org/download/).)*

**Note:** After installing CMake, you may need to restart your terminal or IDE so that the `cmake` command is available in your PATH.

### 3. Build the Project
Once the dependencies are installed, you can build and run the project normally:

```powershell
# Build the project
cargo build

# Run the overlay
cargo run
```

## Updating Dependencies

To avoid breaking changes or recently introduced bugs, we enforce a 7-day cooldown on new dependency versions. If you need to update dependencies, **do not use `cargo update`**. 

Instead, ensure you have the `cargo-cooldown` tool installed and run our `Justfile` recipe:

```powershell
cargo install cargo-cooldown
just update
```

This will run `cargo cooldown update` under the hood to fetch only packages published more than 7 days ago.

## Troubleshooting

- **"is `cmake` not installed?"**: If `cargo build` fails complaining about CMake, verify it is installed by running `cmake --version`. If it is installed but not found, ensure `C:\Program Files\CMake\bin` is added to your System PATH variables.
- **Overlay Disappears / Black Screen**: Ensure Path of Exile is set to **Windowed Fullscreen** or **Borderless Windowed**. Transparent overlays generally do not work over games running in Exclusive Fullscreen mode.
