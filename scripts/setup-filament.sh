#!/bin/bash
# Download and extract Filament pre-built SDK
# Usage: ./scripts/setup-filament.sh [platform]
# Platforms: mac (default), linux, windows, ios, android

set -euo pipefail

FILAMENT_VERSION="v1.70.0"
PLATFORM="${1:-mac}"
THIRD_PARTY_DIR="$(cd "$(dirname "$0")/.." && pwd)/third_party/filament"

case "$PLATFORM" in
    mac)     ASSET="filament-${FILAMENT_VERSION}-mac.tgz" ;;
    linux)   ASSET="filament-${FILAMENT_VERSION}-linux.tgz" ;;
    windows) ASSET="filament-${FILAMENT_VERSION}-windows.tgz" ;;
    ios)     ASSET="filament-${FILAMENT_VERSION}-ios.tgz" ;;
    android) ASSET="filament-${FILAMENT_VERSION}-android-native.tgz" ;;
    *)       echo "Unknown platform: $PLATFORM"; exit 1 ;;
esac

URL="https://github.com/google/filament/releases/download/${FILAMENT_VERSION}/${ASSET}"

if [ -f "$THIRD_PARTY_DIR/lib/arm64/libfilament.a" ] || [ -f "$THIRD_PARTY_DIR/lib/x86_64/libfilament.a" ]; then
    echo "Filament SDK already exists at $THIRD_PARTY_DIR"
    echo "Delete it first if you want to re-download."
    exit 0
fi

echo "Downloading Filament ${FILAMENT_VERSION} for ${PLATFORM}..."
TMP_FILE="/tmp/${ASSET}"
curl -L -o "$TMP_FILE" "$URL"

echo "Extracting to ${THIRD_PARTY_DIR}..."
mkdir -p "$THIRD_PARTY_DIR"
tar -xzf "$TMP_FILE" -C "$THIRD_PARTY_DIR" --strip-components=1

echo "Cleaning up..."
rm -f "$TMP_FILE"

echo "Done. Filament SDK installed at $THIRD_PARTY_DIR"
