#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
first="$(mktemp -d)"
second="$(mktemp -d)"
trap 'rm -rf "$first" "$second"' EXIT

bash "$root/scripts/package-controller-artifacts.sh" "$first" >/dev/null
bash "$root/scripts/package-controller-artifacts.sh" "$second" >/dev/null

for repo in leddy-esp32 leddy-stm32; do
  (
    cd "$first"
    sha256sum -c "$repo.tar.gz.sha256"
  )

  cmp "$first/$repo.tar.gz" "$second/$repo.tar.gz"
  cmp "$first/$repo.tar.gz.sha256" "$second/$repo.tar.gz.sha256"

  listing="$(tar -tzf "$first/$repo.tar.gz")"
  grep -q "^$repo/README.md$" <<<"$listing"
  grep -q "^$repo/platformio.ini$" <<<"$listing"
  grep -q "^$repo/src/main.c$" <<<"$listing"
  grep -q "^$repo/src/common/leddy_port.c$" <<<"$listing"
  grep -q "^$repo/src/common/leddy_port.h$" <<<"$listing"
  grep -q "^$repo/test/native/test_leddy_port/test_main.c$" <<<"$listing"
  grep -q "^$repo/.github/workflows/ci.yml$" <<<"$listing"
  grep -q "^$repo/docs/ORIGIN.md$" <<<"$listing"
done

cmp "$first/controller-artifacts.json" "$second/controller-artifacts.json"
grep -q '"schema_version": 1' "$first/controller-artifacts.json"
grep -q '"repository": "led-dynamo/leddy-esp32"' "$first/controller-artifacts.json"
grep -q '"repository": "led-dynamo/leddy-stm32"' "$first/controller-artifacts.json"

printf 'controller artifact packaging is reproducible\n'
