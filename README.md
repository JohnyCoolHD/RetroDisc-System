<div align="center">

# 🎮 RetroDisc

### Plug 'N' Play for PC Games

**By JohnyCoolHD**

RetroDisc is an open game media standard for plug-and-play Windows games on Linux.

</div>

---

# ¿What is RetroDisc?

RetroDisc brings back the simplicity of classic game consoles.

A RetroDisc game behaves like a physical game release:

* 💿 Connect a disc, USB drive, microSD card or other storage device
* ▶️ Launch the game
* 🎮 Play without installing the game into the operating system

Every RetroDisc game contains everything required to run on a compatible Linux system:

* Game launcher
* Compatibility configuration
* Runtime settings
* Portable game structure

The goal of RetroDisc is to make Windows games as portable and easy to use as console games while preserving the original game files.

---

## 🚀 Features

| Feature                    | Description                                    |
| -------------------------- | ---------------------------------------------- |
| 🎮 Console-like experience | Plug 'N' Play for Windows Games on Linux       |
| 💾 Portable games          | Run games from different storage devices       |
| 🔒 Game preservation       | Original files remain untouched                |
| 🐧 Linux gaming            | Built around Wine and Proton                   |
| 📂 Standardized structure  | Every game follows the same layout             |
| 🔧 Separate data storage   | Saves, mods and fixes are stored independently |

---

# Why RetroDisc Exists

RetroDisc was created to solve several problems at once.

---

## 🎮 Console-like Plug & Play

A RetroDisc game should work like a console game.

No installation wizard.
No system-wide changes.
No rebuilding compatibility environments.

Just connect the storage device and play.

---

## 🔒 Preserve Original Games

The original game files should never be modified.

Many Windows games permanently change their installation:

* Updates overwrite files
* Mods replace assets
* Save files are stored inside the game directory
* Configuration files are modified
* Community patches replace executables
* Compatibility fixes add additional files

RetroDisc keeps these changes separate.

---

## 📦 Portable Game Libraries

Every RetroDisc game follows the same directory structure.

This allows games to move between Linux systems without:

* rebuilding Wine prefixes
* reinstalling dependencies
* reorganizing files

A RetroDisc game is a portable game package.

---

## 🌐 A Common Standard

RetroDisc defines a common directory layout that applications can understand.

Future integrations could include:

* Steam library integration
* Graphical frontends
* Game library managers
* Handheld gaming launchers
* Third-party tools

Applications only need to understand the RetroDisc standard instead of supporting every game individually.

---

# 💾 Supported Storage Media

RetroDisc is storage independent and works on almost any storage medium.

## ⭐ Recommended

These provide the best experience:

* ISO images
* USB flash drives
* External/Internal SSDs
* Internal HDDs
* microSD cards
* NAS storage

## ✅ Supported

Performance depends on drive speed and connection:

* External HDDs
* Blu-ray
* DVD-ROM

## ⚠️ Supported, but not recommended

CD-ROM works, but is generally not recommended due to limited read speed.

---

RetroDisc behaves the same regardless of the storage medium.

A persistent RetroDisc data directory is created inside the user's home directory to store all writable game data.

When launching from read-only media, such as a DVD, RetroDisc automatically copies required configuration files into the data directory.

This allows settings to be changed without modifying the original game media.

---

# 📁 RetroDisc Directory Standard

Every RetroDisc game follows the same standardized directory structure.

A RetroDisc package contains the original game files, launcher and compatibility configuration.  
Writable game data is stored separately on the user's system, allowing RetroDisc games to also work from read-only media such as discs or ISO images.

Example:
```
RetroDisc Game Media
├── RetroDisc.sh
├── RetroDisc.conf
└── Game Files


Internal Storage
└── RetroDisc
    └── GameData
        ├── Saves
        ├── Updates
        ├── Mods
        ├── Configuration
        ├── Wine Prefix
        └── Registry
```

# 🗂️ Game Data Separation

RetroDisc separates original game files from writable game data.

```
Original Game
      │
      ├──────────────────────┐
      │                      │
      ▼                      ▼

Original Files          RetroDisc Data
                        ├── Updates
                        ├── Mods
                        ├── Save Data
                        ├── Wine Prefix
                        ├── Registry
                        └── Configuration
```

The original release stays preserved while the playable version can continue to evolve.

The same RetroDisc package can be used:

* From read-only media for preservation
* From writable storage for long-term use

---

# ⚙️ Requirements

RetroDisc only works on Linux.

## Required

* wine
* fuse-overlayfs
* fusermount3
* mountpoint

## Optional

Depending on the game configuration:

* Proton
* Gamescope
* Vulkan drivers
* DXVK

Not every RetroDisc game requires every component.

The launcher automatically uses the runtime specified by the game's configuration.

---

# 🔧 How RetroDisc Works

Every RetroDisc game contains two important files:

```
RetroDisc.sh
RetroDisc.conf
```

When launching a game, RetroDisc automatically:

1. Checks required dependencies
2. Creates or reuses a dedicated Wine prefix
3. Prepares persistent game data
4. Creates a merged runtime using `fuse-overlayfs`
5. Applies compatibility configuration
6. Launches the game using Wine or Proton
7. Cleans up temporary resources after exit

The original game files are never modified.

Instead, RetroDisc creates a writable runtime around the original installation.

---

# 📜 License

RetroDisc is licensed under the GNU General Public License v3.0.

Everyone is free to use, study, modify and redistribute the project under the terms of the GPL-3.0 license.

