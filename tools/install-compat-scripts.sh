#!/bin/sh

bindir="${DESTDIR}${1}"

# Do not create bindir, because if it is not there now, we have a problem
cp "${MESON_SOURCE_ROOT}/tools/libinput-list-devices.compat" "${bindir}/libinput-list-devices"
cp "${MESON_SOURCE_ROOT}/tools/libinput-debug-events.compat" "${bindir}/libinput-debug-events"
