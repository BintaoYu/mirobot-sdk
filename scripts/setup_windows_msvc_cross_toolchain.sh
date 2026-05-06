#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

LLVM_VERSION="${LLVM_VERSION:-22.1.3}"
XWIN_VERSION="${XWIN_VERSION:-0.6.7}"

LLVM_ARCHIVE="LLVM-${LLVM_VERSION}-Linux-X64.tar.xz"
LLVM_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/${LLVM_ARCHIVE}"
XWIN_ARCHIVE="xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl.tar.gz"
XWIN_URL="https://github.com/Jake-Shadle/xwin/releases/download/${XWIN_VERSION}/${XWIN_ARCHIVE}"

DIST_DIR="${REPO_ROOT}/.toolchains/dist"
LLVM_DIR="${REPO_ROOT}/.toolchains/llvm"
XWIN_DIR="${REPO_ROOT}/.toolchains/xwin"
LLVM_ROOT="${LLVM_DIR}/LLVM-${LLVM_VERSION}-Linux-X64"
XWIN_BIN_DIR="${XWIN_DIR}/bin/xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl"
XWIN_BIN="${XWIN_BIN_DIR}/xwin"

mkdir -p "${DIST_DIR}" "${LLVM_DIR}" "${XWIN_DIR}/bin"

if [ ! -f "${DIST_DIR}/${LLVM_ARCHIVE}" ]; then
    curl -fL -o "${DIST_DIR}/${LLVM_ARCHIVE}" "${LLVM_URL}"
fi

if [ ! -d "${LLVM_ROOT}" ]; then
    tar -xJf "${DIST_DIR}/${LLVM_ARCHIVE}" -C "${LLVM_DIR}"
fi

if [ ! -f "${DIST_DIR}/${XWIN_ARCHIVE}" ]; then
    curl -fL -o "${DIST_DIR}/${XWIN_ARCHIVE}" "${XWIN_URL}"
fi

if [ ! -x "${XWIN_BIN}" ]; then
    tar -xzf "${DIST_DIR}/${XWIN_ARCHIVE}" -C "${XWIN_DIR}/bin"
fi

"${XWIN_BIN}" --accept-license --include-atl \
    --include-debug-runtime \
    --cache-dir "${XWIN_DIR}/cache" \
    splat \
    --output "${XWIN_DIR}/splat"

echo "LLVM root: ${LLVM_ROOT}"
echo "xwin sysroot: ${XWIN_DIR}/splat"
