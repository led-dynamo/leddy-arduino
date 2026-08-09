#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
usage: publish-controller-repos.sh [esp32|stm32|all]

Creates the missing public led-dynamo controller repositories and publishes the
checksum-verified standalone trees generated from this repository.

Required environment:
  GH_TOKEN or GITHUB_TOKEN   GitHub token with permission to create org repos
                             and push repository contents.

Optional environment:
  LEDDY_GITHUB_ORG          Defaults to led-dynamo.
  GIT_AUTHOR_NAME           Defaults to Leddy Publisher.
  GIT_AUTHOR_EMAIL          Defaults to noreply@led-dynamo.invalid.
USAGE
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

mode="${1:-all}"
case "$mode" in
  esp32|stm32|all) ;;
  -h|--help) usage; exit 0 ;;
  *) usage >&2; die "unsupported target: $mode" ;;
esac

command -v curl >/dev/null 2>&1 || die "curl is required"
command -v git >/dev/null 2>&1 || die "git is required"

token="${GH_TOKEN:-${GITHUB_TOKEN:-}}"
[[ -n "$token" ]] || die "set GH_TOKEN or GITHUB_TOKEN"
export GH_TOKEN="$token"

org="${LEDDY_GITHUB_ORG:-led-dynamo}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
api="https://api.github.com"

api_headers=(
  -H "Accept: application/vnd.github+json"
  -H "Authorization: Bearer $GH_TOKEN"
  -H "X-GitHub-Api-Version: 2022-11-28"
)

git_auth=(
  -c credential.helper=
  -c 'credential.helper=!f() { echo username=x-access-token; echo "password=$GH_TOKEN"; }; f'
)

repo_for_target() {
  case "$1" in
    esp32) printf '%s\n' 'leddy-esp32' ;;
    stm32) printf '%s\n' 'leddy-stm32' ;;
  esac
}

description_for_target() {
  case "$1" in
    esp32) printf '%s\n' 'Native ESP-IDF/FreeRTOS firmware for Leddy ESP32-S3 controllers' ;;
    stm32) printf '%s\n' 'STM32Cube HAL firmware for Leddy STM32F4 controllers' ;;
  esac
}

ensure_repo() {
  local target="$1" repo description status response payload
  repo="$(repo_for_target "$target")"
  description="$(description_for_target "$target")"
  response="$(mktemp)"
  status="$(curl -sS -o "$response" -w '%{http_code}' "${api_headers[@]}" "$api/repos/$org/$repo")"

  case "$status" in
    200)
      ;;
    404)
      payload="$(printf '{"name":"%s","description":"%s","private":false,"has_issues":true,"has_projects":true,"has_wiki":false,"auto_init":false}' "$repo" "$description")"
      curl -fsS -X POST "${api_headers[@]}" -H 'Content-Type: application/json' \
        "$api/orgs/$org/repos" -d "$payload" >/dev/null
      ;;
    *)
      cat "$response" >&2 || true
      rm -f "$response"
      die "GitHub returned HTTP $status while checking $org/$repo"
      ;;
  esac
  rm -f "$response"
}

publish_target() {
  local target="$1" repo work remote
  repo="$(repo_for_target "$target")"
  ensure_repo "$target"

  work="$(mktemp -d)"
  trap 'rm -rf "$work"' RETURN
  bash "$root/scripts/extract-controller-repo.sh" "$target" "$work/$repo" >/dev/null

  pushd "$work/$repo" >/dev/null
  git init -q -b main
  git config user.name "${GIT_AUTHOR_NAME:-Leddy Publisher}"
  git config user.email "${GIT_AUTHOR_EMAIL:-noreply@led-dynamo.invalid}"
  git add -A
  git commit -q -m "Publish extracted $repo firmware"
  remote="https://github.com/$org/$repo.git"
  git remote add origin "$remote"

  if [[ -n "$(git "${git_auth[@]}" ls-remote --heads origin)" ]]; then
    popd >/dev/null
    die "$org/$repo already has branches; refusing to overwrite an initialized repository"
  fi

  git "${git_auth[@]}" push -u origin main
  git branch dev
  git "${git_auth[@]}" push -u origin dev
  popd >/dev/null
  rm -rf "$work"
  trap - RETURN
  printf 'published %s/%s\n' "$org" "$repo"
}

targets=()
if [[ "$mode" == all ]]; then
  targets=(esp32 stm32)
else
  targets=("$mode")
fi

for target in "${targets[@]}"; do
  publish_target "$target"
done
