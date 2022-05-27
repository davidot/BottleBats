#!/bin/bash

set -e

if [ $# -lt 2 ]; then
 echo "no arguments"
 exit 1
fi

filename="$1"
container_name="$2"
folder="$(dirname "$filename")"
filename_only="$(basename "$filename")"

pushd "$folder/" > /dev/null

podman build -f /home/david/code/BottleBats/bots-scripts/cs/Dockerfile -t "$container_name" . --build-arg "filename=./$filename_only" 1>&2

popd > /dev/null
