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

A RetroDisc release contains the information and files required to launch a Windows game on a compatible Linux system:

* Game launcher
* Game manifest
* Runtime configuration
* Compatibility configuration
* Original game files
* Optional bundled Wine prefix

RetroDisc does **not** replace Wine or Proton.

Instead, it provides a standardized way to package and launch Windows games while keeping the original release separate from writable user data.

---

# Features

| Feature                    | Description                                                   |
| -------------------------- | ------------------------------------------------------------- |
| 🎮 Plug & Play             | Launch games directly from portable storage                   |
| 💾 Portable Releases       | Move games between compatible drives and systems              |
| 🔒 Game Preservation       | Keep original game files unchanged whenever possible          |
| 📂 Standard Layout         | Every RetroDisc title follows the same basic structure        |
| 💾 Persistent Saves        | Store writable game data separately from the original release |
| 🔧 Writable Separation     | Separate runtime changes from the original game files         |
| 🧪 Temporary Runtime       | Use a temporary writable filesystem while the game is running |
| 📦 Bundled Prefix          | Optionally include a preconfigured Wine prefix                |
| ⚙️ Automatic Configuration | Create persistent `config.json` automatically when required   |

---

# Goals

RetroDisc is built around four principles.

## 🎮 Portable Games

A RetroDisc game is designed to run directly from portable storage.

Games can be stored on USB drives, external SSDs, HDDs, optical media, SD cards or other supported storage devices. The launcher prepares the required runtime environment before starting the game.

The original game release normally does not need to be modified.

If compatibility modifications are required, they are applied to the persistent writable game-data layer rather than the original release whenever possible.

## 🔒 Game Preservation

The original game files are treated as the read-only base of the runtime filesystem.

While the game is running, changes are written to a separate writable layer. This allows games to run from read-only media while still supporting saves, configuration files and other writable data.

The original release therefore remains also suitable for archival.

## 💾 Portable Releases

A RetroDisc release contains the files required to identify and launch a game.

The release can be copied or moved between compatible storage devices without installing the original game into a traditional system-wide location.

Persistent user data may either remain on the Linux system or be stored on portable writable media.

---

# RetroDisc Standard

Every RetroDisc title follows a common structure:

```text
RetroDisc Game/
├── RetroDisc
├── manifest.json
├── config.json (optional)
├── gamedata/
└── pfx/ (optional)
```

## `RetroDisc`

The RetroDisc launcher.

It reads the game metadata and runtime configuration, prepares the runtime filesystem and compatibility environment, launches the game and performs cleanup afterwards.

## `manifest.json`

Contains the basic metadata required to identify and launch the game.

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

* Game ID
* Game name
* Executable
* Default runtime

Runtime-specific settings are stored separately in the persistent game-data directory.

## `config.json`

An optional release configuration containing the default runtime and compatibility settings for the game.

The release configuration acts as the template for the user's persistent configuration.

See [Game Configuration](#-game-configuration).

## `gamedata/`

Contains the original game files.

RetroDisc treats this directory as the original release and does not normally modify it during execution.

## `pfx/`

An optional bundled Wine prefix.

If included, it provides a preconfigured starting environment for the game. It is copied to the persistent game-data directory when no persistent prefix exists.

The bundled prefix itself is never modified directly.

---

# 💾 Persistent Game Data

RetroDisc stores writable game data separately from the original release.

By default, the persistent directory is:

```text
~/Games/RetroDisc/<gameId>/
```

When `--datapath` is supplied, the specified directory is used instead:

```text
<datapath>/
```

A typical persistent directory looks like:

```text
~/Games/RetroDisc/
└── <gameId>/
    ├── config.json
    ├── gamedata/
    └── pfx/
```

Persistent data may contain:

* Runtime configuration
* Wine/Proton prefix
* Registry changes
* Windows user profile
* Save data
* Game configuration files
* Persistent game modifications

The persistent directory is reused between launches.

RetroDisc does not recreate persistent data on every launch.

---

# 🔧 Writable Game Data

The original `gamedata/` is used as a read-only base for a temporary `fuse-overlayfs` filesystem.

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
 Persistent Writable Layer
 ~/Games/RetroDisc/<gameId>/
        │
        ├── gamedata/
        └── ...
```

The writable upper layer is stored in:

```text
~/Games/RetroDisc/<gameId>/gamedata/
```

or:

```text
<datapath>/gamedata/
```

Temporary overlay directories are created under `/tmp` and removed after the game exits whenever cleanup succeeds.

Typical temporary paths are:

```text
/tmp/RetroDisc_<pid>/
/tmp/RetroDiscWork_<pid>/
```

If temporary directories cannot be removed during shutdown, they are expected to disappear when the system cleans `/tmp`, typically after a restart.

---

# ⚙️ Game Configuration

Each game has a persistent `config.json`.

By default:

```text
~/Games/RetroDisc/<gameId>/config.json
```

With `--datapath`:

```text
<datapath>/config.json
```

If the persistent configuration does not exist when the game is started, RetroDisc creates it from the configuration embedded in the RetroDisc release.

Once created, the configuration is persistent.

RetroDisc reuses the existing configuration on subsequent launches and does not recreate or overwrite it automatically.

The generated configuration provides default settings but does **not** guarantee that the game will run correctly.

Depending on the game, additional compatibility settings may be required, such as:

* DLL overrides
* Windows version
* Graphics settings
* Synchronization settings
* Custom launch arguments
* Other Wine/Proton settings

The persistent `config.json` can be edited manually.

### Example

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

---

# 👤 Wine / Proton User Profile

RetroDisc keeps the persistent Wine/Proton user profile independent of the actual Linux username.

The persistent Windows user is:

```text
RetroDisc
```

At runtime, RetroDisc determines the current Linux/Steam user and creates a temporary symbolic link inside the Wine prefix:

```text
drive_c/users/<runtime-user>
        └──> RetroDisc
```

This allows Windows applications to see the expected runtime username while persistent data remains inside the `RetroDisc` profile.

The temporary link is removed after the game exits.

---

# 🧪 Runtime Environment

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

RetroDisc locates the configured Proton installation through Steam and uses the persistent game prefix.

A specific Proton installation can be selected through the game configuration.

RetroDisc therefore acts as the packaging and runtime-preparation layer rather than replacing Wine or Proton.

---

# 🔄 Launch Process

When a RetroDisc game is launched, the launcher:

1. Determines the RetroDisc release directory.
2. Loads `manifest.json`.
3. Determines the persistent game-data directory.
4. Loads the persistent `config.json`.
5. Creates `config.json` from the release configuration if necessary.
6. Selects the configured Wine or Proton runtime.
7. Creates or reuses the persistent game-data directory.
8. Creates a temporary `fuse-overlayfs` filesystem for `gamedata/`.
9. Verifies that the configured executable is available through the overlay.
10. Creates the temporary runtime Wine user link.
11. Applies environment variables and compatibility settings.
12. Launches the game.
13. Removes the temporary runtime user link.
14. Unmounts the temporary filesystem.
15. Removes temporary files.

The original game release remains unchanged whenever the game does not require direct modification.

---

# 💿 Supported Storage Media

RetroDisc is storage independent.

### ⭐ Recommended

* Internal SSDs
* External SSDs
* USB flash drives
* SD / microSD cards

### ✅ Suitable

* NAS storage *(theoretically; currently untested)*
* ISO images

### ⚠️ Possible, but not always recommended

* HDDs
* Blu-Ray
* DVD-ROM
* CD-ROM

Loading performance depends primarily on the underlying storage device and filesystem.

The impact varies by game. Some games frequently access many small files, while others primarily load a smaller number of large files.

Therefore, total game size alone does not determine loading performance.

---

# ⚙️ Requirements

RetroDisc itself is designed to have only a small number of system dependencies.

### Required

* Wine **or** Proton, depending on the game
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

The RetroDisc launcher itself does **not** require CMake, GCC or the `nlohmann-json` development package at runtime.

### Optional Components

Individual games may additionally require:

* Proton
* Vulkan drivers
* DXVK
* Gamescope
* Discord

Not every RetroDisc title requires every optional component.

Discord integration is optional. If Discord is unavailable, RetroDisc continues launching the game normally.

---

# 🔒 Persistence and Safety

RetroDisc follows these rules for persistent data:

* Original `gamedata/` is not used as a writable Wine prefix.
* Original game files are not directly modified during normal execution.
* Runtime changes are written through the temporary overlay.
* Persistent writable data is stored separately.
* Persistent configurations are reused instead of recreated.
* Persistent Wine/Proton prefixes are reused between launches.
* A bundled prefix is copied only when no persistent prefix exists.
* An existing persistent prefix is never automatically overwritten.
* An incomplete persistent prefix is not automatically replaced.

This separation allows the original RetroDisc release to remain portable, reproducible and suitable for preservation.

---

# 📜 License

RetroDisc is licensed under the GNU General Public License v3.0 (GPL-3.0).

Everyone is free to use, study, modify and redistribute RetroDisc under the terms of the GPL.

