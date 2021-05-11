#!/bin/bash
# Simple driver to perform a docker build
set -eu

COMPILE_COMMANDS=compile_commands.json

docker run -it --user `id -u`:`id -g` --rm -v `pwd`:/omulator mgukowsky/oml-builder $@
if [[ -e $COMPILE_COMMANDS ]]; then
  # Fix paths in compile_commands.json now that we're out of the container
  sed -i -re "s/\/omulator/$(pwd | sed -re 's/\//\\\//g')/g" $COMPILE_COMMANDS
fi
