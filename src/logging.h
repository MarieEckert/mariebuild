/* logging.h ; mariebuild logger header
 *
 * Copyright (c) 2023, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#ifndef LOGGING_H
#define LOGGING_H

typedef enum log_level {
	LOG_INVALID = -1,
	LOG_DEBUG = 0,
	LOG_STEPS,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	__LOG_UPPER_BOUND
} log_level_t;

#if !defined(DEFAULT_LOG_LEVEL)
#	define DEFAULT_LOG_LEVEL LOG_STEPS
#endif

extern log_level_t mb_log_level;

log_level_t str_to_loglvl(char *str);

int mb_logf(log_level_t level, const char *format, ...);
int mb_logf_noprefix(log_level_t level, const char *format, ...);
void mb_log(int level, char *msg);

#endif /* #ifndef LOGGING_H */
