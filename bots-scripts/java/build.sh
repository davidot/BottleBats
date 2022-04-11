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
filename_without_extension=$(basename "$filename" | cut -d. -f1)

pushd "$folder/" > /dev/null

podman build -f /home/david/code/BottleBats/bots-scripts/java/Dockerfile -t "$container_name" . --build-arg "filename=./$filename_only" --build-arg "programname=$filename_without_extension"

popd > /dev/null
