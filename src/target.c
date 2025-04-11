/* target.c ; mariebuild build target impl.
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#define _XOPEN_SOURCE	700
#define _POSIX_C_SOURCE 2

#include <string.h>

#include "c_rule.h"
#include "cptrlist.h"
#include "executor.h"
#include "logging.h"
#include "mcfg.h"
#include "mcfg_format.h"
#include "mcfg_util.h"
#include "stringutil.h"
#include "target.h"
#include "types.h"
#include "xmem.h"

bool
remove_dynfield(mcfg_file_t *file, char *name)
{
	ssize_t field_ix = -1;

	for(size_t ix = 0; ix < file->dynfield_count; ix++) {
		if(strcmp(file->dynfields[ix].name, name) == 0) {
			field_ix = ix;
			break;
		}
	}

	if(field_ix == -1) {
		mb_logf(LOG_DEBUG, "could not find field \"%s\" to remove!\n", name);
		return false;
	}

	file->dynfield_count--;

	for(size_t ix = field_ix; ix < file->dynfield_count; ix++) {
		file->dynfields[ix] = file->dynfields[ix + 1];
	}

	return true;
}

CPtrList
link_target_fields(mcfg_file_t *file, mcfg_section_t *target)
{
	const char *prefix = "target_";

	CPtrList ret;
	cptrlist_init(&ret, target->field_count, 1);

	for(size_t ix = 0; ix < target->field_count; ix++) {
		mcfg_field_t *field = &target->fields[ix];
		if(strncmp(field->name, prefix, strlen(prefix)) != 0) {
			continue;
		}

		mcfg_err_t err = mcfg_add_dynfield(file, field->type, field->name,
										   field->data, field->size);

		if(err == MCFG_DUPLICATE_DYNFIELD) {
			mb_logf(LOG_DEBUG,
					"duplicate or not unregistered target dependant field: "
					"%s/%s\n",
					target->name, field->name);
			continue;
		} else if(err != MCFG_OK) {
			char *errstr = mcfg_err_string(err);
			mb_logf(
				LOG_ERROR,
				"failed to link target dependant field (mcfg_add_dynfield): "
				"%s (%d)\n",
				errstr, err);
			XFREE(errstr);
			continue;
		}
		mb_logf(LOG_DEBUG, "linked field \"%s\"\n", field->name);

		cptrlist_append(&ret, strdup(field->name));
	}

	return ret;
}

void
unlink_target_fields(mcfg_file_t *file, CPtrList fields)
{
	for(size_t ix = 0; ix < fields.size; ix++) {
		remove_dynfield(file, fields.items[ix]);
		mb_logf(LOG_DEBUG, "unlinked fields \"%s\"\n", fields.items[ix]);
	}
}

int
run_required_targets(mcfg_file_t *file,
					 mcfg_section_t *target,
					 CPtrList *target_history,
					 const config_t cfg)
{
	mcfg_field_t *field_required_targets =
		mcfg_get_field(target, "required_targets");
	if(field_required_targets == NULL) {
		return 0;
	}

	mcfg_list_t *required_targets = mcfg_data_as_list(*field_required_targets);
	mcfg_sector_t *targets = mcfg_get_sector(file, "targets");

	int ret = 0;

	for(size_t ix = 0; ix < required_targets->field_count; ix++) {
		char *curr_target_name =
			mcfg_data_to_string(required_targets->fields[ix]);
		if(curr_target_name == NULL) {
			mb_logf(LOG_WARNING, "%s/%s:%d: curr_target_name is NULL!\n",
					__FILE__, __FUNCTION__, __LINE__);
			continue;
		}

		mcfg_section_t *curr_target =
			mcfg_get_section(targets, curr_target_name);
		if(curr_target == NULL) {
			mb_logf(LOG_ERROR,
					"target \"%s\" required by target \"%s\" does not exist.\n",
					curr_target_name, target->name);
			XFREE(curr_target_name);

			ret = 1;

			if(cfg.ignore_failures) {
				continue;
			}

			return ret;
		}

		int ret = mb_run_target(file, curr_target, target_history, cfg);
		XFREE(curr_target_name);

		if(ret != 0 && !cfg.ignore_failures) {
			return ret;
		}
	}

	return ret;
}

int
mb_run_target(mcfg_file_t *file,
			  mcfg_section_t *target,
			  CPtrList *target_history,
			  const config_t cfg)
{
	if(target_history == NULL) {
		mb_log(LOG_ERROR,
			   "internal: mb_run_target was passed a NULL target_history!\n");
		return 1;
	}

	if(cptrlist_find(target_history, target->name, &string_cptrlist_search) !=
	   -1) {
		mb_logf(LOG_ERROR, "circular target dependency for target \"%s\"\n",
				target->name);
		mb_log(LOG_ERROR, "target history:\n");
		for(size_t ix = 0; ix < target_history->size; ix++) {
			mb_logf(LOG_ERROR, "  %s\n", target_history->items[ix]);
		}
		return 1;
	}

	cptrlist_append(target_history, strdup(target->name));

	/* "Link" fields with target_ prefix to dynfields with the same name */
	CPtrList linked_fields = link_target_fields(file, target);

	mb_logf(LOG_INFO, "building target \"%s\"\n", target->name);

	/* Target Execution Order
	 * 1. required targets
	 * 2. compilation rules
	 * 3. "exec" field
	 */

	int ret = 0;
	ret = run_required_targets(file, target, target_history, cfg);
	if(ret != 0 && !cfg.ignore_failures) {
		goto exit;
	}

	int tmp_ret = mb_run_c_rules(file, mcfg_get_field(target, "c_rules"),
								 TARGET, target->name, cfg);
	ret = ret > tmp_ret ? ret : tmp_ret;
	if(ret != 0 && !cfg.ignore_failures) {
		goto exit;
	}

	mcfg_field_t *field_exec = mcfg_get_field(target, "exec");
	char *exec = NULL;
	if(field_exec != NULL) {
		char *raw_exec = mcfg_data_to_string(*field_exec);
		mcfg_path_t pathrel = {.absolute = true,
							   .dynfield_path = false,

							   .sector = "targets",
							   .section = target->name,
							   .field = ""};

		mcfg_fmt_res_t fmt_res =
			mcfg_format_field_embeds_str(raw_exec, *file, pathrel);
		if(fmt_res.err != MCFG_FMT_OK) {
			mb_logf(
				LOG_ERROR,
				"[target:exec_format] mcfg_format_field_embeds failed: %d\n",
				fmt_res.err);
		}

		exec = fmt_res.formatted;

		XFREE(raw_exec);
	}

	if(exec != NULL) {
		tmp_ret = mb_exec(exec, target->name);
		ret = ret > tmp_ret ? ret : tmp_ret;
		XFREE(exec);
		if(ret != 0 && !cfg.ignore_failures) {
			goto exit;
		}
	}

	if(ret == 0) {
		mb_logf(LOG_INFO, "built target \"%s\"!\n", target->name);
	}

exit:;
	int ix =
		cptrlist_find(target_history, target->name, &string_cptrlist_search);
	if(ix > -1) {
		XFREE(target_history->items[ix]);
	}

	unlink_target_fields(file, linked_fields);
	cptrlist_destroy(&linked_fields);

	target_history->size--;
	return ret;
}
