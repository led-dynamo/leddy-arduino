#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${1:-$root/dist/controllers}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

mkdir -p "$output_dir"
rm -f \
  "$output_dir/leddy-esp32.tar.gz" \
  "$output_dir/leddy-esp32.tar.gz.sha256" \
  "$output_dir/leddy-stm32.tar.gz" \
  "$output_dir/leddy-stm32.tar.gz.sha256" \
  "$output_dir/controller-artifacts.json"

source_commit="unknown"
if command -v git >/dev/null 2>&1 && git -C "$root" rev-parse --verify HEAD >/dev/null 2>&1; then
  source_commit="$(git -C "$root" rev-parse HEAD)"
fi

package_target() {
  local target="$1"
  local repo_name="$2"
  local tree="$work_dir/$repo_name"
  local archive="$output_dir/$repo_name.tar.gz"

  bash "$root/scripts/extract-controller-repo.sh" "$target" "$tree" >/dev/null

  # Use normalized metadata and gzip -n so the same source tree produces the
  # same archive bytes across repeated runs on GNU tar based CI hosts.
  tar \
    --sort=name \
    --mtime='UTC 1970-01-01' \
    --owner=0 \
    --group=0 \
    --numeric-owner \
    -C "$work_dir" \
    -cf - \
    "$repo_name" | gzip -n > "$archive"

  (
    cd "$output_dir"
    sha256sum "$repo_name.tar.gz" > "$repo_name.tar.gz.sha256"
  )
}

package_target esp32 leddy-esp32
package_target stm32 leddy-stm32

esp32_sha="$(cut -d' ' -f1 "$output_dir/leddy-esp32.tar.gz.sha256")"
stm32_sha="$(cut -d' ' -f1 "$output_dir/leddy-stm32.tar.gz.sha256")"

cat > "$output_dir/controller-artifacts.json" <<EOF
{
  "schema_version": 1,
  "source_repository": "led-dynamo/leddy-arduino",
  "source_commit": "$source_commit",
  "artifacts": [
    {
      "target": "esp32",
      "repository": "led-dynamo/leddy-esp32",
      "archive": "leddy-esp32.tar.gz",
      "sha256": "$esp32_sha"
    },
    {
      "target": "stm32",
      "repository": "led-dynamo/leddy-stm32",
      "archive": "leddy-stm32.tar.gz",
      "sha256": "$stm32_sha"
    }
  ]
}
EOF

printf '%s\n' "$output_dir"
