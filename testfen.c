/**

Copyright (c) Scott Gasch

Module Name:

    testfen.c

Abstract:

    Test the fen parser.

Author:

    Scott Gasch (scott.gasch@gmail.com) 11 May 2004

Revision History:

    $Id: testfen.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

char *_szPieceAbbrevs = "pPnNbBrRqQkK";

#ifdef TEST
CHAR *
GenerateRandomLegalFenString(void)
/**

Routine description:

Parameters:

    void

Return value:

    CHAR

**/
{
    static CHAR buf[MEDIUM_STRING_LEN_CHAR];
    CHAR *pch = buf;              
    static ULONG uPieceLimits[14] = 
      // -  -  p  P  n  N  b  B  r  R  q  Q  k  K
        {0, 0, 8, 8, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1};
    ULONG uPieceCounts[14] = 
      // -  -  p  P  n  N  b  B  r  R  q  Q  k  K
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1};
    ULONG uNumPiecesOnBoard = 2;
    COOR c;
    COOR cKingLoc[2];
    PIECE p;

    memset(buf, 0, sizeof(buf));

    //
    // Do the board.  Note: this can result in a position where one or
    // both kings are en-prise, pawns on the 1st/8th etc.  The purpose
    // is just to test PositionToFen and FenToPosition, not to make 
    // legal chess position.
    //
    cKingLoc[WHITE] = RANDOM_COOR;
    do
    {
        cKingLoc[BLACK] = RANDOM_COOR;
    }
    while(cKingLoc[WHITE] == cKingLoc[BLACK]);
    ASSERT(IS_ON_BOARD(cKingLoc[WHITE]));
    ASSERT(IS_ON_BOARD(cKingLoc[BLACK]));
    
    FOREACH_SQUARE(c)
    {
        if (!IS_ON_BOARD(c)) 
        {
            if (c % 8 == 0) *pch++ = '/';
            continue;
        }
        else
        {
            if (c == cKingLoc[WHITE])
            {
                *pch = 'K';
            }
            else if (c == cKingLoc[BLACK])
            {
                *pch = 'k';
            }
            else
            {
                if ((rand() & 1) &&
                    (uNumPiecesOnBoard < 32))
                {
                    do
                    {
                        p = RANDOM_PIECE;
                        ASSERT(IS_VALID_PIECE(p));
                    }
                    while(uPieceCounts[p] >= uPieceLimits[p]);
                    uPieceCounts[p]++;
                    uNumPiecesOnBoard++;
                    *pch = *(_szPieceAbbrevs + (p - 2));
                }
                else
                {
                    *pch = '-';
                }
            }
        }
        pch++;
    }
    *pch++ = ' ';

    //
    // Do side on move
    //
    *pch = 'w';
    if (rand() & 1)
    {
        *pch = 'b';
    }
    pch++;
    *pch++ = ' ';

    //
    // Do castling permissions
    //
    *pch++ = '-';
    *pch++ = ' ';
    
    //
    // Do the rest
    //
    strcat(buf, "- 0 1");
    
    return(buf);
}


void
TestFenCode(void)
{
    char *p;
    char *q;
    UINT u = 0;
    POSITION pos1, pos2;
    
    Trace("Testing FEN translation code...\n");
    do
    {
        //
        // Generate a random legal FEN string and parse it into pos1.
        //
        p = GenerateRandomLegalFenString();
        if (FALSE == FenToPosition(&pos1, p))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, p, NULL, NULL,
                      __FILE__, __LINE__);
        }

        //
        // Set q to be the FEN code of the position at pos1.
        //
        q = PositionToFen(&pos1);
        if (NULL == q)
        {
            UtilPanic(TESTCASE_FAILURE,
                      &pos1, "q <- PositionToFen", NULL, NULL,
                      __FILE__, __LINE__);
        }

        //
        // Set pos2 to be the position represented in q. 
        //
        if (FALSE == FenToPosition(&pos2, q))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, q, NULL, NULL,
                      __FILE__, __LINE__);
        }

        //
        // Pos1 and pos2 should be identical
        //
        if (FALSE == PositionsAreEquivalent(&pos1, &pos2))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL,
                      "One initial FEN string has generated two "
                      "different positions",
                      &pos1,
                      &pos2,
                      __FILE__, __LINE__);
        }
        SystemFreeMemory(q);
        u++;
    }
    while(u < 10000);
}
#endif
