/* mcfg_format.h ; marie config format field-formatting header
 * for MCFG/2
 *
 * Copyright (c) 2024, Marie Eckert
 * Licensend under the BSD 3-Clause License.
 */

#ifndef MCFG_FORMAT_H
#define MCFG_FORMAT_H

#include "mcfg.h"
#include "mcfg_util.h"

/**
 * @brief Error codes thrown by formatting functions
 * @see mcfg_err_t
 */
typedef enum mcfg_fmt_err {
  /** @brief everything is ok :) */
  MCFG_FMT_OK = MCFG_OK,
  MCFG_FMT_NULLPTR = MCFG_NULLPTR,
  MCFG_FMT_MALLOC_FAIL = MCFG_MALLOC_FAIL, /* TODO: Move rest of impl away from
                                              (re|m)alloc_or_die functions */
  MCFG_FMT_INVALID_TYPE = MCFG_INVALID_TYPE,
  MCFG_FMT_NOT_FOUND,
} mcfg_fmt_err_t;

/**
 * @copydoc mcfg_err_string
 */
char *mcfg_fmt_err_string(mcfg_fmt_err_t err);

typedef struct mcfg_fmt_res {
  mcfg_fmt_err_t err;

  size_t formatted_size;
  char *formatted;
} mcfg_fmt_res_t;

/**
 * @brief Format the embeds of a string field.
 * @param field The field of which the data should be formatted (has to be
 * TYPE_STRING)
 * @param file The file from which to take the data for formatting
 * @param relativity A path which should be used to complete relative paths.
 * @return A new heap allocated string containing the formatted field.
 */
mcfg_fmt_res_t mcfg_format_field_embeds(mcfg_field_t field, mcfg_file_t file,
                                        mcfg_path_t relativity);

/**
 * @brief Format the embeds in a string
 * @param input The string in which the embeds should be formatted
 * @param file The file from which to take the data for formatting
 * @param relativity A path which should be used to complete relative paths.
 * @return A new heap allocated string containing the formatted input string.
 */
mcfg_fmt_res_t mcfg_format_field_embeds_str(char *input, mcfg_file_t file,
                                            mcfg_path_t relativity);

#endif // ifndef MCFG_FORMAT_H
