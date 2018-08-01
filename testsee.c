/**

Copyright (c) Scott Gasch

Module Name:

    testsee.c

Abstract:

    This code is meant to sanity check the SEE routine.  It works by
    using the generator and MakeMove/UnmakeMove to play out a sequence
    of moves on a single square.  It's somewhat useful code but right
    now I don't really use it because it is too good: it detects
    things like pieces that are pinned against the king whereas the
    SEE itself does not.  Still, I'd want to run it and verify the
    discrepencies if I was doing much work in the SEE code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 21 Oct 2004

Revision History:

**/

#include "chess.h"

#ifdef TEST_BROKEN
ULONG g_uRootOnMove;

SCORE
DebugSEERecursive(SEARCHER_THREAD_CONTEXT *ctx,
                  MOVE mv)
{
    SCORE i, iMinMax;
    ULONG x;
    MOVE mvReply;
    ULONG uToMove = ctx->sPosition.uToMove;

    //
    // Make the move, this is not optional
    //
    ASSERT(GET_COLOR(mv.pMoved) == uToMove);
    if (FALSE == MakeMove(ctx, mv))
    {
        return(INVALID_SCORE);
    }

    //
    // Initialize MinMax to the stand pat score here so that the reply
    // can be "something else".
    //
    iMinMax = ((ctx->sPosition.uPawnMaterial[g_uRootOnMove] +
                ctx->sPosition.uNonPawnMaterial[g_uRootOnMove]) -
               (ctx->sPosition.uPawnMaterial[FLIP(g_uRootOnMove)] +
                ctx->sPosition.uNonPawnMaterial[FLIP(g_uRootOnMove)]));

    //
    // Generate the reply moves.
    //
    GenerateMoves(ctx, (MOVE){0}, GENERATE_DONT_SCORE);
    for (x = ctx->sMoveStack.uBegin[ctx->uPly];
         x < ctx->sMoveStack.uEnd[ctx->uPly];
         x++)
    {
        //
        // Only consider replies that end up on the same sq as the
        // move.
        //
        mvReply = ctx->sMoveStack.mvf[x].mv;
        ASSERT(SanityCheckMove(&ctx->sPosition, mvReply));
        if (mvReply.cTo != mv.cTo)
        {
            continue;
        }
        
        i = DebugSEERecursive(ctx, mvReply);
        if (INVALID_SCORE != i)
        {
            if ((ctx->uPly % 2) == 0)
            {
                if (i > iMinMax) iMinMax = i;
            }
            else
            {
                if (i < iMinMax) iMinMax = i;
            }
        }
    }

    //
    // Unmake the original move
    //
    UnmakeMove(ctx, mv);
    return(iMinMax);
}

    
SCORE
DebugSEE(POSITION *pos,
         MOVE mv)
{
    SCORE iRootBalance = ((pos->uPawnMaterial[pos->uToMove] +
                           pos->uNonPawnMaterial[pos->uToMove]) -
                          (pos->uPawnMaterial[FLIP(pos->uToMove)] +
                           pos->uNonPawnMaterial[FLIP(pos->uToMove)]));
    SCORE iAfterExchange = iRootBalance;
    SEARCHER_THREAD_CONTEXT *ctx = 
        malloc(sizeof(SEARCHER_THREAD_CONTEXT));

    g_uRootOnMove = pos->uToMove;
    if (NULL != ctx)
    {
        pos->uDangerCount[BLACK] = pos->uDangerCount[WHITE] = 0;
        InitializeSearcherContext(pos, ctx);
        
        GenerateMoves(ctx, (MOVE){0}, GENERATE_DONT_SCORE);
        mv.bvFlags |= WouldGiveCheck(ctx, mv);
        if (InCheck(pos, pos->uToMove))
        {
            mv.bvFlags |= MOVE_FLAG_ESCAPING_CHECK;
        }

        iAfterExchange = DebugSEERecursive(ctx, mv);
        if (iAfterExchange != INVALID_SCORE)
        {
            iAfterExchange -= iRootBalance;
        }
        free(ctx);
    }
    return(iAfterExchange);
}
#endif

#ifdef TEST
FLAG 
SeeListsAreEqual(SEE_LIST *pA, SEE_LIST *pB)
{
    ULONG u;
    if (pA->uCount != pB->uCount) return FALSE;
    for (u = 0; u < pA->uCount; u++) 
    {
        if ((pA->data[u].pPiece != pB->data[u].pPiece) ||
            (pA->data[u].cLoc != pB->data[u].cLoc) ||
            (pA->data[u].uVal != pB->data[u].uVal)) 
        {
            return FALSE;
        }
    }
    return TRUE;
}

void
TestGetAttacks(void) 
{
    POSITION pos;
    ULONG u;
    COOR c;
    SEE_LIST rgSlowList;
    SEE_LIST rgAsmList;
    ULONG color;
    
#ifndef _X86_
    return;
#endif
    
    Trace("Testing GetAttacks...\n");
    for (u = 0; u < 20000; u++)
    {
        GenerateRandomLegalPosition(&pos);
        FOREACH_SQUARE(c) 
        {
            if (!IS_ON_BOARD(c)) continue;
            for (color = BLACK; color <= WHITE; color++) 
            {
                SlowGetAttacks(&rgSlowList,
                               &pos,
                               c,
                               color);
                GetAttacks(&rgAsmList,
                           &pos,
                           c,
                           color);
                if (!SeeListsAreEqual(&rgSlowList, &rgAsmList))
                {
                    UtilPanic(TESTCASE_FAILURE,
                              &pos, 
                              "SEE_LIST mismatch", &rgSlowList, &rgAsmList,
                              __FILE__, __LINE__);
                }
            }
        }
    }
}
#endif // TEST
