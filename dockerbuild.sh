#!/bin/bash
# Simple driver to perform a docker build
set -ex
docker run -it --rm --user `id -u`:`id -g` -v `pwd`:/omulator oml-builder $@
