# RetroDisc

**RetroDisc is an open directory standard for portable Windows games on Linux.**

RetroDisc brings back the simplicity of classic game consoles.

- Insert a disc, USB drive or microSD card, launch the game and play.
- The game does not need to be installed into the operating system.

Every RetroDisc game contains everything required to run on a compatible Linux system, including its launcher and compatibility configuration.

The goal is to make Windows games as portable and easy to use as console games while preserving the original game files.

---

# Why RetroDisc?

RetroDisc was created to solve several problems at once.

## Console-like Plug & Play

A RetroDisc game should work like a console game.

- Connect the storage device, launch the game and play.
- No installation into the operating system is required.

---

## Preserve Original Games

The original game should never be modified.

Updates, compatibility fixes, save data and mods are stored separately, allowing the original release to remain intact.

---

## Portable Game Libraries

Every RetroDisc game follows the same directory structure.

This makes games portable between Linux systems without rebuilding compatibility environments or reorganizing files.

---

## A Common Standard

Because every RetroDisc game follows the same directory layout, applications can recognize them automatically.

This opens the door for future integrations such as:

- automatic Steam library integration
- graphical frontends
- game library managers
- third-party launcher support

RetroDisc is designed to become a common standard that different applications can build upon.

---

# Supported Storage Media

RetroDisc is storage independent and works on almost any storage medium.

## Recommended

These storage types provide the best experience:
- ISO images
- USB flash drives
- External SSDs
- Internal drives
- microSD cards
- NAS storage
- Blu-ray

## Supported, but performance depends on drive speed and connection

These storage types work, but loading times may vary:
- External HDDs
- DVD-ROM

## Works, but not recommended

- CD-ROM is supported, but generally not recommended due to limited read speed.

---

RetroDisc behaves the same regardless of the storage medium.

A persistent RetroDisc data directory is created inside the user's home directory to store all writable game data.

When a game is launched from read-only media such as a DVD, RetroDisc automatically copies the configuration file into the data directory so game settings can still be customized without modifying the original disc.

---

# Why Separate Original Game Data?

Many Windows games permanently modify their installation.

Updates overwrite files.

Mods replace assets.

Savefiles are added to the installation Folder and Configuration Files are modified.

Community patches replace original executables.

Compatibility fixes often require additional files.

RetroDisc keeps all of these changes separate from the original game.

```
Original Game
      │
      ├──────────────────────┐
      │                      │
      ▼                      ▼
 Original Files         RetroDisc Data
                         ├── Updates
                         ├── Mods
                         ├── Save Data
                         ├── Wine Prefix
                         ├── Registry
                         └── Configuration
```

This keeps the original game preserved while allowing the playable version to evolve independently.

The same RetroDisc package can therefore be used on read-only media for preservation or on writable media for long-term installations.

---

# A Universal Directory Standard

RetroDisc is more than a launcher.

It defines a standardized directory layout that any compatible software can understand.

Imagine inserting a USB drive or microSD card containing several RetroDisc games.

A future Steam plugin could automatically detect the device, scan it for RetroDisc games and temporarily add every title to your Steam library.

When the storage device is removed, the entries disappear again automatically.

The same concept can be used by graphical frontends, handheld launchers or entirely different game libraries.

Because every RetroDisc game follows the same structure, software only has to support the RetroDisc standard instead of individual games.

---

# Requirements

RetroDisc only works on Linux.

## Required

- wine
- fuse-overlayfs
- fusermount3
- mountpoint

## Optional

Depending on the game configuration, RetroDisc can also use:

- Proton
- Gamescope
- Vulkan drivers
- DXVK

Not every RetroDisc game requires every component.

The launcher automatically uses the runtime specified by the game's configuration.

---

# How RetroDisc Works

Every RetroDisc game contains two files:

- `RetroDisc.sh`
- `RetroDisc.conf`

When a game is launched, RetroDisc automatically:

- checks all required dependencies
- creates or reuses a dedicated Wine prefix
- prepares a persistent data directory
- creates a merged runtime using `fuse-overlayfs`
- applies the game's compatibility configuration
- launches the game using Wine or Proton
- cleans up temporary resources after the game exits

The original game files are never modified.

Instead, RetroDisc creates a writable runtime around the original game, allowing updates, mods, save data and compatibility settings to remain completely separate from the original installation.

---

# License

RetroDisc is licensed under the GNU General Public License v3.0.

Everyone is free to use, study, modify and redistribute the project under the terms of the GPL-3.0 license.
