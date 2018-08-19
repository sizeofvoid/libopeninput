.. _gestures:

==============================================================================
Gestures
==============================================================================

libinput supports :ref:`gestures_pinch` and :ref:`gestures_swipe` on most
modern touchpads and other indirect touch devices. Note that libinput **does
not** support gestures on touchscreens, see :ref:`gestures_touchscreens`.

.. _gestures_lifetime:

-----------------------------------------------------------------------------
Lifetime of a gesture
-----------------------------------------------------------------------------

A gesture starts when the finger position and/or finger motion is
unambiguous as to what gesture to trigger and continues until the first
finger belonging to this gesture is lifted.

A single gesture cannot change the finger count. For example, if a user
puts down a fourth finger during a three-finger swipe gesture, libinput will
end the three-finger gesture and, if applicable, start a four-finger swipe
gesture. A caller may however decide that those gestures are semantically
identical and continue the two gestures as one single gesture.

.. _gestures_pinch:

------------------------------------------------------------------------------
Pinch gestures
------------------------------------------------------------------------------

Pinch gestures are executed when two or more fingers are located on the
touchpad and are either changing the relative distance to each other
(pinching) or are changing the relative angle (rotate). Pinch gestures may
change both rotation and distance at the same time. For such gestures,
libinput calculates a logical center for the gestures and provides the
caller with the delta x/y coordinates of that center, the relative angle of
the fingers compared to the previous event, and the absolute scale compared
to the initial finger position.

.. figure:: pinch-gestures.svg
    :align: center

    The pinch and rotate gestures

The illustration above shows a basic pinch in the left image and a rotate in
the right angle. Not shown is a movement of the logical center if the
fingers move unevenly. Such a movement is supported by libinput, it is
merely left out of the illustration.

Note that while position and angle is relative to the previous event, the
scale is always absolute and a multiplier of the initial finger position's
scale.

.. _gestures_swipe:

------------------------------------------------------------------------------
Swipe gestures
------------------------------------------------------------------------------

Swipe gestures are executed when three or more fingers are moved
synchronously in the same direction. libinput provides x and y coordinates
in the gesture and thus allows swipe gestures in any direction, including
the tracing of complex paths. It is up to the caller to interpret the
gesture into an action or limit a gesture to specific directions only.

.. figure:: swipe-gestures.svg
    :align: center

    The swipe gestures

The illustration above shows a vertical three-finger swipe. The coordinates
provided during the gesture are the movements of the logical center.

.. _gestures_touchscreens:

------------------------------------------------------------------------------
Touchscreen gestures
------------------------------------------------------------------------------

Touchscreen gestures are **not** interpreted by libinput. Rather, any touch
point is passed to the caller and any interpretation of gestures is up to
the caller or, eventually, the X or Wayland client.

Interpreting gestures on a touchscreen requires context that libinput does
not have, such as the location of windows and other virtual objects on the
screen as well as the context of those virtual objects:

.. figure:: touchscreen-gestures.svg
    :align: center

    Context-sensitivity of touchscreen gestures

In the above example, the finger movements are identical but in the left
case both fingers are located within the same window, thus suggesting an
attempt to zoom. In the right case  both fingers are located on a window
border, thus suggesting a window movement. libinput has no knowledge of the
window coordinates and thus cannot differentiate the two.

.. _gestures_softbuttons:

------------------------------------------------------------------------------
Gestures with enabled software buttons
------------------------------------------------------------------------------

If the touchpad device is a :ref:`Clickpad <touchpads_buttons_clickpads>`, it
is recommended that a caller switches to :ref:`clickfinger`.
Usually fingers placed in a :ref:`software button area <software_buttons>`
are not considered for gestures, resulting in some gestures to be
interpreted as pointer motion or two-finger scroll events.

.. figure:: pinch-gestures-softbuttons.svg
    :align: center

    Interference of software buttons and pinch gestures

In the example above, the software button area is highlighted in red. The
user executes a three-finger pinch gesture, with the thumb remaining in the
software button area. libinput ignores fingers within the software button
areas, the movement of the remaining fingers is thus interpreted as a
two-finger scroll motion.

.. _gestures_twofinger_touchpads:

------------------------------------------------------------------------------
Gestures on two-finger touchpads
------------------------------------------------------------------------------

As of kernel 4.2, many :ref:`touchpads_touch_partial_mt` provide only two
slots. This affects how gestures can be interpreted. Touchpads with only two
slots can identify two touches by position but can usually tell that there
is a third (or fourth) finger down on the touchpad - without providing
positional information for that finger.

Touchpoints are assigned in sequential order and only the first two touch
points are trackable. For libinput this produces an ambiguity where it is
impossible to detect whether a gesture is a pinch gesture or a swipe gesture
whenever a user puts the index and middle finger down first. Since the third
finger does not have positional information, it's location cannot be
determined.

.. figure:: gesture-2fg-ambiguity.svg
    :align: center

    Ambiguity of three-finger gestures on two-finger touchpads

The image above illustrates this ambiguity. The index and middle finger are
set down first, the data stream from both finger positions looks identical.
In this case, libinput assumes the fingers are in a horizontal arrangement
(the right image above) and use a swipe gesture.
