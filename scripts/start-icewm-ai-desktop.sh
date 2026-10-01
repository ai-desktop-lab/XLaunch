#!/usr/bin/env bash
set -euo pipefail

# Start the native X11 shell for an ICEWM session. The existing Console/API
# services are managed independently by systemd --user.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
XDOCK_BIN="${XDOCK_BIN:-${ROOT_DIR}/../XDock/build-linux/xdock}"
XLAUNCH_BIN="${XLAUNCH_BIN:-${ROOT_DIR}/launcher/build-linux/xlaunch}"
LOG_DIR="${XDG_STATE_HOME:-${HOME}/.local/state}/xworkspace"
mkdir -p "${LOG_DIR}"

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export QT_QUICK_BACKEND="${QT_QUICK_BACKEND:-software}"
export XDG_CURRENT_DESKTOP="${XDG_CURRENT_DESKTOP:-ICEWM}"

start_once() {
  local name="$1"
  local binary="$2"
  local log_file="$3"
  shift 3
  if [[ ! -x "${binary}" ]]; then
    printf '[xworkspace] %s is not built: %s\n' "${name}" "${binary}" >&2
    return 0
  fi
  # Both binaries own a display-scoped singleton; global pgrep would incorrectly
  # block a second desktop session belonging to the same user.
  nohup "${binary}" "$@" >"${log_file}" 2>&1 </dev/null &
}

start_once xdock "${XDOCK_BIN}" "${LOG_DIR}/xdock.log"
start_once xlaunch "${XLAUNCH_BIN}" "${LOG_DIR}/xlaunch.log" --hidden
