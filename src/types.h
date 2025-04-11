/* types.h ; mariebuild shared type-definition header
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>

#include "cptrlist.h"

typedef enum build_type {
	BUILD_TYPE_FULL = 0,
	BUILD_TYPE_INCREMENTAL = 1,
	//  BUILD_TYPE_DIFFERENTIAL = 2, // no, just no (maybe yes)
} build_type_t;

typedef struct config {
	build_type_t build_type;
	char *default_target;
	char *target;
	CPtrList public_targets;
	bool always_force;
	bool ignore_failures;
} config_t;

typedef enum exec_mode {
	EXEC_MODE_SINGULAR = 0,
	EXEC_MODE_UNIFY,
} exec_mode_t;

build_type_t str_to_build_type(char *src, build_type_t fallback);

exec_mode_t str_to_exec_mode(char *src, exec_mode_t fallback);

#endif /* #ifndef TYPES_H */
