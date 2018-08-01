/**

Copyright (c) Scott Gasch

Module Name:

    pawnhash.c

Abstract:

    Pawn structure score hashing code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 14 Jun 2004

Revision History:

    $Id: pawnhash.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"


PAWN_HASH_ENTRY *
PawnHashLookup(SEARCHER_THREAD_CONTEXT *ctx) 
/**

Routine description:

    Called by eval with a pointer to a POSITION, look in our pawn hash
    to see if we have an entry that matches the pawn structure in the
    POSITION.  If so, "check out" that entry and return a pointer to
    it.

    Note: there is a multi-probing scheme in place to remove
    contention on MP machines.

Parameters:

    POSITION *pos

Return value:

    PAWN_HASH_ENTRY *

**/
{
    POSITION *pos = &(ctx->sPosition);
    ULONG u = (ULONG)pos->u64PawnSig & (PAWN_HASH_TABLE_SIZE - 1);
    return(&(ctx->rgPawnHash[u]));
}
