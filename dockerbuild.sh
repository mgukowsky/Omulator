#!/bin/bash
# Simple driver to perform a docker build
set -eu

CONTAINER_NAME=oml-builder

# Check if we need to start the container, from https://stackoverflow.com/a/43723174
if [[ "$(docker container inspect -f '{{.State.Running}}' $CONTAINER_NAME 2>/dev/null)" != "true" ]]; then
  container_hash=$(docker run -dt --rm -v `pwd`:/omulator --name=$CONTAINER_NAME $CONTAINER_NAME)
  echo "Build container created ($(echo $container_hash))"
fi

docker exec -it --user `id -u`:`id -g` $CONTAINER_NAME $@
# Fix paths in compile_commands.json now that we're out of the container
sed -i -re "s/\/omulator/$(pwd | sed -re 's/\//\\\//g')/g" compile_commands.json
