/*
 * mariebuild: mb_parse.c ; author: Marie Eckert
 *
 * Copyright (c) 2023, Marie Eckert
 * Licensed under the BSD 3-Clause License
 * <https://github.com/FelixEcker/mariebuild/blob/master/LICENSE>
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <mariebuild/mb_parse.h>

#include <mariebuild/mb_utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

const char *newline = "\n";
const char *str_terminator = "\0";

/******** Local Utility Functions ********/

/* Note: This function returns a pointer to a substring of the original string.
 * If the given string was allocated dynamically, the caller must not overwrite
 * that pointer with the returned value, since the original pointer must be
 * deallocated using the same allocator with which it was allocated.  
 * The return value must NOT be deallocated using free() etc.
 * */
char *trim_whitespace(char *str) {
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

/******** mb_parse.h ********/

void free_build_file(mb_file* file) {
  for (int i = 0; i < file->sector_count; i++) {
    for (int j = 0; j < file->sectors[i].section_count; j++) {
      for (int k = 0; k < file->sectors[i].sections[j].field_count; k++) {
        free(file->sectors[i].sections[j].fields[k].name);
        free(file->sectors[i].sections[j].fields[k].value);
      }

      if (file->sectors[i].sections[j].field_count > 0)
        free(file->sectors[i].sections[j].fields);
      free(file->sectors[i].sections[j].name);

      if (file->sectors[i].sections[j].lines != NULL)
        free(file->sectors[i].sections[j].lines);
    }

    free(file->sectors[i].sections);
    free(file->sectors[i].name);
  }

  free(file->sectors);
  free(file);
}

/* Parsing Functions */

int register_sector(struct mb_file* file, char *name) {
  if (name == NULL) return MB_ERR_UNKNOWN;

  // Check for duplicate sectors
  for (int i = 0; i < file->sector_count; i++)
    if (strcmp(file->sectors[i].name, name) == 0)
      return MB_PERR_DUPLICATE_SECTOR;

  int wi = file->sector_count;
  file->sector_count++;

  if (wi == 0) {
    file->sectors = malloc(sizeof(mb_sector));
  } else {
    file->sectors = realloc(file->sectors, (wi+1) * sizeof(mb_sector));
  }

  file->sectors[wi].section_count = 0;
  file->sectors[wi].name = malloc(strlen(name) + 1);
  strcpy(file->sectors[wi].name, name);

  mb_logf(MB_LOGLVL_LOW, "registered sector %s\n", name);
  return MB_OK;
}

int register_section(struct mb_sector* sector, char *name) {
  if (name == NULL) return MB_ERR_UNKNOWN;

  // Check for duplicate sections
  for (int i = 0; i < sector->section_count; i++)
    if (strcmp(sector->sections[i].name, name) == 0)
      return MB_PERR_DUPLICATE_SECTION;

  int wi = sector->section_count;
  sector->section_count++;

  if (wi == 0) {
    sector->sections = malloc(sizeof(mb_section));
  } else {
    sector->sections = realloc(sector->sections, (wi+1) * sizeof(mb_section));
  }

  sector->sections[wi].field_count = 0;
  sector->sections[wi].lines = NULL;
  sector->sections[wi].name = malloc(strlen(name) + 1);
  strcpy(sector->sections[wi].name, name);

  mb_logf(MB_LOGLVL_LOW, "registered section %s\n", name);
  return MB_OK;
}

int register_field(struct mb_section* section, char *name, char *value) {
  if (name == NULL || value == NULL) return MB_ERR_UNKNOWN;

  // Check for duplicate fields
  for (int i = 0; i < section->field_count; i++)
    if (strcmp(section->fields[i].name, name) == 0)
      return MB_PERR_DUPLICATE_FIELD;

  int wi = section->field_count;
  section->field_count++;

  if (wi == 0) {
    section->fields = malloc(sizeof(mb_field));
  } else {
    section->fields = realloc(section->fields, (wi+1) * sizeof(mb_field));
  }

  section->fields[wi].name = malloc(strlen(name) + 1);
  strcpy(section->fields[wi].name, name);
  section->fields[wi].value = malloc(strlen(value) + 1);
  strcpy(section->fields[wi].value, value);

  mb_logf(MB_LOGLVL_LOW, "registered field %s\n", name);
  return MB_OK;
}

/* Parses a single line.
 * */
int parse_line(struct mb_file* file, char *line) {
  char delimiter[] = " ";
  line = trim_whitespace(line);
  char *token = strtok(line, delimiter);

  while (token != NULL) {
    if (strcmp(token, "sector") == 0) {
      token = strtok(NULL, delimiter);

      return register_sector(file, token);
    } else if (strncmp(";", token, strlen(";")) == 0) break;

    if (str_endswith(token, ":") == 1) {
      // Remov the colon at the end of the name
      token[strlen(token)-1] = 0;
      return register_section(&file->sectors[file->sector_count-1], token);
    }

    if (file->sector_count == 0)
      return MB_PERR_INVALID_SYNTAX;

    if (file->sectors[file->sector_count-1].section_count == 0)
      return MB_PERR_INVALID_SYNTAX;

    mb_sector *sector = &file->sectors[file->sector_count-1];
    mb_section *section = &sector->sections[sector->section_count-1];

    int sector_type = strcmp(sector->name, ".config");

    char *name = token;

    token = strtok(NULL, delimiter);
    if (token == NULL) {
      // A singular token on a line in a section in the .config sector
      // is invalid syntax, at it would be a field without a value
      if (sector_type == 0)
        return MB_PERR_INVALID_SYNTAX;
    }

    char *content = malloc(strlen(token) + 1);
    strcpy(content, token);

    token = strtok(NULL, delimiter);
    while (token != NULL) {
      content = realloc(content, strlen(content)+strlen(token)+2);
      strcpy(content+strlen(content), delimiter);
      strcpy(content+strlen(content), token);
      token = strtok(NULL, delimiter);
    }

    if (sector_type == 0) {
      // Gets rid of the quotation marks around the value
      // TODO: Pontentially make this less cancerous
      char *cleaned_content = malloc(strlen(content)-1);
      memcpy(cleaned_content, content+1, strlen(content)-2);
      memcpy(cleaned_content+strlen(content)-2, str_terminator, 1);
      register_field(section, name, cleaned_content);
      free(cleaned_content);
    } else {
      // Piece together the complete content
      // TODO: Potentially make this less cancerous
      char *complete_content = malloc(strlen(name)+strlen(content)+3);
      strcpy(complete_content, name);
      strcpy(complete_content+strlen(complete_content), delimiter);
      strcpy(complete_content+strlen(complete_content), content);
      strcpy(complete_content+strlen(complete_content), newline);

      // Write content to section
      if (section->lines != NULL) {
        section->lines = realloc(section->lines,
                                 strlen(section->lines)+
                                 strlen(complete_content)+1);
        strcpy(section->lines+strlen(section->lines), complete_content);
      } else {
        section->lines = malloc(strlen(complete_content)+1);
        strcpy(section->lines, complete_content);
      }

      free(complete_content);
    }

    free(content);

    token = strtok(NULL, delimiter);
  }

  return MB_OK;
}

/* Parses the file under the path in build_file->path line by line,
 * will return MB_OK if there were no errors.
 *
 * Refer to mb_utils.h for error codes.
 * */
int parse_file(struct mb_file* build_file) {
  FILE *file;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  errno = 0;
  file = fopen(build_file->path, "r");
  if (file == NULL)
    return MB_SERR_MASK_ERRNO | errno;

  build_file->sector_count = 0;
  build_file->line = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    build_file->line++;
    int result = parse_line(build_file, line);
    if (result != MB_OK)
      return result;
  }

  fclose(file);
  if (line)
    free(line);

  return MB_OK;
}

/* Navigation Functions */

/* NOTE: The returned string of this function has to be freed after usage!!!
 */
char *get_path_elem(char *path, int n_elem) {
  if (path == NULL) 
    return NULL;

  char delimiter = '/';

  // We do this atrocity of searching through the string since
  // this is the only way I could reliably implement this func
  // without having memory leaks or invalid free's
  // If anyone can do this nicer without creating memory leaks
  // or invalid free's, feel free to do so. I have wasted to
  // much time on this function

  int n = 0;
  int i = 0;
  while (n < n_elem) {
    if (i == strlen(path)) return NULL;
    if (path[i] == delimiter) n++;
    i++;
  }

  // Get len
  int len = 1;

  while ((i+len) < strlen(path)) {
    if (path[i+len] == delimiter) break;

    len++;
  }

  char *result = malloc(len+1);
  memcpy(result, path+i, len);
  memcpy(result+len, str_terminator, 1);

  return result;
}

mb_sector *find_sector(struct mb_file* file, char *sector_name) {
  for (int i = 0; i < file->sector_count; i++)
    if (strcmp(file->sectors[i].name, sector_name) == 0)
      return &file->sectors[i];

  return NULL;
}

mb_section *find_section(struct mb_file* file, char *path) {
  char *sector_name = get_path_elem(path, 0);
  char *section_name = get_path_elem(path, 1);

  if ((sector_name == NULL) || (section_name == NULL))
    return NULL;

  mb_sector *sector = find_sector(file, sector_name);
  free(sector_name);

  if (sector == NULL) {
    return NULL;
  }

  mb_section *section = NULL;
  for (int i = 0; i < sector->section_count; i++) {
    if (strcmp(sector->sections[i].name, section_name) == 0) {
      section = &sector->sections[i];
      break;
    }
  }

  free(section_name);
  return section;
}

mb_field *find_field(struct mb_file* file, char *path) {
  char *sector_name = get_path_elem(path, 0);
  char *section_name = get_path_elem(path, 1);
  char *field_name = get_path_elem(path, 2);
 
  if ((sector_name == NULL) || (section_name == NULL) || (field_name == NULL))
    return NULL; 

  sector_name = realloc(sector_name, 
                        strlen(sector_name)+strlen(section_name)+2);
  mb_section *section = find_section(file, 
                                     strcat(strcat(sector_name, "/"), 
                                            section_name)
                                    );

  free(sector_name);
  free(section_name);
  if (section == NULL) {
    free(field_name);
    return NULL;
  }

  mb_field *field = NULL;

  for (int i = 0; i < section->field_count; i++) {
    if (strcmp(section->fields[i].name, field_name) == 0) {
      field = &section->fields[i];
      break;
    }
  }
  
  free(field_name);

  return field;
}

/* TODO: This is singlehandidly the worst code ive ever written, this needs a
 * desperate cleanup its so fucking long and confusing
 * */
char *resolve_fields(struct mb_file file, char *in, char *context) {
  int n_fields = 0;
  int *field_indexes = malloc(sizeof(int));
  int *field_lens    = malloc(sizeof(int));
  char **fieldvals   = malloc(sizeof(char*));

  // Resolve all fields and store their vals and indexes in the string
  for (int i = 0; i < strlen(in); i++) {
    if ((in[i] == '$') && (in[i+1] == '(')) {
      int len = 0;
      int is_local = 1;

      for (int j = i; j < strlen(in); j++) {
        if (in[j] == '/') is_local = 0;
        if (in[j] == ')') break;
        len++;
      }

      char *name;

      int term_offs = len-2;
      if (is_local) {
        // Subtract two from length to account for $() = -3 
        // and the NULL Byte = +1
        name = malloc(strlen(context) + (len - 1));
        memcpy(name, context, strlen(context));
        memcpy(name+strlen(context), in+i+2, len-2);
        term_offs += strlen(context);
      } else {
        char prefix[] = ".config/";
        int offs = 0;

        // Create temporary copy of raw name for prefix checking
        char *tmp_name = malloc((len - 1));
        memcpy(tmp_name, in+i+2, len-2);
        memcpy(tmp_name+len-2, str_terminator, 1);
        
        if (str_startswith(tmp_name, prefix) != 0) {
          name = malloc(strlen(prefix) + (len - 1));
          memcpy(name, prefix, strlen(prefix));
          offs += strlen(prefix);
        } else {
          name = malloc(len-2);
        }

        memcpy(name+offs, in+i+2, len-2);
        term_offs += offs;
        free(tmp_name);
      }

      memcpy(name+term_offs, str_terminator, 1);

      mb_field *field = find_field(&file, name);
      if (field == NULL)
        goto resolve_fields_stop;

      char *val_tmp = resolve_fields(file, field->value, context);

      n_fields++;
      if (n_fields > 1) {
        field_indexes = realloc(field_indexes, n_fields*sizeof(int));
        field_lens    = realloc(field_lens   , n_fields*sizeof(int));
        fieldvals     = realloc(fieldvals    , n_fields*sizeof(char*));
      }

      field_indexes[n_fields-1] = i;
      field_lens[n_fields-1]    = len;
      fieldvals[n_fields-1]     = val_tmp;

    resolve_fields_stop:
      free(name);
    }
  }

  char *out = NULL;
  if (n_fields == 0) {
    out = malloc(strlen(in)+1);
    strcpy(out, in);

    goto resolve_fields_finished;
  }

  // Copy input string and insert values
  int i_offs = 0; // Offset for copying from in
  int o_offs = 0; // Offset for copying to out

  for (int i = 0; i < n_fields; i++) {
    int len = field_lens[i];
    int ix  = field_indexes[i];
    char *val = fieldvals[i];

    // allocate memory (strlen(val) + ix-i_offs)
    if (out == NULL) {
      out = malloc(strlen(val) + (ix - i_offs));
    } else {
      out = realloc(out, o_offs + (strlen(val) + (ix - i_offs)));
    }

    // copy from in i_offs <-> ix to out with o_offs
    memcpy(out+o_offs, in + i_offs, ix - i_offs);

    // add ix - i_offs to o_offs
    o_offs += ix - i_offs;

    // set i_offs to ix+len
    i_offs = ix + len + 1;

    // copy val to o_offs
    memcpy(out+o_offs, val, strlen(val));

    // add strlen of val to o_offs
    o_offs += strlen(val);
  }

  // Copy remaining bytes from in to out
  if (i_offs < strlen(in)) {
    int missing = strlen(in) - i_offs;

    out = realloc(out, o_offs + missing);
    memcpy(out+o_offs, in+i_offs, missing);
    o_offs += missing;
  }

  // Append Terminator
  out = realloc(out, o_offs+1);
  memcpy(out+o_offs, str_terminator, 1);

resolve_fields_finished:
  if (n_fields > 0) {
    free(field_indexes);
    free(field_lens);
    for (int i = 0; i < n_fields; i++)
      free(fieldvals[i]);
    free(fieldvals);
  }

  return out;
}
