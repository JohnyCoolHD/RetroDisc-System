<div align="center">

# 🎮 RetroDisc

### Plug 'N' Play for PC Games

**By JohnyCoolHD**

RetroDisc is an open game media standard for portable Windows games on Linux.

</div>

---

# What is RetroDisc?

RetroDisc brings back the simplicity of classic game consoles.

A RetroDisc game behaves like a physical game release:

* 💿 Connect a disc, USB drive, SSD, HDD, microSD card or other storage device.
* ▶️ Launch the included RetroDisc launcher.
* 🎮 Play the game.

Every RetroDisc release contains everything needed to launch the game on a compatible Linux system:

* Game launcher
* Compatibility configuration
* Runtime configuration
* Original game files

RetroDisc does **not** replace Wine or Proton.

Instead, it provides a standardized way to package and launch Windows games while keeping the original game files preserved.

---

# 🚀 Features

| Feature                     | Description                                            |
| --------------------------- | ------------------------------------------------------ |
| 🎮 Plug & Play              | Launch games directly from portable storage            |
| 💾 Portable Releases        | Games can be moved between drives and systems          |
| 🔒 Game Preservation        | Original files remain untouched                        |
| 🐧 Linux Focus              | Built around Wine and Proton                           |
| 📂 Standard Layout          | Every RetroDisc title follows the same structure       |
| 🔧 Writable Data Separation | Saves, mods and configuration are stored independently |

---

# Why RetroDisc Exists

RetroDisc was created to solve several problems at once.

---

## 🎮 Console-like Experience

Launching a RetroDisc game should feel similar to using a console game.

Instead of installing a game into the operating system, the included launcher prepares everything automatically.

Depending on the game's configuration, RetroDisc will:

* verify required dependencies
* create or reuse a compatibility prefix
* prepare writable game data
* configure Wine or Proton
* launch the game

System components such as Wine, Proton or Gamescope only need to be installed once.

The game itself never has to be installed.

---

## 🔒 Preserve Original Games

Many collectors preserve their PC games by archiving installers onto optical media.

Unfortunately those installers are rarely used afterwards because reinstalling old Windows games can be inconvenient.

RetroDisc instead stores the complete playable game on the media.

The original game files remain unchanged while all writable data is stored separately on the user's system.

This makes RetroDisc suitable for both long-term preservation and everyday use.

---

## 📦 Portable Releases

A RetroDisc release is completely self-contained.

The same package can be:

* copied to another drive
* burned to optical media
* archived
* transferred between Linux systems

Only the writable game data remains on the local machine.

---

## 🌐 A Common Standard

RetroDisc defines a simple and predictable interface between games and software.

Every RetroDisc title contains two standard files:

```
RetroDisc.sh
RetroDisc.conf
```

Applications only need to recognize these files to support every RetroDisc game.

The launcher (`RetroDisc.sh`) always starts the game, while the configuration (`RetroDisc.conf`) contains information such as the game title, runtime configuration and launch settings.

Because every RetroDisc title follows the same specification, software does not need game-specific launch logic.

---

# 💾 Supported Storage Media

RetroDisc is storage independent and works on almost any storage medium.

## ⭐ Recommended

* USB flash drives
* Internal SSDs
* External SSDs
* Internal HDDs
* microSD cards
* NAS storage

## ✅ Supported

* External HDDs
* Blu-ray
* DVD-ROM

## ⚠️ Supported, but not recommended

* CD-ROM

Performance depends entirely on the speed of the storage device.

---

A persistent RetroDisc data directory is automatically created inside the user's home directory.

When running from read-only media (for example DVDs or ISO images), RetroDisc automatically stores all writable files outside the original game media.

This includes:

* save games
* configuration files
* Wine prefixes
* registry changes
* installed mods
* updates

The original release always remains unchanged.

---

# 📁 Directory Layout

Every RetroDisc title follows the same basic layout.

```
RetroDisc Game
├── RetroDisc.sh
├── RetroDisc.conf
├── README.txt
└── Game Files

```

Writable data is stored separately:

```
~/Games/RetroDisc/
└── Game_Name_Data
    ├── Saves
    ├── Mods
    ├── Updates
    ├── Configuration
    ├── Wine Prefix
    └── Registry
```

This allows RetroDisc games to work equally well from writable storage, optical media and ISO images.

---

# 🗂️ Writable Game Data

RetroDisc separates original game files from user-generated data.

```
Original Game
      │
      ├──────────────────────────────┐
      │                              │
      ▼                              ▼

Original Files                 Game_Name_Data
                               ├── Saves
                               ├── Mods
                               ├── Updates
                               ├── Configuration
                               ├── Wine Prefix
                               └── Registry
```

Whenever possible, all changes are written to the data directory instead of modifying the original release.

This keeps the preserved game untouched while still allowing normal gameplay, configuration changes and mod installation.

---

# ⚙️ Requirements

RetroDisc requires Linux.

## Required Components

* Wine
* fuse-overlayfs
* fusermount3
* mountpoint

## Optional Components

Depending on the individual game:

* Proton
* Gamescope
* Vulkan drivers
* DXVK

Not every RetroDisc title requires every optional component.

The launcher automatically checks which tools are required before starting the game.

---

# 🔧 How RetroDisc Works

Every RetroDisc game includes:

```
RetroDisc.sh
RetroDisc.conf
```

When a game is launched, RetroDisc automatically:

1. Loads the game configuration.
2. Verifies required dependencies.
3. Creates or reuses the compatibility prefix.
4. Prepares the writable data directory.
5. Builds a temporary writable runtime using `fuse-overlayfs`.
6. Applies compatibility settings and registry entries.
7. Starts the game with Wine or Proton.
8. Cleans up temporary resources after the game exits.

Throughout the entire process, the original game files remain unchanged.

---

# 📜 License

RetroDisc is licensed under the GNU General Public License v3.0 (GPL-3.0).

Everyone is free to use, study, modify and redistribute RetroDisc under the terms of the GPL.

