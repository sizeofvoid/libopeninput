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

#include "litest.h"
#include "util-range.h"

#define LITEST_RUNNER_DEFAULT_TIMEOUT 30

/**
 * Result returned from tests or suites.
 */
enum litest_runner_result {
	LITEST_PASS = 75,		/**< test successful */
	LITEST_FAIL = 76,		/**< test failed. Should not be returned directly,
					     Use the litest_ macros instead */
	LITEST_SKIP = 77,		/**< test was skipped */
	LITEST_NOT_APPLICABLE = 78,	/**< test does not apply */
	LITEST_TIMEOUT = 79,		/**< test aborted after timeout */
	LITEST_SYSTEM_ERROR = 80,	/**< unrelated error occurred */
};

/**
 * This struct is passed into every test.
 */
struct litest_runner_test_env {
	int rangeval;			/* The current value within the args.range (or 0) */
};

struct litest_runner_test_description {
	char name[256];			/* The name of the test */
	int rangeval;			/* The current value within the args.range (or 0) */

	/* test function and corresponding setup/teardown, if any */
	enum litest_runner_result (*func)(const struct litest_runner_test_env *);
	void (*setup)(const struct litest_runner_test_description *);
	void (*teardown)(const struct litest_runner_test_description *);

	struct {
		struct range range;	/* The range this test applies to */
		int signal;		/* expected signal for fail tests */
	} args;
};

struct litest_runner;

struct litest_runner *litest_runner_new(void);

/**
 * Default is nprocs * 2.
 * Setting this to 0 means *no* forking. Setting this to 1 means only one test
 * is run at a time but in a child process.
 */
void litest_runner_set_num_parallel(struct litest_runner *runner, size_t num_jobs);
void litest_runner_set_timeout(struct litest_runner *runner, unsigned int timeout);
void litest_runner_set_verbose(struct litest_runner *runner, bool verbose);
void litest_runner_add_test(struct litest_runner *runner,
			    const struct litest_runner_test_description *t);
enum litest_runner_result litest_runner_run_tests(struct litest_runner *runner);

void litest_runner_destroy(struct litest_runner *runner);
