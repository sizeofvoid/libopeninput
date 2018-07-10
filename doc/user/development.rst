.. _development:

==============================================================================
Information for developers
==============================================================================

Below is a list of topics of interest to developers, divided into
information for those **using** libinput as library in a Wayland compositor
or other project. The :ref:`hacking_on_libinput` section applies to developers working on
libinput itself.

.. _using_libinput_as_library:

*************************
Using libinput as library
*************************

.. note:: If you use libinput you should get in touch with the libinput
          developers on the wayland-devel@lists.freedesktop.org mailing
	  lists

The API documentation is available here:
    http://wayland.freedesktop.org/libinput/doc/latest/api/

Topics below explain some behaviors of libinput.

.. toctree::
   :maxdepth: 1

   absolute-axes.rst
   absolute-coordinate-ranges.rst
   normalization-of-relative-motion.rst
   seats.rst
   timestamps.rst

.. _hacking_on_libinput:

*******************
Hacking on libinput
*******************

.. toctree::
   :maxdepth: 1

   contributing.rst
   architecture
   test-suite.rst
   pointer-acceleration.rst
   device-configuration-via-udev.rst
