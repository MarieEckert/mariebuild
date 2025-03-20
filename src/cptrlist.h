/* cptrlist.h ; C Pointer List Header
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

#ifndef ME_CPTRLIST_H
#define ME_CPTRLIST_H

#include <stdbool.h>
#include <stddef.h>

#include <sys/types.h>

typedef struct {
	size_t capacity;
	size_t size;
	void **items;

	bool allow_resize;
	size_t resize_align;
} CPtrList;

/**
 * @brief Initialise a new list with given capacity.
 * The initial allocation will be made with the exact value of capacity.
 * @param list Pointer to where the list should be initialised at.
 * @return Success?
 */
bool cptrlist_init(CPtrList *list, size_t capacity, size_t resize_align);

/**
 * @brief Append an item to the list.
 * If list->allow_resize is true and there is no space left,
 * the list will be reallocated to be equal to:
 * 	(list->capacity + list->resize_align) - ~(list->resize_align - 1)
 * @return The index of the appended item or < 0 on error.
 */
ssize_t cptrlist_append(CPtrList *list, void *item);

/**
 * @brief Append or insert an item to the list.
 * Searches for the first index in the list which is equal to NULL and
 * inserts the item there. If there is no free slot and list->allow_resize
 * is true, the list will be reallocated to be equal to:
 * 	(list->capacity + list->resize_align) - ~(list->resize_align - 1)
 * @return The index of the appended/inserted item or < 0 on error.
 */
ssize_t cptrlist_insert_or_append(CPtrList *list, void *item);

/**
 * @brief Find an item in the list by value using the provided
 * search function.
 * @return The index of the member with the same value or < 0 on error.
 */
ssize_t cptrlist_find(CPtrList *list,
					  void *search,
					  bool (*search_func)(void *, void *));

/**
 * @brief Calls free on the item at given index and sets the slot to NULL.
 */
void cptrlist_free_at(CPtrList *list, size_t index);

/**
 * @brief Searches for the given pointer in the list and calls free on the
 * first hit. Every instance of the pointer within the list is then set to
 * NULL.
 */
void cptrlist_free(CPtrList *list, void *item);

/**
 * @brief Destroy the list, frees every non-NULL item and then list itself.
 * Internally uses cptrlist_free for each item, so it could be quite slow.
 */
void cptrlist_destroy(CPtrList *list);

#endif
