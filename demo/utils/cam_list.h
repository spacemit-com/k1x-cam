/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _CAM_LIST_H_
#define _CAM_LIST_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

/* ------------------------------------------------------------------------ */

typedef void *LIST_HANDLE;

typedef struct LIST_ITERATOR {
    void *const item;
} LIST_ITERATOR_S, *LIST_ITERATOR_S_PTR;

typedef bool (*List_OnExternalFind)(const void *item, const void *condition);

typedef bool (*List_OnForeach)(void *item, void *privateData);

/* ------------------------------------------------------------------------ */

LIST_HANDLE List_Create(bool allowRepeatItem);

bool List_Destroy(LIST_HANDLE handle);

ssize_t List_GetSize(LIST_HANDLE handle);

bool List_IsEmpty(LIST_HANDLE handle);

LIST_ITERATOR_S *List_Push(LIST_HANDLE handle, const void *item);

void *List_Pop(LIST_HANDLE handle);

LIST_ITERATOR_S *List_InsertByIterator(LIST_ITERATOR_S *insertIterator, const void *item);

LIST_ITERATOR_S *List_InsertByItem(LIST_HANDLE handle, const void *insertItem, const void *item);

LIST_ITERATOR_S *List_EraseByIterator(LIST_ITERATOR_S *iterator);

bool List_EraseByItem(LIST_HANDLE handle, const void *item);

bool List_Clear(LIST_HANDLE handle);

LIST_ITERATOR_S *List_GetBeginIterator(LIST_HANDLE handle);

void *List_GetBeginItem(LIST_HANDLE handle);

LIST_ITERATOR_S *List_GetEndIterator(LIST_HANDLE handle);

void *List_GetEndItem(LIST_HANDLE handle);

LIST_ITERATOR_S *List_GetPreviousIterator(const LIST_ITERATOR_S *iterator);

void *List_GetPreviousItem(LIST_HANDLE handle, const void *item);

LIST_ITERATOR_S *List_GetNextIterator(const LIST_ITERATOR_S *iterator);

void *List_GetNextItem(LIST_HANDLE handle, const void *item);

LIST_ITERATOR_S *List_Find(LIST_HANDLE handle, const void *item);

LIST_ITERATOR_S *List_FindIteratorIf(LIST_HANDLE handle, List_OnExternalFind onExternalFind, const void *condition);

void *List_FindItemIf(LIST_HANDLE handle, List_OnExternalFind onExternalFind, const void *condition);

LIST_ITERATOR_S *List_FindRange(const LIST_ITERATOR_S *beginIterator, const LIST_ITERATOR_S *endIterator,
                                const void *item, bool reverseFind);

LIST_ITERATOR_S *List_FindRangeIteratorIf(const LIST_ITERATOR_S *beginIterator, const LIST_ITERATOR_S *endIterator,
                                          List_OnExternalFind onExternalFind, const void *condition, bool reverseFind);

void *List_FindRangeItemIf(const LIST_ITERATOR_S *beginIterator, const LIST_ITERATOR_S *endIterator,
                           List_OnExternalFind onExternalFind, const void *condition, bool reverseFind);

bool List_Foreach(LIST_HANDLE handle, List_OnForeach onForeach, void *privateData);

bool List_ForeachRange(const LIST_ITERATOR_S *beginIterator, const LIST_ITERATOR_S *endIterator,
                       List_OnForeach onForeach, void *privateData, bool reverseForeach);

/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LIST_H_ */
