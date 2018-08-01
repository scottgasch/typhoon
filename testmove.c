/**

Copyright (c) Scott Gasch

Module Name:

    testmove.c

Abstract:

Author:

    Scott Gasch (scott.gasch@gmail.com) 11 May 2004

Revision History:

    $Id: testmove.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

#ifdef TEST

static COOR 
_TestPickRandomPieceLocation(POSITION *pos)
/**

Routine description:

    Test code: supports TestLiftPlaceSlidePiece.  Picks a random square
    where there is a piece on board.

Parameters:

    POSITION *pos

Return value:

    static COOR

**/
{
    ULONG u = RANDOM_COLOR;
    ULONG v = rand() & 1;
    ULONG w;
    COOR c;
    
    if ((TRUE == v) && (pos->uPawnCount[u]))
    {
        w = rand() % pos->uPawnCount[u];
        c = pos->cPawns[u][w];
    }
    else
    {
        ASSERT(pos->uNonPawnCount[u][0]);
        w = rand() % pos->uNonPawnCount[u][0];
        c = pos->cNonPawns[u][w];
    }
    ASSERT(IS_ON_BOARD(c));
    return(c);
}


void 
TestLiftPlaceSlidePiece(void)
/**

Routine description:

    Test code.  Tests LiftPiece, PlacePiece and SlidePiece.

Parameters:

    POSITION *pos

Return value:

    void

**/
{
#define NUM_LOOPS 1000000
    POSITION pos;
    PIECE p[30];
    ULONG u = 0;
    UINT v = 0;
    COOR cFrom, cTo;
    UINT64 u64Start = SystemReadTimeStampCounter();
    UINT64 u64End;

    Trace("Testing Lift/Place/Slide Piece... ");
    if (FALSE ==  FenToPosition(&pos, 
                                "RNBKQBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbkqbnr"
                                " w KQkq - 0 1"))
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, NULL, NULL, NULL, 
                  __FILE__, __LINE__);
    }
    
    memset(p, 0, sizeof(p));
    do
    {
        switch(rand() % 3)
        {
            case 0:                           // slide piece
                cFrom = _TestPickRandomPieceLocation(&pos);
                do
                {
                    cTo = RANDOM_COOR;
                }
                while(!IS_EMPTY(pos.rgSquare[cTo].pPiece));

                //
                // Make sure bishops slide to their color or else
                // pos->uWhiteSqBishopCount gets messed up.
                //
                if (IS_BISHOP(pos.rgSquare[cFrom].pPiece))
                {
                    while ((IS_WHITE_SQUARE_COOR(cTo) !=
                            IS_WHITE_SQUARE_COOR(cFrom)) ||
                            (!IS_EMPTY(pos.rgSquare[cTo].pPiece)))
                    {
                        cTo = RANDOM_COOR;
                    }
                }

                //
                // Note: a speedup in move.c caused a divorce between
                // SlidePiece and SlidePawn and caused the necessity
                // of this test.
                //
                if (IS_PAWN(pos.rgSquare[cFrom].pPiece)) 
                {
                    SlidePawn(&pos, cFrom, cTo);
                }
                else
                {
                    SlidePiece(&pos, cFrom, cTo);
                }
                break;
            
            case 1:                           // LiftPiece
                if (u < 20)
                {
                    do
                    {
                        cFrom = _TestPickRandomPieceLocation(&pos);
                    }
                    while(IS_KING(pos.rgSquare[cFrom].pPiece));
                    p[u] = LiftPiece(&pos, cFrom);
                    u++;
                }
                break;
                    
            case 2:                           // PlacePiece
                if (u > 0)
                {
                    u--;
                    do
                    {
                        cTo = RANDOM_COOR;
                    }
                    while(!IS_EMPTY(pos.rgSquare[cTo].pPiece));
                    PlacePiece(&pos, cTo, p[u]);
                }
                break;
        }
        v++;
    }
    while(v < NUM_LOOPS);
    u64End = SystemReadTimeStampCounter();

    printf("\n");
    printf(" L/P/S bench:    %" COMPILER_LONGLONG_UNSIGNED_FORMAT
           " cycles/loop.\n",
           (u64End - u64Start) / (UINT64)NUM_LOOPS);
}


void 
TestExposesCheck(void)
/**

Routine description:

    Test code.  Tests ExposesCheck.

Parameters:

    void

Return value:

    void

**/
{
    typedef struct _TEST_EXPOSES_CHECK
    {
        CHAR *szFen;
        COOR cFrom;
        COOR cTo;
        COOR cRemove;
    }
    TEST_EXPOSES_CHECK;
    
    ULONG u;
    POSITION pos;
    TEST_EXPOSES_CHECK x[] = 
    {
        { "k2r4/8/8/3p4/8/8/3K4/8 w - - 0 1",
          D8, D2, D5 },
        { "k2q4/8/8/3p4/8/8/3K4/8 w - - 0 1",
          D8, D2, D5 },
        { "k5b1/8/8/3p4/8/1K6/8/8 w - - 0 1",
          G8, B3, D5 },
        { "k5q1/8/8/3p4/8/1K6/8/8 w - - 0 1",
          G8, B3, D5 },
        { "k7/8/8/1K1p2r1/8/8/8/8 w - - 0 1",
          G5, B5, D5 },
        { "k7/8/8/1K1p2q1/8/8/8/8 w - - 0 1",
          G5, B5, D5 },
        { "k7/8/2K5/3p4/8/8/6b1/8 w - - 0 1",
          G2, C6, D5 },
        { "k7/8/2K5/3p4/8/8/6q1/8 w - - 0 1",
          G2, C6, D5 },
        { "k7/3K4/8/3p4/8/8/3r4/8 w - - 0 1",
          D2, D7, D5 },
        { "k7/3K4/8/3p4/8/8/3q4/8 w - - 0 1",
          D2, D7, D5 },
        { "k7/5K2/8/3p4/2b5/8/8/8 w - - 0 1",
          C4, F7, D5 },
        { "k7/5K2/8/3p4/2q5/8/8/8 w - - 0 1",
          C4, F7, D5 },
        { "k7/8/8/1r1pK3/8/8/8/8 w - - 0 1",
          B5, E5, D5 },
        { "k7/8/8/1q1pK3/8/8/8/8 w - - 0 1",
          B5, E5, D5 },
        { "k7/8/2b5/3p4/8/5K2/8/8 w - - 0 1",
          C6, F3, D5 },
        { "k7/8/2q5/3p4/8/5K2/8/8 w - - 0 1",
          C6, F3, D5 },
        //
        { "8/5k2/8/3N4/8/1B6/8/6K1 w - - 0 1",
          B3, F7, D5 },
        { "8/5k2/8/3N4/8/1Q6/8/6K1 w - - 0 1",
          B3, F7, D5 },
        { "3k4/8/8/3N4/8/8/8/3R2K1 w - - 0 1",
          D1, D8, D5 },
        { "8/3k4/8/3N4/8/8/8/3Q2K1 w - - 0 1",
          D1, D7, D5 },
        { "8/8/8/Q2Nk3/8/8/8/6K1 w - - 0 1",
          A5, E5, D5 },
        { "8/8/8/R2Nk3/8/8/8/6K1 w - - 0 1",
          A5, E5, D5 },
        { "8/1Q6/8/3N4/8/5k2/8/6K1 w - - 0 1",
          B7, F3, D5 },
        { "8/1B6/8/3N4/8/5k2/8/6K1 w - - 0 1",
          B7, F3, D5 },
        { "8/3R4/8/3N4/8/3k4/8/6K1 w - - 0 1",
          D7, D3, D5 },
        { "8/3Q4/8/3N4/8/3k4/8/6K1 w - - 0 1",
          D7, D3, D5 },
        { "6Q1/8/8/3N4/8/8/k7/6K1 w - - 0 1",
          G8, A2, D5 },
        { "6B1/8/8/3N4/8/8/k7/6K1 w - - 0 1",
          G8, A2, D5 },
        { "8/8/8/k2N1R2/8/8/8/6K1 w - - 0 1",
          F5, A5, D5 },
        { "8/8/8/k2N1Q2/8/8/8/6K1 w - - 0 1",
          F5, A5, D5 },
        { "8/1k6/8/3N4/8/5Q2/8/6K1 w - - 0 1",
          F3, B7, D5 },
        { "8/1k6/8/3N4/8/5B2/8/6K1 w - - 0 1",
          F3, B7, D5 },
        { "8/5k2/4p3/3N4/8/1Q6/8/6K1 w - - 0 1",
          0x88, F7, D5 },
        { "8/5k2/4p3/3N4/8/1B6/8/6K1 w - - 0 1",
          0x88, F7, D5 },
        { "8/8/8/1Q1Np1k1/8/8/8/6K1 w - - 0 1",
          0x88, G5, D5 },
        { "8/8/8/1R1Np1k1/8/8/8/6K1 w - - 0 1",
          0x88, G5, D5 },
        { "8/1B6/8/3N4/4p3/5k2/8/6K1 w - - 0 1",
          0x88, F3, D5 },
        { "8/1Q6/8/3N4/4p3/5k2/8/6K1 w - - 0 1",
          0x88, F3, D5 },
        { "8/3R4/8/3N4/3p4/3k4/8/6K1 w - - 0 1",
          0x88, D3, D5 },
        { "8/3Q4/8/3N4/3p4/3k4/8/6K1 w - - 0 1",
          0x88, D3, D5 },
        { "8/5B2/8/3N4/2p5/1k6/8/6K1 w - - 0 1",
          0x88, B3, D5 },
        { "8/5Q2/8/3N4/2p5/1k6/8/6K1 w - - 0 1",
          0x88, B3, D5 },
        { "8/8/8/1krN1R2/8/8/8/6K1 w - - 0 1",
          0x88, B5, D5 },
        { "8/8/8/1krN1Q2/8/8/8/6K1 w - - 0 1",
          0x88, B5, D5 },
        { "8/1k6/2r5/3N4/8/5Q2/8/6K1 w - - 0 1",
          0x88, B7, D5 },
    };

    Trace("Testing ExposesCheck...\n");
    for (u = 0; 
         u < ARRAY_LENGTH(x);
         u++)
    {
        if (FALSE == FenToPosition(&pos, x[u].szFen))
        {
            Bug("TestExposesCheck: FEN reader choked on a good FEN.\n");
            ASSERT(FALSE);
        }
        if (x[u].cFrom != ExposesCheck(&pos, x[u].cRemove, x[u].cTo))
        {
            Bug("TestExposesCheck: ExposesCheck is broken on position %u!\n",
                u);
            ASSERT(FALSE);
        }
    }
}


void 
TestIsAttacked(void)
/**

Routine description:

    Test code.  Tests IsAttacked (and InCheck).

Parameters:

    void

Return value:

    void

**/
{
    typedef struct _TEST_IS_ATTACKED
    {
        CHAR *szFen;
        ULONG uSide;
        COOR cSquare;
        FLAG fAnswer;
    }
    TEST_IS_ATTACKED;

    POSITION pos;
    ULONG u;
    TEST_IS_ATTACKED x[] = 
    {
        { "7k/4n3/3P4/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3Pp3/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/2pP4/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/2n5/3P4/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/1n1P4/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P1n2/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "6bk/8/3P4/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "6qk/8/3P4/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "6bk/8/3PN3/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, FALSE },
        { "6bk/8/3Pr3/3K4/8/8/8/8 w - - 0 1",
          BLACK, D5, FALSE },
        { "7k/8/3P4/3K3q/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P4/3K3r/8/8/8/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P4/3KQ2q/8/8/8/8 w - - 0 1",
          BLACK, D5, FALSE },
        { "7k/8/3P4/3K4/8/8/8/7b w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P4/3K4/8/8/8/7q w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P4/3K4/8/3r4/3q4/3r4 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P4/3K4/3P4/3r4/3q4/3r4 w - - 0 1",
          BLACK, D5, FALSE },
        { "7k/8/3P4/3K4/3p4/3r4/3q4/3r4 w - - 0 1",
          BLACK, D5, FALSE },
        { "7k/8/3P4/3K4/3p4/3rn3/3q4/3r4 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P4/3K4/2b5/1b6/b7/8 w - - 0 1",
          BLACK, D5, TRUE },
        { "7k/8/3P4/3K4/2r5/1b6/b7/8 w - - 0 1",
          BLACK, D5, FALSE },
        { "K7/8/8/3k4/3p1N2/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K7/8/8/3k4/3p4/4N3/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K7/8/8/3k4/3p4/2N5/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K7/8/8/3k4/1N1p4/8/8/8 w - - 0 1", 
          WHITE, D5, TRUE },
        { "K7/8/8/3k4/2Pp4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K7/8/8/3k4/3pP3/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K5Q1/5B2/8/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K5Q1/5B2/4R3/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, FALSE },
        { "K5Q1/8/8/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K2R4/8/8/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K2R4/3Q4/8/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K2R4/3Q4/3p4/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, FALSE },
        { "K2R4/3Q4/3p1N2/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K7/1B6/8/3k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K7/8/8/Q2k4/3p4/8/8/8 w - - 0 1",
          WHITE, D5, TRUE },
        { "K7/8/8/3k4/3p4/8/B7/8 w - - 0 1",
          WHITE, D5, TRUE }
    };

    Trace("Testing IsAttacked...\n");
    for (u = 0;
         u < ARRAY_SIZE(x);
         u++)
    {
        if (FALSE == FenToPosition(&pos, x[u].szFen))
        {
            Bug("TestIsAttacked: FEN reader choked on a good FEN.\n");
            ASSERT(FALSE);
        }
        if (x[u].fAnswer != IsAttacked(x[u].cSquare, &pos, x[u].uSide))
        {
            Bug("TestIsAttacked: ExposesCheck is broken on position %u!\n",
                u);
            ASSERT(FALSE);
        }
    }
}


void 
TestMakeUnmakeMove(void)
{
    typedef struct _TEST_MAKE_MOVE
    {
        CHAR *szFen;
        MOVE mv[10];
        FLAG ok[10];
    }
    TEST_MAKE_MOVE;

    ULONG u = 0;
    ULONG v = 0;
    SEARCHER_THREAD_CONTEXT *ctx;
    POSITION p;
    POSITION *pos = NULL;
    FLAG fOk;
    TEST_MAKE_MOVE x[] = 
    {
        { 
            "8/8/8/2k5/4p3/8/2KP4/8 w - - 0 1",
            { 
                { MAKE_MOVE(D2, D4, WHITE_PAWN, 0, 0, MOVE_FLAG_SPECIAL) },
                { MAKE_MOVE(E4, D3, BLACK_PAWN, WHITE_PAWN, 0, MOVE_FLAG_SPECIAL | MOVE_FLAG_CHECKING | MOVE_FLAG_ESCAPING_CHECK)},
                { MAKE_MOVE(C2, D3, WHITE_KING, BLACK_PAWN, 0, MOVE_FLAG_ESCAPING_CHECK) },
                { MAKE_MOVE(0, 0, 0, 0, 0, 0) }
            },
            { TRUE, TRUE, TRUE }
        },
        { 
            "5r1k/3KP3/8/8/8/8/8/8 w - - 0 1",
            { 
                { MAKE_MOVE(E7, F8, WHITE_PAWN, BLACK_ROOK, WHITE_QUEEN, MOVE_FLAG_SPECIAL | MOVE_FLAG_CHECKING) }, 
                { MAKE_MOVE(H8, G8, BLACK_KING, 0, 0, MOVE_FLAG_ESCAPING_CHECK) },
                { MAKE_MOVE(H8, H7, BLACK_KING, 0, 0, MOVE_FLAG_ESCAPING_CHECK) },
                { MAKE_MOVE(0, 0, 0, 0, 0, 0) }
            },
            { TRUE, FALSE, TRUE }
        },
        { 
            "8/8/4K3/8/k2p3R/8/4P3/8 w - - 0 1",
            { 
                { MAKE_MOVE(E2, E4, WHITE_PAWN, 0, 0, MOVE_FLAG_SPECIAL) },
                { MAKE_MOVE(D4, E3, BLACK_PAWN, WHITE_PAWN, 0, MOVE_FLAG_SPECIAL) },
                { MAKE_MOVE(D4, D3, BLACK_PAWN, 0, 0, 0) },
                { MAKE_MOVE(E4, E5, WHITE_PAWN, 0, 0, MOVE_FLAG_CHECKING) },
                { MAKE_MOVE(D3, D2, BLACK_PAWN, 0, 0, MOVE_FLAG_ESCAPING_CHECK) },
                { MAKE_MOVE(A4, B4, BLACK_KING, 0, 0, MOVE_FLAG_ESCAPING_CHECK) },
                { MAKE_MOVE(A4, A3, BLACK_KING, 0, 0, MOVE_FLAG_ESCAPING_CHECK) },
                { MAKE_MOVE(0, 0, 0, 0, 0, 0) }
            },
            { TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE }
        },
        { 
            "3k4/8/6r1/8/8/8/8/R3K2R w KQ - 0 1",
            { 
                { MAKE_MOVE(E1, G1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL) },
                { MAKE_MOVE(H1, H2, WHITE_ROOK, 0, 0, 0) },
                { MAKE_MOVE(G6, D6, BLACK_ROOK, 0, 0, 0) },
                { MAKE_MOVE(E1, C1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL) },
                { MAKE_MOVE(H2, H1, WHITE_ROOK, 0, 0, 0) },
                { MAKE_MOVE(D6, A6, BLACK_ROOK, 0, 0, 0) },
                { MAKE_MOVE(E1, G1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL) },
                { MAKE_MOVE(E1, C1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL | MOVE_FLAG_CHECKING) },
                { MAKE_MOVE(A6, D6, BLACK_ROOK, 0, 0, MOVE_FLAG_ESCAPING_CHECK) },
                { MAKE_MOVE(0, 0, 0, 0, 0, 0) }
            },
            { FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE }
        }
    };

    //
    // Setup
    //
    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    ASSERT(ctx);
    pos = SystemAllocateMemory(sizeof(POSITION) * 10);
    ASSERT(ctx);

    for (u = 0; u < ARRAY_LENGTH(x); u++)
    {
        if (FALSE == FenToPosition(&p, x[u].szFen))
        {
            UtilPanic(INCONSISTENT_STATE,
                      NULL, NULL, NULL, NULL,
                      __FILE__, __LINE__);
        }
        InitializeSearcherContext(&p, ctx);
        
        for (v = 0; v < ARRAY_LENGTH(x[u].mv); v++)
        {
            if (x[u].mv[v].uMove == 0) break;
            memcpy(&(pos[v]), &(ctx->sPosition), sizeof(POSITION));
            
            fOk = MakeUserMove(ctx, x[u].mv[v]);
            if (fOk != x[u].ok[v]) 
            {
                UtilPanic(TESTCASE_FAILURE,
                          NULL, "MakeUserMove", NULL, NULL,
                          __FILE__, __LINE__);
            }
            if (fOk)
            {
                UnmakeMove(ctx, x[u].mv[v]);
                if (FALSE == PositionsAreEquivalent(&(ctx->sPosition),
                                                    &(pos[v])))
                {
                    UtilPanic(TESTCASE_FAILURE,
                              NULL, "MakeMove / UnmakeMove", NULL, NULL,
                              __FILE__, __LINE__);
                }
                fOk = MakeUserMove(ctx, x[u].mv[v]);
                ASSERT(fOk);
            }
        }
    }
    SystemFreeMemory(ctx);
    SystemFreeMemory(pos);
}
#endif
