/**

Copyright (c) Scott Gasch

Module Name:

    testsearch.c

Abstract:

    This is a test module to sanity check the search code.  It
    operates by generating a random legal chess position and
    performing a two-ply search on the position.  It is expected that
    the search will terminate and return a legal move.

Author:

    Scott Gasch (scott.gasch@gmail.com) 2 Jan 2005

Revision History:

**/

#include "chess.h"

#ifdef TEST
#define TIME_LIMIT 200.0

void
SetMoveTimerForTestingSearch(void)
{
    g_MoveTimer.uNodeCheckMask = 0x10000 - 1;
    g_MoveTimer.dStartTime = SystemTimeStamp();
    g_MoveTimer.bvFlags = 0;
    g_MoveTimer.dSoftTimeLimit = g_MoveTimer.dStartTime + TIME_LIMIT;
    g_MoveTimer.dHardTimeLimit = g_MoveTimer.dStartTime + TIME_LIMIT;
}

FLAG
TestSearch(void)
{
    SEARCHER_THREAD_CONTEXT *ctx;
    POSITION pos;
    ULONG u;
    FLAG fOver;
    FLAG fPost = g_Options.fShouldPost;
    FLAG fRet = FALSE;

    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    ASSERT(ctx);
    g_Options.fShouldPost = FALSE;
    Trace("Testing Search routine...\n");
    for (u = 0; u < 20; u++)
    {
        GenerateRandomLegalPosition(&pos);
        InitializeSearcherContext(&pos, ctx);
    
        g_MoveTimer.bvFlags = 0;
        g_Options.fPondering = FALSE;
        g_Options.fThinking = TRUE;
        g_Options.fSuccessfulPonder = FALSE;
        
        MaintainDynamicMoveOrdering();
        DirtyHashTable();
        
        //
        // TODO: Any preEval?
        //
        
        //
        // Set a very long time limit on the search
        //
        SetMoveTimerForTestingSearch();
        g_Options.uMaxDepth = 2;
        
        //
        // TODO: Set draw value
        //
#if (PERF_COUNTERS && MP)
        ClearHelperThreadIdleness();
#endif
        fOver = Iterate(ctx);

        //
        // How long did that take?
        //
        if (SystemTimeStamp() - g_MoveTimer.dStartTime > TIME_LIMIT)
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "TestSearch", NULL, NULL,
                      __FILE__, __LINE__);
        }

        //
        // Did we get a sane move?
        //
        if (GAME_NOT_OVER == fOver)
        {
            if (FALSE == SanityCheckMove(&pos, ctx->mvRootMove))
            {
                UtilPanic(TESTCASE_FAILURE,
                          NULL, "TestSearch", NULL, NULL,
                          __FILE__, __LINE__);
            }
        }
#ifdef DEBUG
        else if (GAME_WHITE_WON == fOver)
        {
            ASSERT(InCheck(&pos, BLACK));
        }
        else if (GAME_BLACK_WON == fOver)
        {
            ASSERT(InCheck(&pos, WHITE));
        }
#endif
    }
    fRet = TRUE;

    g_Options.fShouldPost = fPost;
    SystemFreeMemory(ctx);
    return(fRet);
}
#endif
