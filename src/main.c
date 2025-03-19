/* main.c ; mariebuild main entry point
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <argp.h>

#include "build.h"
#include "logging.h"
#include "mcfg.h"
#include "signals.h"

#define MARIEBUILD_COLORED_LOGO

#define MARIEBUILD_VERSION "0.7.4 (develop)"

/* clang-format off */
/* clang-format fucks this macro definition up really badly somehow */
#ifdef MARIEBUILD_COLORED_LOGO
#define LOGO "\n" \
  "\x1b[91m█▀▄▀█ ▄▀█ █▀█ █ █▀▀ \x1b[0m\x1b[31m█▄▄ █ █ █ █   █▀▄\n\x1b[0m" \
  "\x1b[91m█ ▀ █ █▀█ █▀▄ █ ██▄ \x1b[0m\x1b[31m█▄█ █▄█ █ █▄▄ █▄▀\n" \
  "\n\x1b[0m"
#else
#define LOGO "\n█▀▄▀█ ▄▀█ █▀█ █ █▀▀ █▄▄ █ █ █ █   █▀▄\n" \
  "█ ▀ █ █▀█ █▀▄ █ ██▄ █▄█ █▄█ █ █▄▄ █▄▀\n\n"
#endif
/* clang-format on */

const char *argp_program_version = "mariebuild " MARIEBUILD_VERSION
								   "\n"
								   "using MCFG/2 " MCFG_2_VERSION;
const char *argp_program_bug_address =
	"https://github.com/FelixEcker/mariebuild/issues";
const char description[] =
	"A simple build system inspired by my hate against makefiles\n"
	"Author: Marie Eckert";
const char args_doc[] = "";

static struct argp_option options[] = {
	{"in", 'i', "FILE", 0, "Specify a buildfile", 0},
	{"target", 't', "TARGET", 0, "Specify the build target", 0},
	{"force", 'f', 0, 0, "Force a build, regardless if target is incremental",
	 0},
	{"no-splash", 'n', 0, 0, "Disable splash screen/logo", 0},
	{"keep-going", 'k', 0, 0,
	 "Ignore any failures (if possible) and keep on building", 0},
	{"verbosity", 'v', "LEVEL", 0, "Set the verbosity level (0-3)", 0},
	{0, 0, 0, 0, 0, 0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	args_t *args = state->input;
	switch (key) {
		case 'i':
			args->buildfile = arg;
			break;
		case 't':
			args->target = arg;
			break;
		case 'f':
			args->force = true;
			break;
		case 'n':
			args->no_splash = true;
			break;
		case 'k':
			args->keep_going = true;
			break;
		case 'v':;
			args->verbosity = str_to_loglvl(arg);
			args->verbosity_overriden = true;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, description,
						   NULL,	NULL,	   NULL};

void print_splash(void) {
	fprintf(stderr, LOGO);
	fprintf(
		stderr, "\x1b[1m\x1b[3m// version " MARIEBUILD_VERSION " //\x1b[0m\n");
	fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
	args_t args;
	args.buildfile = "build.mb";
	args.target = NULL;
	args.force = false;
	args.no_splash = false;
	args.keep_going = false;
	args.verbosity = DEFAULT_LOG_LEVEL;
	args.verbosity_overriden = false;

	argp_parse(&argp, argc, argv, 0, 0, &args);

	if (!args.no_splash) {
		print_splash();
	}

	mb_log_level = args.verbosity;

	mb_install_signal_handlers();
	return mb_start(args);
}
