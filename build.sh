#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK="$HOME/.cache/retrodisc-v1"
SYSROOT="$WORK/sysroot"
PACKAGES="$WORK/packages"
BUILD="$ROOT/build-v1"
MIRROR="https://archlinux.org/packages/core/x86_64"

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

echo "==> Checking build tools"

for tool in curl tar zstd cmake c++ readelf; do
    command -v "$tool" >/dev/null 2>&1 ||
        fail "Missing required tool: $tool"
done

[[ "$(uname -m)" == "x86_64" ]] ||
    fail "This build script requires an x86_64 host."

[[ -f "$ROOT/CMakeLists.txt" ]] ||
    fail "CMakeLists.txt not found."

mkdir -p "$PACKAGES"

echo "==> Downloading generic x86-64 runtime packages"

for package in glibc libstdc++ libgcc; do
    archive="$PACKAGES/$package.pkg.tar.zst"
    url="$MIRROR/$package/download/"

    echo "Downloading $package..."

    curl --fail --location --retry 3 \
        "$url" \
        --output "$archive" ||
        fail "Could not download $package."
done

echo "==> Preparing clean sysroot"

rm -rf "$SYSROOT"
mkdir -p "$SYSROOT"

for package in glibc libstdc++ libgcc; do
    archive="$PACKAGES/$package.pkg.tar.zst"

    echo "Extracting $package..."

    tar --zstd -xf "$archive" -C "$SYSROOT" ||
        fail "Could not extract $package."
done

echo "==> Checking runtime files"

for file in \
    usr/lib/libc.so.6 \
    usr/lib/libstdc++.so.6 \
    usr/lib/libgcc_s.so.1 \
    usr/lib/ld-linux-x86-64.so.2 \
    usr/lib/crt1.o \
    usr/lib/Scrt1.o
do
    [[ -e "$SYSROOT/$file" ]] ||
        fail "Missing runtime file: $file"
done

mkdir -p "$SYSROOT/lib64"

ln -sf ../usr/lib/ld-linux-x86-64.so.2 \
    "$SYSROOT/lib64/ld-linux-x86-64.so.2"

echo "==> Configuring x86-64-v1 build"

rm -rf "$BUILD"

cmake -S "$ROOT" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-march=x86-64 -mtune=generic" \
    -DCMAKE_CXX_FLAGS="-march=x86-64 -mtune=generic" \
    -DCMAKE_EXE_LINKER_FLAGS="-B$SYSROOT/usr/lib/ -L$SYSROOT/usr/lib/ -Wl,-rpath-link,$SYSROOT/usr/lib/ -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2" \
    -DCMAKE_SHARED_LINKER_FLAGS="-B$SYSROOT/usr/lib/ -L$SYSROOT/usr/lib/ -Wl,-rpath-link,$SYSROOT/usr/lib/"

echo "==> Compiling RetroDisc"

cmake --build "$BUILD" --parallel

BINARY="$BUILD/RetroDisc"

[[ -x "$BINARY" ]] ||
    fail "Executable was not generated."

echo "==> Verifying ISA requirements"

NOTES="$(readelf -n "$BINARY")"

echo "$NOTES"

ISA_LINE="$(grep -E 'x86 ISA needed:' <<< "$NOTES" || true)"

[[ "$ISA_LINE" == *"x86-64-baseline"* ]] ||
    fail "Could not verify x86-64-baseline."

if [[ "$ISA_LINE" != *"x86 ISA needed: x86-64-baseline"* ]] ||
   [[ "$ISA_LINE" == *"x86-64-v2"* ]] ||
   [[ "$ISA_LINE" == *"x86-64-v3"* ]] ||
   [[ "$ISA_LINE" == *"x86-64-v4"* ]]; then
    fail "Binary requires a higher ISA level than x86-64-v1."
fi

echo
echo "======================================"
echo " BUILD SUCCESSFUL"
echo " Target: x86-64-v1"
echo " Binary: $BINARY"
echo "======================================"
