# RetroDisc

Plug 'n' play for portable Windows games on Linux.

RetroDisc is a small launcher that starts Windows games from a USB drive, SSD, HDD, SD card or any other storage using Wine or Proton.

The idea is simple: the game is the disc, your data is the memory card.

## What it does

- Starts a game directly from its release folder, no installation needed.
- Treats the game files as read-only. Everything the game writes goes into a separate writable layer (`fuse-overlayfs`).
- Stores saves, config, registry changes and the Wine/Proton prefix outside the release, so they survive between launches.
- Optionally ships a preconfigured prefix with the release.

## Disc and memory card

Because the game and its writable data are strictly separated, you can use them like a console:

- **Game as a disc:** The release can live on read-only media. A write-protected USB stick or SD card, a read-only mounted drive or an ISO all work,
  because the game never writes into its own folder. CDs, DVDs and Blu-rays work in principle too, but they are slow and noisy to read from, so in most cases they are not recommended.
  A fast flash drive or SSD is the better choice.
- **Data as a memory card:** With `--datapath <dir>` all persistent data (saves, prefix, runtime changes) goes to the directory you choose. Point it at a separate USB stick or SD card and you have a memory card:
  plug it in, play, take it to another machine with the same game and continue where you stopped.
- **Several cards per game:** Use a different `--datapath` for each player or each playthrough, so every card has its own saves.
- **Shared card for several games:** Keep one card per person and use it with different discs.

Without `--datapath`, persistent data goes to `~/Games/RetroDisc/<gameId>/` on the local drive.

## What else you can do with it

- Carry a game on a drive and play it on any compatible Linux machine.
- Keep the release untouched, so it stays archive-friendly and easy to copy.
- Choose Wine or Proton per game.
- Build your own console-like setup around it, for example a launcher that mounts discs and memory cards and calls RetroDisc.

## Release layout

```text
Game/
├── RetroDisc        launcher
├── manifest.json    game name and executable
├── config.json      optional, runtime settings
├── gamedata/        original game files (read-only)
└── prefix/          optional, bundled prefix
    └── pfx/
```

## Usage

```bash
./RetroDisc
./RetroDisc --datapath /path/to/memory-card
```

## Requirements

Wine or Proton, `fuse-overlayfs`, `fuse3` and `findmnt` (util-linux).


## License

GPL-3.0, see [LICENSE](./LICENSE).
