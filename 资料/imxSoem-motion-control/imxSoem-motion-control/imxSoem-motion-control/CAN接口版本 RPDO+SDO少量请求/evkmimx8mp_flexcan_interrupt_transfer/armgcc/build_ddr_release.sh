#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
MIRROR_ROOT="${MIRROR_ROOT:-/mnt/d/build_ascii/locked_14_2/evkmimx8mp_flexcan_interrupt_transfer}"
MIRROR_ARMGCC_DIR="${MIRROR_ROOT}/armgcc"
BUILD_DIR="${MIRROR_ARMGCC_DIR}/ddr_release"
OUTPUT_DIR="${SCRIPT_DIR}/ddr_release"

export ARMGCC_DIR="${ARMGCC_DIR:-/mnt/d/build_ascii/toolchains_from_zip/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi}"
SDK_ROOT="${SdkRootDirPath:-/mnt/d/build_ascii/sdk_from_zip/SDK_2_14_0_EVK-MIMX8MP1}"
CMAKE_BIN="${CMAKE_BIN:-/home/tnt_jqr/offline_tools/cmake-4.2.3-linux-x86_64/bin/cmake}"
NINJA_BIN="${NINJA_BIN:-/home/tnt_jqr/offline_tools/ninja-linux/ninja}"

rm -rf "${MIRROR_ROOT}"
mkdir -p "${MIRROR_ROOT}"
cp -a "${PROJECT_ROOT}/." "${MIRROR_ROOT}/"
rm -rf "${BUILD_DIR}"

"${CMAKE_BIN}" \
  -S "${MIRROR_ARMGCC_DIR}" \
  -B "${BUILD_DIR}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=ddr_release \
  -DCMAKE_MAKE_PROGRAM="${NINJA_BIN}" \
  -DCMAKE_TOOLCHAIN_FILE="${SDK_ROOT}/tools/cmake_toolchain_files/armgcc.cmake" \
  -DSdkRootDirPath="${SDK_ROOT}"

"${CMAKE_BIN}" --build "${BUILD_DIR}" -- -v 2>&1 | tee "${BUILD_DIR}/build_log.txt"

mkdir -p "${OUTPUT_DIR}"
cp -f "${BUILD_DIR}/flexcan_interrupt_transfer.elf" "${OUTPUT_DIR}/flexcan_interrupt_transfer.elf"
cp -f "${BUILD_DIR}/rpmsg_lite_str_echo_rtos.bin" "${OUTPUT_DIR}/rpmsg_lite_str_echo_rtos.bin"
cp -f "${BUILD_DIR}/output.map" "${OUTPUT_DIR}/output.map"
