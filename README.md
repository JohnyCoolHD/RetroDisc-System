<div align="center">

# 🎮 RetroDisc

### Plug 'N' Play for PC Games on Linux

**RetroDisc** is an open game media format for running portable Windows games on Linux.

It provides a standardized way to package game files, runtime configuration and compatibility settings while keeping the original game release separate from persistent writable data.

</div>

---

# What is RetroDisc?

RetroDisc is designed to bring the simplicity of physical game media to Windows games running on Linux.

A RetroDisc release is intended to behave like a self-contained game release:

* 💿 Store it on a USB drive, SSD, HDD, optical media, SD card or other supported storage.
* ▶️ Launch the included `RetroDisc` launcher.
* 🎮 The launcher prepares the required runtime environment and starts the game.

RetroDisc does **not** replace Wine or Proton.

Instead, it acts as a packaging and runtime-preparation layer around them.

A RetroDisc release can contain:

* The RetroDisc launcher
* Game metadata
* Runtime configuration
* Compatibility settings
* Original game files
* An optional preconfigured Wine/Proton prefix

The original game release is treated as the read-only base whenever possible. Persistent writable data is kept outside the release.

---

# Features

| Feature                     | Description                                                           |
| --------------------------- | --------------------------------------------------------------------- |
| 🎮 Plug & Play              | Launch games directly from portable storage                           |
| 💾 Portable Releases        | Move RetroDisc releases between compatible storage devices            |
| 🔒 Game Preservation        | Keep the original game files separate from writable runtime data      |
| 🐧 Linux Focus              | Designed around Wine and Proton                                       |
| 📂 Standard Layout          | RetroDisc releases follow a common structure                          |
| 💾 Persistent Data          | Saves, configuration and runtime changes survive between launches     |
| 🔧 Writable Separation      | Runtime modifications are separated from the original release         |
| 🧪 Temporary Overlay        | `fuse-overlayfs` provides a writable runtime filesystem               |
| 📦 Bundled Prefix           | Releases can provide a preconfigured Wine/Proton environment          |
| ⚙️ Persistent Configuration | Game-specific runtime settings are stored separately from the release |

---

# Design Goals

RetroDisc is built around three main principles.

## 🎮 Portable Games

A RetroDisc release is designed to run directly from the storage device containing the release.

The game does not need to be installed into a traditional system-wide location.

The launcher prepares the required Wine or Proton environment before starting the game.

---

## 🔒 Game Preservation

The original `gamedata/` directory is treated as the read-only base of the game runtime.

When possible, changes made by the game are redirected to a separate writable layer instead of modifying the original release.

This allows the same RetroDisc release to remain portable and suitable for archival.

---

## 💾 Persistent User Data

Runtime changes that need to survive between launches are stored outside the original RetroDisc release.

This includes things such as:

* Wine/Proton prefixes
* Registry changes
* Windows user data
* Save files
* Game configuration
* Runtime configuration
* Persistent game modifications

The original release can therefore remain unchanged while the user's data persists independently.

---

# RetroDisc Release Format

A typical RetroDisc release follows this structure:

```text
RetroDisc Game/
├── RetroDisc
├── manifest.json
├── config.json       # optional
├── gamedata/
└── pfx/              # optional
```

## `RetroDisc`

The RetroDisc launcher.

It:

1. Loads the game manifest.
2. Determines the persistent data location.
3. Loads or creates the persistent configuration.
4. Selects the configured runtime.
5. Creates the required writable overlay.
6. Prepares the Wine/Proton environment.
7. Launches the game.
8. Cleans up temporary runtime data afterwards.

The launcher itself is implemented in C++ and is built using CMake. The repository contains the launcher source code together with the required headers and runtime components.

---

## `manifest.json`

The manifest contains the basic metadata required to identify and launch a game.

Example:

```json
{
    "game": {
        "id": "example-game",
        "name": "Example Game",
        "executable": "gamedata/Game.exe"
    },
    "runtime": "wine"
}
```

The manifest defines:

* **Game ID**
* **Game name**
* **Executable path**
* **Default runtime**

The executable path must point to a file inside the RetroDisc release, normally inside `gamedata/`.

Supported runtimes are:

```text
wine
proton
```

---

## `config.json`

A release may optionally provide a `config.json`.

This configuration contains game-specific runtime and compatibility settings.

The launcher also contains a default configuration which can be used when no external release configuration is provided.

The release configuration is used as the basis for the user's persistent configuration.

The persistent configuration is kept separately from the RetroDisc release so that user changes survive between launches.

Possible settings include:

* Runtime selection
* Launch arguments
* Environment variables
* Windows version
* Graphics settings
* Synchronization settings
* DLL overrides
* Proton configuration
* Display settings
* Other compatibility options

---

## `gamedata/`

`gamedata/` contains the original Windows game files.

For example:

```text
gamedata/
├── Game.exe
├── data/
├── textures/
└── ...
```

The executable specified by `manifest.json` must be located inside this directory.

RetroDisc uses `gamedata/` as the read-only base of the temporary runtime filesystem.

---

## `pfx/`

A RetroDisc release may optionally contain a preconfigured Wine/Proton prefix.

If present, it can be used as the initial prefix for the game.

The bundled prefix is treated as part of the release and is **not modified directly**.

Once copied into persistent storage, the persistent prefix is used for subsequent launches.

---

# Persistent Game Data

RetroDisc stores writable game data separately from the original release.

The default persistent location is:

```text
~/Games/RetroDisc/<gameId>/
```

A typical persistent directory contains:

```text
~/Games/RetroDisc/
└── <gameId>/
    ├── config.json
    ├── gamedata/
    └── prefix/
```

Persistent data can contain:

* Game-specific configuration
* Wine/Proton prefix data
* Registry changes
* Windows user profile data
* Save files
* Game configuration files
* Persistent game modifications

The persistent directory is reused between launches.

RetroDisc does not recreate persistent data every time the game starts.

---

# Writable Game Data

RetroDisc uses `fuse-overlayfs` to separate the original game files from writable runtime data.

Conceptually:

```text
RetroDisc Release
       │
       │ read-only
       ▼
   gamedata/
       │
       │ fuse-overlayfs
       ▼
 Temporary Runtime
       │
       ├── game changes
       ├── temporary files
       └── runtime modifications
       │
       ▼
 Persistent Data
 ~/Games/RetroDisc/<gameId>/
       │
       └── gamedata/
```

The original:

```text
gamedata/
```

remains the lower/read-only layer.

Writable changes are stored in the persistent game-data directory.

Temporary overlay resources are created under `/tmp` and are cleaned up after the game exits whenever possible.

This allows a RetroDisc release to be used from read-only or otherwise inconvenient storage while still providing a writable runtime.

---

# Wine / Proton

RetroDisc supports two runtime types.

## Wine

A Wine-based release uses:

```json
{
    "runtime": "wine"
}
```

RetroDisc launches the configured Windows executable using Wine and the persistent game prefix.

---

## Proton

A Proton-based release uses:

```json
{
    "runtime": "proton"
}
```

RetroDisc uses the user's Steam/Proton installation and the persistent game prefix.

A specific Proton installation can be selected through the game's configuration.

RetroDisc therefore does not bundle or replace Wine/Proton itself. It prepares and manages the environment in which the selected runtime executes the game.

---

# Runtime User Profile

RetroDisc keeps the Windows-side user profile independent from the actual Linux username.

The persistent Windows user is:

```text
RetroDisc
```

During execution, RetroDisc creates the required runtime user mapping inside the Wine prefix.

Conceptually:

```text
drive_c/users/<runtime-user>
        └──> RetroDisc
```

The mapping is temporary and is removed during cleanup.

This keeps the persistent Windows profile consistent across different Linux systems and usernames.

---

# Launch Process

When a RetroDisc release is started, the launcher performs the following general process:

1. Determines the RetroDisc release directory.
2. Loads `manifest.json`.
3. Determines the persistent game-data directory.
4. Loads the persistent configuration.
5. Creates the persistent configuration when required.
6. Selects Wine or Proton.
7. Creates or reuses the persistent game environment.
8. Creates a temporary `fuse-overlayfs` filesystem for `gamedata/`.
9. Verifies that the configured executable is available.
10. Prepares the runtime environment.
11. Applies environment variables and compatibility settings.
12. Launches the game.
13. Removes temporary runtime mappings.
14. Unmounts the temporary filesystem.
15. Cleans up temporary files.

The exact runtime behavior depends on the selected game configuration.

---

# Configuration

A persistent game configuration can contain runtime-specific settings.

Example:

```json
{
    "runtime": "wine",

    "launch": {
        "arguments": [
            "-fullscreen"
        ]
    },

    "environment": {
        "DXVK_HUD": "0"
    },

    "wine": {
        "windowsVersion": "win10",

        "graphics": {
            "renderer": "vulkan",
            "videoMemory": 4096,
            "strictDrawOrdering": false
        },

        "sync": {
            "esync": true,
            "fsync": true,
            "ntsync": false
        }
    }
}
```

The configuration is game-specific.

Not every setting is required for every game.

A configuration can be adjusted without modifying the original game files.

---

# Storage Media

RetroDisc is designed to be independent of the underlying storage medium.

### ⭐ Recommended

* Internal SSDs
* External SSDs
* USB flash drives
* HDDs
* SD / microSD cards

### ✅ Suitable

* NAS storage
* DVD-ROM
* Blu-ray
* ISO images

### ⚠️ Performance Dependent

* CD-ROM
* Slow USB storage
* Network storage with high latency

Loading performance depends on the storage device and filesystem.

Game size alone does not determine loading performance. Games that frequently access many small files may behave very differently from games that primarily load a few large files.

---

# Requirements

RetroDisc requires a small number of system components at runtime.

### Required

* Wine or Proton, depending on the game
* `fuse-overlayfs`
* `fuse3` / `fusermount3`
* `findmnt` from `util-linux`

On Arch Linux:

```bash
sudo pacman -S --needed fuse-overlayfs fuse3 util-linux
```

For Wine:

```bash
sudo pacman -S --needed wine
```

For Proton:

```bash
sudo pacman -S --needed steam
```

Additional requirements depend on the individual game.

Possible game-specific dependencies include:

* Vulkan drivers
* DXVK
* Gamescope
* Proton
* Discord

These are not required by every RetroDisc release.

---

# Building RetroDisc

The RetroDisc project is built using **CMake**.

The repository contains the C++ source code, headers and runtime components required to build the launcher.

A typical build workflow is:

```bash
cmake -S . -B build
cmake --build build
```

The exact build configuration may depend on the selected CMake options and the current project version.

The source code is intentionally kept available so that RetroDisc can be inspected, modified and built independently.

---

# Older Releases

Older RetroDisc versions are no longer intended to be distributed as ready-to-use packages.

For older versions, only the **source code** is retained for reference and educational purposes.

This allows users to:

* Study how previous versions were implemented
* Understand the evolution of RetroDisc
* Compare different implementations
* Learn from the source code
* Build an older version themselves if required

Older versions must therefore be **compiled from source by the user**.

No precompiled binaries or ready-to-use RetroDisc packages are provided for these versions.

See the [RetroDisc-System releases](https://github.com/JohnyCoolHD/RetroDisc-System/releases/) for the available historical source versions.

---

# Preservation and Safety

RetroDisc is designed to keep the original game release separate from writable runtime data.

The launcher therefore aims to:

* Keep the original `gamedata/` separate from writable data
* Use a temporary writable overlay during execution
* Store persistent runtime data outside the release
* Reuse existing persistent prefixes
* Avoid overwriting an existing persistent prefix automatically
* Keep bundled release prefixes unchanged
* Preserve the original release for reuse and archival

RetroDisc does not guarantee that every game can run without modifying its original files. Individual games may require specific compatibility workarounds.

---

# Project Structure

The RetroDisc source repository is organized roughly as follows:

```text
RetroDisc-System/
├── include/
├── runtime/
├── src/
├── CMakeLists.txt
├── LICENSE
└── README.md
```

The project uses C++ for the launcher and CMake for the build system.

---

# Older Releases

Older RetroDisc versions are no longer intended to be distributed as ready-to-use packages.

For older versions, only the **source code** is retained and made available for reference and educational purposes.

This allows users to:

* Study how previous versions were implemented
* Understand the evolution of RetroDisc
* Compare different implementations
* Learn from the source code
* Build an older version themselves if required

Older versions must therefore be **compiled from source by the user**.

No precompiled binaries or ready-to-use RetroDisc packages are provided for these versions.

See the [RetroDisc-System releases](https://github.com/JohnyCoolHD/RetroDisc-System/releases/) for the available historical source versions.

---


# License

RetroDisc is licensed under the **GNU General Public License v3.0**.

See [`LICENSE`](./LICENSE) for the complete license text.

RetroDisc can be used, studied, modified and redistributed under the terms of the GPL-3.0 license.


