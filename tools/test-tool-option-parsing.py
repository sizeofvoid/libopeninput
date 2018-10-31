#!/usr/bin/python3
# vim: set expandtab shiftwidth=4:
# -*- Mode: python; coding: utf-8; indent-tabs-mode: nil -*- */
#
# Copyright © 2018 Red Hat, Inc.
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

import argparse
import os
import unittest
import sys
import subprocess
import time


class TestLibinputTool(unittest.TestCase):
    libinput_tool = 'libinput'
    subtool = None

    def run_command(self, args):
        args = [self.libinput_tool] + args
        if self.subtool is not None:
            args.insert(1, self.subtool)

        with subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
            time.sleep(0.1)
            p.send_signal(2)
            p.wait()
            return p.returncode, p.stdout.read().decode('UTF-8'), p.stderr.read().decode('UTF-8')

    def run_command_success(self, args):
        rc, stdout, stderr = self.run_command(args)
        # if we're running as user, we might fail the command but we should
        # never get rc 2 (invalid usage)
        self.assertIn(rc, [0, 1])

    def run_command_unrecognised_option(self, args):
        rc, stdout, stderr = self.run_command(args)
        self.assertEqual(rc, 2)
        self.assertTrue(stdout.startswith('Usage') or stdout == '')
        self.assertIn('unrecognized option', stderr)

    def run_command_missing_arg(self, args):
        rc, stdout, stderr = self.run_command(args)
        self.assertEqual(rc, 2)
        self.assertTrue(stdout.startswith('Usage') or stdout == '')
        self.assertIn('requires an argument', stderr)

    def run_command_unrecognised_tool(self, args):
        rc, stdout, stderr = self.run_command(args)
        self.assertEqual(rc, 2)
        self.assertTrue(stdout.startswith('Usage') or stdout == '')
        self.assertIn('is not a libinput command', stderr)


class TestLibinputCommand(TestLibinputTool):
    subtool = None

    def test_help(self):
        rc, stdout, stderr = self.run_command(['--help'])
        self.assertEqual(rc, 0)
        self.assertTrue(stdout.startswith('Usage:'))
        self.assertEqual(stderr, '')

    def test_version(self):
        rc, stdout, stderr = self.run_command(['--version'])
        self.assertEqual(rc, 0)
        self.assertTrue(stdout.startswith('1'))
        self.assertEqual(stderr, '')

    def test_invalid_arguments(self):
        self.run_command_unrecognised_option(['--banana'])
        self.run_command_unrecognised_option(['--foo'])
        self.run_command_unrecognised_option(['--quiet'])
        self.run_command_unrecognised_option(['--verbose'])
        self.run_command_unrecognised_option(['--quiet', 'foo'])

    def test_invalid_tools(self):
        self.run_command_unrecognised_tool(['foo'])
        self.run_command_unrecognised_tool(['debug'])
        self.run_command_unrecognised_tool(['foo', '--quiet'])


class TestToolWithOptions(object):
    options = {
            'pattern': ['sendevents'],
            # enable/disable options
            'enable-disable': [
                        'tap',
                        'drag',
                        'drag-lock',
                        'middlebutton',
                        'natural-scrolling',
                        'left-handed',
                        'dwt'
            ],
            # options with distinct values
            'enums': {
                'set-click-method': ['none', 'clickfinger', 'buttonareas'],
                'set-scroll-method': ['none', 'twofinger', 'edge', 'button'],
                'set-profile': ['adaptive', 'flat'],
                'set-tap-map': ['lrm', 'lmr'],
            },
            # options with a range
            'ranges': {
                'set-speed': (float, -1.0, +1.0),
            }
    }

    def test_udev_seat(self):
        self.run_command_missing_arg(['--udev'])
        self.run_command_success(['--udev', 'seat0'])
        self.run_command_success(['--udev', 'seat1'])

    @unittest.skipIf(os.environ.get('UDEV_NOT_AVAILABLE'), "udev required")
    def test_device(self):
        self.run_command_missing_arg(['--device'])
        self.run_command_success(['--device', '/dev/input/event0'])
        self.run_command_success(['--device', '/dev/input/event1'])
        self.run_command_success(['/dev/input/event0'])

    def test_options_pattern(self):
        for option in self.options['pattern']:
            self.run_command_success(['--disable-{}'.format(option), '*'])
            self.run_command_success(['--disable-{}'.format(option), 'abc*'])

    def test_options_enable_disable(self):
        for option in self.options['enable-disable']:
            self.run_command_success(['--enable-{}'.format(option)])
            self.run_command_success(['--disable-{}'.format(option)])

    def test_options_enums(self):
        for option, values in self.options['enums'].items():
            for v in values:
                self.run_command_success(['--{}'.format(option), v])
                self.run_command_success(['--{}={}'.format(option, v)])

    def test_options_ranges(self):
        for option, values in self.options['ranges'].items():
            range_type, minimum, maximum = values
            self.assertEqual(range_type, float)
            step = (maximum - minimum)/10.0
            value = minimum
            while value < maximum:
                self.run_command_success(['--{}'.format(option), str(value)])
                self.run_command_success(['--{}={}'.format(option, value)])
                value += step
            self.run_command_success(['--{}'.format(option), str(maximum)])
            self.run_command_success(['--{}={}'.format(option, maximum)])


class TestDebugEvents(TestToolWithOptions, TestLibinputTool):
    subtool = 'debug-events'

    def test_verbose_quiet(self):
        rc, stdout, stderr = self.run_command(['--verbose'])
        self.assertEqual(rc, 0)
        rc, stdout, stderr = self.run_command(['--quiet'])
        self.assertEqual(rc, 0)
        rc, stdout, stderr = self.run_command(['--verbose', '--quiet'])
        self.assertEqual(rc, 0)
        rc, stdout, stderr = self.run_command(['--quiet', '--verbose'])
        self.assertEqual(rc, 0)

    def test_invalid_arguments(self):
        self.run_command_unrecognised_option(['--banana'])
        self.run_command_unrecognised_option(['--foo'])
        self.run_command_unrecognised_option(['--version'])


class TestDebugGUI(TestToolWithOptions, TestLibinputTool):
    subtool = 'debug-gui'

    @classmethod
    def setUpClass(cls):
        if not os.getenv('DISPLAY') and not os.getenv('WAYLAND_DISPLAY'):
            raise unittest.SkipTest()

    def test_verbose_quiet(self):
        rc, stdout, stderr = self.run_command(['--verbose'])
        self.assertEqual(rc, 0)

    def test_invalid_arguments(self):
        self.run_command_unrecognised_option(['--quiet'])
        self.run_command_unrecognised_option(['--banana'])
        self.run_command_unrecognised_option(['--foo'])
        self.run_command_unrecognised_option(['--version'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Verify a libinput tool\'s option parsing')
    parser.add_argument('tool_path', metavar='/path/to/builddir/libinput',
                        type=str, nargs='?',
                        help='Path to the libinput tool in the builddir')
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()
    if args.tool_path is not None:
        TestLibinputTool.libinput_tool = args.tool_path
    verbosity = 1
    if args.verbose:
        verbosity = 3
    del sys.argv[1:]
    unittest.main(verbosity=verbosity)
