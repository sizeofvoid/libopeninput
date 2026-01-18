-- SPDX-License-Identifier: MIT
--
-- This is an example libinput plugin
--
-- This plugin controls a mouse/pointer from a tablet device. This
-- effectively hides stylus interactions and sends pointer events
-- instead. In other words: mouse emulation for tablets, implemented
-- by (remote) controlling a mouse device. This allows using a
-- tablet stylus as a mouse replacement without tablet limitations
-- from compositors or clients. Note that axis usually needed for
-- drawing (like pressure, tilt or distance) are no longer emitted
-- when this plugin is active and a mouse is connected. When no
-- mouse is connected, this plugin doesn't change tablet events,
-- thus the stylus works like a normal stylus.

-- UNCOMMENT THIS LINE TO ACTIVATE THE PLUGIN
-- libinput:register({1})

-- globals
pointer_device = nil
tablet_device = nil
maximum_x = nil
maximum_y = nil

function adjust_for_aspect_ratio(y)
    -- adjust y to match monitor 21:9 aspect ratio
    local adj_maximum_y = maximum_x * 1440 / 3440
    return math.floor(math.min(y * maximum_y / adj_maximum_y, maximum_y + 1))
end

function on_tablet_frame(device, frame, time_in_microseconds)
    -- emit tablet frame when there is no pointer device
    if not pointer_device then return nil end

    -- map tablet frame to pointer frame
    local events = {}
    for _, v in ipairs(frame) do
        if v.usage == evdev.ABS_MISC then
            -- save a few cycles on Wacom tablets by discarding a
            -- proximity in / out frame early, non-Wacom tablets should
            -- use BTN_TOOL_PEN/RUBBER/... instead
            return {}
        elseif v.usage == evdev.ABS_X then
            table.insert(events, { usage = evdev.ABS_X, value = v.value })
        elseif v.usage == evdev.ABS_Y then
            -- uncomment the next two lines and comment the original line
            -- for configuring aspect correction, see
            -- adjust_for_aspect_ratio() for details and configuration

            -- local adj_value = adjust_for_aspect_ratio(v.value)
            -- table.insert(events, { usage = evdev.ABS_Y, value = adj_value })
            table.insert(events, { usage = evdev.ABS_Y, value = v.value })
        elseif v.usage == evdev.BTN_TOUCH then
            table.insert(events, { usage = evdev.BTN_LEFT, value = v.value })
        elseif v.usage == evdev.BTN_STYLUS then
            table.insert(events, { usage = evdev.BTN_RIGHT, value = v.value })
        elseif v.usage == evdev.BTN_STYLUS2 then
            table.insert(events, { usage = evdev.BTN_MIDDLE, value = v.value })
        end
    end

    -- emit pointer frame, if any
    if #events > 0 then pointer_device:append_frame(events) end

    -- discard tablet frame
    return {}
end

function on_tablet_removed(device)
    libinput:log_info("Remove tablet device")
    tablet_device = nil
end

function on_pointer_removed(device)
    libinput:log_info("Remove pointer device")
    pointer_device = nil
end

function setup()
    if not pointer_device or not tablet_device then return end

    libinput:log_info("Controlling '" .. pointer_device:name() .. "' with '" .. tablet_device:name() .. "'")

    -- fetch absinfos from tablet
    local absinfo_x = {}
    local absinfo_y = {}
    for a, b in pairs(tablet_device:absinfos()) do
        if a == evdev.ABS_X then absinfo_x = b end
        if a == evdev.ABS_Y then absinfo_y = b end
    end

    -- copy max values for aspect ratio correction later on
    maximum_x = absinfo_x.maximum
    maximum_y = absinfo_y.maximum

    -- copy absinfos to pointer device
    pointer_device:set_absinfo(evdev.ABS_X, absinfo_x)
    pointer_device:set_absinfo(evdev.ABS_Y, absinfo_y)

    -- setup listeners
    pointer_device:connect("device-removed", on_pointer_removed)
    tablet_device:connect("device-removed", on_tablet_removed)
    tablet_device:connect("evdev-frame", on_tablet_frame)
end

function on_new_device(device)
    local udev = device:udev_properties()
    if udev["ID_INPUT_TABLET"] and not udev["ID_INPUT_TABLET_PAD"] then
        libinput:log_info("Found tablet device")
        tablet_device = device
        setup()
    end
    if udev["ID_INPUT_MOUSE"] then
        libinput:log_info("Found pointer device")
        pointer_device = device
        setup()
    end
end

-- setup listener
libinput:connect("new-evdev-device", on_new_device)
