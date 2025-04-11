/* build.c ; mariebuild build functionality impl.
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#define _XOPEN_SOURCE	700
#define _POSIX_C_SOURCE 2

#include "cptrlist.h"
#include "stringutil.h"
#include <stdlib.h>
#include <string.h>

#include "build.h"
#include "logging.h"
#include "mcfg.h"
#include "mcfg_util.h"
#include "target.h"
#include "types.h"
#include "xmem.h"

config_t default_config = {
	.build_type = BUILD_TYPE_INCREMENTAL,
	.default_target = "debug",
	.target = NULL,
	.public_targets = {.capacity = 0},
	.always_force = false,
	.ignore_failures = false,
};

bool
check_file_validity(mcfg_file_t file)
{
	if(file.sector_count == 0) {
		mb_log(LOG_ERROR, "build file is empty!\n");
		return false;
	}

	if(mcfg_get_sector(&file, "targets") == NULL) {
		mb_log(LOG_ERROR, "no targets defined!\n");
		return false;
	}

	return true;
}

config_t
mb_load_configuration(mcfg_file_t file, args_t args)
{
	config_t fallback = default_config;
	config_t ret;

	mcfg_sector_t *sector = mcfg_get_sector(&file, "config");
	if(sector == NULL) {
		return fallback;
	}

	mcfg_section_t *config = mcfg_get_section(sector, "mariebuild");
	if(config == NULL) {
		return fallback;
	}

	mcfg_field_t *field_targets = mcfg_get_field(config, "targets");
	if(field_targets != NULL) {
		mcfg_list_t targets = *mcfg_data_as_list(*field_targets);
		cptrlist_init(&ret.public_targets, 8, 8);

		if(targets.type != TYPE_STRING) {
			mb_log(LOG_WARNING, "mariebuild target list is not a string-list!");
		}

		for(size_t ix = 0; ix < targets.field_count; ix++) {
			char *str = mcfg_data_as_string(targets.fields[ix]);
			if(str != NULL) {
				cptrlist_append(&ret.public_targets, strdup(str));
			}
		}

		mb_logf(LOG_DEBUG, "registered %zu public targets\n",
				ret.public_targets.size);

		cptrlist_destroy(&fallback.public_targets);
	} else {
		ret.public_targets = fallback.public_targets;
	}

	mcfg_field_t *field_default_target = mcfg_get_field(config, "default");
	if(field_default_target != NULL) {
		ret.default_target = mcfg_data_as_string(*field_default_target);
	} else {
		ret.default_target = fallback.default_target;
	}

	mcfg_field_t *field_build_type = mcfg_get_field(config, "build_type");
	if(field_build_type != NULL) {
		ret.build_type = str_to_build_type(
			mcfg_data_as_string(*field_build_type), fallback.build_type);
	} else {
		ret.build_type = fallback.build_type;
	}

	mcfg_field_t *field_default_log_level =
		mcfg_get_field(config, "default_log_level");
	if(field_default_log_level != NULL && !args.verbosity_overriden) {
		log_level_t wanted_log_level =
			mcfg_data_as_int(*field_default_log_level);

		if(wanted_log_level <= LOG_INVALID ||
		   wanted_log_level >= __LOG_UPPER_BOUND) {
			mb_logf(
				LOG_WARNING,
				"/config/mariebuild/default_loglevel: invalid log level %d\n",
				wanted_log_level);
		} else {
			mb_log_level = wanted_log_level;
		}
	}

	return ret;
}

int
mb_start(args_t args)
{
	cptrlist_init(&default_config.public_targets, 1, 8);
	cptrlist_append(&default_config.public_targets, strdup("debug"));

	mb_log(LOG_DEBUG, "using MCFG/2 " MCFG_2_VERSION "\n");

	mcfg_parse_result_t parse_result = mcfg_parse_from_file(args.buildfile);
	if(parse_result.err != MCFG_OK) {
		mb_logf(LOG_ERROR, "buildfile parsing failed: %s (%d)\n",
				mcfg_err_string(parse_result.err), parse_result.err);
		mb_logf(LOG_ERROR, "in file \"%s\" on line %d\n", args.buildfile,
				parse_result.err_linespan.starting_line);

		return 1;
	}

	mcfg_file_t file = parse_result.value;

	if(!check_file_validity(file)) {
		return 1;
	}

	config_t cfg = mb_load_configuration(file, args);
	cfg.target = args.target == NULL ? cfg.default_target : args.target;
	cfg.ignore_failures = args.keep_going;
	cfg.always_force = args.force;

	int return_code = mb_begin_build(&file, cfg);
	if(return_code != 0) {
		mb_log(LOG_ERROR, "build failed!\n");
	} else {
		mb_log(LOG_INFO, "build succeeded!\n");
	}

	cptrlist_destroy(&cfg.public_targets);
	mcfg_free_file(file);
	return return_code;
}

int
mb_begin_build(mcfg_file_t *file, config_t cfg)
{
	mcfg_sector_t *targets = mcfg_get_sector(file, "targets");
	if(targets == NULL || targets->section_count == 0) {
		mb_log(LOG_ERROR, "build file is missing target definitions!\n");
		return 1;
	}

	mcfg_section_t *target = mcfg_get_section(targets, cfg.target);

	if(target == NULL) {
		mb_logf(LOG_ERROR, "target \"%s\" is not declared within build file!\n",
				cfg.target);
		return 1;
	}

	CPtrList history;
	cptrlist_init(&history, 1, 4);
	int ret = mb_run_target(file, target, &history, cfg);
	cptrlist_destroy(&history);

	return ret;
}
