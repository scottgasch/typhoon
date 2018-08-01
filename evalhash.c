/*++

Copyright (c) Scott Gasch

Module Name:

    evalhash.c

Abstract:

    Evaluation score hashing code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 11 Jul 2006

Revision History:

--*/

#include "chess.h"
#ifdef EVAL_HASH

extern ULONG g_uIterateDepth;

SCORE 
GetRoughEvalScore(IN OUT SEARCHER_THREAD_CONTEXT *ctx, 
                  IN SCORE iAlpha,
                  IN SCORE iBeta,
                  IN FLAG fUseHash)
/*++

Routine description:

    This is meant to be the "one place" that code looks when it needs to
    get an idea of the estimated evaluation of a position (in mid tree,
    not at the leaves... at the leaves just call Eval directly).
    
    The thought is that:
    
    1. The top of the tree (near the root) represents very few nodes
    which have big subtrees.  Use the real Eval here.
    
    2. Otherwise possibly probe the eval hash -- if there's a score in
    it then we can return quickly.
    
    3. Otherwise do a (very) rough material estimate.

Parameters:

    IN OUT SEARCHER_THREAD_CONTEXT *ctx,
    IN SCORE iAlpha,
    IN SCORE iBeta,
    IN FLAG fUseHash

Return value:

    SCORE

--*/
{
    POSITION *pos = &(ctx->sPosition);
    UINT64 u64Key;
    ULONG u;

    if (ctx->uPly <= 4)
    {
        return Eval(ctx, iAlpha, iBeta);
    }
    else if ((fUseHash) || (ctx->uPly <= (g_uIterateDepth / 2)))
    {
        u64Key = (pos->u64PawnSig ^ pos->u64NonPawnSig);
        u = (ULONG)u64Key & (EVAL_HASH_TABLE_SIZE - 1);
        if (ctx->rgEvalHash[u].u64Key == u64Key)
        {
            ctx->uPositional = ctx->rgEvalHash[u].uPositional;
            return(ctx->rgEvalHash[u].iEval);
        }
    }
    return(pos->iMaterialBalance[pos->uToMove] + ctx->uPositional);
}


SCORE 
ProbeEvalHash(IN OUT SEARCHER_THREAD_CONTEXT *ctx)
/*++

Routine description:

    Probe the eval hash; return a real score if there's a hit
    otherwise return INVALID_SCORE.

Parameters:

    IN OUT SEARCHER_THREAD_CONTEXT *ctx : note that in the case
        of a hit, *ctx is modified also.

Return value:

    SCORE

--*/
{
    POSITION *pos = &(ctx->sPosition);
    UINT64 u64Key = (pos->u64PawnSig ^ pos->u64NonPawnSig);
    ULONG u = (ULONG)u64Key & (EVAL_HASH_TABLE_SIZE - 1);

    if (ctx->rgEvalHash[u].u64Key == u64Key)
    {
        ctx->uPositional = ctx->rgEvalHash[u].uPositional;
        pos->cTrapped[WHITE] = ctx->rgEvalHash[u].cTrapped[WHITE];
        pos->cTrapped[BLACK] = ctx->rgEvalHash[u].cTrapped[BLACK];
        return(ctx->rgEvalHash[u].iEval);
    }
    return(INVALID_SCORE);
}

void 
StoreEvalHash(IN OUT SEARCHER_THREAD_CONTEXT *ctx, 
              IN SCORE iScore)
/*++

Routine description:

    Store a score in the eval hash table (which is pointed to
    indirectly via ctx).

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    SCORE iScore

Return value:

    void

--*/
{
    POSITION *pos = &(ctx->sPosition);
    UINT64 u64Key = (pos->u64PawnSig ^ pos->u64NonPawnSig);
    ULONG u = (ULONG)u64Key & (EVAL_HASH_TABLE_SIZE - 1);
    ctx->rgEvalHash[u].u64Key = u64Key;
    ctx->rgEvalHash[u].iEval = iScore;
    ctx->rgEvalHash[u].uPositional = ctx->uPositional;
    ctx->rgEvalHash[u].cTrapped[WHITE] = pos->cTrapped[WHITE];
    ctx->rgEvalHash[u].cTrapped[BLACK] = pos->cTrapped[BLACK];
}
#endif // EVAL_HASH
