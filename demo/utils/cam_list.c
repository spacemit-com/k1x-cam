/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#include "cam_list.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------ */

#define LIST_MAGIC_ID 0x4C495354

#define MEMORY_MALLOC_OBJECT(type) ((type *)malloc(sizeof(type)))

#define MEMORY_ZERO_OBJECT(objectPtr)               \
    do {                                            \
        memset((objectPtr), 0, sizeof(*objectPtr)); \
    } while (0)

#define MEMORY_FREE(pointer)  \
    do {                      \
        if (pointer) {        \
            free(pointer);    \
            (pointer) = NULL; \
        }                     \
    } while (0)

struct LIST;

typedef struct LIST_NODE {
    void *item;
    struct LIST *list;
    struct LIST_NODE *previous;
    struct LIST_NODE *next;
} LIST_NODE_S, *LIST_NODE_S_PTR;

typedef struct LIST {
    uint32_t magicId;
    bool allowRepeatItem;
    ssize_t size;
    LIST_NODE_S *head;
    LIST_NODE_S *tail;
    const LIST_NODE_S *lastFound;
} LIST_S, *LIST_S_PTR;

inline static LIST_S *List_HandleToList(LIST_HANDLE handle)
{
    LIST_S *result = (LIST_S *)handle;

    return ((result && (LIST_MAGIC_ID == result->magicId)) ? result : NULL);
}

inline static LIST_HANDLE List_ListToHandle(const LIST_S *list)
{
    return ((LIST_HANDLE)list);
}

inline static LIST_ITERATOR_S *List_NodeToIterator(const LIST_NODE_S *node)
{
    return ((LIST_ITERATOR_S *)node);
}

inline static LIST_NODE_S *List_IteratorToNode(const LIST_ITERATOR_S *iterator)
{
    LIST_NODE_S *result = (LIST_NODE_S *)iterator;

    return ((result && result->list && (LIST_MAGIC_ID == result->list->magicId)) ? result : NULL);
}

LIST_HANDLE List_Create(bool allowRepeatItem)
{
    LIST_S *result = MEMORY_MALLOC_OBJECT(LIST_S);

    if (!result) {
        return NULL;
    }

    MEMORY_ZERO_OBJECT(result);
    result->magicId = LIST_MAGIC_ID;
    result->allowRepeatItem = allowRepeatItem;

    return List_ListToHandle(result);
}

bool List_Destroy(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);

    if (!list) {
        return false;
    }

    List_Clear(handle);
    list->magicId = 0;

    MEMORY_FREE(list);

    return true;
}

inline ssize_t List_GetSize(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);

    return (list ? list->size : -1);
}

inline bool List_IsEmpty(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);

    return (list ? (list->size < 1) : true);
}

LIST_ITERATOR_S *List_Push(LIST_HANDLE handle, const void *item)
{
    LIST_S *list = List_HandleToList(handle);
    LIST_NODE_S *result = NULL;

    if (!list || (!list->allowRepeatItem && List_Find(handle, item))) {
        return NULL;
    }

    result = MEMORY_MALLOC_OBJECT(LIST_NODE_S);

    if (!result) {
        return NULL;
    }

    MEMORY_ZERO_OBJECT(result);
    result->item = (void *)item;
    result->list = list;
    result->previous = list->tail;

    if (!list->head) {
        list->head = result;
    }

    if (list->tail) {
        list->tail->next = result;
    }

    list->tail = result;
    list->size++;

    return List_NodeToIterator(result);
}

void *List_Pop(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);
    void *result = NULL;

    if (!list) {
        return NULL;
    }

    if (list->head) {
        result = list->head->item;
    }

    List_EraseByIterator(List_NodeToIterator(list->head));

    return result;
}

LIST_ITERATOR_S *List_InsertByIterator(LIST_ITERATOR_S *insertIterator, const void *item)
{
    LIST_NODE_S *node = List_IteratorToNode(insertIterator);
    LIST_NODE_S *result = NULL;

    if (!node || (!node->list->allowRepeatItem && List_Find(List_ListToHandle(node->list), item))) {
        return NULL;
    }

    result = MEMORY_MALLOC_OBJECT(LIST_NODE_S);

    if (!result) {
        return NULL;
    }

    MEMORY_ZERO_OBJECT(result);
    result->item = (void *)item;
    result->list = node->list;
    result->previous = node->previous;
    result->next = node;
    node->previous = result;

    if (result->previous) {
        result->previous->next = result;
    }

    if (node->list->head == node) {
        node->list->head = result;
    }

    node->list->size++;

    return List_NodeToIterator(result);
}

inline LIST_ITERATOR_S *List_InsertByItem(LIST_HANDLE handle, const void *insertItem, const void *item)
{
    return List_InsertByIterator(List_Find(handle, insertItem), item);
}

LIST_ITERATOR_S *List_EraseByIterator(LIST_ITERATOR_S *iterator)
{
    LIST_NODE_S *node = List_IteratorToNode(iterator);
    LIST_NODE_S *result = NULL;

    if (!node) {
        return NULL;
    }

    result = node->next;
    node->list->lastFound = result;

    if (node->previous) {
        node->previous->next = node->next;
    } else {
        node->list->head = node->next;

        if (node->list->head) {
            node->list->head->previous = NULL;
        }
    }

    if (node->next) {
        node->next->previous = node->previous;
    } else {
        node->list->tail = node->previous;

        if (node->list->tail) {
            node->list->tail->next = NULL;
        }
    }

    node->list->size--;
    node->list = NULL;

    MEMORY_FREE(node);

    return List_NodeToIterator(result);
}

bool List_EraseByItem(LIST_HANDLE handle, const void *item)
{
    LIST_ITERATOR_S *iterator = List_Find(handle, item);

    if (!iterator) {
        return false;
    }

    List_EraseByIterator(iterator);

    return true;
}

bool List_Clear(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);
    LIST_NODE_S *node = NULL;

    if (!list) {
        return false;
    }

    while (list->head) {
        node = list->head;
        list->head = list->head->next;
        node->list = NULL;

        MEMORY_FREE(node);
    }

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    return true;
}

inline LIST_ITERATOR_S *List_GetBeginIterator(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);

    return (list ? List_NodeToIterator(list->head) : NULL);
}

inline void *List_GetBeginItem(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);

    return ((list && list->head) ? list->head->item : NULL);
}

inline LIST_ITERATOR_S *List_GetEndIterator(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);

    return (list ? List_NodeToIterator(list->tail) : NULL);
}

inline void *List_GetEndItem(LIST_HANDLE handle)
{
    LIST_S *list = List_HandleToList(handle);

    return ((list && list->tail) ? list->tail->item : NULL);
}

inline LIST_ITERATOR_S *List_GetPreviousIterator(const LIST_ITERATOR_S *iterator)
{
    LIST_NODE_S *node = List_IteratorToNode(iterator);

    return (node ? List_NodeToIterator(node->previous) : NULL);
}

inline void *List_GetPreviousItem(LIST_HANDLE handle, const void *item)
{
    LIST_ITERATOR_S *iterator = List_GetPreviousIterator(List_Find(handle, item));

    return (iterator ? iterator->item : NULL);
}

inline LIST_ITERATOR_S *List_GetNextIterator(const LIST_ITERATOR_S *iterator)
{
    LIST_NODE_S *node = List_IteratorToNode(iterator);

    return (node ? List_NodeToIterator(node->next) : NULL);
}

inline void *List_GetNextItem(LIST_HANDLE handle, const void *item)
{
    LIST_ITERATOR_S *iterator = List_GetNextIterator(List_Find(handle, item));

    return (iterator ? iterator->item : NULL);
}

inline static bool List_Equal(const void *item, const void *condition)
{
    return (item == condition);
}

static LIST_ITERATOR_S *List_FindRangeIf(const LIST_NODE_S *beginNode, const LIST_NODE_S *endNode,
                                         List_OnExternalFind onExternalFind, const void *condition, bool reverseFind)
{
    LIST_S *list = NULL;
    const LIST_NODE_S *result = NULL;
    const LIST_NODE_S *lastFound = NULL;

    if (!beginNode || !endNode || (beginNode->list != endNode->list) || !onExternalFind) {
        return NULL;
    }

    list = beginNode->list;
    lastFound = list->lastFound;

    if (lastFound) {
        result = lastFound;
    } else {
        result = reverseFind ? endNode : beginNode;
    }

    while (result) {
        bool changed = false;

        if (onExternalFind(result->item, condition)) {
            list->lastFound = result;

            return List_NodeToIterator(result);
        }

        if (lastFound) {
            if (reverseFind && (result == beginNode)) {
                result = endNode;
                changed = true;
            } else if (!reverseFind && (result == endNode)) {
                result = beginNode;
                changed = true;
            }
        } else {
            if ((reverseFind && (result == beginNode)) || (!reverseFind && (result == endNode))) {
                return NULL;
            }
        }

        if (!lastFound || !changed) {
            if (reverseFind) {
                result = result->previous;
            } else {
                result = result->next;
            }
        }

        if (lastFound && (result == lastFound)) {
            return NULL;
        }
    }

    return NULL;
}

inline LIST_ITERATOR_S *List_Find(LIST_HANDLE handle, const void *item)
{
    LIST_S *list = List_HandleToList(handle);

    return (list ? List_FindRangeIf(list->head, list->tail, List_Equal, item, false) : NULL);
}

inline LIST_ITERATOR_S *List_FindIteratorIf(LIST_HANDLE handle, List_OnExternalFind onExternalFind,
                                            const void *condition)
{
    LIST_S *list = List_HandleToList(handle);

    return (list ? List_FindRangeIf(list->head, list->tail, onExternalFind, condition, false) : NULL);
}

inline void *List_FindItemIf(LIST_HANDLE handle, List_OnExternalFind onExternalFind, const void *condition)
{
    LIST_ITERATOR_S *result = List_FindIteratorIf(handle, onExternalFind, condition);

    return (result ? result->item : NULL);
}

inline LIST_ITERATOR_S *List_FindRange(const LIST_ITERATOR_S *beginIterator, const LIST_ITERATOR_S *endIterator,
                                       const void *item, bool reverseFind)
{
    return List_FindRangeIf(List_IteratorToNode(beginIterator), List_IteratorToNode(endIterator), List_Equal, item,
                            reverseFind);
}

inline LIST_ITERATOR_S *List_FindRangeIteratorIf(const LIST_ITERATOR_S *beginIterator,
                                                 const LIST_ITERATOR_S *endIterator, List_OnExternalFind onExternalFind,
                                                 const void *condition, bool reverseFind)
{
    return List_FindRangeIf(List_IteratorToNode(beginIterator), List_IteratorToNode(endIterator), onExternalFind,
                            condition, reverseFind);
}

inline void *List_FindRangeItemIf(const LIST_ITERATOR_S *beginIterator, const LIST_ITERATOR_S *endIterator,
                                  List_OnExternalFind onExternalFind, const void *condition, bool reverseFind)
{
    LIST_ITERATOR_S *result =
        List_FindRangeIteratorIf(beginIterator, endIterator, onExternalFind, condition, reverseFind);

    return (result ? result->item : NULL);
}

static bool List_ForeachRangeIf(const LIST_NODE_S *beginNode, const LIST_NODE_S *endNode, List_OnForeach onForeach,
                                void *privateData, bool reverseForeach)
{
    const LIST_NODE_S *current = NULL;
    const LIST_NODE_S *next = NULL;

    if (!beginNode || !endNode || (beginNode->list != endNode->list) || !onForeach) {
        return false;
    }

    current = reverseForeach ? endNode : beginNode;

    while (current) {
        if (reverseForeach) {
            next = current->previous;
        } else {
            next = current->next;
        }

        if (!onForeach(current->item, privateData) || (reverseForeach && (current == beginNode))
            || (!reverseForeach && (current == endNode))) {
            break;
        }

        current = next;
    }

    return true;
}

inline bool List_Foreach(LIST_HANDLE handle, List_OnForeach onForeach, void *privateData)
{
    LIST_S *list = List_HandleToList(handle);

    return (list ? List_ForeachRangeIf(list->head, list->tail, onForeach, privateData, false) : false);
}

inline bool List_ForeachRange(const LIST_ITERATOR_S *beginIterator, const LIST_ITERATOR_S *endIterator,
                              List_OnForeach onForeach, void *privateData, bool reverseForeach)
{
    return List_ForeachRangeIf(List_IteratorToNode(beginIterator), List_IteratorToNode(endIterator), onForeach,
                               privateData, reverseForeach);
}
