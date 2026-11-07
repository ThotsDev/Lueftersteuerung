#!/usr/bin/env bash
# Run inside Ubuntu/WSL. SDKs live on the Linux filesystem, outside OneDrive.
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "$project_dir/sdk-versions.env"
sdk_dir="${VENTILATION_SDK_DIR:-$HOME/esp-ventilation}"
if [[ "$(uname -s)" != Linux ]]; then
    echo "Use Ubuntu/WSL2 for this setup." >&2
    exit 1
fi
for tool in git python3 cmake ninja; do
    command -v "$tool" >/dev/null || { echo "Missing prerequisite: $tool" >&2; exit 1; }
done
mkdir -p "$sdk_dir"
# Existing directories are left untouched.
if [[ ! -d "$sdk_dir/esp-idf" ]]; then
    git clone --branch "$ESP_IDF_REF" --depth 1 --recursive https://github.com/espressif/esp-idf.git "$sdk_dir/esp-idf"
fi
if [[ "$(git -C "$sdk_dir/esp-idf" describe --tags --exact-match)" != "$ESP_IDF_REF" ]]; then
    echo "Existing ESP-IDF is not $ESP_IDF_REF" >&2; exit 1
fi
if [[ ! -d "$sdk_dir/esp-matter" ]]; then
    git clone --branch "$ESP_MATTER_BRANCH" --depth 1 https://github.com/espressif/esp-matter.git "$sdk_dir/esp-matter"
    git -C "$sdk_dir/esp-matter" fetch --depth 1 origin "$ESP_MATTER_COMMIT"
    git -C "$sdk_dir/esp-matter" checkout --detach "$ESP_MATTER_COMMIT"
    git -C "$sdk_dir/esp-matter" submodule update --init --depth 1
    git -C "$sdk_dir/esp-matter/connectedhomeip/connectedhomeip" submodule update --init --depth 1
fi
if [[ "$(git -C "$sdk_dir/esp-matter" rev-parse HEAD)" != "$ESP_MATTER_COMMIT" ]]; then
    echo "Existing ESP-Matter differs from sdk-versions.env" >&2; exit 1
fi
# Connected hardware was identified as classic ESP32; install only that toolchain.
(cd "$sdk_dir/esp-idf" && ./install.sh esp32)
source "$sdk_dir/esp-idf/export.sh"
(cd "$sdk_dir/esp-matter" && ./install.sh)
echo "SDK installed. Source both export.sh files in your build shell."
