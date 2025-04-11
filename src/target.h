/* target.h ; mariebuild build target header
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#ifndef TARGET_H
#define TARGET_H

#include "cptrlist.h"
#include "mcfg.h"
#include "types.h"

int mb_run_target(mcfg_file_t *file,
				  mcfg_section_t *target,
				  CPtrList *target_history,
				  const config_t cfg);

#endif /* #ifndef TARGET_H */
