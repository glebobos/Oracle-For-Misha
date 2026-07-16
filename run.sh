#!/usr/bin/env bash
set -euo pipefail

PROJECT_IMAGE="misha-gadalka-pico"
UF2_PATH="build/misha_gadalka.uf2"
PORT=""
BAUD="115200"

usage() {
  cat <<'EOF'
Usage: ./run.sh <build|flash|monitor|all> [--port /dev/ttyACM0] [--baud 115200]

All toolchains run inside Docker. The host needs only Docker and USB access.
  build     Build the RP2350 UF2 in the Docker container.
  flash     Build, then flash a board in BOOTSEL mode using picotool in Docker.
  monitor   Open USB serial output through Docker.
  all       Build, flash, then open the serial monitor.
EOF
}

detect_port() {
  for candidate in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyUSB0 /dev/ttyUSB1; do
    if [[ -e "$candidate" ]]; then printf '%s\n' "$candidate"; return 0; fi
  done
  echo "No serial port found. Pass --port after the firmware is running." >&2
  return 1
}

build() {
  docker compose build firmware-builder
  docker compose run --rm --user "$(id -u):$(id -g)" firmware-builder sh -lc \
    "cmake -S . -B build -DPICO_BOARD=pico2 && cmake --build build -j\$(nproc)"
}

flash() {
  build
  echo "Build complete. Press and hold BOOT on the XIAO RP2350, connect USB, release — then press Enter to flash."
  read -r
  docker run --rm --privileged -v /dev/bus/usb:/dev/bus/usb \
    -v "$(pwd)/firmware:/project" -w /project "$PROJECT_IMAGE" \
    /usr/local/picotool/picotool load -f "$UF2_PATH"
}

monitor() {
  if [[ -z "$PORT" ]]; then PORT=$(detect_port); fi
  docker compose run --rm --user "$(id -u):$(id -g)" --device="$PORT" firmware-builder \
    picocom --baud "$BAUD" "$PORT"
}

[[ $# -gt 0 ]] || { usage; exit 1; }
COMMAND="$1"; shift
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2 ;;
    --baud) BAUD="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
  esac
done

case "$COMMAND" in
  build) build ;;
  flash) flash ;;
  monitor) monitor ;;
  all) flash; monitor ;;
  *) usage; exit 1 ;;
esac
