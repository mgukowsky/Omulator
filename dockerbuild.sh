#!/bin/sh
# Simple driver to perform a docker build
docker run -it --rm --user `id -u`:`id -g` -v `pwd`:/omulator oml-builder $@
