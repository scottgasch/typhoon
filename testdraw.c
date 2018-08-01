/**

Copyright (c) Scott Gasch

Module Name:

    testdraw.c

Abstract:

    Test the draw detection code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 30 Jun 2004

Revision History:

    $Id: testdraw.c 345 2007-12-02 22:56:42Z scott $

**/

#ifdef TEST
#include "chess.h"

void
TestDraw(void)
{
    SEARCHER_THREAD_CONTEXT *ctx;
    POSITION pos;
    MOVE mv;

    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    ASSERT(ctx);

    Trace("Testing draw detection code...\n");

    (void)FenToPosition(&pos,
              "rnbqkbnr/ppp2ppp/8/3pp3/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 0");
    InitializeSearcherContext(&pos, ctx);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }

    mv.uMove = 0;
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    UnmakeMove(ctx, mv);
    
    mv.uMove = MAKE_MOVE(F1, D3, WHITE_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 1)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    mv.uMove = MAKE_MOVE(F8, D6, BLACK_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 2)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    mv.uMove = MAKE_MOVE(D3, F1, WHITE_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 3)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    mv.uMove = MAKE_MOVE(D6, F8, BLACK_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (FALSE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 4)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    UnmakeMove(ctx, mv);

    // ====
    
    mv.uMove = MAKE_MOVE(E5, D4, BLACK_PAWN, WHITE_PAWN, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 0)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    mv.uMove = MAKE_MOVE(E4, D5, WHITE_PAWN, BLACK_PAWN, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 0)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }

    // ---
    
    mv.uMove = MAKE_MOVE(D6, C5, BLACK_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 1)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    mv.uMove = MAKE_MOVE(F1, E2, WHITE_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 2)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    mv.uMove = MAKE_MOVE(C5, D6, BLACK_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (TRUE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 3)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    mv.uMove = MAKE_MOVE(E2, F1, WHITE_BISHOP, 0, 0, 0);
    MakeMove(ctx, mv);
    if (FALSE == IsDraw(ctx))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }
    if (ctx->sPosition.uFifty != 4)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "IsDraw", NULL, NULL,
                  __FILE__, __LINE__);
    }

    //
    // TODO: make some moves "officially" and test rolling back into
    // the gamelist to detect a draw.
    //
    SystemFreeMemory(ctx);
}
#endif
