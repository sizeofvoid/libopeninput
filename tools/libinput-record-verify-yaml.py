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
import sys
import unittest
import yaml
import re

from pkg_resources import parse_version


class TestYaml(unittest.TestCase):
    filename = ''

    @classmethod
    def setUpClass(cls):
        with open(cls.filename) as f:
            cls.yaml = yaml.safe_load(f)

    def test_sections_exist(self):
        sections = ['version', 'ndevices', 'libinput', 'system', 'devices']
        for section in sections:
            self.assertIn(section, self.yaml)

    def test_version(self):
        version = self.yaml['version']
        self.assertTrue(isinstance(version, int))
        self.assertEqual(version, 1)

    def test_ndevices(self):
        ndevices = self.yaml['ndevices']
        self.assertTrue(isinstance(ndevices, int))
        self.assertGreaterEqual(ndevices, 1)
        self.assertEqual(ndevices, len(self.yaml['devices']))

    def test_libinput(self):
        libinput = self.yaml['libinput']
        version = libinput['version']
        self.assertTrue(isinstance(version, str))
        self.assertGreaterEqual(parse_version(version), parse_version('1.10.0'))
        git = libinput['git']
        self.assertTrue(isinstance(git, str))
        self.assertNotEqual(git, 'unknown')

    def test_system(self):
        system = self.yaml['system']
        kernel = system['kernel']
        self.assertTrue(isinstance(kernel, str))
        self.assertEqual(kernel, os.uname().release)

        dmi = system['dmi']
        self.assertTrue(isinstance(dmi, str))
        with open('/sys/class/dmi/id/modalias') as f:
            sys_dmi = f.read()[:-1]  # trailing newline
            self.assertEqual(dmi, sys_dmi)

    def test_devices_sections_exist(self):
        devices = self.yaml['devices']
        for d in devices:
            self.assertIn('node', d)
            self.assertIn('evdev', d)
            self.assertIn('udev', d)

    def test_evdev_sections_exist(self):
        sections = ['name', 'id', 'codes', 'properties']
        devices = self.yaml['devices']
        for d in devices:
            evdev = d['evdev']
            for s in sections:
                self.assertIn(s, evdev)

    def test_evdev_name(self):
        devices = self.yaml['devices']
        for d in devices:
            evdev = d['evdev']
            name = evdev['name']
            self.assertTrue(isinstance(name, str))
            self.assertGreaterEqual(len(name), 5)

    def test_evdev_id(self):
        devices = self.yaml['devices']
        for d in devices:
            evdev = d['evdev']
            id = evdev['id']
            self.assertTrue(isinstance(id, list))
            self.assertEqual(len(id), 4)
            self.assertGreater(id[0], 0)
            self.assertGreater(id[1], 0)

    def test_evdev_properties(self):
        devices = self.yaml['devices']
        for d in devices:
            evdev = d['evdev']
            properties = evdev['properties']
            self.assertTrue(isinstance(properties, list))

    def test_udev_sections_exist(self):
        sections = ['properties']
        devices = self.yaml['devices']
        for d in devices:
            udev = d['udev']
            for s in sections:
                self.assertIn(s, udev)

    def test_udev_properties(self):
        devices = self.yaml['devices']
        for d in devices:
            udev = d['udev']
            properties = udev['properties']
            self.assertTrue(isinstance(properties, list))
            self.assertGreater(len(properties), 0)

            self.assertIn('ID_INPUT=1', properties)
            for p in properties:
                self.assertTrue(re.match('[A-Z0-9_]+=.+', p))

    def test_udev_id_inputs(self):
        devices = self.yaml['devices']
        for d in devices:
            udev = d['udev']
            properties = udev['properties']
            id_inputs = [p for p in properties if p.startswith('ID_INPUT')]
            # We expect ID_INPUT and ID_INPUT_something, but might get more
            # than one of the latter
            self.assertGreaterEqual(len(id_inputs), 2)

    def test_events_have_evdev(self):
        devices = self.yaml['devices']
        for d in devices:
            events = d['events']
            for e in events:
                self.assertIn('evdev', e)

    def test_events_evdev(self):
        devices = self.yaml['devices']
        for d in devices:
            events = d['events']
            for e in events:
                evdev = e['evdev']
                for ev in evdev:
                    self.assertEqual(len(ev), 5)

                # Last event in each frame is SYN_REPORT
                ev_syn = evdev[-1]
                self.assertEqual(ev_syn[2], 0)
                self.assertEqual(ev_syn[3], 0)
                self.assertEqual(ev_syn[4], 0)

    def test_events_evdev_syn_report(self):
        devices = self.yaml['devices']
        for d in devices:
            events = d['events']
            for e in events:
                evdev = e['evdev']
                for ev in evdev[:-1]:
                    self.assertFalse(ev[2] == 0 and ev[3] == 0)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Verify a YAML recording')
    parser.add_argument('recording', metavar='recorded-file.yaml',
                        type=str, help='Path to device recording')
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()
    TestYaml.filename = args.recording
    verbosity = 1
    if args.verbose:
        verbosity = 3
    del sys.argv[1:]
    unittest.main(verbosity=verbosity)
