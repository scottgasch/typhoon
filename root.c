/**

Copyright (c) Scott Gasch

Module Name:

    root.c

Abstract:

    Routines dealing with the root of the search tree.  Specifically
    move ordering at the root node, searching, search support, pondering,
    pondering support, and the main iterative deepening loop.

Author:

    Scott Gasch (scott.gasch@gmail.com) 21 May 2004

Revision History:

    $Id: root.c 356 2008-07-02 16:18:12Z scott $

**/

#include "chess.h"

volatile MOVE_TIMER g_MoveTimer;
ULONG g_uIterateDepth;
ULONG g_uSoftExtendLimit;
ULONG g_uHardExtendLimit;
ULONG g_uExtensionReduction[MAX_PLY_PER_SEARCH];
FLAG g_fCanSplit[MAX_PLY_PER_SEARCH];
SCORE g_iRootScore[2] = {0, 0};
SCORE g_iScore;


static void
_SetMoveTimerForPonder(void)
/**

Routine description:

    Set the move timer before a ponder operation.  A time limit of -1
    means search forever... so the only way out of the ponder is for
    new user input to interrupt it.

Parameters:

    void

Return value:

    static void

**/
{
    g_MoveTimer.dStartTime = SystemTimeStamp();
    g_MoveTimer.dSoftTimeLimit = -1;
    g_MoveTimer.dHardTimeLimit = -1;
    g_MoveTimer.uNodeCheckMask = 0x200 - 1;
    g_MoveTimer.bvFlags = 0;
}


void
SetMoveTimerToThinkForever(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    g_MoveTimer.dStartTime = SystemTimeStamp();
    g_MoveTimer.dSoftTimeLimit = -1;
    g_MoveTimer.dHardTimeLimit = -1;
    g_MoveTimer.uNodeCheckMask = 0x1000000 - 1;
    g_MoveTimer.bvFlags = 0;
}


void
SetMoveTimerForSearch(FLAG fSwitchOver, ULONG uColor)
/**

Routine description:

    Set the move timer for a search operation.

Parameters:

    FLAG fSwitchOver : if TRUE then this search is a converted ponder
    (i.e. they made the ponder move and now we are searching).

Return value:

    void

**/
{
    ULONG uMovesDone = GetMoveNumber(uColor);
    double dClock = (double)g_Options.uMyClock;
    double dSec = 0;
    ULONG uMargin = 0;
    ULONG uMovesToDo;

    //
    // If switching over from a ponder search leave the start time and
    // move flags alone (i.e. N seconds in the past already).
    //
    if (FALSE == fSwitchOver)
    {
        g_MoveTimer.dStartTime = SystemTimeStamp();
        g_MoveTimer.bvFlags = 0;
    }

    //
    // Check the clock more often in search if there is little time
    // left on the clock -- we can't afford an oversearch when the
    // bullet game is down to the wire.
    //
#ifdef DEBUG
    g_MoveTimer.uNodeCheckMask = 0x800 - 1;
#else
    if (WeAreRunningASuite())
    {
        g_MoveTimer.uNodeCheckMask = 0x20000 - 1;
    }
    else if (g_Options.u64MaxNodeCount) 
    {
        g_MoveTimer.uNodeCheckMask = 0x1000 - 1;
    }
    else
    {
        if (dClock < 10.0)
        {
            g_MoveTimer.uNodeCheckMask = 0x400 - 1;
        }
        else
        {
            g_MoveTimer.uNodeCheckMask = 0x8000 - 1;
        }
    }
#endif

    switch (g_Options.eClock)
    {
        //
        // Fixed time per move.  Think for g_uMyIncrement seconds exactly and
        // no hard time limit.
        //
        case CLOCK_FIXED:
            dSec = (double)g_Options.uMyIncrement;
            uMargin = 0;
            break;

        //
        // N moves per time control or entire game in time control.
        //
        case CLOCK_NORMAL:
            if (g_Options.uMovesPerTimePeriod != 0)
            {
                //
                // N moves per time control.
                //
                ASSERT(g_Options.uMovesPerTimePeriod < 256);
                uMovesToDo =
                    (g_Options.uMovesPerTimePeriod -
                     ((uMovesDone - 1) % g_Options.uMovesPerTimePeriod));
#ifdef DEBUG
                Trace("SetMoveTimer: %u moves left to do this in %s sec.\n",
                      uMovesToDo, TimeToString(dClock));
#endif
                ASSERT(uMovesToDo <= g_Options.uMovesPerTimePeriod);
                dSec = (dClock / (double)uMovesToDo);
                dSec *= 0.95;
                uMargin = (ULONG)(dSec * 2.2);
                if (uMovesToDo <= 5)
                {
                    dSec *= 0.75;
                    uMargin /= 2;
                }
                break;
            }
            else
            {
                //
                // Fixed time finish entire game.
                //
#ifdef DEBUG
                Trace("SetMoveTimer: finish the game in %s sec.\n",
                      TimeToString(dClock));
#endif
                dSec = dClock / 60;
                uMargin = (ULONG)(dSec * 2.2);
            }
            break;

        //
        // We get back a certain number of seconds with each move made.
        //
        case CLOCK_INCREMENT:
            dSec = g_Options.uMyIncrement;
            dSec += (dClock / 50);
            uMargin = (int)(dSec * 2.2);

#ifdef DEBUG
            Trace("SetMoveTimer: finish the game in %s sec (+%u per move).\n",
                  TimeToString(dClock), g_Options.uMyIncrement);
#endif

            //
            // If we are running out of time, think for less than the
            // increment.
            //
            if (dClock < 20.0)
            {
                dSec -= g_Options.uMyIncrement;
                uMargin = 0;
            }
            ASSERT(dSec > 0);
            break;

        case CLOCK_NONE:
#ifdef DEBUG
            Trace("SetMoveTimer: no time limit, think until interrupted.\n");
#endif
            g_MoveTimer.dHardTimeLimit = -1;
            g_MoveTimer.dSoftTimeLimit = -1;
            goto post;

        default:
            ASSERT(FALSE);
            break;
    }
    if ((dSec + uMargin) >= (dClock * 7 / 8)) uMargin = 0;
    g_MoveTimer.dSoftTimeLimit = g_MoveTimer.dStartTime + dSec;
    g_MoveTimer.dHardTimeLimit = g_MoveTimer.dStartTime + dSec + uMargin;

 post:
    if (TRUE == g_Options.fShouldPost)
    {
        if (g_MoveTimer.dSoftTimeLimit > 0)
        {
            Trace("SetMoveTimer: Soft time limit %s seconds.\n",
                  TimeToString(g_MoveTimer.dSoftTimeLimit -
                               g_MoveTimer.dStartTime));
            ASSERT(g_MoveTimer.dHardTimeLimit > 0);
            Trace("SetMoveTimer: Hard time limit %s seconds.\n",
                  TimeToString(g_MoveTimer.dHardTimeLimit -
                               g_MoveTimer.dStartTime));
            if (TRUE == fSwitchOver)
            {
                Trace("SetMoveTimer: Already searched %s seconds.\n",
                      TimeToString(SystemTimeStamp() -
                                   g_MoveTimer.dStartTime));
            }
        }
#ifdef DEBUG
        Trace("SetMoveTimer: Checking input and timer every %u nodes.\n",
              g_MoveTimer.uNodeCheckMask);
#endif
    }
}


void
ReInitializeSearcherContext(POSITION *pos,
                            SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    Reinitializes a SEARCHER_THREAD_CONTEXT structure.

Parameters:

    POSITION *pos,
    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    void

**/
{
    if (NULL != pos)
    {
        memmove(&(ctx->sPosition), pos, sizeof(POSITION));
    }
    ctx->uPly = 0;
    ctx->uPositional = 133;
    memset(&(ctx->sCounters), 0, sizeof(COUNTERS));
}


void
InitializeSearcherContext(POSITION *pos,
                          SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    Initialize a SEARCHER_THREAD_CONTEXT structure.

Parameters:

    POSITION *pos,
    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    void

**/
{
    ULONG u;

    memset(ctx, 0, sizeof(SEARCHER_THREAD_CONTEXT));
    ReInitializeSearcherContext(pos, ctx);
    ctx->sMoveStack.uUnblockedKeyValue[0] = 1;
    for (u = 1;
         u < MAX_PLY_PER_SEARCH;
         u++)
    {
        ctx->sMoveStack.uUnblockedKeyValue[u] =
            ctx->sMoveStack.uUnblockedKeyValue[u-1] + 0x28F5C28;
    }
}


void
InitializeLightweightSearcherContext(POSITION *pos,
                                     LIGHTWEIGHT_SEARCHER_CONTEXT *ctx)
/**

Routine description:

    Initialize a LIGHTWEIGHT_SEARCHER_CONTEXT structure.

Parameters:

    POSITION *pos,
    LIGHTWEIGHT_SEARCHER_CONTEXT *ctx

Return value:

    void

**/
{
    ULONG u;

    memset(ctx, 0, sizeof(LIGHTWEIGHT_SEARCHER_CONTEXT));
    if (NULL != pos)
    {
        memmove(&(ctx->sPosition), pos, sizeof(POSITION));
    }
    ctx->uPly = 0;
    ctx->uPositional = 133;
    ctx->sMoveStack.uUnblockedKeyValue[0] = 1;
    for (u = 1;
         u < MAX_PLY_PER_SEARCH;
         u++)
    {
        ctx->sMoveStack.uUnblockedKeyValue[u] =
            ctx->sMoveStack.uUnblockedKeyValue[u-1] + 0x28F5C28;
    }
}


void
PostMoveSearchReport(SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    Dump a report to stdout after every move.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,

Return value:

    void

**/
{
    double d;
    char buf[256];
#ifdef PERF_COUNTERS
    double n;
#endif
    d = (double)g_MoveTimer.dEndTime - (double)g_MoveTimer.dStartTime + 0.01;
    ASSERT(d);
    Trace("---------------------------------------------\n"
          "Searched for %5.1f seconds, saw %"
          COMPILER_LONGLONG_UNSIGNED_FORMAT
          " nodes (%"
          COMPILER_LONGLONG_UNSIGNED_FORMAT
          " qnodes) (%6.0f nps).\n",
          d, ctx->sCounters.tree.u64TotalNodeCount,
          ctx->sCounters.tree.u64QNodeCount,
          ((double)ctx->sCounters.tree.u64TotalNodeCount / d));
    if (g_Options.fThinking)
    {
        snprintf(buf, ARRAY_LENGTH(buf), "d%u, %s, %3.1fs, %6.0fknps, PV=%s",
                 ctx->uRootDepth / ONE_PLY,
                 ScoreToString(ctx->iRootScore),
                 d,
                 ((double)ctx->sCounters.tree.u64TotalNodeCount / d / 1000),
                 ctx->szLastPV);
        Trace("tellothers %s\n", buf);
    }
#ifdef MP
#ifdef PERF_COUNTERS
    if (ctx->sCounters.parallel.uNumSplits > 0)
    {
        n = (double)ctx->sCounters.parallel.uNumSplits;
        Trace("Split %u times total ",
              ctx->sCounters.parallel.uNumSplits);
        Trace("(~%7.2fx/sec).\n", (n / d));
        d = n + 1;
        ASSERT(d);
        n = (double)ctx->sCounters.parallel.uNumSplitsTerminated;
        Trace("  ...%u (%5.2f percent) terminated early.\n",
              ctx->sCounters.parallel.uNumSplitsTerminated, ((n/d) * 100.0));
        DumpHelperIdlenessReport();
    }
#endif
#endif

#ifdef TEST
    AnalyzeFullHashTable();
#endif
#ifdef PERF_COUNTERS
    d = (double)(ctx->sCounters.hash.u64Probes) + 1;
    ASSERT(d);
    Trace("Hashing percentages: (%5.3f total, %5.3f useful)\n",
          ((double)(ctx->sCounters.hash.u64OverallHits) / d) * 100.0,
          ((double)(ctx->sCounters.hash.u64UsefulHits) / d) * 100.0);
    d = (double)(ctx->sCounters.pawnhash.u64Probes) + 1;
    ASSERT(d);
    n = (double)(ctx->sCounters.pawnhash.u64Hits);
    Trace("Pawn hash hitrate: %5.3f percent.\n", (n/d) * 100.0);
    n = (double)(ctx->sCounters.tree.u64NullMoveSuccess);
    d = (double)(ctx->sCounters.tree.u64NullMoves) + 1;
    ASSERT(d);
    Trace("Null move cutoff rate: %5.3f percent.\n",
          ((n / d) * 100.0));
#ifdef TEST_ENDGAME
    n = (double)(ctx->sCounters.tree.u64EndgamePruningSuccess);
    d = n + (double)(ctx->sCounters.tree.u64EndgamePruningFailures);
    if (d != 0.0)
    {
        Trace("Endgame pruning heuristic success rate was %5.3f percent.\n"
              "Endgame pruning heuristic was "
              "%"COMPILER_LONGLONG_UNSIGNED_FORMAT " / "
              "%"COMPILER_LONGLONG_UNSIGNED_FORMAT ".\n",
              ((n / d) * 100.0),
              ctx->sCounters.tree.u64EndgamePruningSuccess,
              (ctx->sCounters.tree.u64EndgamePruningSuccess +
               ctx->sCounters.tree.u64EndgamePruningFailures));
    }
    else
    {
        Trace("Never used endgame pruning heuristic.\n");
    }
#endif
#ifdef TEST_NULL
    n = (double)(ctx->sCounters.tree.u64AvoidNullSuccess);
    d = n + (double)(ctx->sCounters.tree.u64AvoidNullFailures);
    if (d != 0.0)
    {
        Trace("Avoid null move heuristic rate was %5.3f percent.\n"
              "Avoid null move heuristic was "
              "%"COMPILER_LONGLONG_UNSIGNED_FORMAT " / "
              "%"COMPILER_LONGLONG_UNSIGNED_FORMAT ".\n",
              ((n / d) * 100.0),
              ctx->sCounters.tree.u64AvoidNullSuccess,
              (ctx->sCounters.tree.u64AvoidNullSuccess +
               ctx->sCounters.tree.u64AvoidNullFailures));
    }
    else
    {
        Trace("Never used the avoid null move heuristic.\n");
    }
    n = (double)(ctx->sCounters.tree.u64QuickNullSuccess);
    n += (double)(ctx->sCounters.tree.u64QuickNullDeferredSuccess);
    d = n + (double)(ctx->sCounters.tree.u64QuickNullFailures);
    if (d != 0.0)
    {
        Trace("Quick null move heuristic rate was %5.3f percent.\n"
              "Quick null move heuristic rate was "
              "%"COMPILER_LONGLONG_UNSIGNED_FORMAT " ("
              "%"COMPILER_LONGLONG_UNSIGNED_FORMAT ") / "
              "%"COMPILER_LONGLONG_UNSIGNED_FORMAT ".\n",
              ((n / d) * 100.0),
              ctx->sCounters.tree.u64QuickNullSuccess,
              ctx->sCounters.tree.u64QuickNullDeferredSuccess,
              (ctx->sCounters.tree.u64QuickNullSuccess +
               ctx->sCounters.tree.u64QuickNullDeferredSuccess +
               ctx->sCounters.tree.u64QuickNullFailures));
    }
    else
    {
        Trace("Never used the quick null move heuristic.\n");
    }
#endif
    n = (double)ctx->sCounters.tree.u64BetaCutoffsOnFirstMove;
    d = (double)ctx->sCounters.tree.u64BetaCutoffs + 1;
    Trace("First move beta cutoff rate was %5.3f percent.\n",
          ((n / d) * 100.0));
#ifdef LAZY_EVAL
    d = (double)ctx->sCounters.tree.u64LazyEvals;
    d += (double)ctx->sCounters.tree.u64FullEvals;
    d += (double)ctx->sCounters.tree.u64EvalHashHits;
    d += 1;
    ASSERT(d);
    Trace("Eval percentages: (%5.2f hash, %5.2f lazy, %5.2f full)\n",
          ((double)ctx->sCounters.tree.u64EvalHashHits / d) * 100.0,
          ((double)ctx->sCounters.tree.u64LazyEvals / d) * 100.0,
          ((double)ctx->sCounters.tree.u64FullEvals / d) * 100.0);
#endif
    Trace("Extensions: (%u +, %u q+, %u 1mv, %u !kmvs, %u mult+, %u pawn\n"
          "             %u threat, %u zug, %u sing, %u endg, %u bm, %u recap)\n",
          ctx->sCounters.extension.uCheck,
          ctx->sCounters.extension.uQExtend,
          ctx->sCounters.extension.uOneLegalMove,
          ctx->sCounters.extension.uNoLegalKingMoves,
          ctx->sCounters.extension.uMultiCheck,
          ctx->sCounters.extension.uPawnPush,
          ctx->sCounters.extension.uMateThreat,
          ctx->sCounters.extension.uZugzwang,
          ctx->sCounters.extension.uSingularReplyToCheck,
          ctx->sCounters.extension.uEndgame,
          ctx->sCounters.extension.uBotvinnikMarkoff,
          ctx->sCounters.extension.uRecapture);
#ifdef EVAL_TIME
    n = (double)ctx->sCounters.tree.u64CyclesInEval;
    Trace("Avg. cpu cycles in eval: %8.1f.\n", (n / d));
#endif
#endif
}


SCORE
RootSearch(SEARCHER_THREAD_CONTEXT *ctx,
           SCORE iAlpha,
           SCORE iBeta,
           ULONG uDepth)
/**

Routine description:

    Search at the root of the whole tree.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    SCORE iAlpha,
    SCORE iBeta,
    ULONG uDepth,

Return value:

    SCORE

**/
{
    PLY_INFO *pi = &ctx->sPlyInfo[ctx->uPly];
    SCORE iBestScore = -INFINITY;
    SCORE iInitialAlpha = iAlpha;
    MOVE mv;
    ULONG x, y;
    SCORE iScore;
    ULONG uNextDepth;
    UINT64 u64StartingNodeCount;
    ULONG uNumLegalMoves;

#ifdef DEBUG
	POSITION *pos = &ctx->sPosition;
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT((ctx->uPly == 0) || (ctx->uPly == 1)); // 1 is for under a ponder
    memcpy(&pi->sPosition, pos, sizeof(POSITION));
#endif

    ctx->sCounters.tree.u64TotalNodeCount++;
    pi->PV[ctx->uPly] = NULLMOVE;
    pi->mvBest = NULLMOVE;
    g_MoveTimer.bvFlags |= TIMER_SEARCHING_FIRST_MOVE;
#ifdef DEBUG
    Trace("---- depth=%u, a=%d, b=%d ----\n", uDepth / ONE_PLY, iAlpha, iBeta);
#endif

    uNumLegalMoves = 0;
    for (x = ctx->sMoveStack.uBegin[ctx->uPly];
         x < ctx->sMoveStack.uEnd[ctx->uPly];
         x++)
    {
        SelectMoveAtRoot(ctx, x);
        if (ctx->sMoveStack.mvf[x].bvFlags & MVF_MOVE_SEARCHED) break;
        ctx->sMoveStack.mvf[x].bvFlags |= MVF_MOVE_SEARCHED;
        mv = ctx->sMoveStack.mvf[x].mv;
        mv.bvFlags |= WouldGiveCheck(ctx, mv);

        if (MakeMove(ctx, mv))
        {
            uNumLegalMoves++;
            u64StartingNodeCount = ctx->sCounters.tree.u64TotalNodeCount;

            //
            // TODO: Fancy recap shit here?
            //

            uNextDepth = uDepth - ONE_PLY;
            if (IS_CHECKING_MOVE(mv))
            {
                uNextDepth += HALF_PLY;
                ctx->sPlyInfo[ctx->uPly].iExtensionAmount = HALF_PLY;
            }

            ctx->sSearchFlags.fVerifyNullmove = TRUE;
            ctx->sSearchFlags.uQsearchDepth = 0;
            if (uNumLegalMoves == 1)
            {
                //
                // Move 1, full a..b window
                //
                iScore = -Search(ctx, -iBeta, -iAlpha, uNextDepth);
            }
            else
            {
                //
                // Moves 2..N, minimal window search
                //
                ASSERT(x > 0);
                ASSERT(uNumLegalMoves > 1);
                iScore = -Search(ctx, -iAlpha - 1, -iAlpha, uNextDepth);
                if ((iAlpha < iScore) && (iScore < iBeta))
                {
                    iScore = -Search(ctx, -iBeta, -iAlpha, uNextDepth);
                }
            }
            ctx->sSearchFlags.fVerifyNullmove = FALSE;
            UnmakeMove(ctx, mv);
            if (WE_SHOULD_STOP_SEARCHING)
            {
                iBestScore = INVALID_SCORE;
                goto end;
            }
#ifdef DEBUG
            Trace("%2u. %-8s => node=%" COMPILER_LONGLONG_UNSIGNED_FORMAT
                  ", predict=%d, actual=%d, ",
                  x + 1,
                  MoveToSan(mv, &ctx->sPosition),
                  u64StartingNodeCount,
                  ctx->sMoveStack.mvf[x].iValue,
                  iScore);
            ASSERT(PositionsAreEquivalent(&pi->sPosition, &ctx->sPosition));
#endif
            //
            // Keep track of how many moves are under each one we
            // search and use this as the basis for root move ordering
            // next iteration.
            //
            u64StartingNodeCount = (ctx->sCounters.tree.u64TotalNodeCount -
                                    u64StartingNodeCount);
            u64StartingNodeCount >>= 9;
            u64StartingNodeCount &= (MAX_INT / 4);
            ctx->sMoveStack.mvf[x].iValue =
                (SCORE)(u64StartingNodeCount + iScore);
#ifdef DEBUG
            Trace("next_predict: %d\n", ctx->sMoveStack.mvf[x].iValue);
            ASSERT(iBestScore <= iAlpha);
            ASSERT(iAlpha < iBeta);
#endif
            if (iScore > iBestScore)
            {
                iBestScore = iScore;
                pi->mvBest = mv;
                if (iScore > iAlpha)
                {
                    //
                    // We have a best move so far.  As of now it is
                    // the one we'll be playing.  Also make sure to
                    // search it first at the next depth.
                    //
#ifdef DEBUG
                    Trace("Best so far... root score=%d.\n", iScore);
#endif
                    ctx->mvRootMove = mv;
                    ctx->iRootScore = iScore;
                    ctx->uRootDepth = uDepth;
                    ctx->sMoveStack.mvf[x].iValue = MAX_INT;

                    //
                    // If there was a previous PV move then knock its
                    // score down so that it will be considered second
                    // on the next depth.
                    //
                    for (y = ctx->sMoveStack.uBegin[ctx->uPly];
                         y < x;
                         y++)
                    {
                        if (ctx->sMoveStack.mvf[y].iValue == MAX_INT)
                        {
                            ctx->sMoveStack.mvf[y].iValue /= 2;
                        }
                    }

                    UpdatePV(ctx, mv);
                    if (iScore >= iBeta)
                    {
                        //
                        // Root fail high...
                        //
                        UpdateDynamicMoveOrdering(ctx,
                                                  uDepth,
                                                  mv,
                                                  iScore,
                                                  x + 1);
                        UtilPrintPV(ctx, iAlpha, iBeta, iScore, mv);
                        KEEP_TRACK_OF_FIRST_MOVE_FHs(iBestScore == -INFINITY);
                        ctx->sMoveStack.mvf[x].bvFlags &= ~MVF_MOVE_SEARCHED;
                        goto end;
                    }
                    else
                    {
                        //
                        // Root PV change...
                        //
                        UtilPrintPV(ctx, iAlpha, iBeta, iScore, mv);
                        iAlpha = iScore;
                    }
				}
            }
            g_MoveTimer.bvFlags &= ~TIMER_SEARCHING_FIRST_MOVE;
        }
    }

    if (iAlpha == iInitialAlpha)
    {
        //
        // Root fail low...
        //
        ASSERT(iBestScore <= iAlpha);
        ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
        UtilPrintPV(ctx, iAlpha, iBeta, iBestScore, mv);
    }

 end:
    return(iBestScore);
}



static GAME_RESULT
_DetectPreMoveTerminalStates(SEARCHER_THREAD_CONTEXT *ctx,
                             FLAG fInCheck,
                             MOVE *pmv) {
    ULONG u;
    MOVE mv;
    MOVE mvLegal;
    ULONG uNumLegal = 0;
    GAME_RESULT ret;
    POSITION *pos = &ctx->sPosition;
    ULONG uMajors[2], uMinors[2], uWhiteBishops[2], uBlackBishops[2];

    ret.eResult = RESULT_IN_PROGRESS;
    ret.szDescription[0] = '\0';
    pmv->uMove = 0;
    
    // Look for insufficient material.
    FOREACH_COLOR(u) {
        uMajors[u] = (pos->uNonPawnCount[u][ROOK] +
                      pos->uNonPawnCount[u][QUEEN]);
        uMinors[u] = (pos->uNonPawnCount[u][KNIGHT] +
                      pos->uNonPawnCount[u][BISHOP]);
        uWhiteBishops[u] = pos->uWhiteSqBishopCount[u];
        uBlackBishops[u] = pos->uNonPawnCount[u][BISHOP] - uWhiteBishops[u];
    }
    FOREACH_COLOR(u) {
        if ((pos->uPawnCount[u] != 0) || 
            (pos->uPawnCount[FLIP(u)] != 0)) {
              continue;
        }
        if (pos->uNonPawnCount[u][0] == 1) {
            if (uMajors[FLIP(u)] == 0 &&
                uMinors[FLIP(u)] == 1) {
                ret.eResult = RESULT_DRAW;
                strcpy(ret.szDescription, "insufficient material");
                return ret;
            } else if (uMajors[FLIP(u)] == 0 &&
                       (uMinors[FLIP(u)] == uWhiteBishops[FLIP(u)] ||
                        uMinors[FLIP(u)] == uBlackBishops[FLIP(u)])) {
                ret.eResult = RESULT_DRAW;
                strcpy(ret.szDescription, "insufficient material");
                return ret;
            }
        } else if ((pos->uNonPawnCount[u][0] == 1 + uWhiteBishops[u] &&
                   pos->uNonPawnCount[FLIP(u)][0] == 
                   1 + uWhiteBishops[FLIP(u)]) ||
                   (pos->uNonPawnCount[u][0] == 1 + uBlackBishops[u] &&
                    pos->uNonPawnCount[FLIP(u)][0] ==
                    1 + uBlackBishops[FLIP(u)])) {
            ret.eResult = RESULT_DRAW;
            strcpy(ret.szDescription, "insufficient material");
            return ret;
        }
    }
    
    // Count the number of legal moves the side on move has in this position.
    for (u = ctx->sMoveStack.uBegin[ctx->uPly];
         u < ctx->sMoveStack.uEnd[ctx->uPly];
         u++) {
        mv = ctx->sMoveStack.mvf[u].mv;
        if (MakeMove(ctx, mv)) {
            mvLegal = mv;
            uNumLegal += 1;
            UnmakeMove(ctx, mv);
            if (uNumLegal > 1) {
                return ret;
            }
        }
    }
    
    // If there's only one legal move then tell the caller what it is.
    if (uNumLegal == 1) {
        *pmv = mvLegal;
        return ret;
    }

    // There are no legal moves so its checkmate or stalemate.
    ASSERT(uNumLegal == 0);
    if (TRUE == fInCheck) {
      if (ctx->sPosition.uToMove == WHITE) {
        ret.eResult = RESULT_BLACK_WON;
        strcpy(ret.szDescription, "black checkmated white");
        return ret;
      }
      ret.eResult = RESULT_WHITE_WON;
      strcpy(ret.szDescription, "white checkmated black");
      return ret;
    }
    ret.eResult = RESULT_DRAW;
    sprintf(ret.szDescription, "%s stalemated %s",
            (ctx->sPosition.uToMove == WHITE) ? "black" : "white",
            (ctx->sPosition.uToMove == WHITE) ? "while" : "black");
    return ret;
}



static GAME_RESULT
_DetectPostMoveTerminalStates(SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    This function is called by Iterate and MakeTheMove to declare
    wins/losses/draws.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    FLAG

**/
{
    GAME_RESULT ret;
    FLAG fInCheck = InCheck(&ctx->sPosition, ctx->sPosition.uToMove);

    // Did we just make the 50th+ move without progress?
    if (ctx->sPosition.uFifty >= 100) {
        ret.eResult = RESULT_DRAW;
        strcpy(ret.szDescription, "fifty moves without progress");
        return ret;
    }
    GenerateMoves(ctx, NULLMOVE, (fInCheck ? GENERATE_ESCAPES :
                                             GENERATE_ALL_MOVES));
    
    // Did we just repeat a position for the 3rd time?
    if (IsLegalDrawByRepetition()) {
        ret.eResult = RESULT_DRAW;
        strcpy(ret.szDescription, "third repetition of position");
        return ret;
    }
    
    // Otherwise look for checkmates, stalemates and insufficient material.
    MOVE mvUnused;
    return _DetectPreMoveTerminalStates(ctx, 
                                        fInCheck, 
                                        &mvUnused);
}


static void
_IterateSetSearchGlobals(ULONG uDepth)
{
    ULONG u, v;
    ULONG uDontSplitLessThan;

    //
    // Set some globals that will be used throughout this search.
    //
    g_uIterateDepth = uDepth;
    ASSERT(g_uIterateDepth > 0);
    ASSERT(g_uIterateDepth < MAX_PLY_PER_SEARCH);

    //
    // For use in scaling back extensions that are very far from
    // the root.
    //
    //          Iterate   g_Soft              g_Hard
    //   |    |    |    |    |    |    |    |    |
    //   0         1         2   2.5   3   3.5   4       (x Iterate)
    //   |-------------------|----|----|---------|----...
    //   |full full full full|-1/4|-1/2|-3/4 -3/4|none...
    //
    g_uSoftExtendLimit = g_uIterateDepth * 2;
    g_uHardExtendLimit = g_uIterateDepth * 4;
    for (u = 0; u < MAX_PLY_PER_SEARCH; u++)
    {
        if (u < g_uSoftExtendLimit)
        {
            g_uExtensionReduction[u] = 0;
        }
        else if (u < (g_uSoftExtendLimit + g_uIterateDepth / 2))
        {
            g_uExtensionReduction[u] = QUARTER_PLY;
        }
        else if (u < (g_uSoftExtendLimit + g_uIterateDepth))
        {
            g_uExtensionReduction[u] = HALF_PLY;
        }
        else if (u < g_uHardExtendLimit)
        {
            g_uExtensionReduction[u] = THREE_QUARTERS_PLY;
        }
        else
        {
            g_uExtensionReduction[u] = 5 * ONE_PLY;
        }
    }

    //
    // Determine how much depth we need below a point in the tree in
    // order to split the search (which is based on iteration depth).
    //
    uDontSplitLessThan = (ULONG)((double)(uDepth) * 0.30);
    for (v = 0; v < MAX_PLY_PER_SEARCH; v++)
    {
        g_fCanSplit[v] = ((v + 1) >= uDontSplitLessThan);
    }
	g_fCanSplit[0] = FALSE;
}


#define SKIP_GRADUAL_OPENING 600
#define INITIAL_HALF_WINDOW  75
#define FIRST_FAIL_STEP      150
#define SECOND_FAIL_STEP     375

void
_IterateWidenWindow(ULONG uColor,
                    SCORE iScore,
                    SCORE *piBound,
                    ULONG *puState,
                    int iDir)
{
    SCORE iRoughScore = g_iRootScore[uColor];
    static SCORE _Steps[] =
    {
        FIRST_FAIL_STEP,
        SECOND_FAIL_STEP,
        INFINITY
    };

//    Trace("State = %u.\n", *puState);
//    Trace("Bound from %d to ", *piBound);
    if ((abs(iRoughScore - iScore) > SKIP_GRADUAL_OPENING) ||
        (*puState == 2))
    {
        *piBound = iDir * INFINITY;
        *puState = 2;
    }
    else
    {
        ASSERT(*puState < 2);
        *piBound = iScore;
        *piBound += iDir * _Steps[*puState];
        *puState += 1;
    }
//    Trace("%d.\n", *piBound);
//    Trace("State = %u\n", *puState);
}


GAME_RESULT Iterate(SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    Main iterative deepening loop for both thinking and pondering.
    Call RootSearch with progressively deeper depths while there is
    remaining time, we haven't found a forced mate, and there's no
    user interruption.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    FLAG : is the game over?

**/
{
    ULONG uNumRootFailLows = 0;
    ULONG uDepth;
    ULONG uFailState = 0;
    ULONG uColor;
    ULONG u;
    SCORE iAlpha = -INFINITY;
    SCORE iBeta = +INFINITY;
    SCORE iScore;
    FLAG fInCheck = InCheck(&(ctx->sPosition), ctx->sPosition.uToMove);
    MOVE mv;
    GAME_RESULT ret;
#ifdef DEBUG
    POSITION board;
    memcpy(&board, &(ctx->sPosition), sizeof(POSITION));
    VerifyPositionConsistency(&board, FALSE);
#endif

    uColor = ctx->sPosition.uToMove;
    g_iRootScore[uColor] = GetRoughEvalScore(ctx, iAlpha, iBeta, TRUE);
    g_iRootScore[FLIP(uColor)] = -g_iRootScore[uColor];

    //
    // Generate moves here so that RootSearch doesn't have to.
    //
    ASSERT((ctx->uPly == 0) || (ctx->uPly == 1)); // 1 is under a ponder
    ctx->sPlyInfo[ctx->uPly].fInCheck = fInCheck;
    GenerateMoves(ctx, NULLMOVE, (fInCheck ? GENERATE_ESCAPES : 
                                             GENERATE_ALL_MOVES));
    ASSERT(ctx->sMoveStack.uBegin[0] == 0);

    //
    // See if we are sitting at a checkmate or stalemate position; no
    // sense searching if so.
    //
    ret = _DetectPreMoveTerminalStates(ctx, 
                                       fInCheck, 
                                       &mv);
    if (ret.eResult != RESULT_IN_PROGRESS) {
        goto end;
    }
    
    // Game still in progress but only one legal move available?
    if (mv.uMove != 0) {
        ctx->mvRootMove = mv;
        ctx->iRootScore = 0;
        ctx->uRootDepth = 0;
        ctx->sPlyInfo[0].PV[0] = mv;
        ctx->sPlyInfo[0].PV[1] = NULLMOVE;
        strcpy(ctx->szLastPV, "only");
        if (!g_Options.fPondering) {
            Trace("ONLY MOVE --> stop searching now\n");
            g_MoveTimer.bvFlags |= TIMER_STOPPING;
            g_MoveTimer.bvFlags |= TIMER_CURRENT_OBVIOUS;
        } else {
            Trace("ONLY PONDER --> move immediately if prediction correct.\n");
            g_MoveTimer.bvFlags |= TIMER_MOVE_IMMEDIATELY;
            g_MoveTimer.bvFlags |= TIMER_CURRENT_OBVIOUS;
        }
        goto end;
    }

    for (uDepth = 1;
         uDepth <= g_Options.uMaxDepth;
         uDepth++)
    {
        //
        // Re-rank the moves based on nodecounts and mark all moves as
        // not yet searched at this depth.
        //
        for (u = ctx->sMoveStack.uBegin[ctx->uPly];
             u < ctx->sMoveStack.uEnd[ctx->uPly];
             u++)
        {
            ctx->sMoveStack.mvf[u].bvFlags &= ~MVF_MOVE_SEARCHED;
        }

        //
        // Make sure a..b window makes sense and set some other per-depth
        // globals used by the search.
        //
        _IterateSetSearchGlobals(uDepth);

        //
        // Try to get a PV for this depth before we're out of time...
        //
        do
        {
            if (iBeta > INFINITY) iBeta = +INFINITY;
            if (iAlpha < -INFINITY) iAlpha = -INFINITY;
            if (iAlpha >= iBeta) iAlpha = iBeta - 1;
            iScore = RootSearch(ctx,
                                iAlpha,
                                iBeta,
                                uDepth * ONE_PLY + HALF_PLY);
            if (g_MoveTimer.bvFlags & TIMER_STOPPING) break;
            mv = ctx->mvRootMove;

            //
            // Got a PV, we're done with this depth.
            //
            if ((iAlpha < iScore) && (iScore < iBeta))
            {
                ASSERT(iScore == ctx->iRootScore);
                ASSERT(mv.uMove);
                ASSERT(SanityCheckMove(&ctx->sPosition, mv));
                iAlpha = iScore - INITIAL_HALF_WINDOW;
                iBeta = iScore + INITIAL_HALF_WINDOW;
                g_iRootScore[uColor] = iScore;
                g_iRootScore[FLIP(uColor)] = -iScore;
                g_MoveTimer.bvFlags &= ~TIMER_RESOLVING_ROOT_FH;
                g_MoveTimer.bvFlags &= ~TIMER_RESOLVING_ROOT_FL;
                uFailState = 0;
                (void)CheckTestSuiteMove(mv, iScore, uDepth);
                break;
            }

            //
            // Root fail low?  We'll have to research with a wider
            // window.
            //
            else if (iScore <= iAlpha)
            {
                ASSERT(ctx->mvRootMove.uMove);
                g_MoveTimer.bvFlags |= TIMER_RESOLVING_ROOT_FL;
                uNumRootFailLows++;
                if (uNumRootFailLows > 3)
                {
                    g_MoveTimer.bvFlags |= TIMER_MANY_ROOT_FLS;
                }
                _IterateWidenWindow(uColor, iScore, &iAlpha, &uFailState, -1);

                //
                // Consider all moves again with wider window...
                //
                for (u = ctx->sMoveStack.uBegin[ctx->uPly];
                     u < ctx->sMoveStack.uEnd[ctx->uPly];
                     u++)
                {
                    ctx->sMoveStack.mvf[u].bvFlags &= ~MVF_MOVE_SEARCHED;
                }
            }

            //
            // Must be a root fail high.
            //
            else
            {
                ASSERT(iScore >= iBeta);
                ASSERT(mv.uMove);
                ASSERT(SanityCheckMove(&ctx->sPosition, mv));
                g_MoveTimer.bvFlags |= TIMER_RESOLVING_ROOT_FH;
                _IterateWidenWindow(uColor, iScore, &iBeta, &uFailState, +1);
            }
        }
        while(1); // repeat until we run out of time or fall inside a..b

        //
        // We are here because either:
        //
        //    1. we completed a search depth,
        // or 2. we ran out of time,
        // or 3. there is one legal move and we want to make it immediately,
        // or 4. there was user input and we unrolled the search.
        //

        //
        // TODO: Scale back history between iterative depths?
        //

        //
        // Mate-in-n when we have searched n?  Stop searching.
        //
        if (IS_VALID_SCORE(iScore) &&
            (ULONG)abs(iScore) >= (INFINITY - uDepth))
        {
            Trace("FORCED MATE --> stop searching now\n");
            g_MoveTimer.bvFlags |= TIMER_STOPPING;
        }

        //
        // Have we reached user's maxdepth option?  Stop searching.
        //
        if (uDepth >= g_Options.uMaxDepth)
        {
            Trace("DEPTH LIMIT --> stop searching now\n");
            g_MoveTimer.bvFlags |= TIMER_STOPPING;
        }

        //
        // Did the move timer expire?  Stop searching.
        //
        if (g_MoveTimer.bvFlags & TIMER_STOPPING) break;
    }
    g_Options.u64NodesSearched = ctx->sCounters.tree.u64TotalNodeCount;
    g_MoveTimer.dEndTime = SystemTimeStamp();

    //
    // Here we are at the end of a search.  If we were:
    //
    // ...Searching then:               ...Pondering then:
    //
    // 1. we ran out of time            1. they made a different move than
    //                                     the predicted move and we aborted
    // 2. we ran out of depth
    //                                  2. we ran out of depth
    // 3. we found a mate-in-n
    //                                  3. we found a mate-in-n
    // TODO: node count limit?
    //                                 (4. if they made the predicted move
    //                                     then we converted to a search)
    //
    ASSERT(ctx->mvRootMove.uMove);
    ASSERT(SanityCheckMove(&board, ctx->mvRootMove));
    if (g_MoveTimer.bvFlags & TIMER_RESOLVING_ROOT_FL)
    {
        ctx->sPlyInfo[ctx->uPly].PV[ctx->uPly] = ctx->mvRootMove;
        ctx->sPlyInfo[ctx->uPly].PV[ctx->uPly+1] = NULLMOVE;
    }
    ret.eResult = RESULT_IN_PROGRESS;
    ret.szDescription[0] = '\0';
 end:
    return ret;
}

void
MakeTheMove(SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    Make the move that search selected in the official game list.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    FLAG

**/
{
    MOVE mv = ctx->mvRootMove;
    ASSERT(mv.uMove);

    if (TRUE == g_Options.fThinking)
    {
        //
        // TODO: keep track of how many moves in a row we are at or
        // below a draw score and use this to respond to draw offers.
        //

        //
        // Was this a search that converted from a ponder?
        //
        if (TRUE == g_Options.fSuccessfulPonder)
        {
            if (FALSE == OfficiallyMakeMove(g_Options.mvPonder, 0, FALSE))
            {
                UtilPanic(CANNOT_OFFICIALLY_MAKE_MOVE,
                          GetRootPosition(),
                          (void *)g_Options.mvPonder.uMove,
                          NULL,
                          NULL,
                          __FILE__,
                          __LINE__);
            }
            g_Options.fSuccessfulPonder = FALSE;
        }

        // Only do d2d4 stuff if we're under xboard -- it's the standard.
        if (g_Options.fRunningUnderXboard) {
            Trace("move %s\n", MoveToIcs(mv));
        } else {
            Trace("move %s\n", MoveToSan(mv, &ctx->sPosition));
        }

        if (FALSE == OfficiallyMakeMove(mv, g_iScore, FALSE))
        {
            UtilPanic(CANNOT_OFFICIALLY_MAKE_MOVE,
                      GetRootPosition(),
                      (void *)mv.uMove,
                      NULL,
                      NULL,
                      __FILE__,
                      __LINE__);
        }
        if (g_Options.fVerbosePosting)
        {
            PostMoveSearchReport(ctx);
            PostMoveTestSuiteReport(ctx);
        }
    }
    g_Options.fPondering = g_Options.fThinking = FALSE;
}


GAME_RESULT
Ponder(POSITION *pos)
/**

Routine description:

    Prepare to ponder.

Parameters:

    POSITION *pos

Return value:

    FLAG

**/
{
    static SEARCHER_THREAD_CONTEXT ctx;
    GAME_RESULT ret;

    InitializeSearcherContext(pos, &ctx);
    g_Options.fPondering = TRUE;
    g_Options.fThinking = FALSE;
    g_Options.fSuccessfulPonder = FALSE;
    ret.eResult = RESULT_IN_PROGRESS;
    ret.szDescription[0] = '\0';

    //
    // When do we not want to ponder
    //
    if ((g_Options.ePlayMode == FORCE_MODE) || (g_Options.uMyClock < 10))
    {
        g_Options.fPondering = FALSE;
        return ret;
    }

    //
    // What should we ponder?
    //
    g_Options.mvPonder = GetPonderMove(pos);
    if (g_Options.mvPonder.uMove == 0)
    {
        g_Options.fPondering = FALSE;
        return ret;
    }
    ASSERT(SanityCheckMove(pos, g_Options.mvPonder));

    //
    // Prepare to ponder by doing some maintenance on the dynamic move
    // ordering scheme counters, changing the dirty tag in the hash
    // code, and clearing the root nodes-per-move hash.
    //
    MaintainDynamicMoveOrdering();
    DirtyHashTable();

    //
    // Make the move ponder move.
    //
    if (FALSE == MakeMove(&ctx, g_Options.mvPonder))
    {
        ASSERT(FALSE);
        g_Options.fPondering = FALSE;
        return ret;
    }

    //
    // TODO: probe the book
    //

    _SetMoveTimerForPonder();

    //
    // TODO: Any preEval?
    //

    //
    // TODO: Set draw value
    //

    MakeStatusLine();
#if (PERF_COUNTERS && MP)
    ClearHelperThreadIdleness();
#endif
    ASSERT(ctx.uPly == 1);
    ret = Iterate(&ctx);
    ASSERT(ctx.uPly == 1);
    if (ret.eResult == RESULT_IN_PROGRESS) {
        MakeTheMove(&ctx);
        return _DetectPostMoveTerminalStates(&ctx);
    } else {
        return ret;
    }
}


GAME_RESULT Think(POSITION *pos)
/**

Routine description:

    Prepare to think.

Parameters:

    POSITION *pos

Return value:

    FLAG

**/
{
    static SEARCHER_THREAD_CONTEXT ctx;
    static ULONG _resign_count = 0;
    MOVE mvBook;
	  CHAR *p;

    InitializeSearcherContext(pos, &ctx);
    g_MoveTimer.bvFlags = 0;
    g_Options.fPondering = FALSE;
    g_Options.fThinking = TRUE;
    g_Options.fSuccessfulPonder = FALSE;

    //
    // Prepare to think by doing some maintenance on the dynamic move
    // ordering scheme counters, changing the dirty tag in the hash
    // code, and clearing the root nodes-per-move hash.
    //
    if (NULL != (p = PositionToFen(&(ctx.sPosition))))
    {
        Trace("The root position is: %s\n", p);
        SystemFreeMemory(p);
    }
    MaintainDynamicMoveOrdering();
    DirtyHashTable();

    //
    // Check the opening book, maybe it has a move for us.
    //
    if (g_uBookProbeFailures < 3 && 
        !WeAreRunningASuite() &&
        g_Options.szBookName[0] != '\0') {
        mvBook = BookMove(pos, BOOKMOVE_SELECT_MOVE);
        if (mvBook.uMove == 0)
        {
            g_uBookProbeFailures++;
        }
        else
        {
            g_uBookProbeFailures = 0;
            ASSERT(SanityCheckMove(pos, mvBook));
            ctx.mvRootMove = mvBook;
            MakeTheMove(&ctx);
            return _DetectPostMoveTerminalStates(&ctx);
        }
    }

    //
    // TODO: Any preEval?
    //

    //
    // Set time limit for move
    //
    SetMoveTimerForSearch(FALSE, pos->uToMove);

    //
    // TODO: Set draw value
    //
#if (PERF_COUNTERS && MP)
    ClearHelperThreadIdleness();
#endif
    GAME_RESULT ret = Iterate(&ctx);
    if (ret.eResult == RESULT_IN_PROGRESS) {
        if (g_Options.iResignThreshold != 0 &&
            g_iRootScore[ctx.sPosition.uToMove] < g_Options.iResignThreshold) {
            _resign_count++;
            if (_resign_count > 2) {
                if (ctx.sPosition.uToMove == WHITE) {
                    ret.eResult = RESULT_BLACK_WON;
                    strcpy(ret.szDescription, "white resigned");
                } else {
                    ret.eResult = RESULT_WHITE_WON;
                    strcpy(ret.szDescription, "black resigned");                                }
                Trace("tellics resign\n");
            }
            return ret;
        } else {
            _resign_count = 0;
        }
        MakeTheMove(&ctx);
        return _DetectPostMoveTerminalStates(&ctx);
    } else {
        return ret;
    }
}
