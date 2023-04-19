#!/bin/bash

set -xe

FLAGS="-Wall -Wextra -std=c99 -pedantic -O2"

gcc $FLAGS -o backlight-control main.c
