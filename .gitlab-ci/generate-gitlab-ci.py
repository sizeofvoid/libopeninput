#!/usr/bin/env python3
# vim: set expandtab shiftwidth=4:

# This file generates the .gitlab-ci.yml file that defines the pipeline.

import jinja2

distributions = [
    {'name': 'fedora', 'version': '30'},
    {'name': 'fedora', 'version': '31'},
    {'name': 'ubuntu', 'version': '19.10'},
    {'name': 'ubuntu', 'version': '19.04'},
    {'name': 'arch', 'version': 'rolling',
     'flavor': 'archlinux'},  # see https://gitlab.freedesktop.org/wayland/ci-templates/merge_requests/19
    {
        'name': 'alpine', 'version': 'latest',
        'build': {
            'extra_variables': [
                'MESON_ARGS: \'-Ddocumentation=false\' # alpine does not have python-recommonmark',
                # We don't run the tests on alpine. The litest-selftest fails
                # for any tcase_add_exit_test/tcase_add_test_raise_signal
                # but someone more invested in musl will have to figure that out.
                'MESON_TEST_ARGS: \'\' # litest-selftest fails on musl',
            ]
        },
    }
]

templates = sorted(set([x['name'] for x in distributions]))

# in reverse order of duration to get the slowest ones started first
test_suites = [
    {'name': 'touchpad', 'suites': 'touchpad'},
    {'name': 'tap', 'suites': 'tap'},
    {'name': 'tablet', 'suites': 'tablet'},
    {'name': 'gestures-device', 'suites': 'gestures device'},
    {'name': 'others',
     'suites': 'context config misc events totem udev lid log timer tablet-mode quirks trackball pad path keyboard switch touch trackpoint'},
    {'name': 'pointer', 'suites': 'pointer'}
]


def generate_template():
    env = jinja2.Environment(loader=jinja2.FileSystemLoader('./.gitlab-ci'),
                             trim_blocks=True, lstrip_blocks=True)

    template = env.get_template('gitlab-ci.tmpl')
    config = {'distributions': distributions,
              'test_suites': test_suites,
              'templates': templates}
    with open('.gitlab-ci.yml', 'w') as fd:
        template.stream(config).dump(fd)


if __name__ == '__main__':
    generate_template()
