.. _test-suite:

==============================================================================
libinput test suite
==============================================================================

libinput ships with a number of tests all run automatically on ``ninja test``.
The primary test suite is the ``libinput-test-suite``. When testing,
the ``libinput-test-suite`` should always be invoked to check for
behavior changes. The test suite relies on the kernel and udev to function
correctly. It is not suitable for running inside containers.

.. note:: ``ninja test`` runs more than just the test suite, you **must**
	run all tests for full coverage.

The test suite runner uses
`Check <http://check.sourceforge.net/doc/check_html/>`_ underneath the hood
but most of the functionality is abstracted into *litest* wrappers.

The test suite runner has a make-like job control enabled by the ``-j`` or
``--jobs`` flag and will fork off as many parallel processes as given by this
flag. The default if unspecified is 8. When debugging a specific test case
failure it is recommended to employ test filtures (see :ref:`test-filtering`)
and disable parallel tests. The test suite automatically disables parallel
make when run in gdb.

.. _test-config:

------------------------------------------------------------------------------
X.Org config to avoid interference
------------------------------------------------------------------------------

uinput devices created by the test suite are usually recognised by X as
input devices. All events sent through these devices will generate X events
and interfere with your desktop.

Copy the file ``$srcdir/test/50-litest.conf`` into your ``/etc/X11/xorg.conf.d``
and restart X. This will ignore any litest devices and thus not interfere
with your desktop.

.. _test-root:

------------------------------------------------------------------------------
Permissions required to run tests
------------------------------------------------------------------------------

Most tests require the creation of uinput devices and access to the
resulting ``/dev/input/eventX`` nodes. Some tests require temporary udev rules.
**This usually requires the tests to be run as root**. If not run as
root, the test suite runner will exit with status 77, interpreted as
"skipped" by ninja.

.. _test-filtering:

------------------------------------------------------------------------------
Selective running of tests
------------------------------------------------------------------------------

litest's tests are grouped into test groups, test names and devices. A test
group is e.g.  "touchpad:tap" and incorporates all tapping-related tests for
touchpads. Each test function is (usually) run with one or more specific
devices. The ``--list`` commandline argument shows the list of suites and
tests. This is useful when trying to figure out if a specific test is
run for a device.


::

     $ ./builddir/libinput-test-suite --list
     ...
     pointer:left-handed:
	pointer_left_handed_during_click_multiple_buttons:
		trackpoint
		ms-surface-cover
		mouse-wheelclickcount
		mouse-wheelclickangle
		low-dpi-mouse
		mouse-roccat
		mouse-wheel-tilt
		mouse
		logitech-trackball
		cyborg-rat
		magicmouse
	pointer_left_handed_during_click:
		trackpoint
		ms-surface-cover
		mouse-wheelclickcount
		mouse-wheelclickangle
		low-dpi-mouse
		mouse-roccat
		mouse-wheel-tilt
		mouse
		logitech-trackball
		cyborg-rat
		litest-magicmouse-device
	pointer_left_handed:
		trackpoint
		ms-surface-cover
		mouse-wheelclickcount
		mouse-wheelclickangle
		low-dpi-mouse
		mouse-roccat
		mouse-wheel-tilt
		mouse
     ...


In the above example, the "pointer:left-handed" suite contains multiple
tests, e.g. "pointer_left_handed_during_click" (this is also the function
name of the test, making it easy to grep for). This particular test is run
for various devices including the trackpoint device and the magic mouse
device.

The "no device" entry signals that litest does not instantiate a uinput
device for a specific test (though the test itself may
instantiate one).

The ``--filter-test`` argument enables selective running of tests through
basic shell-style function name matching. For example:


::

     $ ./builddir/libinput-test-suite --filter-test="*1fg_tap*"


The ``--filter-device`` argument enables selective running of tests through
basic shell-style device name matching. The device names matched are the
litest-specific shortnames, see the output of ``--list``. For example:


::

     $ ./builddir/libinput-test-suite --filter-device="synaptics*"


The ``--filter-group`` argument enables selective running of test groups
through basic shell-style test group matching. The test groups matched are
litest-specific test groups, see the output of ``--list``. For example:


::

     $ ./builddir/libinput-test-suite --filter-group="touchpad:*hover*"


The ``--filter-device`` and ``--filter-group`` arguments can be combined with
``--list`` to show which groups and devices will be affected.

.. _test-verbosity:

------------------------------------------------------------------------------
Controlling test output
------------------------------------------------------------------------------

Each test supports the ``--verbose`` commandline option to enable debugging
output, see **libinput_log_set_priority()** for details. The ``LITEST_VERBOSE``
environment variable, if set, also enables verbose mode.


::

     $ ./builddir/libinput-test-suite --verbose
     $ LITEST_VERBOSE=1 ninja test

.. _test-installed:

------------------------------------------------------------------------------
Installing the test suite
------------------------------------------------------------------------------

If libinput is configured to install the tests, the test suite is available
as the ``libinput test-suite`` command. When run as installed binary, the
behavior of the test suite changes:

- the ``libinput.so`` used is the one in the library lookup paths
- no system-wide quirks are installed by the test suite, only those specific
  to the test devices
- test device-specific quirks are installed in the system-wide quirks
  directory, usually ``/usr/share/libinput/``.

It is not advisable to run ``libinput test-suite`` on a production machine.
Data loss may occur. The primary use-case for the installed test suite is
verification of distribution composes.

.. note:: The ``prefix`` is still used by the test suite. For verification
	of a system package, the test suite must be configured with the same prefix.

To configure libinput to install the tests, use the ``-Dinstall-tests=true``
meson option::

  $ meson builddir -Dtests=true -Dinstall-tests=true <other options>
