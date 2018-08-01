/**

Copyright (c) Scott Gasch

Module Name:

    testics.c

Abstract:

    Test the ICS-style (d2d4) move parser.

Author:

    Scott Gasch (scott.gasch@gmail.com) 17 May 2004

Revision History:

    $Id: testics.c 345 2007-12-02 22:56:42Z scott $

**/
#ifdef TEST

#include "chess.h"

void TestIcs(void)
/**

Routine description:

    Test the ICS-style (d2d4) move parser.

Parameters:

    void

Return value:

    void

**/
{
    typedef struct _TEST_ICS_MOVE
    {
        CHAR *szICS;
        MOVE mv;
    }
    TEST_SAN_MOVE;
    TEST_SAN_MOVE x[] = 
    {
        { "e1c1"  , { MAKE_MOVE(E1, C1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL) } },
        { "o-o-o" , { MAKE_MOVE(E1, C1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL) } },
        { "a2xc4" , { MAKE_MOVE(A2, C4, WHITE_BISHOP, BLACK_PAWN, 0, 0) } },
        { "h2-h3" , { MAKE_MOVE(H2, H3, WHITE_PAWN, 0, 0, 0) } },
        { "h2h4"  , { MAKE_MOVE(H2, H4, WHITE_PAWN, 0, 0, MOVE_FLAG_SPECIAL) } },
        { "h4h5"  , { MAKE_MOVE(0, 0, 0, 0, 0, 0 ) } },
        { "b2c4"  , { MAKE_MOVE(B2, C4, WHITE_KNIGHT, BLACK_PAWN, 0, 0) } },
        { "b6c4"  , { MAKE_MOVE(B6, C4, WHITE_KNIGHT, BLACK_PAWN, 0, 0) } },
        { "b6d7"  , { MAKE_MOVE(B6, D7, WHITE_KNIGHT, BLACK_BISHOP, 0, 0) } },
        { "h2xg3" , { MAKE_MOVE(H2, G3, WHITE_PAWN, BLACK_KNIGHT, 0, 0) } },
        { "d2d7++", { MAKE_MOVE(D2, D7, WHITE_QUEEN, BLACK_BISHOP, 0, 0) } },
        { "d2d7#" , { MAKE_MOVE(D2, D7, WHITE_QUEEN, BLACK_BISHOP, 0, 0) } },
        { "f5f6"  , { MAKE_MOVE(F5, F6, WHITE_PAWN, 0, 0, 0) } },
        { "b7b8=Q", { MAKE_MOVE(B7, B8, WHITE_PAWN, 0, WHITE_QUEEN, MOVE_FLAG_SPECIAL) } },
        { "b7b8Q" , { MAKE_MOVE(B7, B8, WHITE_PAWN, 0, WHITE_QUEEN, MOVE_FLAG_SPECIAL) } },
        { "b7xa8N", { MAKE_MOVE(B7, A8, WHITE_PAWN, BLACK_ROOK, WHITE_KNIGHT, MOVE_FLAG_SPECIAL) } },
    };
    POSITION pos;
    ULONG u;
    MOVE mvICS;
    
    Trace("Testing ICS parser code...\n");
    for (u = 0;
         u < ARRAY_LENGTH(x);
         u++)
    {
        if (FALSE == FenToPosition(&pos, 
                                   "r2kr3/pP1b2nb/1N6/5P2/2p5/6n1/BN1QBP1P/R3K2R w KQ - 0 1"))
        {
            UtilPanic(INCONSISTENT_STATE,
                      NULL, NULL, NULL, NULL,
                      __FILE__, __LINE__);
        }

        mvICS = ParseMoveIcs(x[u].szICS, &pos);
        if (mvICS.uMove != x[u].mv.uMove)
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "ParseMoveICS", NULL, NULL,
                      __FILE__, __LINE__);
        }
    }
}
#endif
