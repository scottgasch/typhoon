/**

Copyright (c) Scott Gasch

Module Name:

    testgenerate.c

Abstract:

    Test the move generator.

Author:

    Scott Gasch (scott.gasch@gmail.com) 12 May 2004

Revision History:

    $Id: testgenerate.c 345 2007-12-02 22:56:42Z scott $

**/

#ifdef TEST
#include "chess.h"

UINT64 g_uPlyTestLeafNodeCount = 0;
UINT64 g_uPlyTestTotalNodeCount = 0;

extern CHAR *GenerateRandomLegalFenString();

void 
PlyTest(SEARCHER_THREAD_CONTEXT *ctx,
        ULONG uDepth,
        FLAG fInCheck)
{
    ULONG u;
    MOVE mv;
    ULONG uPly = ctx->uPly;
    ULONG uLegalMoves = 0;
    FLAG fGivesCheck;
#if DEBUG
    POSITION board;
    memcpy(&board, &(ctx->sPosition), sizeof(POSITION));
#endif
    
    if (uDepth > MAX_PLY_PER_SEARCH) 
    {
        return;
    }
    else if (uDepth == 0)
    {
        g_uPlyTestTotalNodeCount++;
        g_uPlyTestLeafNodeCount++;
        return;
    }
    g_uPlyTestTotalNodeCount++;

    mv.uMove = 0;
    GenerateMoves(ctx, mv, (fInCheck ? GENERATE_ESCAPES : GENERATE_ALL_MOVES));
    for (u = ctx->sMoveStack.uBegin[uPly];
         u < ctx->sMoveStack.uEnd[uPly];
         u++)
    {
        mv = ctx->sMoveStack.mvf[u].mv;
        mv.bvFlags |= WouldGiveCheck(ctx, mv);
        
        if (MakeMove(ctx, mv))
        {
            uLegalMoves++;
            ASSERT(!InCheck(&ctx->sPosition, GET_COLOR(mv.pMoved)));
            fGivesCheck = IS_CHECKING_MOVE(mv);
#ifdef DEBUG
            if (fGivesCheck)
            {
                ASSERT(InCheck(&ctx->sPosition, FLIP(GET_COLOR(mv.pMoved))));
            }
            else
            {
                ASSERT(!InCheck(&ctx->sPosition, FLIP(GET_COLOR(mv.pMoved))));
            }
#endif
            PlyTest(ctx, uDepth - 1, fGivesCheck);
            UnmakeMove(ctx, mv);
            ASSERT(PositionsAreEquivalent(&board, &ctx->sPosition));
        }
    }
}


void 
TestMoveGenerator(void)
{
    typedef struct _TEST_MOVEGEN
    {
        char *szFen;
        UINT64 uLeaves[7];
    }
    TEST_MOVEGEN;
    
    TEST_MOVEGEN x[] = 
    {
     {
      "3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1",
      { 218, 0, 0, 0, 0, 0, 0 }
     },
     {
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - - 0 0",
      { 48, 2039, 97862, 4085603, 193690690ULL, 8031647685ULL }
     },
     {
      "8/PPP4k/8/8/8/8/4Kppp/8 w - - 0 0",
      { 18, 290, 5044, 89363, 1745545, 34336777ULL, 749660761ULL }
     },
     { 
      "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0",
      { 14, 191, 2812, 43238, 674624, 11030083ULL, 78633661ULL }
     },
     {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 0",
      { 20, 400, 8902, 197281, 4865609, 119060324ULL, 3195901860ULL }
     },
    };
    ULONG u, v;
    
    SEARCHER_THREAD_CONTEXT *ctx;
    POSITION pos;

    Trace("Testing move generator...\n");
    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    ASSERT(ctx);
    for (u = 0; u < ARRAY_LENGTH(x); u++)
    {
        if (FALSE == FenToPosition(&pos, x[u].szFen)) 
        {
            UtilPanic(INCONSISTENT_STATE,
                      NULL, NULL, NULL, NULL,
                      __FILE__, __LINE__);
        }
        InitializeSearcherContext(&pos, ctx);

        for (v = 1; v <= 4; v++)
        {
            g_uPlyTestLeafNodeCount = 0;
            g_uPlyTestTotalNodeCount = 0;
            PlyTest(ctx, v, FALSE);
            
            if ((x[u].uLeaves[v-1]) &&
                (g_uPlyTestLeafNodeCount != x[u].uLeaves[v-1]))
            {
                UtilPanic(TESTCASE_FAILURE,
                          NULL, "Perft", NULL, NULL,
                          __FILE__, __LINE__);
            }
        }
    }
    SystemFreeMemory(ctx);
}


void
TestLegalMoveGenerator(void)
{
    ULONG u, v;
    CHAR *p;
    POSITION pos;
    SEARCHER_THREAD_CONTEXT *ctx;
    MOVE mv;
    ULONG uLegalKingMoves;
    ULONG uTotalLegalMoves;
    ULONG uGen;

    Trace("Testing legal move generator...\n");
    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    ASSERT(ctx);
    u = 0;
    do
    {
        switch(rand() % 10) 
        {
            case 0:
                p = "8/2K1p2k/1Q2B1p1/2N5/q4q2/3p2Pp/7P/8 w  - - 2 0";
                break;
            default:
                p = GenerateRandomLegalFenString();
                break;
        }

        if (FALSE == FenToPosition(&pos, p))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "FenToPosition", NULL, NULL,
                      __FILE__, __LINE__);
        }
        
        if (FALSE == IsBoardLegal(&pos))
        {
            continue;
        }

        if (TRUE == InCheck(&pos, pos.uToMove))
        {
            uLegalKingMoves = uTotalLegalMoves = 0;
            InitializeSearcherContext(&pos, ctx);
            
            mv.uMove = 0;
            GenerateMoves(ctx, mv, GENERATE_ESCAPES);
            for (v = ctx->sMoveStack.uBegin[0]; 
                 v < ctx->sMoveStack.uEnd[0];
                 v++)
            {
                mv = ctx->sMoveStack.mvf[v].mv;
                ASSERT(mv.bvFlags & MOVE_FLAG_ESCAPING_CHECK);
                if (TRUE == MakeMove(ctx, mv))
                {
                    uTotalLegalMoves++;
                    if (IS_KING(mv.pMoved))
                    {
                        uLegalKingMoves++;
                    }
                    UnmakeMove(ctx, mv);
                }
                else
                {
                    ctx->sMoveStack.mvf[v].mv.uMove = 0;
                }
            }

            ASSERT(NUM_KING_MOVES(ctx, ctx->uPly) == uLegalKingMoves);
            if (uTotalLegalMoves > 1) 
            {
                ASSERT(!ONE_LEGAL_MOVE(ctx, ctx->uPly));
            }
            else if (uTotalLegalMoves == 1) 
            {
                ASSERT(ONE_LEGAL_MOVE(ctx, ctx->uPly));
            }
            u++;
        }
    }
    while(u < 1000);
    SystemFreeMemory(ctx);
}
#endif
