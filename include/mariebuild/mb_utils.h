/*
 * mariebuild: mb_utils.h ; author: Marie Eckert
 *
 * Copyright (c) 2023, Marie Eckert
 * Licensed under the BSD 3-Clause License
 * <https://github.com/FelixEcker/mariebuild/blob/master/LICENSE>
 */

#ifndef MB_UTILS_H
#define MB_UTILS_H

/******** Return Codes ********/

#define MB_OK          0x00000000
#define MB_ERR_UNKNOWN 0x00000001

/* Parsing Errors */

#define MB_PERR_MISSING_REQUIRED   0x00000101
#define MB_PERR_DUPLICATE_SECTION  0x00000102
#define MB_PERR_DUPLICATE_SECTOR   0x00000103
#define MB_PERR_DUPLICATE_FIELD    0x00000104
#define MB_PERR_INVALID_IDENTIFIER 0x00000105
#define MB_PERR_INVALID_SYNTAX     0x00000106

/* Build Errors */

#define MB_BERR_MISSING_FILES   0x00000201
#define MB_BERR_MISSING_COMPCMD 0x00000202
#define MB_BERR_SCRIPT_ERROR    0x00000203

/******** Log Levels *********/

#define MB_LOGLVL_LOW 0
#define MB_LOGLVL_STD 1
#define MB_LOGLVL_IMP 2
#define MB_LOGLVL_SCR 3

/******** Globals ********/

extern int mb_logging_level;
extern char *mb_errtext;

/******** Logging Functions ********/

void mb_logf(int level, char *msg, const char *fmt, ...);
void mb_log(int level, char *msg);

/******** Misc. Functions ********/

char *errcode_msg(int err);

int str_startswith(char *str, char *start);
int str_endswith(char *str, char *end);

#endif
