/* c_rule.h ; mariebuild build rule header
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#ifndef C_RULE_H
#define C_RULE_H

#include "mcfg.h"
#include "types.h"

#define TARGET 0
#define C_RULE 1

int mb_run_c_rules(mcfg_file_t *file,
				   mcfg_field_t *field_required_c_rules,
				   int org_type,
				   char *org_name,
				   const config_t cfg);

int mb_run_c_rule(mcfg_file_t *file, mcfg_section_t *rule, const config_t cfg);

#endif /* #infdef C_RULE_H */
