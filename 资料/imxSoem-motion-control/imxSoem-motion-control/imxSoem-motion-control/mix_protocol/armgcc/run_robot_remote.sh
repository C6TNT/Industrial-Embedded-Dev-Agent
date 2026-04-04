#!/bin/sh
set -eu

SERIAL_DEV="${SERIAL_DEV:-/dev/ttymxc1}"
MAIN_PY="${MAIN_PY:-/home/main.py}"
LIB_PATH="${LIB_PATH:-/home/librobot.so.1.0.0}"
RUNTIME_LOG="${RUNTIME_LOG:-/home/robot_runtime.log}"

log() {
  printf '[robot] %s\n' "$1" >> "${RUNTIME_LOG}"
  if [ -e "${SERIAL_DEV}" ]; then
    printf '[robot] %s\n' "$1" >> "${SERIAL_DEV}" 2>/dev/null || true
  fi
}

wait_for_path() {
  path="$1"
  retries="$2"
  delay="$3"
  i=0
  while [ "$i" -lt "$retries" ]; do
    if [ -e "$path" ]; then
      return 0
    fi
    i=$((i + 1))
    sleep "$delay"
  done
  return 1
}

modprobe imx_rpmsg_tty >/dev/null 2>&1 || true

# Start each service run with a fresh runtime log so the latest boot is easy to inspect.
: > "${RUNTIME_LOG}"

log "robot launcher started at $(date '+%Y-%m-%d %H:%M:%S')"

if ! wait_for_path /dev/rpmsg_ctrl0 30 1; then
  log "rpmsg_ctrl0 not ready"
  exit 1
fi

if ! wait_for_path /dev/rpmsg0 30 1; then
  log "rpmsg0 not ready"
  exit 1
fi

if [ ! -f "${MAIN_PY}" ]; then
  log "missing ${MAIN_PY}"
  exit 1
fi

if [ ! -f "${LIB_PATH}" ]; then
  log "missing ${LIB_PATH}"
  exit 1
fi

chmod 755 "${LIB_PATH}" >/dev/null 2>&1 || true
cd /home

log "starting python3 /home/main.py"
if [ -e "${SERIAL_DEV}" ]; then
  python3 -u "${MAIN_PY}" 2>&1 | tee -a "${RUNTIME_LOG}" >> "${SERIAL_DEV}" || true
else
  python3 -u "${MAIN_PY}" >> "${RUNTIME_LOG}" 2>&1
fi
rc=$?
log "main.py exit code: ${rc}"
exit "${rc}"
