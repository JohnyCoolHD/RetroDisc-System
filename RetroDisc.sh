#!/bin/bash
#
# RetroDisc - Windows Game Preservation Launcher
#
# Copyright (C) 2026 JohnyCoolHD
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY.
#


# =========================================
# Setup
# =========================================

ROOT="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"

BOOT_CONF="$ROOT/RetroDisc.conf"

export WINEDEBUG=-all

if [ ! -f "$BOOT_CONF" ]; then
    echo "ERROR: Boot config not found:"
    echo "$BOOT_CONF"
    exit 1
fi

source "$BOOT_CONF"

echo "DEBUG DATA_NAME=$DATA_NAME"

DATA="$HOME/Games/RetroDisc/${DATA_NAME}"

TMP="/tmp/${GAME// /_}_merged_$$"

MERGED="$TMP/merged"
WORK="$TMP/work"

WATCHER=""
GAME_PID=""

# =========================================
# Dependency check
# =========================================

check_command()
{
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "ERROR: Missing dependency:"
        echo "$1"
        exit 1
    fi
}

echo "Checking dependencies..."

check_command fuse-overlayfs
check_command fusermount3
check_command mountpoint
check_command wineserver

if [ "$USE_GAMESCOPE" = "1" ]; then
    check_command gamescope
fi

# =========================================
# Steam / Proton detection
# =========================================

find_proton()
{
    STEAM_PATHS=(
        "$HOME/.steam/steam/steamapps/common"
        "$HOME/.local/share/Steam/steamapps/common"
        "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common"
    )
    for STEAM in "${STEAM_PATHS[@]}"
    do
        if [ -x "$STEAM/$PROTON_NAME/proton" ]; then
            echo "$STEAM/$PROTON_NAME/proton"
            return
        fi
    done
    echo ""
}


if [ "$USE_PROTON" = "1" ]; then
    PROTON=$(find_proton)
    if [ -z "$PROTON" ]; then
        echo
        echo "ERROR:"
        echo "Proton version not found:"
        echo "$PROTON_NAME"
        exit 1
    fi
else
    PROTON=""
fi

# =========================================
# Prepare writable storage
# =========================================

echo
echo "Preparing Data folder..."
mkdir -p "$DATA"

if [ ! -w "$ROOT" ]; then

    echo "Read-only installation detected."
    if [ -f "$ROOT/RetroDisc.conf" ]; then
        cp -n "$ROOT/RetroDisc.conf" "$DATA/RetroDisc.conf"
        chmod u+w "$DATA/RetroDisc.conf"
    fi
else
    echo "Writable installation detected."
fi


# =========================================
# Create overlay folders
# =========================================

echo "Creating Data structure..."

find "$ROOT" -type d | while read DIR
do
    REL="${DIR#$ROOT/}"
    if [ "$REL" != "$DIR" ]; then
        mkdir -p "$DATA/$REL"
    fi
done


# =========================================
# Overlay
# =========================================

echo "Preparing overlay..."

mkdir -p "$TMP"
mkdir -p "$MERGED"
mkdir -p "$WORK"

echo "Mounting overlay..."

fuse-overlayfs \
-o lowerdir="$ROOT" \
-o upperdir="$DATA" \
-o workdir="$WORK" \
"$MERGED"

if [ $? -ne 0 ]; then
    echo "ERROR: Overlay mount failed."
    exit 1
fi

echo "Overlay mounted."

# =========================================
# Reload merged config
# =========================================

CONF="$MERGED/RetroDisc.conf"

if [ ! -f "$CONF" ]; then
    echo "ERROR:"
    echo "Config missing:"
    echo "$CONF"
    exit 1
fi


source "$CONF"

echo "-------------------------------------------"
echo "RetroDisc configuration"
echo "-------------------------------------------"
echo "Game:"
echo "$GAME"
echo "Executable:"
echo "$GAME_EXE"
echo "Proton:"
echo "$USE_PROTON"

if [ "$USE_PROTON" = 1 ]; then
    echo "Proton path:"
    echo "$PROTON"
fi

echo "-------------------------------------------"

cd "$MERGED" || exit 1

if [ ! -f "$GAME_EXE" ]; then
    echo
    echo "ERROR:"
    echo "$GAME_EXE not found."
    exit 1
fi

# =========================================
# Compatibility Prefix
# =========================================

PREFIX="$HOME/Games/RetroDisc/${DATA_NAME}/pfx"
echo "DATA_NAME = '$DATA_NAME'"
echo "PREFIX    = '$PREFIX'"

# =========================================
# Create Compatibility Prefix
# =========================================

create_prefix()
{
    echo "Creating compatibility prefix..."

    mkdir -p "$PREFIX/drive_c/users/RetroDiscUser"

    mkdir -p "$PREFIX/drive_c/users"

    rm -rf "$PREFIX/drive_c/users/maxim"
    rm -rf "$PREFIX/drive_c/users/steamuser"

    ln -s RetroDiscUser "$PREFIX/drive_c/users/maxim"
    ln -s RetroDiscUser "$PREFIX/drive_c/users/steamuser"

    mkdir -p "$PREFIX/dosdevices"

    ln -sf ../drive_c "$PREFIX/dosdevices/c:"
    ln -sf / "$PREFIX/dosdevices/z:"

    echo "Shared user directory created."
}

# =========================================
# Configure Wine Prefix
# =========================================

configure_wine()
{
    echo "Configuring compatibility prefix..."

    export LANG="$LOCALE"
    export LC_ALL="$LOCALE"
    export WINEESYNC="$USE_ESYNC"
    export WINEFSYNC="$USE_FSYNC"

    # =====================================
    # Windows Version
    # =====================================

    echo "Setting Windows version: $WINE_WINDOWS_VERSION"
    winecfg -v "$WINE_WINDOWS_VERSION" >/dev/null 2>&1

    # =====================================
    # X11 Window Settings
    # =====================================

    echo "Applying window settings..."

    wine reg add \
    "HKCU\Software\Wine\X11 Driver" \
    /v Decorated \
    /t REG_SZ \
    /d "$([ "$WINE_ALLOW_WINDOW_DECORATION" = "1" ] && echo Y || echo N)" \
    /f >/dev/null

    wine reg add \
    "HKCU\Software\Wine\X11 Driver" \
    /v Managed \
    /t REG_SZ \
    /d "$([ "$WINE_ALLOW_WINDOW_MANAGER" = "1" ] && echo Y || echo N)" \
    /f >/dev/null

    wine reg add \
    "HKCU\Software\Wine\X11 Driver" \
    /v GrabFullscreen \
    /t REG_SZ \
    /d "$([ "$WINE_MOUSE_CAPTURE" = "1" ] && echo Y || echo N)" \
    /f >/dev/null

    # =====================================
    # Direct3D / Graphics Settings
    # =====================================

    echo "Applying Direct3D settings..."

    wine reg add \
    "HKCU\Software\Wine\Direct3D" \
    /v VideoMemorySize \
    /t REG_SZ \
    /d "$WINE_VIDEO_MEMORY" \
    /f >/dev/null

    wine reg add \
    "HKCU\Software\Wine\Direct3D" \
    /v Renderer \
    /t REG_SZ \
    /d "$WINE_RENDERER" \
    /f >/dev/null

    # =====================================
    # Virtual Desktop
    # =====================================

    if [ "$WINE_VIRTUAL_DESKTOP" = "1" ]; then

        echo "Enabling virtual desktop ${WINE_DESKTOP_WIDTH}x${WINE_DESKTOP_HEIGHT}"

        wine reg add \
        "HKCU\Software\Wine\Explorer\Desktops" \
        /v Default \
        /t REG_SZ \
        /d "${WINE_DESKTOP_WIDTH}x${WINE_DESKTOP_HEIGHT}" \
        /f >/dev/null

        wine reg add \
        "HKCU\Software\Wine\Explorer" \
        /v Desktop \
        /t REG_SZ \
        /d Default \
        /f >/dev/null
    else
        wine reg delete \
        "HKCU\Software\Wine\Explorer" \
        /v Desktop \
        /f >/dev/null 2>&1
    fi

    # =====================================
    # DPI
    # =====================================

    wine reg add \
    "HKCU\Control Panel\Desktop" \
    /v LogPixels \
    /t REG_DWORD \
    /d "$WINE_DPI" \
    /f >/dev/null

    # =====================================
    # DLL Overrides
    # =====================================

    if [ -n "$WINEDLLOVERRIDES" ]; then

        echo "Applying DLL overrides..."

        IFS=';'

        for DLL in $WINEDLLOVERRIDES
        do
            NAME="${DLL%%=*}"
            VALUE="${DLL#*=}"

            wine reg add \
            "HKCU\Software\Wine\DllOverrides" \
            /v "$NAME" \
            /t REG_SZ \
            /d "$VALUE" \
            /f >/dev/null
        done
        unset IFS
    fi

    # =====================================
    # Extra Environment Variables
    # =====================================

    for VAR in "${ENV_VARS[@]}"
    do
        export "$VAR"
    done
    echo "Compatibility configuration finished."

}

# =========================================
# Registry Importer
# =========================================

apply_registry()
{
    REG_DIR="$MERGED/Registry"

    if [ ! -d "$REG_DIR" ]; then
        return
    fi

    for REG in "$REG_DIR"/*.reg
    do
        [ -e "$REG" ] || continue
        echo "Applying registry:"
        echo "$(basename "$REG")"

        if [ "$USE_PROTON" = "1" ]; then
            "$PROTON" run regedit "$REG"
        else
            export WINEPREFIX="$PREFIX"
            wine regedit "$REG"
        fi
    done
}

# =========================================
# Initialize Compatibility Prefix
# =========================================

if [ ! -f "$PREFIX/system.reg" ]; then
    create_prefix
else
    echo "Compatibility prefix already exists."
fi

export WINEPREFIX="$PREFIX"

echo "Initializing Wine environment..."
wineboot --init

wine reg add \
"HKCU\Software\Wine" \
/v UserName \
/t REG_SZ \
/d RetroDiscUser \
/f



configure_wine
apply_registry

# =========================================
# Launch Game
# =========================================

echo
echo "Starting $GAME..."
echo

if [ "$USE_PROTON" = "1" ]; then

    export STEAM_COMPAT_CLIENT_INSTALL_PATH="$HOME/.steam/steam"
    export STEAM_COMPAT_DATA_PATH="$HOME/Games/RetroDisc/${DATA_NAME}"
    export PROTON_LOG=0

    echo
    echo "-------------------------------------------"
    echo "Runtime: Proton"
    echo "$PROTON"
    echo "Prefix: $STEAM_COMPAT_DATA_PATH"
    echo "-------------------------------------------"

    if [ "$USE_GAMESCOPE" = "1" ]; then
        RUN=(
            gamescope
            "${GAMESCOPE_ARGS[@]}"
            --
            "$PROTON"
            waitforexitandrun
            "./$GAME_EXE"
            "${GAME_ARGS[@]}"
        )
    else
        RUN=(
            "$PROTON"
            waitforexitandrun
            "./$GAME_EXE"
            "${GAME_ARGS[@]}"
        )
    fi
else

    export WINEPREFIX="$PREFIX"

    echo "-------------------------------------------"
    echo "Runtime: Wine"
    echo "$WINEPREFIX"
    echo "-------------------------------------------"

    if [ "$USE_GAMESCOPE" = "1" ]; then
        RUN=(
            gamescope
            "${GAMESCOPE_ARGS[@]}"
            --
            wine
            "./$GAME_EXE"
            "${GAME_ARGS[@]}"
        )
    else
        RUN=(
            wine
            "./$GAME_EXE"
            "${GAME_ARGS[@]}"
        )
    fi
fi

"${RUN[@]}" &
GAME_PID=$!
wait "$GAME_PID"

# =========================================
# Wait for Wine processes
# =========================================

echo
echo "Waiting for Wine processes..."

wineserver -w

echo
echo "Game closed."

cleanup()
{
    echo
    echo "-------------------------------------------"
    echo "Cleaning up..."

    # Stop optional watcher
    if [ -n "$WATCHER" ]; then
        kill "$WATCHER" 2>/dev/null || true
    fi

    # Stop game process if still alive
    if [ -n "$GAME_PID" ]; then
        if kill -0 "$GAME_PID" 2>/dev/null; then
            echo "Stopping game process..."
            kill "$GAME_PID" 2>/dev/null || true
        fi
    fi

    # Wait for Wine / Proton wineserver
    echo "Waiting for Wine processes..."
    wineserver -w 2>/dev/null || true
    cd "$HOME" || true

    # Unmount overlay
    if mountpoint -q "$MERGED"; then
        echo "Unmounting overlay..."
        fusermount3 -uz "$MERGED" 2>/dev/null || true
        sleep 1
    fi

    # Second attempt
    if mountpoint -q "$MERGED"; then
        echo "Force unmounting overlay..."
        fusermount3 -uz "$MERGED" 2>/dev/null || true
    fi

    # Remove empty folders only
    if [ -d "$DATA" ]; then
        echo "Removing empty directories..."
        find "$DATA" -type d -empty -delete 2>/dev/null || true
    fi

    # Remove temporary overlay
    if [ -d "$TMP" ]; then
        echo "Removing temporary files..."
        rm -rf "$TMP"
    fi
    echo "Cleanup finished."
}

trap cleanup EXIT INT TERM