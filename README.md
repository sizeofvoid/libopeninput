libinput
========

libinput is a library that provides a full input stack for display servers
and other applications that need to handle input devices provided by the
kernel.

libinput provides device detection, event handling and abstraction so
minimize the amount of custom input code the user of libinput need to
provide the common set of functionality that users expect. Input event
processing includes scaling touch coordinates, generating
relative pointer events from touchpads, pointer acceleration, etc.

Architecture
------------

libinput is not used directly by applications. Think of it more as a device
driver than an application library. It is used by the xf86-input-libinput
X.Org driver or Wayland compositors. The typical software stack for a system
running Wayland is:

@dotfile libinput-stack-wayland.gv

The Wayland compositor may be Weston, mutter, KWin, etc. Note that
Wayland encourages the use of toolkits, so the Wayland client (your
application) does not usually talk directly to the compositor but rather
employs a toolkit (e.g. GTK) to do so. The Wayland client does not know
whether libinput is in use.

The simplified software stack for a system running X.Org is:

@dotfile libinput-stack-xorg.gv

libinput is not employed directly by the X server but by the
xf86-input-libinput driver instead. That driver is loaded by the server
on demand, depending on the xorg.conf.d configuration snippets. The X client
does not know whether libinput is in use.

Source code
-----------

The source code of libinput can be found at:
http://cgit.freedesktop.org/wayland/libinput

For a list of current and past releases visit:
http://www.freedesktop.org/wiki/Software/libinput/

Build instructions:
http://wayland.freedesktop.org/libinput/doc/latest/building_libinput.html

Reporting Bugs
--------------

Bugs can be filed in the libinput component of Wayland:
https://bugs.freedesktop.org/enter_bug.cgi?product=Wayland&component=libinput

Where possible, please provide the `libinput record` output
of the input device and/or the event sequence in question.

See @ref reporting_bugs for more info.

Documentation
-------------

- Developer API documentation: http://wayland.freedesktop.org/libinput/doc/latest/modules.html
- High-level documentation about libinput's features:
http://wayland.freedesktop.org/libinput/doc/latest/pages.html
- Build instructions:
http://wayland.freedesktop.org/libinput/doc/latest/building_libinput.html
- Documentation for previous versions of libinput: https://wayland.freedesktop.org/libinput/doc/

Examples of how to use libinput are the debugging tools in the libinput
repository. Developers are encouraged to look at those tools for a
real-world (yet simple) example on how to use libinput.

- A commandline debugging tool: https://cgit.freedesktop.org/wayland/libinput/tree/tools/libinput-debug-events.c
- A GTK application that draws cursor/touch/tablet positions: https://cgit.freedesktop.org/wayland/libinput/tree/tools/libinput-debug-gui.c

License
-------

libinput is licensed under the MIT license.

> Permission is hereby granted, free of charge, to any person obtaining a
> copy of this software and associated documentation files (the "Software"),
> to deal in the Software without restriction, including without limitation
> the rights to use, copy, modify, merge, publish, distribute, sublicense,
> and/or sell copies of the Software, and to permit persons to whom the
> Software is furnished to do so, subject to the following conditions: [...]

See the [COPYING](http://cgit.freedesktop.org/wayland/libinput/tree/COPYING)
file for the full license information.
