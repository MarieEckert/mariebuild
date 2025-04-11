/* stringutil.c ; mariebuild build string utilities
 *
 * Copyright (c) 2025, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#define _XOPEN_SOURCE	700
#define _POSIX_C_SOURCE 2

#include "stringutil.h"
#include <stdlib.h>
#include <string.h>

bool
string_cptrlist_search(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	if(str_a == str_b) {
		return true;
	}
	if(str_a == NULL || str_b == NULL) {
		return false;
	}

	return strcmp(str_a, str_b) == 0;
}
