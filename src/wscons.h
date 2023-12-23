#ifndef _LIBINPUT_WSCONS_H
#define _LIBINPUT_WSCONS_H

#include <dev/wscons/wsconsio.h>

#include "libinput-private.h"

#define WSCONS_TYPE_POINTER 1
#define WSCONS_TYPE_KEYBOARD 2

struct TransMapRec {
    int begin;
    int end;
    uint32_t *map;
};

struct wscons_device {
	struct libinput_device base;
	enum libinput_device_capability capability;
	struct TransMapRec *scanCodeMap;
};


static inline struct wscons_device *
wscons_device(struct libinput_device *device)
{
	return container_of(device, struct wscons_device, base);
}
extern int wscons_keyboard_init(struct wscons_device *);
extern uint32_t wskey_transcode(struct TransMapRec *, int);
extern void post_device_event(struct libinput_device *, uint64_t ,
    enum libinput_event_type , struct libinput_event *);

#endif
