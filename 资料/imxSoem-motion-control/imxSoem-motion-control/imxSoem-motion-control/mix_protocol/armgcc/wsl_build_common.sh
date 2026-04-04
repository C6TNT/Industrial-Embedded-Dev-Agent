#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)

BUILD_ASCII_ROOT="${BUILD_ASCII_ROOT:-/mnt/d/build_ascii}"
SDK_ZIP_PATH="${SDK_ZIP_PATH:-}"
TOOLCHAIN_ZIP_PATH="${TOOLCHAIN_ZIP_PATH:-}"

SDK_EXTRACT_ROOT="${SDK_EXTRACT_ROOT:-${BUILD_ASCII_ROOT}/sdk_from_zip}"
TOOLCHAIN_EXTRACT_ROOT="${TOOLCHAIN_EXTRACT_ROOT:-${BUILD_ASCII_ROOT}/toolchains_from_zip}"
MIRROR_PARENT_ROOT="${MIRROR_PARENT_ROOT:-${BUILD_ASCII_ROOT}/locked_14_2}"
PROJECT_NAME="${PROJECT_NAME:-$(basename "${PROJECT_ROOT}")}"
MIRROR_ROOT="${MIRROR_ROOT:-${MIRROR_PARENT_ROOT}/${PROJECT_NAME}}"

SDK_ROOT="${SdkRootDirPath:-${SDK_EXTRACT_ROOT}/SDK_2_14_0_EVK-MIMX8MP1}"
ARMGCC_DIR="${ARMGCC_DIR:-${TOOLCHAIN_EXTRACT_ROOT}/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi}"

PIP_USER_BIN="${HOME}/.local/bin"
CMAKE_BIN_DEFAULT="${PIP_USER_BIN}/cmake"
NINJA_BIN_DEFAULT="${PIP_USER_BIN}/ninja"
CMAKE_BIN="${CMAKE_BIN:-${CMAKE_BIN_DEFAULT}}"
NINJA_BIN="${NINJA_BIN:-${NINJA_BIN_DEFAULT}}"

export ARMGCC_DIR
export PATH="${PIP_USER_BIN}:${PATH}"

log() {
  printf '[wsl-build] %s\n' "$*"
}

require_file() {
  if [ ! -f "$1" ]; then
    printf 'Required file not found: %s\n' "$1" >&2
    exit 1
  fi
}

ensure_python_tooling() {
  if ! command -v python3 >/dev/null 2>&1; then
    printf 'python3 is required inside WSL but was not found.\n' >&2
    exit 1
  fi

  if ! python3 -m pip --version >/dev/null 2>&1; then
    log "Bootstrapping pip in WSL"
    tmp_get_pip="$(mktemp)"
    python3 - "$tmp_get_pip" <<'PY'
import pathlib
import sys
import urllib.request

target = pathlib.Path(sys.argv[1])
urllib.request.urlretrieve("https://bootstrap.pypa.io/get-pip.py", target)
PY
    python3 "${tmp_get_pip}" --user --break-system-packages >/dev/null
    rm -f "${tmp_get_pip}"
  fi

  if [ ! -x "${CMAKE_BIN}" ] || [ ! -x "${NINJA_BIN}" ]; then
    log "Installing user-local cmake and ninja in WSL"
    python3 -m pip install --user --upgrade --break-system-packages cmake ninja >/dev/null
  fi

  if [ ! -x "${CMAKE_BIN}" ] || [ ! -x "${NINJA_BIN}" ]; then
    printf 'Failed to prepare cmake/ninja in WSL.\n' >&2
    exit 1
  fi
}

extract_zip_if_needed() {
  zip_path="$1"
  output_dir="$2"
  marker_name="$3"

  require_file "${zip_path}"

  if [ -d "${output_dir}" ] && [ -f "${output_dir}/.${marker_name}.ok" ]; then
    return 0
  fi

  log "Extracting $(basename "${zip_path}") -> ${output_dir}"
  rm -rf "${output_dir}"
  mkdir -p "${output_dir}"

  python3 - "$zip_path" "$output_dir" <<'PY'
import pathlib
import sys
import zipfile

zip_path = pathlib.Path(sys.argv[1])
out_dir = pathlib.Path(sys.argv[2])

with zipfile.ZipFile(zip_path) as zf:
    zf.extractall(out_dir)
PY

  touch "${output_dir}/.${marker_name}.ok"
}

prepare_sdk_and_toolchain() {
  mkdir -p "${BUILD_ASCII_ROOT}" "${SDK_EXTRACT_ROOT}" "${TOOLCHAIN_EXTRACT_ROOT}" "${MIRROR_PARENT_ROOT}"
  ensure_python_tooling

  if [ -z "${SDK_ZIP_PATH}" ]; then
    printf 'SDK_ZIP_PATH is not set.\n' >&2
    exit 1
  fi

  if [ -z "${TOOLCHAIN_ZIP_PATH}" ]; then
    printf 'TOOLCHAIN_ZIP_PATH is not set.\n' >&2
    exit 1
  fi

  extract_zip_if_needed "${SDK_ZIP_PATH}" "${SDK_EXTRACT_ROOT}" "sdk_extract"
  extract_zip_if_needed "${TOOLCHAIN_ZIP_PATH}" "${TOOLCHAIN_EXTRACT_ROOT}" "toolchain_extract"

  if [ ! -d "${SDK_ROOT}" ]; then
    printf 'SDK root not found after extraction: %s\n' "${SDK_ROOT}" >&2
    exit 1
  fi

  if [ ! -d "${ARMGCC_DIR}" ]; then
    printf 'Toolchain root not found after extraction: %s\n' "${ARMGCC_DIR}" >&2
    exit 1
  fi
}

prepare_project_mirror() {
  log "Mirroring project to ASCII workspace"
  rm -rf "${MIRROR_ROOT}"
  mkdir -p "${MIRROR_ROOT}"
  cp -a "${PROJECT_ROOT}/." "${MIRROR_ROOT}/"
}
