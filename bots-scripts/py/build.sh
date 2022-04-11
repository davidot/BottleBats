#!/bin/bash

set -e
#set -x

if [ $# -lt 2 ]; then
 echo "no arguments"
 exit 1
fi

filename="$1"
container_name="$2"
folder="$(dirname "$filename")"
filename_only="$(basename "$filename")"


pushd "$folder/" > /dev/null

podman build -f /home/david/code/BottleBats/bots-scripts/py/Dockerfile -t "$container_name" . --build-arg "filename=./$filename_only"

popd > /dev/null
