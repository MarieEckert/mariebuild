/* signals.c ; mariebuild build signal handling
 *
 * Copyright (c) 2025, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#define _XOPEN_SOURCE	700
#define _POSIX_C_SOURCE 2

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cptrlist.h"
#include "logging.h"
#include "signals.h"
#include "stringutil.h"

#define SIGNAL_CHECKED(s, h)                                                 \
	do {                                                                     \
		if(signal(s, h) == SIG_ERR) {                                        \
			mb_logf(LOG_WARNING, "failed to install handle for signal %d\n", \
					s);                                                      \
			perror("signal failure");                                        \
		}                                                                    \
	} while(0)

CPtrList tmp_files;
bool initialised = false;

void
mb_signal_generic_handler(int signal)
{
	mb_logf(LOG_ERROR, "signal %d received, quitting...\n", signal);
	for(size_t ix = 0; ix < tmp_files.size; ix++) {
		char *item = tmp_files.items[ix];
		if(item == NULL) {
			continue;
		}

		remove(item);
	}

	exit(-1);
}

void
mb_install_signal_handlers(void)
{
	SIGNAL_CHECKED(SIGHUP, &mb_signal_generic_handler);
	SIGNAL_CHECKED(SIGINT, &mb_signal_generic_handler);
	SIGNAL_CHECKED(SIGQUIT, &mb_signal_generic_handler);
	SIGNAL_CHECKED(SIGTERM, &mb_signal_generic_handler);

	cptrlist_init(&tmp_files, 16, 8);
	initialised = true;
}

void
mb_register_tmp_file(char *path)
{
	if(!initialised) {
		return;
	}

	cptrlist_append(&tmp_files, strdup(path));
}

void
mb_unregister_tmp_file(char *path)
{
	size_t ix = cptrlist_find(&tmp_files, path, &string_cptrlist_search);
	cptrlist_free_at(&tmp_files, ix);
}
