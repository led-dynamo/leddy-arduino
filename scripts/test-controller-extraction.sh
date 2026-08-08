#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

for target in esp32 stm32; do
  destination="$tmp/leddy-$target"
  bash "$root/scripts/extract-controller-repo.sh" "$target" "$destination" >/dev/null

  test -f "$destination/README.md"
  test -f "$destination/platformio.ini"
  test -f "$destination/.github/workflows/ci.yml"
  test -f "$destination/src/main.c"
  test -f "$destination/src/common/leddy_port.c"
  test -f "$destination/src/common/leddy_port.h"
  test -f "$destination/test/native/test_leddy_port/test_main.c"

  cc -std=c11 -Wall -Wextra -Werror -pedantic \
    -I"$destination/src/common" \
    -c "$destination/src/common/leddy_port.c" \
    -o "$tmp/$target-leddy-port.o"
done

grep -q 'esp32-s3-devkitc-1' "$tmp/leddy-esp32/platformio.ini"
grep -q 'framework = espidf' "$tmp/leddy-esp32/platformio.ini"
grep -q 'nucleo_f446re' "$tmp/leddy-stm32/platformio.ini"
grep -q 'framework = stm32cube' "$tmp/leddy-stm32/platformio.ini"

echo 'controller extraction smoke tests passed'
