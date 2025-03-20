/* xmem.h ; mariebuild memory helpers header
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#ifndef XMEM_H
#define XMEM_H

#include <stdlib.h>

#include <stdio.h>

#include "logging.h"

#define _STR(x) #x
#define STR(x)	_STR(x)

#define PANIC(s)                    \
	do {                            \
		fprintf(stderr, "%s\n", s); \
		abort();                    \
	} while(0)

#define XMALLOC(s)                           \
	({                                       \
		void *ret;                           \
		ret = malloc(s);                     \
		if(ret == NULL) {                    \
			PANIC("XMALLOC returned NULL!"); \
		}                                    \
		ret;                                 \
	})

#define XCALLOC(nmemb, s)                    \
	({                                       \
		void *ret;                           \
		ret = calloc(nmemb, s);              \
		if(ret == NULL) {                    \
			PANIC("XCALLOC returned NULL!"); \
		}                                    \
		ret;                                 \
	})

#define XREALLOC(o, s)                        \
	({                                        \
		void *ret;                            \
		ret = realloc(o, s);                  \
		if(ret == NULL) {                     \
			PANIC("XREALLOC returned NULL!"); \
		}                                     \
		ret;                                  \
	})

#define XFREE(p)                                                         \
	do {                                                                 \
		if(p != NULL) {                                                  \
			free(p);                                                     \
		} else {                                                         \
			mb_log(LOG_ERROR,                                            \
				   __FILE__ ":" STR(__LINE__) ": XFREE received NULL!"); \
		}                                                                \
	} while(0)

#endif /* #ifndef XMEM_H */
