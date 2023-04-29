#!/usr/bin/env bash

set -o errexit
set -o pipefail

usage() {
  echo "Usage: $0 [-s SketchName] [-c COMMAND] [-b BOARD] [-p SERIAL_PORT]" 1>&2;
  exit 1;
}

while getopts ":c:s:b:p:d:" o; do
  case "${o}" in
    c)
      COMMAND=${OPTARG}
      ;;
    s)
      s=${OPTARG}
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
SKETCH=${s:-Sonos}
FQBN=${b:-arduino:samd:mkrwifi1010}
PORT=${p:-/dev/cu.usbmodem3101}

EXTRA_FLAGS=""
if [ ! "${#d[@]}" -eq 0 ]; then
  EXTRA_FLAGS="$EXTRA_FLAGS -DDEBUG=true"
fi
for i in "${d[@]}"; do
  EXTRA_FLAGS="$EXTRA_FLAGS -DDEBUG_$i=true"
done

function compile () {
  echo "$@"
  arduino-cli compile --fqbn $FQBN "$SKETCH" --build-property "compiler.cpp.extra_flags=$EXTRA_FLAGS" $RAW_OPTS
}

function upload () {
  arduino-cli upload -p $PORT --fqbn $FQBN "$SKETCH" $RAW_OPTS
}

function monitor () {
  arduino-cli monitor -p $PORT $RAW_OPTS
}

if [[ "$COMMAND" == "compile" ]]; then
  compile
elif [[ "$COMMAND" == "upload" ]]; then
  upload
elif [[ "$COMMAND" == "monitor" ]]; then
  monitor
elif [[ -z "$COMMAND" ]]; then
  compile
  upload
  monitor
else
  arduino-cli "$COMMAND" -p $PORT --fqbn $FQBN "$SKETCH" $RAW_OPTS
fi
