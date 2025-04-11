/* types.c ; mariebuild util functions for shared type-definitions
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#include <stddef.h>
#include <string.h>

#include "logging.h"
#include "types.h"

struct build_type_id {
	char *name;
	build_type_t value;
};

struct build_type_id build_type_lookup[] = {
	{.name = "incremental", .value = BUILD_TYPE_INCREMENTAL},
	{.name = "full", .value = BUILD_TYPE_FULL}};

const size_t BUILD_TYPE_LOOKUP_SIZE =
	sizeof(build_type_lookup) / sizeof(struct build_type_id);

build_type_t
str_to_build_type(char *src, build_type_t fallback)
{
	if(src == NULL) {
		return fallback;
	}

	for(size_t ix = 0; ix < BUILD_TYPE_LOOKUP_SIZE; ix++) {
		if(strcmp(src, build_type_lookup[ix].name) == 0) {
			return build_type_lookup[ix].value;
		}
	}

	mb_logf(LOG_WARNING, "unrecognized build_type \"%s\"\n", src);

	return fallback;
}

struct exec_mode_id {
	char *name;
	exec_mode_t value;
};

struct exec_mode_id exec_mode_lookup[] = {
	{.name = "singular", .value = EXEC_MODE_SINGULAR},
	{.name = "unify", .value = EXEC_MODE_UNIFY}};

const size_t EXEC_MODE_LOOKUP_SIZE =
	sizeof(exec_mode_lookup) / sizeof(struct exec_mode_id);

exec_mode_t
str_to_exec_mode(char *src, exec_mode_t fallback)
{
	if(src == NULL) {
		return fallback;
	}

	for(size_t ix = 0; ix < EXEC_MODE_LOOKUP_SIZE; ix++) {
		if(strcmp(src, exec_mode_lookup[ix].name) == 0) {
			return exec_mode_lookup[ix].value;
		}
	}

	mb_logf(LOG_WARNING, "unrecognized exec_mode \"%s\"\n", src);

	return fallback;
}
