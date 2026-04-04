#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "${SCRIPT_DIR}/wsl_build_common.sh"

MIRROR_ARMGCC_DIR="${MIRROR_ROOT}/armgcc"
BUILD_DIR="${MIRROR_ARMGCC_DIR}/ddr_release"
OUTPUT_DIR="${SCRIPT_DIR}/ddr_release"

prepare_sdk_and_toolchain
prepare_project_mirror
rm -rf "${BUILD_DIR}"

log "Configuring ddr_release with SDK=${SDK_ROOT}"
"${CMAKE_BIN}" \
  -S "${MIRROR_ARMGCC_DIR}" \
  -B "${BUILD_DIR}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=ddr_release \
  -DCMAKE_MAKE_PROGRAM="${NINJA_BIN}" \
  -DCMAKE_TOOLCHAIN_FILE="${SDK_ROOT}/tools/cmake_toolchain_files/armgcc.cmake" \
  -DSdkRootDirPath="${SDK_ROOT}"

log "Building ddr_release"
"${CMAKE_BIN}" --build "${BUILD_DIR}" -- -v 2>&1 | tee "${BUILD_DIR}/build_log.txt"

mkdir -p "${OUTPUT_DIR}"
cp -f "${BUILD_DIR}/rpmsg_lite_str_echo_rtos_imxcm7.elf" "${OUTPUT_DIR}/rpmsg_lite_str_echo_rtos_imxcm7.elf"
cp -f "${BUILD_DIR}/rpmsg_lite_str_echo_rtos.bin" "${OUTPUT_DIR}/rpmsg_lite_str_echo_rtos.bin"
cp -f "${BUILD_DIR}/output.map" "${OUTPUT_DIR}/output.map"
