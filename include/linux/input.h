#ifdef __linux__
#include "linux/input.h"
#elif __OpenBSD__
#include "freebsd/input.h"
#elif __FreeBSD__
#include "freebsd/input.h"
#endif
