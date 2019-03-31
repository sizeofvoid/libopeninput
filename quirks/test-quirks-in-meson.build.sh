#!/bin/bash -e

pushd "$1" > /dev/null
diff -u1 <(grep -o 'quirks/.*\.quirks' meson.build) <(ls quirks/*.quirks)
popd > /dev/null
