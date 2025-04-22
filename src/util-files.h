/*
 * Copyright © 2024 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "config.h"

#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>

#include "util-strings.h"

static inline int
mkdir_p(const char *dir)
{
	char *path, *parent;
	int rc;

	if (streq(dir, "/"))
		return 0;

	path = safe_strdup(dir);
	parent = dirname(path);
	rc = mkdir_p(parent);
	free(path);

	if (rc < 0)
		return rc;

	rc = mkdir(dir, 0755);

	return (rc == -1 && errno != EEXIST) ? -errno : 0;
}

static inline void
xclose(int *fd)
{
	if (*fd > -1) {
		close(*fd);
		*fd = -1;
	}
}

/**
 * In the NULL-terminated list of directories
 * search for files with the given suffix and return
 * a filename-ordered NULL-terminated list of those
 * full paths.
 *
 * The directories are given in descending priority order.
 * Any file with a given filename shadows the same file
 * in another directory of lower sorting order.
 *
 * If nfiles is not NULL, it is set to the number of
 * files returned (not including the NULL terminator).
 */
char **
list_files(const char **directories,
	   const char *suffix,
	   size_t *nfiles);
