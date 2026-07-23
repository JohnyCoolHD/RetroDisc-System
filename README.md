<div align="center">

# 🎮 RetroDisc

### Plug 'N' Play for PC Games

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

# Goals of RetroDisc

RetroDisc is built around four simple ideas.

## 🎮 Portable Gaming

A RetroDisc game is designed to be played directly from portable storage.

Whether the game is stored on a USB drive, external SSD, DVD or ISO image, the experience remains the same: connect the media, launch the included RetroDisc launcher and play.

The launcher automatically prepares the required runtime environment before starting the game.

---

## 🔒 Game Preservation

RetroDisc separates original game files from all writable data.

Save files, configuration, registry changes, compatibility prefixes, updates and mods are stored outside the original release whenever possible.

This allows the original game to remain preserved while the playable copy continues to evolve over time.

---

## 📦 Portable Releases

Every RetroDisc title is self-contained.

The game, launcher and configuration travel together as a single package, making RetroDisc releases easy to archive, copy, move between storage devices or distribute as physical media.

Only the writable game data remains on the local machine.

---

## 🌐 A Common Standard

RetroDisc defines a consistent interface between games and software.

Every RetroDisc title contains two standardized files:

```text
RetroDisc.sh
RetroDisc.conf
```

Applications only need to recognize these files to support every RetroDisc game.

`RetroDisc.sh` is responsible for launching the game, while `RetroDisc.conf` provides metadata and runtime configuration such as the game title, executable, compatibility settings and launch options.

Because every RetroDisc title follows the same structure, software can discover and launch RetroDisc games without requiring game-specific support.

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

Loading times depend entirely on the speed of the storage device. Some games only load small files at a time while other games are loading big files ( It's not always proportional to the game size itself )

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
      ├────────────────────────────┐
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

