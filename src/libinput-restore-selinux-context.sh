#!/bin/sh
#
# $1: abs path to libdir
# $2: abs path to .so file

libdir="$1"
sofile=$(basename "$2")

if command -v restorecon >/dev/null; then
	echo "Restoring SELinux context on $MESON_INSTALL_DESTDIR_PREFIX/$libdir/$sofile"
	restorecon "$MESON_INSTALL_DESTDIR_PREFIX/$libdir/$sofile"
fi
