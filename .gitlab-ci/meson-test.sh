#!/usr/bin/env bash

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. $DIR/meson-prep.sh

if [[ -z "$MESON_TEST_ARGS" ]]; then
    echo "\$MESON_TEST_ARGS undefined."
    exit 1
fi

meson test -C "$MESON_BUILDDIR" $MESON_TEST_ARGS --print-errorlogs
