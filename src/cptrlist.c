/* cptrlist.c ; C Pointer List Implementation
 *
 * A simple implementation of a pointer-list with access functions
 * that I intended to reuse every so often.
 *
 *******************************************************************************
 *
 * Copyright (c) 2025, Marie Eckert
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "cptrlist.h"
#include <stdlib.h>

bool
cptrlist_init(CPtrList *list, size_t capacity, size_t resize_align)
{
	if(list == NULL) {
		return false;
	}

	*list = (CPtrList){
		.capacity = capacity,
		.size = 0,
		.items = calloc(capacity, sizeof(void *)),
		.allow_resize = true,
		.resize_align = resize_align,
	};

	return list->items != NULL;
}

static bool
_cptrlist_resize(CPtrList *list)
{
	if(list == NULL) {
		return false;
	}

	size_t newcap =
		(list->capacity + list->resize_align) - ~(list->resize_align - 1);
	if(list->items == NULL) {
		list->items = calloc(newcap, sizeof(void *));
		goto exit;
	}

	list->items = realloc(list->items, newcap * sizeof(void *));
	if(list->items == NULL) {
		return false;
	}

	for(size_t ix = list->size; ix < newcap; ix++) {
		list->items[ix] = NULL;
	}

exit:;
	const bool null = list->items == NULL;
	if(!null) {
		list->capacity = newcap;
	}

	return !null;
}

ssize_t
cptrlist_append(CPtrList *list, void *item)
{
	if(list == NULL) {
		return -1;
	}

	if(list->size == list->capacity && list->allow_resize) {
		if(!_cptrlist_resize(list)) {
			return -1;
		}
	}

	list->items[list->size] = item;
	return list->size++;
}

ssize_t
cptrlist_insert_or_append(CPtrList *list, void *item)
{
	if(list == NULL) {
		return -1;
	}

	for(size_t ix = 0; ix < list->size; ix++) {
		if(list->items[ix] == NULL) {
			list->items[ix] = item;
			return ix;
		}
	}

	return cptrlist_append(list, item);
}

ssize_t
cptrlist_find(CPtrList *list, void *search, bool (*search_func)(void *, void *))
{
	if(list == NULL) {
		return -1;
	}

	for(size_t ix = 0; ix < list->size; ix++) {
		if(search_func(search, list->items[ix])) {
			return ix;
		}
	}

	return -1;
}

void
cptrlist_free_at(CPtrList *list, size_t index)
{
	if(list == NULL || index >= list->size || list->items[index] == NULL) {
		return;
	}

	free(list->items[index]);
	if(index == list->size - 1) {
		list->size--;
	}
}

void
cptrlist_free(CPtrList *list, void *item)
{
	if(list == NULL || item == NULL) {
		return;
	}

	bool hit = false;
	size_t last_hit = 0;
	for(size_t ix = 0; ix < list->size; ix++) {
		if(list->items[ix] != item) {
			continue;
		}

		if(!hit) {
			free(list->items[ix]);
		}

		list->items[ix] = NULL;
		hit = true;
		last_hit = ix;
	}

	if(hit && last_hit == list->size - 1) {
		list->size--;
	}
}

void
cptrlist_destroy(CPtrList *list)
{
	if(list == NULL) {
		return;
	}

	for(size_t ix = 0; ix < list->size; ix++) {
		cptrlist_free(list, list->items[ix]);
	}

	free(list->items);
}
