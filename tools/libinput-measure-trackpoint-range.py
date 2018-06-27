#!/usr/bin/env python3
# vim: set expandtab shiftwidth=4:
# -*- Mode: python; coding: utf-8; indent-tabs-mode: nil -*- */
#
# Copyright © 2017 Red Hat, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

from math import atan, sqrt, pi, floor, ceil
import sys
import argparse
try:
    import evdev
    import evdev.ecodes
    import pyudev
except ModuleNotFoundError as e:
    print('Error: {}'.format(str(e)), file=sys.stderr)
    print('One or more python modules are missing. Please install those '
          'modules and re-run this tool.')
    sys.exit(1)

# This should match libinput's DEFAULT_TRACKPOINT_RANGE
DEFAULT_RANGE = 20
MINIMUM_EVENT_COUNT = 1000


class InvalidDeviceError(Exception):
    pass


class Delta(object):
    def __init__(self, x=0, y=0):
        self.x = x
        self.y = y

    def __bool__(self):
        return self.x != 0 or self.y != 0

    def r(self):
        return sqrt(self.x**2 + self.y**2)

class Device(object):
    def __init__(self, path):
        if path is None:
            path = self._find_trackpoint_device()
        self.path = path

        self.device = evdev.InputDevice(self.path)

        print("Using {}: {}\n".format(self.device.name, path))

        self.deltas = []
        self.nxdeltas = 0
        self.nydeltas = 0

        self.current_delta = Delta()
        self.max_delta = Delta(0, 0)

    def _find_trackpoint_device(self):
        context = pyudev.Context()
        for device in context.list_devices(subsystem='input'):
            if not device.get('ID_INPUT_POINTINGSTICK', 0):
                continue

            if not device.device_node or \
               not device.device_node.startswith('/dev/input/event'):
                continue

            return device.device_node

        raise InvalidDeviceError("Unable to find a trackpoint device")

    def handle_rel(self, event):
        if event.code == evdev.ecodes.REL_X:
            self.current_delta.x = event.value
            if self.max_delta.x < abs(event.value):
                self.max_delta.x = abs(event.value)
        elif event.code == evdev.ecodes.REL_Y:
            self.current_delta.y = event.value
            if self.max_delta.y < abs(event.value):
                self.max_delta.y = abs(event.value)

    def handle_syn(self, event):
        self.deltas.append(self.current_delta)
        if self.current_delta.x != 0:
            self.nxdeltas += 1
        if self.current_delta.y != 0:
            self.nydeltas += 1

        self.current_delta = Delta()

        print("\rTrackpoint sends: max x:{:3d}, max y:{:3} samples [{}, {}]"
              .format(
                self.max_delta.x, self.max_delta.y,
                self.nxdeltas, self.nydeltas,
              ), end="")

    def read_events(self):
        for event in self.device.read_loop():
            if event.type == evdev.ecodes.EV_REL:
                self.handle_rel(event)
            elif event.type == evdev.ecodes.EV_SYN:
                self.handle_syn(event)

    def print_summary(self):
        print("\n")  # undo the \r from the status line
        if not self.deltas:
            return

        if len(self.deltas) < MINIMUM_EVENT_COUNT:
            print("WARNING: *******************************************\n"
                  "WARNING: Insufficient samples, data is not reliable\n"
                  "WARNING: *******************************************\n")

        print("Histogram for x axis deltas, in counts of 5")
        xs = [d.x for d in self.deltas]
        minx = min(xs)
        maxx = max(xs)
        for i in range(minx, maxx + 1):
            xc = len([x for x in xs if x == i])
            xc = int(xc/5)  # counts of 5 is enough
            print("{:4}: {}".format(i, "+" * xc, end=""))

        print("Histogram for y axis deltas, in counts of 5")
        ys = [d.y for d in self.deltas]
        miny = min(ys)
        maxy = max(ys)
        for i in range(miny, maxy + 1):
            yc = len([y for y in ys if y == i])
            yc = int(yc/5)  # counts of 5 is enough
            print("{:4}: {}".format(i, "+" * yc, end=""))

        print("Histogram for radius (amplitude) deltas")
        rs = [d.r() for d in self.deltas if d]
        nr = 50
        minr = 0
        maxr = ceil(max(rs))
        for x in range(0, nr):
            yc = len([y for y in rs if y >= x * maxr/nr
                      and y < (x+1) * maxr/nr])
            print("{:>6.1f}-{:<6.1f}: {:6} {}".
                  format(x * maxr/nr, (x+1) * maxr/nr,
                         yc, "+" * int(yc/5), end=""))

        minr = min(rs)

        axs = sorted([abs(x) for x in xs])
        ays = sorted([abs(y) for y in ys])
        ars = sorted([y for y in rs])

        avgx = int(sum(axs)/len(axs))
        avgy = int(sum(ays)/len(ays))
        avgr = sum(ars)/len(ars)

        medx = axs[int(len(axs)/2)]
        medy = ays[int(len(ays)/2)]
        medr = ars[int(len(ars)/2)]

        pc95x = axs[int(len(axs) * 0.95)]
        pc95y = ays[int(len(ays) * 0.95)]
        pc95r = ars[int(len(ars) * 0.95)]

        print("Min r: {:6.1f}, Max r: {:6.1f}, Max/Min: {:6.1f}".
              format(minr, max(rs), max(rs)/minr))
        print("Average for abs deltas: x: {:3} y: {:3} r: {:6.1f}".format(avgx, avgy, avgr))
        print("Median for abs deltas: x: {:3} y: {:3} r: {:6.1f}".format(medx, medy, medr))
        print("95% percentile for abs deltas: x: {:3} y: {:3} r: {:6.1f}"
              .format(pc95x, pc95y, pc95r)
              )
        if (minr > 2):
            suggested = 10 * ceil(minr * DEFAULT_RANGE / 10)
            print("""\
The minimum amplitude is too big for precise pointer movements.
The recommended value for LIBINPUT_ATTR_TRACKPOINT_RANGE
is 20 * {} ~= {} or higher, which would result in a corrected
delta range of {:>.1f}-{:<.1f}.
""".format(ceil(minr), suggested,
           minr*DEFAULT_RANGE/suggested, maxr*DEFAULT_RANGE/suggested))

def main(args):
    parser = argparse.ArgumentParser(
                description="Measure the trackpoint delta coordinate range"
             )
    parser.add_argument('path', metavar='/dev/input/event0',
                        nargs='?', type=str, help='Path to device (optional)')

    args = parser.parse_args()

    try:
        device = Device(args.path)

        print(
           "This tool measures the commonly used pressure range of the\n"
           "trackpoint. Start by pushing the trackpoint very gently in\n"
           "slow, small circles. Slowly increase pressure until the pointer\n"
           "moves quickly around the screen edges, but do not use excessive\n"
           "pressure that would not be used during day-to-day movement.\n"
           "Also make diagonal some movements, both slow and quick.\n"
           "When you're done, start over, until the displayed event count\n"
           "is {} or more for both x and y axis.\n\n"
           "Hit Ctrl-C to stop the measurement and display results.\n"
           "For best results, run this tool several times to get an idea\n"
           "of the common range.\n".format(MINIMUM_EVENT_COUNT))
        device.read_events()
    except KeyboardInterrupt:
        device.print_summary()
    except (PermissionError, OSError):
        print("Error: failed to open device. Are you running as root?")
    except InvalidDeviceError as e:
        print("Error: {}".format(e))


if __name__ == "__main__":
    main(sys.argv)
