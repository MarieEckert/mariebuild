/* c_rule.c ; mariebuild build rule implementation
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#define _XOPEN_SOURCE	700
#define _POSIX_C_SOURCE 2

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "c_rule.h"
#include "executor.h"
#include "logging.h"
#include "mcfg.h"
#include "mcfg_format.h"
#include "mcfg_util.h"
#include "types.h"
#include "xmem.h"

#define FMT_ERR_CHECK(fmt_res, tag)                                           \
	do {                                                                      \
		if(fmt_res.err != MCFG_FMT_OK) {                                      \
			mb_logf(LOG_ERROR,                                                \
					"[c_rule:%s] mcfg_format_field_embeds failed: %d (%s)\n", \
					tag, fmt_res.err, mcfg_fmt_err_string(fmt_res.err));      \
			return fmt_res.err;                                               \
		}                                                                     \
	} while(0)

#define ADD_DYNFIELD(file, name)                                             \
	do {                                                                     \
		if(mcfg_get_dynfield(file, name) == NULL) {                          \
			mcfg_err_t err =                                                 \
				mcfg_add_dynfield(file, TYPE_STRING, strdup(name), NULL, 0); \
			if(err != MCFG_OK) {                                             \
				mb_logf(LOG_ERROR,                                           \
						"[c_rule:%s] mcfg_add_dynfield failed: %s (%d)\n",   \
						name, mcfg_err_string(err), err);                    \
				return 1;                                                    \
			}                                                                \
		}                                                                    \
	} while(0)

struct io_fields {
	mcfg_field_t *input;
	mcfg_field_t *output;
};

size_t
_size_t_max(size_t a, size_t b)
{
	return a > b ? a : b;
}

void
_append_char(char **dest, size_t wix, size_t *dest_size, char chr)
{
	if(dest == NULL || *dest == NULL) {
		return;
	}

	if(wix >= *dest_size) {
		size_t size_diff = wix - *dest_size + 1;
		size_t new_size =
			*dest_size +
			_size_t_max(MCFG_EMBED_FORMAT_RESIZE_AMOUNT, size_diff);

		*dest = XREALLOC(*dest, new_size);
		*dest_size = new_size;
	}

	(*dest)[wix] = chr;
}

size_t
_append_str(char **dest, size_t wix, size_t *dest_size, char *src)
{
	if(dest == NULL || *dest == NULL || src == NULL) {
		return 0;
	}

	for(size_t ix = 0; ix < strlen(src); ix++) {
		_append_char(dest, wix, dest_size, src[ix]);
		wix++;
	}

	return wix;
}

bool
is_file_newer(char *file1, char *file2)
{
	FILE *f_1 = fopen(file1, "r");
	FILE *f_2 = fopen(file2, "r");

	int f_1_mtime = 1;
	int f_2_mtime = 0;

	if(f_1 == NULL || f_2 == NULL) {
		if(errno != ENOENT) {
			mb_logf(LOG_DEBUG,
					"file opening for file %d failed: OS Error %d (%s)\n",
					f_1 == NULL ? 1 : 2, errno, strerror(errno));
		}
		goto exit;
	}

	int fd_1 = fileno(f_1);
	int fd_2 = fileno(f_2);

	struct stat f_1_stat;
	if(fstat(fd_1, &f_1_stat) != 0) {
		mb_logf(LOG_ERROR, "%s/%s:%d: fstat for f_1 failed: OS Error %d (%s)\n",
				__FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
		goto exit;
	}

	struct stat f_2_stat;
	if(fstat(fd_2, &f_2_stat) != 0) {
		mb_logf(LOG_ERROR, "%s/%s:%d: fstat for f_2 failed: OS Error %d (%s)\n",
				__FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
		goto exit;
	}

#ifdef __APPLE__
#	if __DARWIN_64_BIT_INO_T
	f_1_mtime = f_1_stat.st_mtimensec;
	f_2_mtime = f_1_stat.st_mtimensec;
#	else
	f_1_mtime = f_1_stat.st_mtimespec.tv_sec;
	f_2_mtime = f_2_stat.st_mtimespec.tv_sec;
#	endif
#else
	f_1_mtime = f_1_stat.st_mtim.tv_sec;
	f_2_mtime = f_2_stat.st_mtim.tv_sec;
#endif

#ifdef LOG_TIMESTAMPS
	fprintf(stderr, "%s: %d ; %s: %d\n", file1, f_1_mtime, file2, f_2_mtime);
#endif

exit:
	if(f_1 != NULL) {
		fclose(f_1);
	}
	if(f_2 != NULL) {
		fclose(f_2);
	}

	return f_1_mtime > f_2_mtime;
}

bool
get_io_fields(mcfg_file_t *file, mcfg_section_t *rule, struct io_fields *dest)
{
	mcfg_field_t *field_input = mcfg_get_field(rule, "input");
	if(field_input == NULL) {
		mcfg_field_t *field_input_src = mcfg_get_field(rule, "input_src");
		if(field_input_src == NULL) {
			mb_log(LOG_ERROR, "missing input element list!\n");
			return false;
		}

		char *raw_path = mcfg_data_to_string(*field_input_src);
		mcfg_path_t path = mcfg_parse_path(raw_path);

		field_input = mcfg_get_field_by_path(file, path);

		mcfg_free_path(path);
		XFREE(raw_path);

		if(field_input == NULL) {
			raw_path = mcfg_data_to_string(*field_input_src);
			mb_logf(LOG_ERROR, "field \"%s\" does not exit!\n", raw_path);

			XFREE(raw_path);

			return false;
		}
	}

	if(field_input->type != TYPE_LIST) {
		mb_log(LOG_ERROR, "field \"input\" is not of type list!\n");
		return false;
	}
	if(field_input->data == NULL) {
		mb_log(LOG_ERROR, "field \"input\" has no data!\n");
		return false;
	}

	mcfg_field_t *field_output = mcfg_get_field(rule, "output");
	if(field_output == NULL) {
		mcfg_field_t *field_output_src = mcfg_get_field(rule, "input_src");
		if(field_output_src == NULL) {
			field_output = field_input;
			goto field_out_null_done;
		}

		char *raw_path = mcfg_data_to_string(*field_output_src);
		mcfg_path_t path = mcfg_parse_path(raw_path);

		field_output = mcfg_get_field_by_path(file, path);

		mcfg_free_path(path);
		XFREE(raw_path);

		if(field_output == NULL) {
			raw_path = mcfg_data_to_string(*field_output_src);
			mb_logf(LOG_ERROR, "field \"%s\" does not exit!\n", raw_path);

			XFREE(raw_path);

			return false;
		}

	field_out_null_done:;
	} else if(field_output->type != TYPE_LIST) {
		mb_log(LOG_ERROR, "field \"output\" is not of type list!\n");
		return false;
	}

	dest->input = field_input;
	dest->output = field_output;
	return true;
}

int
mb_run_c_rules(mcfg_file_t *file,
			   mcfg_field_t *field_required_c_rules,
			   int org_type,
			   char *org_name,
			   const config_t cfg)
{
	if(field_required_c_rules == NULL) {
		return 0;
	}

	mcfg_sector_t *c_rules = mcfg_get_sector(file, "c_rules");
	if(c_rules == NULL || c_rules->section_count == 0) {
		mb_log(LOG_ERROR, "No c_rules defined!\n");
		return 1;
	}

	mcfg_list_t *required_c_rules = mcfg_data_as_list(*field_required_c_rules);

	int ret = 0;

	for(size_t ix = 0; ix < required_c_rules->field_count; ix++) {
		char *curr_c_rule_name =
			mcfg_data_to_string(required_c_rules->fields[ix]);
		if(curr_c_rule_name == NULL) {
			mb_logf(LOG_WARNING, "%s/%s:%d: curr_c_rule_name is NULL!\n",
					__FILE__, __FUNCTION__, __LINE__);
			continue;
		}

		mcfg_section_t *curr_c_rule =
			mcfg_get_section(c_rules, curr_c_rule_name);
		if(curr_c_rule == NULL) {
			mb_logf(LOG_ERROR,
					"c_rule \"%s\" required by %s \"%s\" does not exist.\n",
					curr_c_rule_name, org_type == TARGET ? "target" : "c_rule",
					org_name);
			XFREE(curr_c_rule_name);

			ret = 1;
			if(cfg.ignore_failures) {
				continue;
			}

			return ret;
		}

		int tmp_ret = mb_run_c_rule(file, curr_c_rule, cfg);
		ret = ret > tmp_ret ? ret : tmp_ret;
		XFREE(curr_c_rule_name);

		if(ret != 0 && !cfg.ignore_failures) {
			return ret;
		}
	}

	return ret;
}

/**
 * @brief helper function to find a position within an array of process_t
 * which can be reused for a new process. A position can be reused
 * if the associated process (via process.pid) has exited.
 * The script will be deleted from the system and the location string freed.
 *
 * @param max_procs The amount of processes in the processes_array.
 * @param process_ix Pointer to the output variable for the reusable slot.
 *
 * @return The exit code of the process which freed up the slot
 */
int
_find_process_slot(const size_t max_procs,
				   const process_t *processes,
				   size_t *used_processes,
				   size_t *process_ix)
{
	bool found = false;
	int exit_status = 0;

	/* previously unused process slot */
	for(size_t pix = 0; pix < max_procs; pix++) {
		if(processes[pix].pid == 0) {
			*process_ix = pix;
			*used_processes += 1;
			return 0;
		}
	}

	/* wait for slot to free up */
	while(!found) {
		for(size_t pix = 0; pix < max_procs; pix++) {
			int stat = 0;
			if(waitpid(processes[pix].pid, &stat, WNOHANG) <= 0) {
				continue;
			}

			found = true;
			exit_status = WEXITSTATUS(stat);
			if(exit_status != 0) {
				break;
			}

			mb_remove_script(processes[pix].location);
			XFREE(processes[pix].location);
			*process_ix = pix;
			break;
		}
	}

	return exit_status;
}

int
run_singular(mcfg_file_t *file,
			 mcfg_section_t *rule,
			 const config_t cfg,
			 build_type_t build_type)
{
	mcfg_field_t *field_exec = mcfg_get_field(rule, "exec");
	if(field_exec == NULL || field_exec->data == NULL) {
		mb_log(LOG_ERROR, "c_rule missing field \"exec\"\n");
		return 1;
	}

	mcfg_field_t *field_input_format = mcfg_get_field(rule, "input_format");
	mcfg_field_t *field_output_format = mcfg_get_field(rule, "output_format");

	if(field_input_format == NULL || field_output_format == NULL) {
		mb_logf(LOG_ERROR, "c_rule missing field \"%s\"!\n",
				field_input_format == NULL ? "input_format" : "output_format");
		return 1;
	} else if(field_input_format->type != TYPE_STRING ||
			  field_output_format->type != TYPE_STRING) {
		mb_logf(LOG_ERROR, "invalid datatype for field \"%s\"! Expected str\n",
				field_input_format->type != TYPE_STRING ? "input_format"
														: "output_format");
		return 1;
	}

	char *input_format = mcfg_data_as_string(*field_input_format);
	char *output_format = mcfg_data_as_string(*field_output_format);

	if(input_format == NULL || output_format == NULL) {
		mb_logf(LOG_ERROR, "field \"%s\" is missing data!\n",
				input_format == NULL ? "input_format" : "output_format");
		return 1;
	}

	struct io_fields io_fields;
	if(!get_io_fields(file, rule, &io_fields)) {
		return 1;
	}

	mcfg_list_t *list_input = mcfg_data_as_list(*io_fields.input);
	mcfg_list_t *list_output = mcfg_data_as_list(*io_fields.output);

	mcfg_path_t pathrel = {.absolute = true,
						   .dynfield_path = false,

						   .sector = "c_rules",
						   .section = rule->name,
						   .field = ""};

	ADD_DYNFIELD(file, "element");
	ADD_DYNFIELD(file, "input");
	ADD_DYNFIELD(file, "output");

	mcfg_field_t *dynfield_element = mcfg_get_dynfield(file, "element");
	mcfg_field_t *dynfield_input = mcfg_get_dynfield(file, "input");
	mcfg_field_t *dynfield_output = mcfg_get_dynfield(file, "output");

	bool run_parallel = false;
	size_t max_procs = 4; /* only important if run_parallel is true */

	mcfg_field_t *field_parallel = mcfg_get_field(rule, "parallel");
	mcfg_field_t *field_max_procs = mcfg_get_field(rule, "max_procs");

	if(field_parallel != NULL) {
		if(field_parallel->type != TYPE_BOOL) {
			mb_log(LOG_ERROR, "field \"parallel\" should be of type bool\n");
			return 1;
		}

		run_parallel = mcfg_data_as_bool(*field_parallel);
	}

	if(field_max_procs != NULL && run_parallel) {
		if(field_max_procs->type != TYPE_U8) {
			mb_log(LOG_ERROR, "field \"max_procs\" should be of type u8\n");
			return 1;
		}

		max_procs = mcfg_data_as_u8(*field_max_procs);
	}

	process_t *processes = NULL;
	if(run_parallel) {
		mb_logf(LOG_DEBUG, "running parallel with max procs of %d\n",
				max_procs);
		processes = XCALLOC(max_procs, sizeof(*processes));
	}

	/* reused for mcfg_format_field_embeds(_str) calls */
	mcfg_fmt_res_t fmt_res;

	int ret = 0;
	size_t used_processes = 0;

	for(size_t ix = 0; ix < list_output->field_count; ix++) {
		size_t process_ix = used_processes;

		char *raw_in = mcfg_data_to_string(list_input->fields[ix]);
		char *raw_out = mcfg_data_to_string(list_output->fields[ix]);

		dynfield_element->data = raw_in;
		dynfield_element->size = strlen(raw_in) + 1;

		fmt_res = mcfg_format_field_embeds_str(input_format, *file, pathrel);
		FMT_ERR_CHECK(fmt_res, "singular_input_format");

		char *in = fmt_res.formatted;

		dynfield_element->data = raw_out;
		dynfield_element->size = strlen(raw_in) + 1;

		fmt_res = mcfg_format_field_embeds_str(output_format, *file, pathrel);
		FMT_ERR_CHECK(fmt_res, "singular_output_format");

		char *out = fmt_res.formatted;

		if(build_type == BUILD_TYPE_INCREMENTAL && !is_file_newer(in, out) &&
		   !cfg.always_force) {
			goto build_loop_continue;
		}

		dynfield_output->data = out;
		dynfield_output->size = strlen(out) + 1;
		dynfield_input->data = in;
		dynfield_input->size = strlen(in) + 1;

		fmt_res = mcfg_format_field_embeds(*field_exec, *file, pathrel);
		FMT_ERR_CHECK(fmt_res, "singular_script_format");

		char *script = fmt_res.formatted;

		mb_logf(LOG_STEPS, "exec: %s > %s\n", in, out);

		if(!run_parallel) {
			int tmp_ret = mb_exec(script, rule->name);
			ret = ret > tmp_ret ? ret : tmp_ret;
		} else {
			int exit_status = _find_process_slot(max_procs, processes,
												 &used_processes, &process_ix);

			if(!cfg.ignore_failures && exit_status != 0) {
				ret = exit_status;
				goto build_loop_continue;
			}

			processes[process_ix] = mb_exec_parallel(script, rule->name);
		}

		XFREE(script);
	build_loop_continue:
		XFREE(raw_in);
		XFREE(raw_out);
		XFREE(in);
		XFREE(out);

		if(ret != 0 && !cfg.ignore_failures) {
			break;
		}
	}

	/* We have to do this to avoid double-frees when running mcfg_free_file at
	 * exit in build.c
	 */
	dynfield_element->data = NULL;
	dynfield_input->data = NULL;
	dynfield_output->data = NULL;

	if(used_processes == 0) {
		goto exit;
	}

	/* cleanup remaining child processes */
	for(size_t pix = 0; pix < max_procs; pix++) {
		if(processes[pix].pid == 0 || processes[pix].location == NULL) {
			continue;
		}

		int stat;
		if(waitpid(processes[pix].pid, &stat, 0) <= 0) {
			continue;
		}

		stat = WEXITSTATUS(stat);
		if(stat != 0) {
			ret = stat;
		}

		mb_remove_script(processes[pix].location);
		XFREE(processes[pix].location);
	}

exit:;
	if(processes != NULL) {
		XFREE(processes);
	}

	return ret;
}

int
run_unify(mcfg_file_t *file,
		  mcfg_section_t *rule,
		  const config_t cfg,
		  build_type_t build_type)
{
	mcfg_field_t *field_exec = mcfg_get_field(rule, "exec");
	if(field_exec == NULL || field_exec->data == NULL) {
		mb_log(LOG_ERROR, "c_rule missing field \"exec\"\n");
		return 1;
	}

	mcfg_field_t *field_input_format = mcfg_get_field(rule, "input_format");
	mcfg_field_t *field_output_format = mcfg_get_field(rule, "output_format");

	if(field_input_format == NULL || field_output_format == NULL) {
		mb_logf(LOG_ERROR, "c_rule missing field \"%s\"!\n",
				field_input_format == NULL ? "input_format" : "output_format");
		return 1;
	} else if(field_input_format->type != TYPE_STRING ||
			  field_output_format->type != TYPE_STRING) {
		mb_logf(LOG_ERROR, "invalid datatype for field \"%s\"! Expected str\n",
				field_input_format->type != TYPE_STRING ? "input_format"
														: "output_format");
		return 1;
	}

	char *input_format = mcfg_data_as_string(*field_input_format);
	char *output_format = mcfg_data_as_string(*field_output_format);

	if(input_format == NULL || output_format == NULL) {
		mb_logf(LOG_ERROR, "field \"%s\" is missing data!\n",
				input_format == NULL ? "input_format" : "output_format");
		return 1;
	}

	struct io_fields io_fields;
	if(!get_io_fields(file, rule, &io_fields)) {
		return 1;
	}

	mcfg_list_t *list_input = mcfg_data_as_list(*io_fields.input);

	mcfg_path_t pathrel = {.absolute = true,
						   .dynfield_path = false,

						   .sector = "c_rules",
						   .section = rule->name,
						   .field = ""};

	ADD_DYNFIELD(file, "element");
	ADD_DYNFIELD(file, "input");
	ADD_DYNFIELD(file, "output");

	mcfg_field_t *dynfield_element = mcfg_get_dynfield(file, "element");
	mcfg_field_t *dynfield_input = mcfg_get_dynfield(file, "input");
	mcfg_field_t *dynfield_output = mcfg_get_dynfield(file, "output");

	mcfg_fmt_res_t fmt_res =
		mcfg_format_field_embeds_str(output_format, *file, pathrel);
	FMT_ERR_CHECK(fmt_res, "unify_output_format");

	dynfield_output->data = fmt_res.formatted;
	dynfield_output->size = strlen(dynfield_output->data) + 1;

	size_t incount = 0;
	size_t wix = 0;

	dynfield_input->size = 16;
	dynfield_input->data = XMALLOC(dynfield_input->size);

	int ret = 0;

	for(size_t ix = 0; ix < list_input->field_count; ix++) {
		char *raw_in = mcfg_data_to_string(list_input->fields[ix]);
		dynfield_element->data = raw_in;
		dynfield_element->size = strlen(raw_in) + 1;

		fmt_res = mcfg_format_field_embeds_str(input_format, *file, pathrel);
		FMT_ERR_CHECK(fmt_res, "unify_input_format");

		char *fmted = fmt_res.formatted;

		if(build_type == BUILD_TYPE_INCREMENTAL &&
		   !is_file_newer(fmted, dynfield_output->data) && !cfg.always_force) {
			goto input_assembly_continue;
		}

		wix = _append_str((char **)&dynfield_input->data, wix,
						  &dynfield_input->size, fmted);
		_append_char((char **)&dynfield_input->data, wix, &dynfield_input->size,
					 ' ');
		wix++;

		incount++;
	input_assembly_continue:
		XFREE(raw_in);
		XFREE(fmted);
	}

	_append_char((char **)&dynfield_input->data, wix, &dynfield_input->size, 0);
	wix++;

	if(incount == 0) {
		mb_log(LOG_INFO, "no inputs, skipping!\n");
		goto exit;
	}

	mb_logf(LOG_STEPS, "exec: %s > %s\n", mcfg_data_as_string(*dynfield_input),
			mcfg_data_as_string(*dynfield_output));

	fmt_res = mcfg_format_field_embeds(*field_exec, *file, pathrel);
	FMT_ERR_CHECK(fmt_res, "unify_script_format");

	char *script = fmt_res.formatted;

	int tmp_ret = mb_exec(script, rule->name);
	ret = ret > tmp_ret ? ret : tmp_ret;

	XFREE(script);
exit:
	XFREE(dynfield_input->data);
	XFREE(dynfield_output->data);

	dynfield_element->data = NULL;
	dynfield_input->data = NULL;
	dynfield_output->data = NULL;

	return ret;
}

int
mb_run_c_rule(mcfg_file_t *file, mcfg_section_t *rule, const config_t cfg)
{
	mb_logf(LOG_INFO, "fulfilling c_rule \"%s\"\n", rule->name);

	mcfg_field_t *field_c_rules = mcfg_get_field(rule, "c_rules");
	if(field_c_rules != NULL) {
		if(field_c_rules->type != TYPE_LIST) {
			mb_log(LOG_WARNING,
				   "field c_rules is of incorrect type! ignoring.\n");
		} else {
			int ret =
				mb_run_c_rules(file, field_c_rules, C_RULE, rule->name, cfg);
			if(ret != 0) {
				return ret;
			}
		}
	}

	build_type_t build_type = cfg.build_type;
	if(mcfg_get_field(rule, "build_type") != NULL) {
		char *data = mcfg_data_to_string(*mcfg_get_field(rule, "build_type"));

		build_type = str_to_build_type(data, build_type);

		XFREE(data);
	}

	exec_mode_t exec_mode = EXEC_MODE_SINGULAR;
	if(mcfg_get_field(rule, "exec_mode") != NULL) {
		char *data = mcfg_data_to_string(*mcfg_get_field(rule, "exec_mode"));

		exec_mode = str_to_exec_mode(data, exec_mode);

		XFREE(data);
	}

	int ret = 0;

	switch(exec_mode) {
		case EXEC_MODE_SINGULAR:
			ret = run_singular(file, rule, cfg, build_type);
			break;
		case EXEC_MODE_UNIFY:
			ret = run_unify(file, rule, cfg, build_type);
			break;
	}

	if(ret == 0) {
		mb_logf(LOG_INFO, "fulfilled c_rule \"%s\"!\n", rule->name);
	}

	return ret;
}
