#!/usr/bin/env python3
# -*- coding: utf-8
# vim: set expandtab shiftwidth=4:
# -*- Mode: python; coding: utf-8; indent-tabs-mode: nil -*- */
#
# Copyright © 2018 Red Hat, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the 'Software'),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
#
# Measures the relative motion between touch events (based on slots)
#
# Input is a libinput record yaml file

import argparse
import math
import sys
import yaml
import libevdev


COLOR_RESET = '\x1b[0m'
COLOR_RED = '\x1b[6;31m'


class SlotState:
    NONE = 0
    BEGIN = 1
    UPDATE = 2
    END = 3


class Slot:
    index = 0
    state = SlotState.NONE
    x = 0
    y = 0
    dx = 0
    dy = 0
    dirty = False


class InputEvent:
    def __init__(self, data):
        self.sec = data[0]
        self.usec = data[1]
        self.evtype = data[2]
        self.evcode = data[3]
        self.value = data[4]


def main(argv):
    global COLOR_RESET
    global COLOR_RED

    slots = []
    xres, yres = 1, 1

    parser = argparse.ArgumentParser(description="Measure delta between event frames for each slot")
    parser.add_argument("--use-mm", action='store_true', help="Use mm instead of device deltas")
    parser.add_argument("--use-st", action='store_true', help="Use ABS_X/ABS_Y instead of ABS_MT_POSITION_X/Y")
    parser.add_argument("--use-absolute", action='store_true', help="Use absolute coordinates, not deltas")
    parser.add_argument("path", metavar="recording",
                        nargs=1, help="Path to libinput-record YAML file")
    args = parser.parse_args()

    if not sys.stdout.isatty():
        COLOR_RESET = ''
        COLOR_RED = ''

    yml = yaml.safe_load(open(args.path[0]))
    device = yml['devices'][0]
    absinfo = device['evdev']['absinfo']
    try:
        nslots = absinfo[libevdev.EV_ABS.ABS_MT_SLOT.value][1] + 1
    except KeyError:
        args.use_st = True

    if args.use_st:
        nslots = 1

    slots = [Slot() for _ in range(0, nslots)]

    marker_begin_slot = "   ++++++    | "  # noqa
    marker_end_slot   = "   ------    | "  # noqa
    marker_empty_slot = " *********** | "  # noqa
    marker_no_data    = "             | "  # noqa
    marker_button     = "..............."  # noqa

    if args.use_mm:
        xres = 1.0 * absinfo[libevdev.EV_ABS.ABS_X.value][4]
        yres = 1.0 * absinfo[libevdev.EV_ABS.ABS_Y.value][4]
        if not xres or not yres:
            print("Error: device doesn't have a resolution, cannot use mm")
            sys.exit(1)

        marker_empty_slot = " ************* | "  # noqa
        marker_no_data =    "               | "  # noqa
        marker_begin_slot = "    ++++++     | "  # noqa
        marker_end_slot =   "    ------     | "  # noqa

    if args.use_st:
        print("Warning: slot coordinates on FINGER/DOUBLETAP change may be incorrect")

    slot = 0
    last_time = None

    for event in device['events']:
        for evdev in event['evdev']:
            s = slots[slot]
            e = InputEvent(evdev)
            evbit = libevdev.evbit(e.evtype, e.evcode)

            if args.use_st:
                # Note: this relies on the EV_KEY events to come in before the
                # x/y events, otherwise the last/first event in each slot will
                # be wrong.
                if (evbit == libevdev.EV_KEY.BTN_TOOL_FINGER or
                        evbit == libevdev.EV_KEY.BTN_TOOL_PEN):
                    slot = 0
                    s = slots[slot]
                    s.dirty = True
                    if e.value:
                        s.state = SlotState.BEGIN
                    else:
                        s.state = SlotState.END
                elif evbit == libevdev.EV_KEY.BTN_TOOL_DOUBLETAP:
                    if len(slots) > 1:
                        slot = 1
                    s = slots[slot]
                    s.dirty = True
                    if e.value:
                        s.state = SlotState.BEGIN
                    else:
                        s.state = SlotState.END
                elif evbit == libevdev.EV_ABS.ABS_X:
                    if s.state == SlotState.UPDATE:
                        s.dx = e.value - s.x
                    s.x = e.value
                    s.dirty = True
                elif evbit == libevdev.EV_ABS.ABS_Y:
                    if s.state == SlotState.UPDATE:
                        s.dy = e.value - s.y
                    s.y = e.value
                    s.dirty = True
            else:
                if evbit == libevdev.EV_ABS.ABS_MT_SLOT:
                    slot = e.value
                    s = slots[slot]
                    s.dirty = True
                elif evbit == libevdev.EV_ABS.ABS_MT_TRACKING_ID:
                    if e.value == -1:
                        s.state = SlotState.END
                    else:
                        s.state = SlotState.BEGIN
                        s.dx = 0
                        s.dy = 0
                    s.dirty = True
                elif evbit == libevdev.EV_ABS.ABS_MT_POSITION_X:
                    if s.state == SlotState.UPDATE:
                        s.dx = e.value - s.x
                    s.x = e.value
                    s.dirty = True
                elif evbit == libevdev.EV_ABS.ABS_MT_POSITION_Y:
                    if s.state == SlotState.UPDATE:
                        s.dy = e.value - s.y
                    s.y = e.value
                    s.dirty = True

            if (evbit == libevdev.EV_KEY.BTN_TOUCH or
                    (evbit == libevdev.EV_KEY.BTN_TOOL_DOUBLETAP and nslots < 2) or
                    (evbit == libevdev.EV_KEY.BTN_TOOL_TRIPLETAP and nslots < 3) or
                    (evbit == libevdev.EV_KEY.BTN_TOOL_QUADTAP and nslots < 4) or
                    (evbit == libevdev.EV_KEY.BTN_TOOL_QUINTTAP and nslots < 5)):
                print('            {} {} {} {}'.format(marker_button,
                                                       evbit.name,
                                                       e.value,
                                                       marker_button))

            if evbit == libevdev.EV_SYN.SYN_REPORT:
                if last_time is None:
                    last_time = e.sec * 1000000 + e.usec
                    tdelta = 0
                else:
                    t = e.sec * 1000000 + e.usec
                    tdelta = int((t - last_time) / 1000)  # ms
                    last_time = t

                print("{:2d}.{:06d} {:+4d}ms: ".format(e.sec, e.usec, tdelta), end='')
                for sl in slots:
                    if sl.state == SlotState.NONE:
                        print(marker_empty_slot, end='')
                    elif sl.state == SlotState.BEGIN:
                        print(marker_begin_slot, end='')
                    elif sl.state == SlotState.END:
                        print(marker_end_slot, end='')
                    elif not sl.dirty:
                        print(marker_no_data, end='')
                    else:
                        if sl.dx != 0 and sl.dy != 0:
                            t = math.atan2(sl.dx, sl.dy)
                            t += math.pi  # in [0, 2pi] range now

                            if t == 0:
                                t = 0.01
                            else:
                                t = t * 180.0 / math.pi

                            directions = ['↖↑', '↖←', '↙←', '↙↓', '↓↘', '→↘', '→↗', '↑↗']
                            direction = "{:3.0f}".format(t)
                            direction = directions[int(t / 45)]
                        elif sl.dy == 0:
                            if sl.dx < 0:
                                direction = '←←'
                            else:
                                direction = '→→'
                        else:
                            if sl.dy < 0:
                                direction = '↑↑'
                            else:
                                direction = '↓↓'

                        color = COLOR_RESET

                        if args.use_mm:
                            sl.dx /= xres
                            sl.dy /= yres
                            if math.hypot(sl.dx, sl.dy) > 7:
                                color = COLOR_RED
                            print("{} {}{:+3.2f}/{:+03.2f}{} | ".format(direction, color, sl.dx, sl.dy, COLOR_RESET), end='')
                        elif args.use_absolute:
                            print("{} {}{:4d}/{:4d}{} | ".format(direction, color, sl.x, sl.y, COLOR_RESET), end='')
                        else:
                            print("{} {}{:4d}/{:4d}{} | ".format(direction, color, sl.dx, sl.dy, COLOR_RESET), end='')
                        s.dx = 0
                        s.dy = 0
                    if sl.state == SlotState.BEGIN:
                        sl.state = SlotState.UPDATE
                    elif sl.state == SlotState.END:
                        sl.state = SlotState.NONE

                    sl.dirty = False
                print("")


if __name__ == '__main__':
    main(sys.argv)
