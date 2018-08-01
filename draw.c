/**

Copyright (c) Scott Gasch

Module Name:

    draw.c

Abstract:

    Function for detecting a draw by repetition from the search tree.

Author:

    Scott Gasch (scott.gasch@gmail.com) 19 May 2004

Revision History:

    $Id: draw.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

FLAG
IsDraw(SEARCHER_THREAD_CONTEXT *ctx)
{
    ULONG uPly;
    UINT64 u64CurrentSig;
    
    //
    // Recognize 50-moves w/o progress draw rule
    // 
    if (ctx->sPosition.uFifty >= 100)
    {
        return(TRUE);
    }

    //
    // Check for repeated positions if needed.
    // 
    if (ctx->sPosition.uFifty < 4)
    {
        return(FALSE);
    }

    u64CurrentSig = (ctx->sPosition.u64NonPawnSig ^
                     ctx->sPosition.u64PawnSig);
    uPly = ctx->uPly - 2;
    while(uPly < MAX_PLY_PER_SEARCH)
    {
#ifdef DEBUG
        if ((GET_COLOR(ctx->sPlyInfo[uPly].mv.pMoved) != 
             ctx->sPosition.uToMove) &&
            (ctx->sPlyInfo[uPly].mv.uMove != 0))
        {
            ASSERT(FALSE);
        }
        ASSERT(ctx->sPlyInfo[uPly].u64Sig == (ctx->sPlyInfo[uPly].u64PawnSig ^
                                           ctx->sPlyInfo[uPly].u64NonPawnSig));
#endif
        if (ctx->sPlyInfo[uPly].u64Sig == u64CurrentSig)
        {
            return(TRUE);
        }
        
        if (IS_PAWN(ctx->sPlyInfo[uPly].mv.pMoved) ||
            (ctx->sPlyInfo[uPly].mv.pCaptured))
        {
            return(FALSE);
        }
        uPly -= 2;
    }
    
    //
    // Keep looking in the official game record.
    //
    return(DoesSigAppearInOfficialGameList(u64CurrentSig));
}
