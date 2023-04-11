#!/usr/bin/env bash

set -o errexit
set -o pipefail

usage() {
  echo "Usage: $0 [-s SketchName] [-c COMMAND] [-b BOARD] [-p SERIAL_PORT]" 1>&2;
  exit 1;
}

while getopts ":c:s:b:p:dtx" o; do
  case "${o}" in
    c)
      c=${OPTARG}
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
      d="true"
      ;;
    t)
      t="true"
      ;;
    x)
      x="true"
      ;;
    *)
    
      usage
      ;;
  esac
done
shift $((OPTIND-1))

COMMAND=${c}
SKETCH=${s:-Sonos}
FQBN=${b:-arduino:samd:mkrwifi1010}
PORT=${p:-/dev/cu.usbmodem4101}

EXTRA_FLAGS=""
if [[ "$d" == "true" ]]; then
  EXTRA_FLAGS="$EXTRA_FLAGS -DDEBUG"
fi
if [[ "$t" == "true" ]]; then
  EXTRA_FLAGS="$EXTRA_FLAGS -DTRACE"
fi
if [[ "$x" == "true" ]]; then
  EXTRA_FLAGS="$EXTRA_FLAGS -DPERF"
fi

function compile () {
  arduino-cli compile --fqbn $FQBN "$SKETCH" --build-property "compiler.cpp.extra_flags=$EXTRA_FLAGS"
}

function upload () {
  arduino-cli upload -p $PORT --fqbn $FQBN "$SKETCH"
}

function monitor () {
  arduino-cli monitor -p $PORT 
}

if [[ "$COMMAND" == "compile" ]]; then
  compile
elif [[ "$COMMAND" == "upload" ]]; then
  upload
elif [[ "$COMMAND" == "monitor" ]]; then
  monitor
else
  compile
  upload
  monitor
fi
