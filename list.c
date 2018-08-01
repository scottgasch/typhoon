/*++

Copyright (c) Scott Gasch

Module Name:

    list.c

Abstract:

    Random doubly linked list helper routines.

Author:

    Scott Gasch (scott.gasch@gmail.com) 29 Nov 2007

Revision History:

--*/

#include "chess.h"

INLINE void
InitializeListHead(IN DLIST_ENTRY *pListHead)
{
    pListHead->pFlink = pListHead->pBlink = pListHead;
}

INLINE FLAG
IsListEmpty(IN DLIST_ENTRY *pListHead)
{
    return (FLAG)(pListHead->pFlink == pListHead);
}

INLINE FLAG
RemoveEntryList(IN DLIST_ENTRY *pEntry)
{
    DLIST_ENTRY *pBlink;
    DLIST_ENTRY *pFlink;

    pFlink = pEntry->pFlink;
    pBlink = pEntry->pBlink;
    pBlink->pFlink = pFlink;
    pFlink->pBlink = pBlink;
    return (FLAG)(pFlink == pBlink);
}

INLINE DLIST_ENTRY *
RemoveHeadList(IN DLIST_ENTRY *pListHead)
{
    DLIST_ENTRY *pFlink;
    DLIST_ENTRY *pEntry;

    pEntry = pListHead->pFlink;
    pFlink = pEntry->pFlink;
    pListHead->pFlink = pFlink;
    pFlink->pBlink = pListHead;

    return(pEntry);
}

INLINE DLIST_ENTRY *
RemoveTailList(IN DLIST_ENTRY *pListHead)
{
    DLIST_ENTRY *pBlink;
    DLIST_ENTRY *pEntry;

    pEntry = pListHead->pBlink;
    pBlink = pEntry->pBlink;
    pListHead->pBlink = pBlink;
    pBlink->pFlink = pListHead;

    return(pEntry);
}

INLINE void
InsertTailList(IN DLIST_ENTRY *pListHead,
               IN DLIST_ENTRY *pEntry)
{
    DLIST_ENTRY *pBlink;

    pBlink = pListHead->pBlink;
    pEntry->pFlink = pListHead;
    pEntry->pBlink = pBlink;
    pBlink->pFlink = pEntry;
    pListHead->pBlink = pEntry;
}

INLINE void
InsertHeadList(IN DLIST_ENTRY *pListHead,
               IN DLIST_ENTRY *pEntry)
{
    DLIST_ENTRY *pFlink;

    pFlink = pListHead->pFlink;
    pEntry->pFlink = pFlink;
    pEntry->pBlink = pListHead;
    pFlink->pBlink = pEntry;
    pListHead->pFlink = pEntry;
}
