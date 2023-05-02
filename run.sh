#!/usr/bin/env bash

set -o errexit
set -o pipefail

usage() {
  echo "Usage: $0 [-c COMMAND] [-b BOARD] [-p SERIAL_PORT]" 1>&2;
  exit 1;
}

SKETCH="Sonos"

while getopts ":c:b:p:d:" o; do
  case "${o}" in
    c)
      COMMAND=${OPTARG}
      ;;
    b)
      b=${OPTARG}
      ;;
    p)
      p=${OPTARG}
      ;;
    d)
      d+=("$OPTARG")
      ;;
    *)
      ;;
  esac
done
shift $((OPTIND-1))

RAW_OPTS="$@"
FQBN=${b:-arduino:samd:mkrwifi1010}
PORT=${p:-/dev/cu.usbmodem3101}

EXTRA_FLAGS=""
if [ ! "${#d[@]}" -eq 0 ]; then
  EXTRA_FLAGS="$EXTRA_FLAGS -DDEBUG=true"
fi
for i in "${d[@]}"; do
  EXTRA_FLAGS="$EXTRA_FLAGS -DDEBUG_$i=true"
done

EXTRA_FLAGS="$EXTRA_FLAGS -DAUTO_LOCK"
# EXTRA_FLAGS="$EXTRA_FLAGS -DUSE_BUZZER"

function arduino_cli {
  arduino-cli --config-file ./arduino-cli.yaml "$@"
}

function compile () {
  echo "Flags: $EXTRA_FLAGS"
  arduino_cli compile --fqbn $FQBN "$SKETCH" --build-property "compiler.cpp.extra_flags=$EXTRA_FLAGS" $RAW_OPTS
}

function install () {
  arduino_cli lib install Arduino_MKRIoTCarrier
  arduino_cli lib install ArduinoHttpClient
  arduino_cli lib install ArduinoJson
  arduino_cli lib install --git-url https://github.com/lukekarrys/WiFiNINA.git
}

function upload () {
  arduino_cli upload -p $PORT --fqbn $FQBN "$SKETCH" $RAW_OPTS
}

function monitor () {
  arduino_cli monitor -p $PORT $RAW_OPTS
}

if [[ "$COMMAND" == "compile" ]]; then
  compile
elif [[ "$COMMAND" == "install" ]]; then
  install
elif [[ "$COMMAND" == "upload" ]]; then
  upload
elif [[ "$COMMAND" == "monitor" ]]; then
  monitor
elif [[ -z "$COMMAND" ]]; then
  compile
  upload
  monitor
else
  arduino_cli "$COMMAND" -p $PORT --fqbn $FQBN "$SKETCH" $RAW_OPTS
fi
