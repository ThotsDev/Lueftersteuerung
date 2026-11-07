#!/usr/bin/env bash
# Run in the Linux project copy, after scripts/setup-sdk.sh.
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "$project_dir/sdk-versions.env"
sdk_dir="${VENTILATION_SDK_DIR:-$HOME/esp-ventilation}"
if [[ "$(uname -s)" != Linux || "$project_dir" == /mnt/* ]]; then
    echo "Build from a project copy on the Linux filesystem, e.g. ~/projects/ventilation." >&2
    exit 1
fi
if [[ ! -f "$sdk_dir/esp-idf/export.sh" || ! -f "$sdk_dir/esp-matter/export.sh" ]]; then
    echo "Run scripts/setup-sdk.sh first." >&2
    exit 1
fi
if [[ "$(git -C "$sdk_dir/esp-idf" describe --tags --exact-match)" != "$ESP_IDF_REF" ||
      "$(git -C "$sdk_dir/esp-matter" rev-parse HEAD)" != "$ESP_MATTER_COMMIT" ]]; then
    echo "SDK versions differ from sdk-versions.env; refusing an unverified combination." >&2
    exit 1
fi
source "$sdk_dir/esp-idf/export.sh"
source "$sdk_dir/esp-matter/export.sh"
export IDF_TARGET=esp32
cd "$project_dir"
idf.py build
echo "Build complete. Copy build artifacts back to Windows for flashing on COM5."
echo "Use build/flasher_args.json for the flash offsets; do not guess them."
