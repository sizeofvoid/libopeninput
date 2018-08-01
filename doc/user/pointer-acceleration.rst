.. _pointer-acceleration:

==============================================================================
 Pointer acceleration
==============================================================================

libinput uses device-specific pointer acceleration methods, with the default
being the :ref:`ptraccel-linear`. The methods share common properties, such as
:ref:`ptraccel-velocity`.

This page explains the high-level concepts used in the code. It aims to
provide an overview for developers and is not necessarily useful for
users.

.. _ptraccel-profiles:

------------------------------------------------------------------------------
Pointer acceleration profiles
------------------------------------------------------------------------------

The profile decides the general method of pointer acceleration.
libinput currently supports two profiles: "adaptive" and "flat". The adaptive
profile is the default profile for all devices and takes the current speed
of the device into account when deciding on acceleration. The flat profile
is simply a constant factor applied to all device deltas, regardless of the
speed of motion (see :ref:`ptraccel-profile-flat`). Most of this document
describes the adaptive pointer acceleration.

.. _ptraccel-velocity:

------------------------------------------------------------------------------
Velocity calculation
------------------------------------------------------------------------------

The device's speed of movement is measured across multiple input events
through so-called "trackers". Each event prepends a the tracker item, each
subsequent tracker contains the delta of that item to the current position,
the timestamp of the event that created it and the cardinal direction of the
movement at the time. If a device moves into the same direction, the
velocity is calculated across multiple trackers. For example, if a device
moves steadily for 10 events to the left, the velocity is calculated across
all 10 events.

Whenever the movement changes direction or significantly changes speed, the
velocity is calculated from the direction/speed change only. For example, if
a device moves steadily for 8 events to the left and then 2 events to the
right, the velocity is only that of the last 2 events.

An extra time limit prevents events that are too old to factor into the
velocity calculation. For example, if a device moves steadily for 5 events
to the left, then pauses, then moves again for 5 events to the left, only
the last 5 events are used for velocity calculation.

The velocity is then used to calculate the acceleration factor

.. _ptraccel-factor:

------------------------------------------------------------------------------
Acceleration factor
------------------------------------------------------------------------------

The acceleration factor is the final outcome of the pointer acceleration
calculations. It is a unitless factor that is applied to the current delta,
a factor of 2 doubles the delta (i.e. speeds up the movement), a factor of
less than 1 reduces the delta (i.e. slows the movement).

Any factor less than 1 requires the user to move the device further to move
the visible pointer. This is called deceleration and enables high precision
target selection through subpixel movements. libinput's current maximum
deceleration factor is 0.3 (i.e. slow down to 30% of the pointer speed).

A factor higher than 1 moves the pointer further than the physical device
moves. This is acceleration and allows a user to cross the screen quickly
but effectively skips pixels. libinput's current maximum acceleration factor
is 3.5.

.. _ptraccel-linear:

------------------------------------------------------------------------------
Linear pointer acceleration
------------------------------------------------------------------------------

The linear pointer acceleration method is the default for most pointer
devices. It provides deceleration at very slow movements, a 1:1 mapping for
regular movements and a linear increase to the maximum acceleration factor
for fast movements.

Linear pointer acceleration applies to devices with above 1000dpi resolution
and after :ref:`motion_normalization` is applied.

.. figure:: ptraccel-linear.svg
    :align: center

    Linear pointer acceleration

The image above shows the linear pointer acceleration settings at various
speeds. The line for 0.0 is the default acceleration curve, speed settings
above 0.0 accelerate sooner, faster and to a higher maximum acceleration.
Speed settings below 0 delay when acceleration kicks in, how soon the
maximum acceleration is reached and the maximum acceleration factor.

Extremely low speed settings provide no acceleration and additionally
decelerate all movement by a constant factor.

.. _ptraccel-low-dpi:

------------------------------------------------------------------------------
Pointer acceleration for low-dpi devices
------------------------------------------------------------------------------

Low-dpi devices are those with a physical resolution of less than 1000 dots
per inch (dpi). The pointer acceleration is adjusted to provide roughly the
same feel for all devices at normal to high speeds. At slow speeds, the
pointer acceleration works on device-units rather than normalized
coordinates (see :ref:`motion_normalization`).

.. figure:: ptraccel-low-dpi.svg
    :align: center

    Pointer acceleration for low-dpi devices

The image above shows the default pointer acceleration curve for a speed of
0.0 at different DPI settings. A device with low DPI has the acceleration
applied sooner and with a stronger acceleration factor.

.. _ptraccel-touchpad:

------------------------------------------------------------------------------
Pointer acceleration on touchpads
------------------------------------------------------------------------------

Touchpad pointer acceleration uses the same approach as the
:ref:`ptraccel-linear` profile, with a constant deceleration factor applied. The
user expectation of how much a pointer should move in response to finger
movement is different to that of a mouse device, hence the constant
deceleration factor.

.. figure:: ptraccel-touchpad.svg
    :align: center

    Pointer acceleration curve for touchpads

The image above shows the touchpad acceleration profile in comparison to the
:ref:`ptraccel-linear`. The shape of the curve is identical but vertically squashed.

.. _ptraccel-trackpoint:

------------------------------------------------------------------------------
Pointer acceleration on trackpoints
------------------------------------------------------------------------------

The main difference between trackpoint hardware and mice or touchpads is
that trackpoint speed is a function of pressure rather than moving speed.
But trackpoint hardware is quite varied in how it reacts to user pressure
and unlike other devices it cannot easily be normalized for physical
properties. Measuring pressure objectively across a variety of hardware is
nontrivial. See :ref:`trackpoints` for more details.

The deltas for trackpoints are converted units/ms but there is no common
physical reference point for a unit. Thus, the same pressure on different
trackpoints will generate different speeds and thus different acceleration
behaviors. Additionally, some trackpoints provide the ability to adjust the
sensitivity in hardware by modifying a sysfs file on the serio node. A
higher sensitivity results in higher deltas, thus changing the definition of
what is a unit again.

libinput attempts to normalize unit data to the best of its abilities, see
:ref:`trackpoint_multiplier`. Beyond this, it is not possible to have
consistent behavior across different touchpad devices.

.. figure:: ptraccel-trackpoint.svg
    :align: center

    Pointer acceleration curves for trackpoints

The image above shows the trackpoint acceleration profile for the speed in
units/ms.

.. _ptraccel-profile-flat:

------------------------------------------------------------------------------
The flat pointer acceleration profile
------------------------------------------------------------------------------

In a flat profile, the acceleration factor is constant regardless of the
velocity of the pointer and each delta (dx, dy) results in an accelerated delta
(dx * factor, dy * factor). This provides 1:1 movement between the device
and the pointer on-screen.

.. _ptraccel-tablet:

------------------------------------------------------------------------------
Pointer acceleration on tablets
------------------------------------------------------------------------------

Pointer acceleration for relative motion on tablet devices is a flat
acceleration, with the speed setting slowing down or speeding up the pointer
motion by a constant factor. Tablets do not allow for switchable profiles.
