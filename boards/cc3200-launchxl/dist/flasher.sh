#!/bin/sh

# change app fodler since god damn cc3200prog tool cannot find its dlls otherwise...
TOOL_DIR=$1
PORT=$2 
ELFILE=$3


cd ${TOOL_DIR}
# setup JLink command file
echo "[FLASHING] CC3200: port="${PORT}" file="${ELFILE}
./cc3200prog ${PORT} ${ELFILE}