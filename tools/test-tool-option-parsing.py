#!/usr/bin/env python3
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
import resource
import sys
import subprocess
import tempfile
from pathlib import Path


def _disable_coredump():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def run_command(args):
    with subprocess.Popen(args, preexec_fn=_disable_coredump,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
        try:
            p.wait(0.7)
        except subprocess.TimeoutExpired:
            p.send_signal(3)  # SIGQUIT
        stdout, stderr = p.communicate(timeout=5)
        if p.returncode == -3:
            p.returncode = 0
        return p.returncode, stdout.decode('UTF-8'), stderr.decode('UTF-8')


class TestLibinputTool(unittest.TestCase):
    libinput_tool = 'libinput'
    subtool = None

    def run_command(self, args):
        args = [self.libinput_tool] + args
        if self.subtool is not None:
            args.insert(1, self.subtool)

        return run_command(args)

    def run_command_success(self, args):
        rc, stdout, stderr = self.run_command(args)
        # if we're running as user, we might fail the command but we should
        # never get rc 2 (invalid usage)
        self.assertIn(rc, [0, 1], msg=(stdout, stderr))

    def run_command_unrecognized_option(self, args):
        rc, stdout, stderr = self.run_command(args)
        self.assertEqual(rc, 2)
        self.assertTrue(stdout.startswith('Usage') or stdout == '')
        self.assertIn('unrecognized option', stderr)

    def run_command_missing_arg(self, args):
        rc, stdout, stderr = self.run_command(args)
        self.assertEqual(rc, 2)
        self.assertTrue(stdout.startswith('Usage') or stdout == '')
        self.assertIn('requires an argument', stderr)

    def run_command_unrecognized_tool(self, args):
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
        self.run_command_unrecognized_option(['--banana'])
        self.run_command_unrecognized_option(['--foo'])
        self.run_command_unrecognized_option(['--quiet'])
        self.run_command_unrecognized_option(['--verbose'])
        self.run_command_unrecognized_option(['--quiet', 'foo'])

    def test_invalid_tools(self):
        self.run_command_unrecognized_tool(['foo'])
        self.run_command_unrecognized_tool(['debug'])
        self.run_command_unrecognized_tool(['foo', '--quiet'])


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
            step = (maximum - minimum) / 10.0
            value = minimum
            while value < maximum:
                self.run_command_success(['--{}'.format(option), str(value)])
                self.run_command_success(['--{}={}'.format(option, value)])
                value += step
            self.run_command_success(['--{}'.format(option), str(maximum)])
            self.run_command_success(['--{}={}'.format(option, maximum)])

    def test_apply_to(self):
        self.run_command_missing_arg(['--apply-to'])
        self.run_command_success(['--apply-to', '*foo*'])
        self.run_command_success(['--apply-to', 'foobar'])
        self.run_command_success(['--apply-to', 'any'])


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
        self.run_command_unrecognized_option(['--banana'])
        self.run_command_unrecognized_option(['--foo'])
        self.run_command_unrecognized_option(['--version'])

    def test_multiple_devices(self):
        self.run_command_success(['--device', '/dev/input/event0', '/dev/input/event1'])
        # same event path multiple times? meh, your problem
        self.run_command_success(['--device', '/dev/input/event0', '/dev/input/event0'])
        self.run_command_success(['/dev/input/event0', '/dev/input/event1'])

    def test_too_many_devices(self):
        # Too many arguments just bails with the usage message
        rc, stdout, stderr = self.run_command(['/dev/input/event0'] * 61)
        self.assertEqual(rc, 2, msg=(stdout, stderr))


class TestDebugGUI(TestToolWithOptions, TestLibinputTool):
    subtool = 'debug-gui'

    @classmethod
    def setUpClass(cls):
        # This is set by meson
        debug_gui_enabled = @MESON_ENABLED_DEBUG_GUI@ # noqa
        if not debug_gui_enabled:
            raise unittest.SkipTest()

        if not os.getenv('DISPLAY') and not os.getenv('WAYLAND_DISPLAY'):
            raise unittest.SkipTest()

        # 77 means gtk_init() failed, which is probably because you can't
        # connect to the display server.
        rc, _, _ = run_command([TestLibinputTool.libinput_tool, cls.subtool, '--help'])
        if rc == 77:
            raise unittest.SkipTest()

    def test_verbose_quiet(self):
        rc, stdout, stderr = self.run_command(['--verbose'])
        self.assertEqual(rc, 0)

    def test_invalid_arguments(self):
        self.run_command_unrecognized_option(['--quiet'])
        self.run_command_unrecognized_option(['--banana'])
        self.run_command_unrecognized_option(['--foo'])
        self.run_command_unrecognized_option(['--version'])


class TestRecord(TestLibinputTool):
    subtool = 'record'

    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.outfile = Path(self.tmpdir.name, 'record.out')

    def tearDown(self):
        self.tmpdir.cleanup()

    def test_args(self):
        self.run_command_success(['--help'])
        self.run_command_success(['--show-keycodes'])
        self.run_command_success(['--with-libinput'])

    def test_multiple_deprecated(self):
        # this arg is deprecated and a noop
        self.run_command_success(['--multiple'])

    def test_all(self):
        self.run_command_success(['--all', '-o', self.outfile])

    def test_autorestart(self):
        self.run_command_success(['--autorestart=2'])

    def test_outfile(self):
        self.run_command_success(['-o', self.outfile])
        self.run_command_success(['--output-file', self.outfile])
        self.run_command_success(['--output-file={}'.format(self.outfile)])

    def test_device_single(self):
        self.run_command_success(['/dev/input/event0'])

    def test_device_multiple(self):
        self.run_command_success(['-o', self.outfile, '/dev/input/event0', '/dev/input/event1'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Verify a libinput tool\'s option parsing')
    parser.add_argument('--tool-path', metavar='/path/to/builddir/libinput',
                        type=str,
                        help='Path to the libinput tool in the builddir')
    parser.add_argument('--verbose', action='store_true')
    args, remainder = parser.parse_known_args()
    if args.tool_path is not None:
        TestLibinputTool.libinput_tool = args.tool_path
    verbosity = 1
    if args.verbose:
        verbosity = 3

    argv = [sys.argv[0], *remainder]
    unittest.main(verbosity=verbosity, argv=argv)
