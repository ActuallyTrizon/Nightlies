#!/usr/bin/env bash
# Verify a packaged DXVK .wcp before it leaves CI.
#
# Used by .github/workflows/dxvk-stable-patched.yml. Proves the archive
# actually opens, that profile.json is valid JSON carrying the expected
# versionName/type/versionCode, that every DLL named in the files array is
# really in the archive, and that the DLLs are PE binaries of the right
# machine type. Any failure is fatal — a broken asset must never reach the
# artifact upload.
#
# Usage: verify-dxvk-wcp.sh <wcp> <expected-versionName> <std|arm64ec> [upstream-commit]

set -euo pipefail

WCP="${1:?usage: verify-dxvk-wcp.sh <wcp> <versionName> <std|arm64ec> [commit]}"
EXPECT_VERSION_NAME="${2:?missing expected versionName}"
VARIANT="${3:?missing variant (std|arm64ec)}"
UPSTREAM_COMMIT="${4:-}"

DLLS="d3d10core.dll d3d11.dll d3d8.dll d3d9.dll dxgi.dll"

fail() { echo "::error::$*"; exit 1; }

[ -f "$WCP" ] || fail "archive $WCP does not exist"

echo "=== verifying $WCP ==="
echo "file type      : $(file -b "$WCP")"
echo "size           : $(stat -c%s "$WCP") bytes"
echo "sha256         : $(sha256sum "$WCP" | cut -d' ' -f1)"
if [ -n "$UPSTREAM_COMMIT" ]; then echo "upstream commit: $UPSTREAM_COMMIT"; fi

# --- 1. the archive opens -------------------------------------------------
MEMBERS=$(tar -tf "$WCP") || fail "$WCP does not open as a tar archive"
echo "--- members ---"
echo "$MEMBERS" | sort

# Member paths are compared against this normalised list. Not every .wcp is
# packed the same way: most store "./system32/d3d11.dll" (packed with
# `tar -C dir .`) but some store "system32/d3d11.dll" (packed from inside the
# directory), e.g. the vanilla ARM64EC asset. Both are valid tar and both
# install identically, so strip a leading "./" before matching rather than
# rejecting one layout.
MEMBERS_N=$(echo "$MEMBERS" | sed 's|^\./||')

# --- 2. profile.json parses and says the right things ---------------------
PROFILE=$(tar -xOf "$WCP" ./profile.json 2>/dev/null || tar -xOf "$WCP" profile.json) \
  || fail "profile.json missing from $WCP"
echo "--- profile.json ---"
echo "$PROFILE"

echo "$PROFILE" | jq -e . >/dev/null || fail "profile.json is not valid JSON"

GOT_NAME=$(echo "$PROFILE" | jq -r '.versionName')
GOT_TYPE=$(echo "$PROFILE" | jq -r '.type')
GOT_CODE=$(echo "$PROFILE" | jq -r '.versionCode')
GOT_DESC=$(echo "$PROFILE" | jq -r '.description')

[ "$GOT_NAME" = "$EXPECT_VERSION_NAME" ] \
  || fail "versionName is '$GOT_NAME', expected '$EXPECT_VERSION_NAME'"
[ "$GOT_TYPE" = "DXVK" ]  || fail "type is '$GOT_TYPE', expected 'DXVK'"
[ "$GOT_CODE" = "0" ]     || fail "versionCode is '$GOT_CODE', expected 0"
if [ -z "$GOT_DESC" ] || [ "$GOT_DESC" = "null" ]; then fail "description is empty"; fi

# The app resolves ${system32}/${syswow64} at install time — those must have
# survived packaging as LITERAL text, not been expanded by the packaging shell.
echo "$PROFILE" | grep -q '\${system32}/d3d11.dll' \
  || fail 'profile.json lost the literal ${system32} placeholder'
echo "$PROFILE" | grep -q '\${syswow64}/d3d11.dll' \
  || fail 'profile.json lost the literal ${syswow64} placeholder'

# --- 3. every file listed in the profile is really in the archive ---------
while read -r src; do
  echo "$MEMBERS_N" | grep -qx "$src" \
    || fail "profile.json lists '$src' but it is not in the archive"
done < <(echo "$PROFILE" | jq -r '.files[].source')

# --- 4. the expected DLL set is complete, both arches ---------------------
for dir in system32 syswow64; do
  for dll in $DLLS; do
    echo "$MEMBERS_N" | grep -qx "$dir/$dll" \
      || fail "missing $dir/$dll"
  done
done

# --- 5. the DLLs are real PE binaries of the right flavour ---------------
# NOTE: an ARM64EC PE carries the AMD64 machine type in its COFF header (that
# is what "EC" means), so `file` reports BOTH std and arm64ec system32 DLLs as
# "PE32+ ... x86-64". The machine type therefore cannot tell them apart. The
# real discriminator is the ARM64EC-only sections the linker emits:
#   .hexpthk — the x64->ARM64 entry thunks
#   .a64xrm  — the ARM64X range map
# Grepping the raw image for those names needs no extra tooling; when
# llvm-readobj is on PATH (it is in the ARM64EC job, via LLVM-MinGW) the
# CHPE load-config pointer is checked too.
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
tar -xf "$WCP" -C "$TMP"

is_arm64ec() {
  grep -qa '\.hexpthk' "$1" && grep -qa '\.a64xrm' "$1"
}

echo "--- DLL types ---"
for dll in $DLLS; do
  F64="$TMP/system32/$dll"
  F32="$TMP/syswow64/$dll"
  D64=$(file -b "$F64")
  D32=$(file -b "$F32")
  echo "system32/$dll : $D64"
  echo "syswow64/$dll : $D32"

  case "$D64" in *PE32+*) ;; *) fail "system32/$dll is not a PE32+ binary: $D64" ;; esac
  case "$D32" in *PE32\ *)  ;; *) fail "syswow64/$dll is not a PE32 binary: $D32" ;; esac
  echo "$D32" | grep -qi "80386\|Intel" \
    || fail "syswow64/$dll is not 32-bit x86 (got: $D32)"

  if [ "$VARIANT" = "arm64ec" ]; then
    if ! is_arm64ec "$F64"; then
      fail "system32/$dll has no ARM64EC sections (.hexpthk/.a64xrm) — an x64 build was packaged as ARM64EC"
    fi
    if command -v llvm-readobj >/dev/null 2>&1; then
      CHPE=$(llvm-readobj --coff-load-config "$F64" 2>/dev/null \
             | grep -m1 'CHPEMetadataPointer' || true)
      echo "system32/$dll : ${CHPE:-<no CHPE metadata>}"
      case "$CHPE" in
        *CHPEMetadataPointer*0x*) ;;
        *) fail "system32/$dll has no CHPE metadata pointer — not a real ARM64EC image" ;;
      esac
    fi
  else
    if is_arm64ec "$F64"; then
      fail "system32/$dll contains ARM64EC sections — an ARM64EC build was packaged as standard x64"
    fi
  fi

  if is_arm64ec "$F32"; then
    fail "syswow64/$dll contains ARM64EC sections — syswow64 must be plain 32-bit x86"
  fi
done

# --- 6. summary for the run page -----------------------------------------
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
  {
    echo "### \`$WCP\`"
    echo ""
    echo "| field | value |"
    echo "|---|---|"
    echo "| size | $(stat -c%s "$WCP") bytes |"
    echo "| sha256 | \`$(sha256sum "$WCP" | cut -d' ' -f1)\` |"
    echo "| versionName | \`$GOT_NAME\` |"
    echo "| compression | $(file -b "$WCP" | cut -d, -f1) |"
    if [ -n "$UPSTREAM_COMMIT" ]; then echo "| upstream commit | \`$UPSTREAM_COMMIT\` |"; fi
    echo ""
  } >> "$GITHUB_STEP_SUMMARY"
fi

echo "=== $WCP OK ==="
