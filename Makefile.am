SUBDIRS = src doc test tools udev

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

valgrind:
	(cd test; $(MAKE) valgrind)

AM_DISTCHECK_CONFIGURE_FLAGS = --disable-test-run
