#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
usage: extract-controller-repo.sh <esp32|stm32> [destination]

Materialize one native Leddy controller port as a standalone repository tree.
The destination must be empty. No network access is required.
USAGE
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

target="${1:-}"
[[ -n "$target" ]] || { usage >&2; exit 2; }
[[ "$target" != "-h" && "$target" != "--help" ]] || { usage; exit 0; }

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

case "$target" in
  esp32)
    repo_name="leddy-esp32"
    port_dir="esp32-idf"
    firmware_command="pio run -e esp32s3-idf"
    ;;
  stm32)
    repo_name="leddy-stm32"
    port_dir="stm32cube"
    firmware_command="pio run -e nucleo_f446re"
    ;;
  *)
    usage >&2
    die "unsupported controller target: $target"
    ;;
esac

destination="${2:-$PWD/$repo_name}"
if [[ -e "$destination" ]] && [[ -n "$(find "$destination" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]]; then
  die "destination must be empty: $destination"
fi

mkdir -p \
  "$destination/.github/workflows" \
  "$destination/docs" \
  "$destination/src/common" \
  "$destination/test/native/test_leddy_port"

for file in LICENSE CONTRIBUTING.md SECURITY.md .gitignore; do
  cp "$root/$file" "$destination/$file"
done
cp "$root/src/ports/common/leddy_port.c" "$destination/src/common/leddy_port.c"
cp "$root/src/ports/common/leddy_port.h" "$destination/src/common/leddy_port.h"
cp "$root/src/ports/$port_dir/main.c" "$destination/src/main.c"
cp "$root/test/native/test_leddy_port/test_main.c" "$destination/test/native/test_leddy_port/test_main.c"

if [[ "$target" == "esp32" ]]; then
  cat > "$destination/CMakeLists.txt" <<'EOF_CMAKE_ROOT'
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(leddy_esp32s3)
EOF_CMAKE_ROOT

  cat > "$destination/src/CMakeLists.txt" <<'EOF_CMAKE_SRC'
idf_component_register(
    SRCS
        "main.c"
        "common/leddy_port.c"
    INCLUDE_DIRS
        "common"
    REQUIRES
        esp_timer
        freertos
)
EOF_CMAKE_SRC

  cat > "$destination/platformio.ini" <<'EOF_PIO'
[platformio]
default_envs = esp32s3-idf

[env:esp32s3-idf]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf
monitor_speed = 115200
build_flags =
  -I src/common
  -D LEDDY_FIRMWARE_VERSION=\"0.2.0\"

[env:native]
platform = native
test_framework = unity
test_build_src = yes
build_src_filter =
  -<*>
  +<common/leddy_port.c>
build_flags =
  -I src/common
EOF_PIO
else
  cat > "$destination/platformio.ini" <<'EOF_PIO'
[platformio]
default_envs = nucleo_f446re

[env:nucleo_f446re]
platform = ststm32
board = nucleo_f446re
framework = stm32cube
monitor_speed = 115200
build_src_filter =
  -<*>
  +<main.c>
  +<common/leddy_port.c>
build_flags =
  -I src/common
  -D LEDDY_FIRMWARE_VERSION=\"0.2.0\"

[env:native]
platform = native
test_framework = unity
test_build_src = yes
build_src_filter =
  -<*>
  +<common/leddy_port.c>
build_flags =
  -I src/common
EOF_PIO
fi

cat > "$destination/.github/workflows/ci.yml" <<EOF_CI
name: ci

on:
  pull_request:
  push:
    branches: [main, dev]

permissions:
  contents: read

jobs:
  native:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
      - run: pipx install platformio
      - run: pio test -e native
  firmware:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
      - run: pipx install platformio
      - run: $firmware_command
EOF_CI

cat > "$destination/docs/ORIGIN.md" <<EOF_ORIGIN
# Origin and extraction contract

This repository tree is generated from \`led-dynamo/leddy-arduino\` while the
native controller ports are being split into independent repositories.

Source port: \`src/ports/$port_dir\`
Shared source: \`src/ports/common\`
Target repository: \`led-dynamo/$repo_name\`

Until the standalone repository is created, \`leddy-arduino\` remains the
canonical source. After creation, history should record the extraction commit
and both repositories should cross-link the migration PRs before duplicate
sources are removed.
EOF_ORIGIN

cat > "$destination/README.md" <<EOF_README
# $repo_name

Native Leddy firmware for the **$target** controller family, extracted from
\`led-dynamo/leddy-arduino\` behind the same portable display-capability,
frame-planning, and playback contracts used by the rest of the Leddy fleet.

## Build and test

\`\`\`sh
pio test -e native
$firmware_command
\`\`\`

The generated tree includes CI, host-native contract tests, the controller
entrypoint, and the shared embedded C portability layer. See
\`docs/ORIGIN.md\` for the migration contract.
EOF_README

printf '%s\n' "$destination"
