<div align="center">

# 🎮 RetroDisc

### Plug 'N' Play for PC Games on Linux

RetroDisc is an open game media standard for portable Windows games on Linux.

</div>

---

# What is RetroDisc?

RetroDisc brings back the simplicity of classic game consoles.

A RetroDisc game behaves like a physical game release:

* 💿 Connect a disc, USB drive, SSD, HDD, microSD card or other storage device.
* ▶️ Launch the included RetroDisc launcher.
* 🎮 Play the game.

A RetroDisc release contains the game files together with the information required to launch the game on a compatible Linux system:

* Game launcher
* Game manifest
* Runtime configuration
* Compatibility configuration
* Original game files
* Optional bundled Wine prefix

RetroDisc does **not** replace Wine or Proton.

Instead, RetroDisc provides a standardized way to package and launch Windows games while keeping the original game release separate from writable user data.

---

# 🚀 Features

| Feature                | Description                                                                  |
| ---------------------- | ---------------------------------------------------------------------------- |
| 🎮 Plug & Play         | Launch games directly from portable storage                                  |
| 💾 Portable Releases   | Games can be moved between drives and systems                                |
| 🔒 Game Preservation   | Original game files remain untouched                                         |
| 🐧 Linux Focus         | Built around Wine and Proton                                                 |
| 📂 Standard Layout     | Every RetroDisc title follows the same basic structure                       |
| 💾 Persistent Saves    | Writable game data is stored outside the original release                    |
| 🔧 Writable Separation | Game modifications and runtime changes are separated from the original files |
| 🧪 Temporary Runtime   | A temporary writable game filesystem is created during execution             |
| 📦 Bundled Prefix      | A release can include a preconfigured Wine prefix                            |

---

# Goals of RetroDisc

RetroDisc is built around four simple ideas.

## 🎮 Portable Gaming

A RetroDisc game is designed to be played directly from portable storage.

Whether the game is stored on a USB drive, external SSD, HDD, optical media or another supported filesystem, the launcher prepares the required runtime environment before starting the game.

The original game release does not need to be modified in order to play the game.

---

## 🔒 Game Preservation

RetroDisc separates the original game release from writable data whenever possible.

The original game files are used as the read-only base of a temporary filesystem.

Changes made while the game is running are written to the writable game-data layer instead of directly modifying the original release.

This allows the original release to remain preserved while the user can still use saves, configuration files and other writable data.

---

## 📦 Portable Releases

A RetroDisc title contains the files required to identify and launch the game.

The release can be copied or moved between compatible storage devices without requiring the original game files to be installed into a traditional system-wide location.

Persistent writable data is stored locally on the user's Linux system.

This means that the original game media can remain portable and unchanged while user data follows the local machine.

---

# 🌐 A Common Standard

RetroDisc defines a consistent structure for games and their launch configuration.

Each RetroDisc title contains a `manifest.json` file describing the game itself.

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

Game-specific runtime settings are stored separately in:

```text
~/.config/RetroDisc/games/<gameId>/config.json
```

This configuration can define:

* Wine or Proton
* launch arguments
* environment variables
* Proton version
* Proton path
* Windows version
* graphics settings
* synchronization settings
* display settings
* virtual desktop settings
* DLL overrides

Because the game metadata and runtime configuration follow a consistent structure, applications can discover and launch RetroDisc titles without requiring game-specific launcher implementations.

---

# 💾 Supported Storage Media

RetroDisc is designed to be storage independent.

## ⭐ Recommended

* Internal SSDs
* External SSDs
* USB flash drives
* Internal HDDs
* External HDDs
* microSD cards

## ✅ Suitable

* NAS storage
* DVD-ROM
* Blu-ray
* ISO images

## ⚠️ Possible, but not recommended

* CD-ROM

Loading times depend primarily on the performance of the underlying storage device and filesystem.

The impact varies between games. Some games frequently access many small files, while others primarily load a smaller number of large files. Therefore, total game size alone does not determine loading performance.

---

# 📁 Directory Layout

A RetroDisc release follows this basic structure:

```text
RetroDisc Game/
├── RetroDisc
├── manifest.json
├── gamedata/
└── pfx/
```

### `RetroDisc`

The RetroDisc launcher.

It reads the manifest and configuration, prepares the filesystem and compatibility environment, launches the game and performs cleanup afterwards.

### `manifest.json`

Contains the basic game metadata:

```text
Game ID
Game name
Executable
Default runtime
```

### `gamedata/`

Contains the original game files.

RetroDisc treats this directory as the original game release and does not directly write changes into it during normal execution.

### `pfx/`

An optional bundled Wine prefix.

If a bundled prefix is present and no persistent prefix exists for the game, RetroDisc copies it to the user's local RetroDisc data directory.

The bundled prefix is never modified directly.

---

# 🗂️ Persistent Game Data

Writable data is stored outside the original RetroDisc release.

The persistent directory is:

```text
~/Games/RetroDisc/<gameId>/
```

A typical installation looks like:

```text
~/Games/RetroDisc/
└── <gameId>/
    ├── gamedata/
    └── pfx/
```

The Wine/Proton prefix is stored at:

```text
~/Games/RetroDisc/<gameId>/pfx/
```

The prefix is persistent and is **not recreated on every launch**.

If a complete persistent prefix already exists, RetroDisc reuses it.

If no persistent prefix exists, RetroDisc can copy a bundled `pfx/` from the RetroDisc release.

---

# 🔧 Writable Game Files

RetroDisc separates the original game files from writable runtime data.

Conceptually:

```text
RetroDisc Release
        │
        │ read-only base
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
 Persistent Game Data
 ~/Games/RetroDisc/<gameId>/
```

The game is launched through a temporary `fuse-overlayfs` filesystem.

The original `gamedata/` directory remains untouched.

The writable upper layer is stored in:

```text
~/Games/RetroDisc/<gameId>/gamedata/
```

Temporary overlay directories are created under `/tmp` and removed after the game exits.

---

# 👤 Wine User Profile

RetroDisc keeps the persistent Wine profile independent from the Linux user's actual username.

The persistent Wine user is:

```text
RetroDisc
```

At runtime, RetroDisc determines the current Linux/Steam user and creates a temporary symbolic link inside the Wine prefix:

```text
drive_c/users/<runtime-user>
        └──> RetroDisc
```

This allows Windows applications to see their expected runtime username while the actual persistent data remains inside the `RetroDisc` profile.

The temporary user link is removed after the game exits.

---

# ⚙️ Requirements

RetroDisc itself is designed to require only a small number of runtime dependencies.

### Required

* Wine **or** Proton, depending on the game
* `fuse-overlayfs`
* `fuse3` / `fusermount3`
* `findmnt` from `util-linux`

On Arch Linux, the required system packages are typically:

```bash
sudo pacman -S --needed fuse-overlayfs fuse3 util-linux
```

For Wine games:

```bash
sudo pacman -S --needed wine
```

For Proton games:

```bash
sudo pacman -S --needed steam
```

The RetroDisc launcher itself does **not** require CMake, GCC or the `nlohmann-json` development package at runtime.

### Optional Components

Depending on the individual game:

* Proton
* Vulkan drivers
* DXVK
* Gamescope
* Discord

Not every RetroDisc title requires every optional component.

Discord integration is optional. If Discord is unavailable, RetroDisc continues launching the game normally.

---

# 🔄 How RetroDisc Works

When a RetroDisc game is launched, RetroDisc performs the following steps:

1. Determines the RetroDisc release directory.
2. Loads `manifest.json`.
3. Loads the user's game configuration.
4. Determines the selected Wine or Proton runtime.
5. Creates or reuses the persistent game directory.
6. Creates a temporary `fuse-overlayfs` filesystem for `gamedata/`.
7. Verifies that the configured executable is available through the overlay.
8. Creates the temporary runtime Wine user link.
9. Applies environment variables and compatibility settings.
10. Launches the game using Wine or Proton.
11. Removes the temporary runtime user link.
12. Unmounts the temporary filesystem.
13. Removes temporary files.

The original game release remains unchanged throughout the process.

---

# 🛡️ Persistent vs. Temporary Data

RetroDisc intentionally separates persistent data from temporary runtime data.

### Persistent

Stored under:

```text
~/Games/RetroDisc/<gameId>/
```

Includes:

* Wine/Proton prefix
* Registry changes
* Windows user profile
* Save data
* Configuration data
* Persistent game modifications

### Temporary

Stored under `/tmp` during execution:

```text
/tmp/RetroDisc_<pid>/
/tmp/RetroDiscWork_<pid>/
```

Includes:

* Temporary overlay mount
* Overlay work directory
* Temporary runtime user link

Temporary data is removed after the game exits whenever cleanup succeeds.

---

# 🧩 Wine and Proton

RetroDisc supports both Wine and Proton.

## Wine

For:

```json
{
    "runtime": "wine"
}
```

RetroDisc launches the configured executable using Wine and the persistent game prefix.

## Proton

For:

```json
{
    "runtime": "proton"
}
```

RetroDisc locates the configured Proton version through the user's Steam installation and uses the same persistent game prefix.

A specific Proton installation can also be selected through the configuration.

---

# 🎮 Example Configuration

A minimal game configuration can look like:

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

The configuration is game-specific and can be extended without modifying the original game files.

---

# 🔒 Safety and Preservation

RetroDisc is designed around the principle that the original game release should remain untouched.

The launcher therefore:

* does not use the original game directory as a writable Wine prefix
* does not directly modify the original `gamedata/`
* creates a temporary writable overlay
* stores the persistent Wine/Proton prefix separately
* reuses existing persistent prefixes instead of overwriting them
* refuses to overwrite an incomplete persistent prefix automatically

This makes the original RetroDisc release suitable for archival and redistribution.

---

# 📜 License

RetroDisc is licensed under the GNU General Public License v3.0 (GPL-3.0).

Everyone is free to use, study, modify and redistribute RetroDisc under the terms of the GPL.
