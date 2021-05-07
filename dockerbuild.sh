#!/bin/bash
# Simple driver to perform a docker build
set -eu

docker run -it --user `id -u`:`id -g` --rm -v `pwd`:/omulator oml-builder $@
# Fix paths in compile_commands.json now that we're out of the container
sed -i -re "s/\/omulator/$(pwd | sed -re 's/\//\\\//g')/g" compile_commands.json
