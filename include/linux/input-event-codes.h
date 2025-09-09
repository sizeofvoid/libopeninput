#ifdef __linux__
#include "linux/input-event-codes.h"
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
#include "freebsd/input-event-codes.h"
#endif
