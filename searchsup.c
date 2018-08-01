/**

Copyright (c) Scott Gasch

Module Name:

    searchsup.c

Abstract:

    Search support routines.  See also search.c, root.c, and split.c.

Author:

    Scott Gasch (scott.gasch@gmail.com) 06 Oct 2005

Revision History:

**/

#include "chess.h"

extern SCORE g_iRootScore[2];
extern ULONG g_uHardExtendLimit;
extern ULONG g_uIterateDepth;

void
UpdatePV(SEARCHER_THREAD_CONTEXT *ctx, MOVE mv)
/**

Routine description:

    Update the principle variation at this depth.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    MOVE mv

Return value:

    void

**/
{
    PLY_INFO *this = &(ctx->sPlyInfo[ctx->uPly]);
    PLY_INFO *next = (this + 1);
    ULONG u = ctx->uPly;

    ASSERT(u < MAX_PLY_PER_SEARCH);
    this->PV[u] = mv;
    u++;
    while(u < MAX_PLY_PER_SEARCH)
    {
        this->PV[u] = next->PV[u];
        if (0 == this->PV[u].uMove)
        {
            break;
        }
        u++;
    }
}


FLAG
CheckInputAndTimers(IN SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    This code is called from search when the total number of nodes
    searched is a multiple of g_MoveTimer.uNodeCheckMask.  Its job is
    to check for user input waiting on the input queue and to check to
    see if we are over a time limit.

Parameters:

    void

Return value:

    static FLAG : TRUE if the search should terminate and unroll,
                  FALSE if the search can continue

**/
{
    double dTimeStamp;

    //
    // See if there's user input to process.
    //
    if ((ctx->uThreadNumber == 0) && (NumberOfPendingInputEvents() != 0))
    {
        ParseUserInput(TRUE);

        //
        // If the user input can be handled in the middle of the
        // search then it will have been.  If it cannot be handled
        // without stopping the search then ParseUserInput will flip
        // this bit in the MoveTimer to tell us (any any other
        // searcher threads) to stop and unroll.
        //
        if (WE_SHOULD_STOP_SEARCHING)
        {
            return(TRUE);
        }
    }

    //
    // Also check time limits now.
    //
    if (-1 != g_MoveTimer.dSoftTimeLimit)
    {
        dTimeStamp = SystemTimeStamp();
        if ((g_MoveTimer.bvFlags & TIMER_CURRENT_OBVIOUS) ||
            (g_MoveTimer.bvFlags & TIMER_CURRENT_WONT_UNBLOCK))
        {
            Trace("OBVIOUS MOVE / WON'T UNBLOCK --> stop searching now\n");
            g_MoveTimer.bvFlags |= TIMER_STOPPING;
            return(TRUE);
        }

        if (dTimeStamp > g_MoveTimer.dSoftTimeLimit)
        {
            //
            // If we have exceeded the soft time limit, move now unless...
            //
            if ((!(g_MoveTimer.bvFlags & TIMER_JUST_OUT_OF_BOOK)) &&
                (!(g_MoveTimer.bvFlags & TIMER_ROOT_POSITION_CRITICAL)) &&
                (!(g_MoveTimer.bvFlags & TIMER_RESOLVING_ROOT_FL)) &&
                (!(g_MoveTimer.bvFlags & TIMER_RESOLVING_ROOT_FH)) &&
                (!(g_MoveTimer.bvFlags & TIMER_MANY_ROOT_FLS)) &&
                (g_MoveTimer.bvFlags & TIMER_SEARCHING_FIRST_MOVE))
            {
                Trace("SOFT TIMER (%3.1f sec) --> "
                      "stop searching now\n",
                      dTimeStamp - g_MoveTimer.dStartTime);
                g_MoveTimer.bvFlags |= TIMER_STOPPING;
                return(TRUE);
            }
        }

        //
        // If we have exceeded the hard limit, we have to move no matter
        // what.
        //
        if (dTimeStamp > g_MoveTimer.dHardTimeLimit)
        {
            Trace("HARD TIMER (%3.1f sec) --> "
                  "stop searching now\n",
                  dTimeStamp - g_MoveTimer.dStartTime);
            g_MoveTimer.bvFlags |= TIMER_STOPPING;
            return(TRUE);
        }
    }

    // 
    // Also check raw node count limit here.
    //
    if (g_Options.u64MaxNodeCount != 0ULL &&
        ctx->sCounters.tree.u64TotalNodeCount > g_Options.u64MaxNodeCount) 
    {
        Trace("NODE COUNT LIMIT (limit %llu, searched %llu) "
              "--> stop searching now\n", g_Options.u64MaxNodeCount,
              ctx->sCounters.tree.u64TotalNodeCount);
        g_MoveTimer.bvFlags |= TIMER_STOPPING;
        return(TRUE);
    }
    return(FALSE);
}


FLAG
ThreadUnderTerminatingSplit(IN SEARCHER_THREAD_CONTEXT *ctx)
{
    ULONG u;

    for (u = 0; u < NUM_SPLIT_PTRS_IN_CONTEXT; u++)
    {
        if (ctx->pSplitInfo[u] != NULL)
        {
            if (ctx->pSplitInfo[u]->fTerminate == TRUE) return(TRUE);
            return(FALSE);
        }
    }
    return(FALSE);
}


FLAG
WeShouldDoHistoryPruning(IN SCORE iRoughEval,
                         IN SCORE iAlpha,
                         IN SCORE iBeta,
                         IN SEARCHER_THREAD_CONTEXT *ctx,
                         IN ULONG uRemainingDepth,
                         IN ULONG uLegalMoves,
                         IN MOVE mv,
                         IN ULONG uMoveNum,
                         IN INT iExtend)
/**

Routine description:

    Decide whether or not to do history pruning at this node for the
    given move.  Note: this function is called after the move has
    been played on the board.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG uRemainingDepth,
    ULONG uLegalMoves,
    MOVE mv,
    INT iExtend

Return value:

    FLAG

**/
{
    ASSERT(ctx->uPly > 0);
    ASSERT(mv.uMove);
    ASSERT((uMoveNum > 0) || (uLegalMoves == 0));
    if ((uRemainingDepth >= TWO_PLY) &&
        (iBeta == (iAlpha + 1)) &&
        (uLegalMoves > 5) &&
        (0 == iExtend) &&
//        (iRoughEval + ComputeMoveScore(ctx, mv, uMoveNum - 1) + 200 < iAlpha) &&
        (!IS_ESCAPING_CHECK(mv)) &&
        (!IS_CAPTURE_OR_PROMOTION(mv)) &&
        (!IS_CHECKING_MOVE(mv)) &&
        (!IS_SAME_MOVE(mv, ctx->mvKiller[ctx->uPly-1][0])) &&
        (!IS_SAME_MOVE(mv, ctx->mvKiller[ctx->uPly-1][1])) &&
        ((ctx->uPly < 3) ||
         (!IS_SAME_MOVE(mv, ctx->mvKiller[ctx->uPly-3][0]) &&
          !IS_SAME_MOVE(mv, ctx->mvKiller[ctx->uPly-3][1]))) &&
        (GetMoveFailHighPercentage(mv) <= 10))
    {
        ASSERT(!InCheck(&ctx->sPosition, ctx->sPosition.uToMove));
        return(TRUE);
    }
    return(FALSE);
}


SCORE
ComputeMoveScore(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN MOVE mv,
                 IN ULONG uMoveNum) 
{
    SCORE iMoveScore = (PIECE_VALUE(mv.pCaptured) +
                        PIECE_VALUE(mv.pPromoted));
    if (uMoveNum != (ULONG)-1)
    {
        ASSERT(uMoveNum < MAX_MOVE_STACK);
        ASSERT(IS_SAME_MOVE(mv, ctx->sMoveStack.mvf[uMoveNum].mv));
        iMoveScore = ctx->sMoveStack.mvf[uMoveNum].iValue;
        if (iMoveScore >= SORT_THESE_FIRST)
        {
            ASSERT(iMoveScore > 0);
            iMoveScore &= STRIP_OFF_FLAGS;
        }
        else
        {
            iMoveScore = MIN0(iMoveScore);
        }
    }
    return(iMoveScore);
}


void
ComputeMoveExtension(IN SEARCHER_THREAD_CONTEXT *ctx,
                     IN SCORE iAlpha,
                     IN SCORE iBeta,
                     IN ULONG uMoveNum,
                     IN SCORE iRoughEval,
                     IN ULONG uDepth,
                     IN OUT INT *piExtend)
/**

Routine description:

    This routine is called by Search and by HelpSearch (in split.c).
    It's job is to assign a move-based extension for the move mv.
    This extension should be between -ONE_PLY (reduction) and +ONE_PLY
    (extension).

    Note: this code is executed after the move has already been played
    on the board!

Parameters:

    IN SEARCHER_THREAD_CONTEXT *ctx : the searcher thread's context
    IN SCORE iAlpha : current alpha
    IN SCORE iBeta : current beta
    IN ULONG uMoveNum : move number in stack
    IN ULONG uDepth : remaining depth
    IN OUT INT *piExtend : extension amount (in/out)

Return value:

    void

**/
{
    static const SCORE _window[MAX_PLY_PER_SEARCH] = {
         -1,  -1, 500, 500, 425, 425, 350, 350, 275, 275, 200, 200, // 0..11
        150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, // 12..23
        150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, // 24..35
        150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, // 36..47
        150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, // 48..59
        150, 150, 150, 150                                          // 60..63
    };
    POSITION *pos = &ctx->sPosition;
    MOVE mvLast = ctx->sPlyInfo[ctx->uPly-2].mv;
    MOVE mv = ctx->sPlyInfo[ctx->uPly-1].mv;
    ULONG uColor = GET_COLOR(mv.pMoved);
    SCORE iMoveScore;
#ifdef DEBUG
    int iM;

    //
    // Preconditions
    //
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT(ctx->uPly >= 2);
    ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);
    ASSERT(mv.uMove);
    ASSERT(*piExtend >= 0);
#endif

    // Checking move extension.
    if (IS_CHECKING_MOVE(mv))
    {
        ASSERT(InCheck(pos, pos->uToMove));
        INC(ctx->sCounters.extension.uCheck);
        *piExtend += ONE_PLY;
        if (pos->uNonPawnMaterial[FLIP(pos->uToMove)] >
            (VALUE_KING + VALUE_BISHOP))
        {
            goto enforce_ceiling;
        }
        *piExtend -= THREE_QUARTERS_PLY;
    }

    // Pawn push extension
    if (IS_PAWN(mv.pMoved))
    {
        if (((GET_COLOR(mv.pMoved) == WHITE) && RANK7(mv.cTo)) ||
            ((GET_COLOR(mv.pMoved) == BLACK) && RANK2(mv.cTo)))
        {
            *piExtend += THREE_QUARTERS_PLY;
            if (DISTANCE(pos->cNonPawns[GET_COLOR(mv.pMoved)][0], mv.cTo) < 3)
            {
                *piExtend += QUARTER_PLY;
            }
            INC(ctx->sCounters.extension.uPawnPush);
            goto enforce_ceiling;
        }
    }

    // Responses to check extensions:
    if (IS_CHECKING_MOVE(mvLast))
    {
        ASSERT(IS_ESCAPING_CHECK(mv));
        iMoveScore = iRoughEval + ComputeMoveScore(ctx, mv, uMoveNum);
        
        // One legal move in reply to check...
        if (ONE_LEGAL_MOVE(ctx, ctx->uPly - 1) &&
            CountKingSafetyDefects(&ctx->sPosition, uColor) > 1) 
        {
            *piExtend += (QUARTER_PLY +
                          HALF_PLY * 
                          (iMoveScore + VALUE_KNIGHT > iAlpha));
            INC(ctx->sCounters.extension.uOneLegalMove);
        }

        // Singular response to check...
        if ((mv.pCaptured) &&
            (iRoughEval + 225 < iAlpha) && 
            (iMoveScore + 75 > iAlpha))
        {
            *piExtend += ONE_PLY;
            INC(ctx->sCounters.extension.uSingularReplyToCheck);
            goto enforce_ceiling;
        }
    }

    // These last ones only happen if we are close to the root score.
    ASSERT(_window[ctx->uPly] > 0);
    if ((iRoughEval >= (g_iRootScore[uColor] - _window[ctx->uPly] * 2)) &&
        (iRoughEval <= (g_iRootScore[uColor] + _window[ctx->uPly] * 2)))
    {
        // Endgame extension
        if ((ctx->sPlyInfo[1].uTotalNonPawns > 2) &&
            ((pos->uNonPawnCount[BLACK][0] + 
              pos->uNonPawnCount[WHITE][0]) == 2))
        {
            if ((mv.pCaptured && !IS_PAWN(mv.pCaptured)) ||
                (mvLast.pCaptured && !IS_PAWN(mvLast.pCaptured)))
            {
                *piExtend += HALF_PLY;
                INC(ctx->sCounters.extension.uEndgame);
            }
        }

        // Recapture extension
        if ((iRoughEval >= (g_iRootScore[uColor] - _window[ctx->uPly])) &&
            (iRoughEval <= (g_iRootScore[uColor] + _window[ctx->uPly])))
        {
            if (uDepth <= THREE_PLY)
            {
                if ((mv.pCaptured) && !IS_PAWN(mv.pCaptured) &&
                    ((PIECE_VALUE(mv.pCaptured) == 
                      PIECE_VALUE(mvLast.pCaptured)) ||
                     ((mvLast.pPromoted) && (mv.cTo == mvLast.cTo))))
                {
                    iMoveScore = ComputeMoveScore(ctx, mv, uMoveNum);
                    if (iMoveScore >= 0)
                    {
                        *piExtend += HALF_PLY;
                        INC(ctx->sCounters.extension.uRecapture);
                    }
                }
            }
        }
    }

 enforce_ceiling:

    // Don't let extensions add up to > 1 ply
#ifdef DEBUG
    iM = MIN(ONE_PLY, *piExtend);
#endif
    ASSERT(!(*piExtend & 0x80000000));
    *piExtend = MINU(ONE_PLY, *piExtend);
    ASSERT(iM == *piExtend);

    // Scale back extensions that are very far from the root:
    //
    //        Iterate   g_Soft              g_Hard
    // |    |    |    |    |    |    |    |    |
    // 0         1         2   3/2   3         4       (x Iterate)
    // |-------------------|----|----|---------|----...
    // |full full full full|-1/4|-1/2|-3/4 -3/4|none...
    ASSERT(ctx->uPly != 0);
    ASSERT(ctx->uPly <= MAX_PLY_PER_SEARCH);
    *piExtend -= g_uExtensionReduction[ctx->uPly-1];
#ifdef DEBUG
    iM = MAX(0, *piExtend);
#endif
    *piExtend = MAX0(*piExtend);
    ASSERT(iM == *piExtend);
    ctx->sPlyInfo[ctx->uPly-1].iExtensionAmount = *piExtend;
    ASSERT((0 <= *piExtend) && (*piExtend <= ONE_PLY));
}




#ifdef DO_IID
SCORE
RescoreMovesViaSearch(IN SEARCHER_THREAD_CONTEXT *ctx,
                      IN ULONG uDepth,
                      IN SCORE iAlpha,
                      IN SCORE iBeta)
/**

Routine description:

    This is my implementation of "internal iterative deepening".  It's
    not really IID because it doesn't recurse on the same position.
    This code is called when we are at a PV node and have no hash move
    and no good capture from the generator.  Instead of relying on
    only history / killer heuristics to order the moves instead we
    reduce the depth by two ply and search all the moves generated.
    The scores that come out of the reduced-depth search are then used
    to order the moves at the regular-depth search.  This is somewhat
    recursive because the first move considered by the reduced-depth
    search may also have no hash move / good capture and invoke the
    whole process again.

Parameters:

    IN SEARCHER_THREAD_CONTEXT *ctx,
    IN ULONG uDepth,
    IN SCORE iAlpha,
    IN SCORE iBeta

Return value:

    SCORE

**/
{
    ULONG x;
    MOVE mv;
    SCORE iScore;
    SCORE iBestScore = -INFINITY;
    ULONG uBest;

    //
    // Preconditions
    //
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT(uDepth >= (IID_R_FACTOR + ONE_PLY));

    //
    // Compute next depth, possibly recurse
    //
    uDepth -= (IID_R_FACTOR + ONE_PLY);
    if (uDepth >= (IID_R_FACTOR + ONE_PLY))
    {
        (void)RescoreMovesViaSearch(ctx, uDepth, iAlpha, iBeta);
    }
    ASSERT(uDepth < MAX_DEPTH_PER_SEARCH);

    //
    // Search the moves and remember the scores
    //
    for (x = uBest = ctx->sMoveStack.uBegin[ctx->uPly];
         x < ctx->sMoveStack.uEnd[ctx->uPly];
         x++)
    {
        SelectBestNoHistory(ctx, x);
        mv = ctx->sMoveStack.mvf[x].mv;
        mv.bvFlags |= WouldGiveCheck(ctx, mv);
        if (TRUE == MakeMove(ctx, mv))
        {
            if (-INFINITY == iBestScore)
            {
                iScore = -Search(ctx, -iBeta, -iAlpha, uDepth);
            }
            else
            {
                iScore = -Search(ctx, -iAlpha - 1, -iAlpha, uDepth);
                if (iScore > iAlpha)
                {
                    iScore = -Search(ctx, -iBeta, -iAlpha, uDepth);
                }
            }
            UnmakeMove(ctx, mv);
            ctx->sMoveStack.mvf[x].iValue = iScore;
            if (iScore > iBestScore)
            {
                uBest = x;
                iBestScore = iScore;
                if (iScore >= iBeta)
                {
                    goto end;
                }
            }
        }
    }

 end:
    //
    // Clear the values from the current move to the end of the
    // moves in the list so that if we failed high we don't have
    // stale move values in the list for moves we did not search
    // the subtrees for.
    //
    while(x < ctx->sMoveStack.uEnd[ctx->uPly])
    {
        ctx->sMoveStack.mvf[x].iValue = -INFINITY;
        x++;
    }
    ctx->sMoveStack.mvf[uBest].iValue |= SORT_THESE_FIRST;
    ASSERT(IS_VALID_SCORE(iBestScore));
    return(iBestScore);
}
#endif


FLAG
MateDistancePruningCutoff(IN ULONG uPly,
                          IN FLAG fInCheck,
                          IN OUT SCORE *piBestScore,
                          IN OUT SCORE *piAlpha,
                          IN OUT SCORE *piBeta)
/**

Routine description:

    This idea (shamelessly?) "borrowed" from Fruit.  The idea is that
    the worst score we can return from any node in the tree is if we
    are in checkmate right now (indeed, if we're not in check right
    now than the worst score we can return is mated-in-2-ply).

    Likewise the best score we can return from here is mate-in-1-ply.
    Use these facts to attempt to narrow the alpha..beta window.
    Indeed there are some cases where we can prune this entire tree
    branch.

Parameters:

    ULONG uPly : ply from root
    FLAG InCheck : are we in check?
    SCORE *piBestScore : pointer to bestscore
    SCORE *piAlpha : pointer to alpha
    SCORE *piBeta : pointer to beta

Return value:

    FLAG : TRUE if we can prune the branch outright, FALSE otherwise

**/
{
    SCORE iScore = MATED_SCORE(uPly + 2 * !fInCheck);

    //
    // Do the alpha side
    //
    if (*piAlpha < iScore)
    {
        *piAlpha = iScore;
        if (iScore >= *piBeta)
        {
            *piBestScore = iScore;
            return(TRUE);
        }
    }

    //
    // Do the beta side
    //
    iScore = -1 * MATED_SCORE(uPly + 1);
    if (iScore < *piBeta)
    {
        *piBeta = iScore;
        if (iScore <= *piAlpha)
        {
            *piBestScore = iScore;
            return(TRUE);
        }
    }
    return(FALSE);
}


void
DumpPlyInfoStack(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN SCORE iAlpha,
                 IN SCORE iBeta)
/**

Routine description:

    Dump some information about the current search line; mainly here as
    a debugging aide.

    Note: this will not work on an MP searcher thread that is under a
    split node.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : searcher context to dump

Return value:

    void

**/
{
    ULONG u, v;
    POSITION *pos = GetRootPosition();
    CUMULATIVE_SEARCH_FLAGS *pf = &ctx->sSearchFlags;
    char *p = PositionToFen(pos);
    PLY_INFO *q;
    FLAG fSeenQ = FALSE;
    SCORE iEval;
    static SEARCHER_THREAD_CONTEXT temp;

    InitializeSearcherContext(pos, &temp);

    Trace("Iterate Depth: %2u\tCurrent Ply: %2u\n", g_uIterateDepth,
          ctx->uPly);
    Trace("Qsearch Check: %2u\tCurrent Qdepth: %2u\n", pf->uQsearchCheckDepth,
          pf->uQsearchDepth);
    Trace("Current a..b: %d .. %d\n", iAlpha, iBeta);
    if (NULL != p)
    {
        Trace("Root: %s\n", p);
        SystemFreeMemory(p);
    }
    Trace("\n");
    Trace("move\t number\t exten\t   eval       b.keval      w.keval   Danger Squares\n"
          "----------------------------------------------------------------------------\n");

    ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);
    for (u = 0; u < ctx->uPly; u++)
    {
        q = &ctx->sPlyInfo[u];
        ASSERT(PositionsAreEquivalent(&q->sPosition, &temp.sPosition));

        if ((FALSE == fSeenQ) && (q->fInQsearch == TRUE))
        {
            p = PositionToFen(&temp.sPosition);
            if (NULL != p)
            {
                Trace("\nHorizon: %s\n\n", p);
                SystemFreeMemory(p);
            }
            fSeenQ = TRUE;
        }

        Trace("%02u.%-6s ", u, MoveToSan(q->mv, &temp.sPosition));
        for (v = ctx->sMoveStack.uBegin[u];
             v < ctx->sMoveStack.uEnd[u];
             v++)
        {
            if (IS_SAME_MOVE(q->mv, ctx->sMoveStack.mvf[v].mv))
            {
                Trace("%u/%u", v - ctx->sMoveStack.uBegin[u] + 1,
                      MOVE_COUNT(ctx, u));
            }
        }
        Trace("\t");

        if (FALSE == MakeMove(&temp, q->mv))
        {
            Trace("Illegal move?!\n");
            return;
        }

        if (q->iExtensionAmount != 0)
        {
            Trace("%-3u\t", q->iExtensionAmount);
        }
        else
        {
            Trace("   \t");
        }

        iEval = Eval(&temp, -INFINITY, +INFINITY);
        Trace("%5s (%+2d) | ", ScoreToString(iEval),
              temp.sPosition.iMaterialBalance[temp.sPosition.uToMove] / 100);
        Trace("%5s (%2u) | ",
              ScoreToString(temp.sPlyInfo[temp.uPly].iKingScore[BLACK]),
              CountKingSafetyDefects(&temp.sPosition, BLACK));
        Trace("%5s (%2u)  ",
              ScoreToString(temp.sPlyInfo[temp.uPly].iKingScore[WHITE]),
              CountKingSafetyDefects(&temp.sPosition, WHITE));
        Trace("\n");
    }

    p = PositionToFen(&ctx->sPosition);
    if (NULL != p)
    {
        Trace("\n");
        Trace("Now: %s\n", p);
        SystemFreeMemory(p);
    }
}


FLAG
CommonSearchInit(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN OUT SCORE *piAlpha,
                 IN OUT SCORE *piBeta,
                 IN OUT SCORE *piScore)
/**

Routine description:

    A set of common code that is called by Search and QSearch.  It does
    the following:

       1. Increment the total node counter
       2. Clears pi->mvBest and cDanger squares
       3. Possibly check for user input or timer expiration
       4. Checks to see if the position is a draw
       5. Makes sure ctx->uPly is not too deep
       6. Sets pi->fInCheck
       7. Does mate-distance pruning

Parameters:

    IN SEARCHER_THREAD_CONTEXT *ctx,
    IN OUT SCORE *piAlpha,
    IN OUT SCORE *piBeta,
    IN OUT SCORE *piScore

Return value:

    FLAG : TRUE if the search node should be aborted (return *piScore)
           FALSE otherwise (*piAlpha, *piBeta possibly adjusted)

**/
{
    PLY_INFO *pi = &ctx->sPlyInfo[ctx->uPly];
    FLAG fRet = TRUE;

    ASSERT(*piAlpha < *piBeta);

    //
    // Increment node count and see if we need to check input / timer
    //
    ctx->sCounters.tree.u64TotalNodeCount++;
    ASSERT(ctx->sCounters.tree.u64TotalNodeCount > 0);
    if ((ctx->sCounters.tree.u64TotalNodeCount &
         g_MoveTimer.uNodeCheckMask) == 0)
    {
        (void)CheckInputAndTimers(ctx);
    }

    //
    // Make sure that the time allotted to search in has not expired
    // and (if this is an MP search) that we are still doing relevant work.
    //
    if (WE_SHOULD_STOP_SEARCHING)
    {
        pi->mvBest.uMove = 0;
        *piScore = INVALID_SCORE;
        goto end;
    }

    //
    // Make sure we have not exceeded max ply; note that since this
    // is checked at the beginning of search/qsearch and if we let
    // the past they will call MakeMove this must cut off one ply
    // before the hard limit.
    //
    if ((ctx->uPly >= (MAX_PLY_PER_SEARCH - 1)) ||
        (ctx->sSearchFlags.uQsearchDepth >= g_uHardExtendLimit))
    {
        *piScore = *piBeta;
        DTTRACE("<toodeep/>");
        goto end;
    }

    //
    // Erase deeper killer moves
    //
    if (ctx->uPly + 2 < MAX_PLY_PER_SEARCH)
    {
        ctx->mvKiller[ctx->uPly + 2][0].uMove = 0;
        ctx->mvKiller[ctx->uPly + 2][1].uMove = 0;
    }

    //
    // Set fInCheck in this ply info struct
    //
    pi->fInCheck = (IS_CHECKING_MOVE((pi-1)->mv) != 0);
    ASSERT((pi->fInCheck &&
            (InCheck(&ctx->sPosition, ctx->sPosition.uToMove))) ||
           (!pi->fInCheck &&
            (!InCheck(&ctx->sPosition, ctx->sPosition.uToMove))));

    //
    // This code is for a debug option where I dump paths from root
    // to leaf along with extensions and evals at each node in the
    // path for debugging/brainstorming purposes.
    //
    pi->PV[ctx->uPly].uMove = 0;
    pi->mvBest.uMove = 0;
    pi->iExtensionAmount = 0;
    pi->iEval = INVALID_SCORE;
#if 0
    if ((g_uIterateDepth > 5) &&
        (ctx->uPly > 2.5 * g_uIterateDepth))
    {
        DumpPlyInfoStack(ctx, *piAlpha, *piBeta);
    }
#endif

    //
    // See if this position is a draw
    //
    if (TRUE == IsDraw(ctx))
    {
        INC(ctx->sCounters.tree.u64LeafCount);
        *piScore = 0;
        if ((*piAlpha < *piScore) && (*piScore < *piBeta))
        {
            UpdatePV(ctx, DRAWMOVE);
        }
        DTTRACE("<i bold=\"draw\"/>");
        goto end;
    }

    //
    // Do mate-distance pruning
    //
    if (TRUE == MateDistancePruningCutoff(ctx->uPly,
                                          pi->fInCheck,
                                          piScore,
                                          piAlpha,
                                          piBeta))
    {
        ASSERT(*piScore != -INFINITY);
        ASSERT((*piScore <= *piAlpha) || (*piScore >= *piBeta));
        goto end;
    }
#ifdef DEBUG
    pi->iAlpha = *piAlpha;
    pi->iBeta = *piBeta;
#endif
    fRet = FALSE;

 end:
    ASSERT(IS_VALID_FLAG(fRet));
    return(fRet);
}


ULONG
SelectNullmoveRFactor(SEARCHER_THREAD_CONTEXT *ctx,
                      INT uDepth)
{
    ULONG u = TWO_PLY;
    ULONG uMaxPiecesPerSide = MAXU(ctx->sPosition.uNonPawnCount[WHITE][0],
                                   ctx->sPosition.uNonPawnCount[BLACK][0]);
    ASSERT(uMaxPiecesPerSide <= 16);

    if ((uDepth > 8 * ONE_PLY) ||
        ((uDepth > 6 * ONE_PLY) && (uMaxPiecesPerSide >= 3)))
    {
        u = THREE_PLY;
    }
    ASSERT((u == TWO_PLY) || (u == THREE_PLY));
    return(u);
}


FLAG
WeShouldTryNullmovePruning(SEARCHER_THREAD_CONTEXT *ctx,
                           SCORE iAlpha,
                           SCORE iBeta,
                           SCORE iRoughEval,
                           ULONG uNullDepth) 
{
    static SCORE _iDistAlphaSkipNull[] = {
      700, 950, 1110, 1150, 1190, 1230, 1270, 0
    };
    POSITION *pos = &ctx->sPosition;
    PLY_INFO *pi = &ctx->sPlyInfo[ctx->uPly];
    CUMULATIVE_SEARCH_FLAGS *pf = &ctx->sSearchFlags;
    ULONG u;

    ASSERT(IS_VALID_SCORE(iAlpha) && IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT(IS_VALID_SCORE(iRoughEval));
    ASSERT(uNullDepth < MAX_PLY_PER_SEARCH * ONE_PLY);

    if ((FALSE == pf->fAvoidNullmove) &&
        (pos->uNonPawnCount[pos->uToMove][0] > 2) &&
        (FALSE == pi->fInCheck) &&
        (iBeta != +INFINITY) &&
        (iBeta == iAlpha + 1)) // <--- TODO: test this one please...
    { 
        if (uNullDepth <= 6 * ONE_PLY)
        {
            u = uNullDepth / ONE_PLY;
            ASSERT(u <= 6);
            if ((iRoughEval + _iDistAlphaSkipNull[u] <= iAlpha) ||
                ((iRoughEval + _iDistAlphaSkipNull[u] / 2 <= iAlpha) &&
                 (ValueOfMaterialInTroubleAfterNull(pos, pos->uToMove))))
            {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}
        

FLAG
TryNullmovePruning(SEARCHER_THREAD_CONTEXT *ctx,
                   FLAG *pfThreat,
                   SCORE iAlpha,
                   SCORE iBeta,
                   ULONG uNullDepth,
                   INT *piOrigExtend,
                   SCORE *piNullScore)
{
    static INT _bm_by_piece[7] = {
        -1, -1, QUARTER_PLY, QUARTER_PLY, HALF_PLY, HALF_PLY, -1
    };
    POSITION *pos = &ctx->sPosition;
    PLY_INFO *pi = &ctx->sPlyInfo[ctx->uPly];
    CUMULATIVE_SEARCH_FLAGS *pf = &ctx->sSearchFlags;
    SCORE iVerifyScore;
    MOVE mv;
    MOVE mvRef;
       
    ASSERT(IS_VALID_SCORE(iAlpha) && IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    
    // TODO: more experiments with quick null

    // Ok, do it.
    ASSERT(!InCheck(pos, pos->uToMove));
    VERIFY(MakeMove(ctx, NULLMOVE));
    ASSERT(pos->u64NonPawnSig != pi->sPosition.u64NonPawnSig);
    ASSERT(pos->uToMove != pi->sPosition.uToMove);
    INC(ctx->sCounters.tree.u64NullMoves);

    // (Reduced depth) search under the nullmove; don't allow two
    // nullmoves in a row.
    ASSERT(pf->fAvoidNullmove == FALSE);
    pf->fAvoidNullmove = TRUE;
    *piNullScore = -Search(ctx, -iBeta, -iBeta + 1, uNullDepth);
    pf->fAvoidNullmove = FALSE;

    // Check the results
    if (*piNullScore >= iBeta)
    {
        UnmakeMove(ctx, NULLMOVE);
        ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));

        // If there are very few pieces on the board, launch a
        // separate "verification search".
        if ((pos->uNonPawnCount[pos->uToMove][0] < 4) &&
            (pf->fVerifyNullmove == TRUE))
        {
            ASSERT(pf->fAvoidNullmove == FALSE);
            uNullDepth -= ONE_PLY;
            if (uNullDepth > MAX_DEPTH_PER_SEARCH) uNullDepth = 0;
            pf->fVerifyNullmove = FALSE;
            iVerifyScore = Search(ctx, iAlpha, iBeta, uNullDepth);
            pf->fVerifyNullmove = TRUE;
            if (iVerifyScore < iBeta)
            {
                // The nullmove failed high but the verify search
                // did not; we are in zug so extend.
                *piOrigExtend += ONE_PLY;
                INC(ctx->sCounters.extension.uZugzwang);
                return FALSE;
            }
        }

        // Nullmove succeeded and either no need to verify or verify
        // succeeded too.
        INC(ctx->sCounters.tree.u64NullMoveSuccess);
        if (*piNullScore > +NMATE) *piNullScore = +NMATE;
        return TRUE;
    }

    // Nullmove failed...
    mv = (pi+1)->mvBest;
    if (mv.uMove)
    {
        // See what we can learn from the move that refuted our nullmove.
        ASSERT(SanityCheckMove(pos, mv));
        ASSERT(GET_COLOR(mv.pMoved) == pos->uToMove);
        ASSERT(ctx->uPly >= 2);
        
        // If we make a nullmove that fails because we lose a
        // piece, remember that the piece in question is in danger.
        if ((mv.pCaptured) && !IS_PAWN(mv.pCaptured))
        {
            ASSERT(GET_COLOR(mv.pCaptured) == FLIP(pos->uToMove));
            ASSERT(mv.pCaptured == pos->rgSquare[mv.cTo].pPiece);
            StoreEnprisePiece(pos, mv.cTo);
            mvRef = ctx->mvNullmoveRefutations[ctx->uPly - 2];
            if ((mvRef.uMove) &&
                (mvRef.pCaptured == mv.pCaptured) &&
                (mvRef.pMoved == mv.pMoved) &&
                (mvRef.cTo != mv.cTo))
            {
                ASSERT(GET_COLOR(mvRef.pCaptured) == FLIP(pos->uToMove));
                ASSERT(mvRef.pCaptured);
                ASSERT(_bm_by_piece[PIECE_TYPE(mv.pCaptured)] > 0);
                ASSERT(_bm_by_piece[PIECE_TYPE(mv.pCaptured)] <=
                       ONE_PLY);
                *piOrigExtend += _bm_by_piece[PIECE_TYPE(mv.pCaptured)];
                INC(ctx->sCounters.extension.uBotvinnikMarkoff);
            }
            ctx->mvNullmoveRefutations[ctx->uPly] = mv;
        }
        
        // This is an idea from Tord Romstad on ccc: if we are below a
        // history reduced node and we try a nullmove and the nullmove
        // fails low and the move that refuted the nullmove involves
        // the same piece moved in the reduced move at ply-1 then bail
        // out of the reduced depth search now and research at full
        // depth.  The move that was reduced had some tactical
        // significance.
        ASSERT(IS_ON_BOARD(mv.cFrom));
        if ((mv.cFrom == (pi-1)->mv.cTo) && ((pi-1)->iExtensionAmount < 0))
        {
            ASSERT(GET_COLOR(mv.pMoved) == GET_COLOR((pi-1)->mv.pMoved));
            UnmakeMove(ctx, NULLMOVE);
            ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
            *piNullScore = iAlpha - 1;
            return TRUE;
        }
    }
    
    // If we do nothing we are mated-in-n, therefore this is a
    // dangerous position so extend.  This is Bruce Moreland's idea
    // originally.
    if (*piNullScore <= -NMATE)
    {
        if (mv.uMove && !IS_CAPTURE_OR_PROMOTION(mv))
        {
            ASSERT(SanityCheckMove(pos, mv));
            ASSERT(GET_COLOR(mv.pMoved) == pos->uToMove);
            UpdateDynamicMoveOrdering(ctx, 
                                      uNullDepth + TWO_PLY, 
                                      mv, 
                                      *piNullScore, 
                                      0);
        }
        *pfThreat = TRUE;
    }
    UnmakeMove(ctx, NULLMOVE);
    ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
    return FALSE;
}


#ifdef DEBUG
FLAG
SanityCheckMoves(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN ULONG uCurrent,
                 IN ULONG uType)
/**

Routine description:

    Support routine to sanity check that:

        1. The moves before uCurrent on the move stack were all marked
           as searched (or pruned)
        2. The moves after uCurrent on the move stack are all zero
           value moves and not checking moves.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG uCurrent,
    ULONG uType

Return value:

    FLAG

**/
{
    ULONG x;

    if (WE_SHOULD_STOP_SEARCHING)
    {
        return(TRUE);
    }

    //
    // Check moves yet to be searched
    //
    if (uType & VERIFY_AFTER)
    {
        for (x = uCurrent; x < ctx->sMoveStack.uEnd[ctx->uPly]; x++)
        {
            ASSERT(ctx->sMoveStack.mvf[x].iValue <= 0);
            ASSERT(!IS_CHECKING_MOVE(ctx->sMoveStack.mvf[x].mv));
        }
    }

    //
    // Check moves already searched
    //
    if (uType & VERIFY_BEFORE)
    {
        for (x = uCurrent - 1;
             ((x >= ctx->sMoveStack.uBegin[ctx->uPly]) && (x != (ULONG)-1));
             x--)
        {
            ASSERT(ctx->sMoveStack.mvf[x].bvFlags & MVF_MOVE_SEARCHED);
        }
    }
    return(TRUE);
}
#endif
