#ifndef _LIBINPUT_WSCONS_H
#define _LIBINPUT_WSCONS_H

#include <dev/wscons/wsconsio.h>

#include "libinput-private.h"

struct wscons_device {
	struct libinput_device base;
	int wskbd_type;
};

static inline struct wscons_device *
wscons_device(struct libinput_device *device)
{
	return container_of(device, struct wscons_device, base);
}

extern uint32_t wskey_transcode(int, int);
extern void post_device_event(struct libinput_device *, uint64_t ,
    enum libinput_event_type , struct libinput_event *);

#endif
