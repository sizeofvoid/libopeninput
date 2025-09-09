#ifdef __linux__
#include "linux/input.h"
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
#include "freebsd/input.h"
#endif
