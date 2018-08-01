/**

Copyright (c) Scott Gasch

Module Name:

    testbitboard.c

Abstract:

    Unit test for the bitboard code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 19 Oct 2004

Revision History:

**/

#ifdef TEST
#include "chess.h"

void TestBitboards(void)
/**

Routine description:

    Sanity check bitboard code and benchmark the different ways to
    twiddle bits.

Parameters:

    void

Return value:

    void

**/
{
    COOR c;
    ULONG b;
    BITBOARD bb;
    BITBOARD bbSpeed[1000];
    ULONG u, v, w, z;
    UINT64 u64;

    Trace("Testing bitboard operations...\n");

    //
    // Make a table of 1000 random bitboards
    //
    for (u = 0;
         u < 1000;
         u++)
    {
        bbSpeed[u] = 0;
        w = rand() % 24;
        for (v = 0; v < w; v++)
        {
            z = RANDOM_COOR;
            ASSERT(SLOWCOOR_TO_BB(z) == COOR_TO_BB(z));
            bb = COOR_TO_BB(z);
            bbSpeed[u] |= bb;
        }
    }

#if 1
    //
    // Various benchmaring code.
    //
    u64 = SystemReadTimeStampCounter();
    for (u = 0; u < 1000000; u++)
    {
        b = u % 64;
        bb = SLOWCOOR_TO_BB(b);
    }
    printf(" SLOWCOOR_TO_BB:   %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / 1000000);
    u64 = SystemReadTimeStampCounter();
    for (u = 0; u < 1000000; u++)
    {
        b = u % 64;
        bb = COOR_TO_BB(b);
    }
    printf(" COOR_TO_BB:       %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / 1000000);

    u64 = SystemReadTimeStampCounter();
    for (v = 1; v < 1000; v++)
    {
        for (u = 0; u < 1000; u++)
        {
            b = SlowCountBits(bbSpeed[u]);
        }
    }
    printf(" SlowCountBits:    %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / (1000 * 1000));

    u64 = SystemReadTimeStampCounter();
    for (v = 0; v < 1000; v++)
    {
        for (u = 0; u < 1000; u++)
        {
            b = CountBits(bbSpeed[u]);
        }
    }
    printf(" CountBits:        %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / (1000 * 1000));

    u64 = SystemReadTimeStampCounter();
    for (v = 0; v < 1000; v++)
    {
        for (u = 0; u < 1000; u++)
        {
            b = SlowLastBit(bbSpeed[u]);
        }
    }
    printf(" SlowLastBit:      %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / (1000 * 1000));

    u64 = SystemReadTimeStampCounter();
    for (v = 0; v < 1000; v++)
    {
        for (u = 0; u < 1000; u++)
        {
            b = LastBit(bbSpeed[u]);
        }
    }
    printf(" LastBit:          %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / (1000 * 1000));

    u64 = SystemReadTimeStampCounter();
    for (v = 0; v < 1000; v++)
    {
        for (u = 0; u < 1000; u++)
        {
            b = SlowFirstBit(bbSpeed[u]);
        }
    }
    printf(" SlowFirstBit:     %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / (1000 * 1000));

    u64 = SystemReadTimeStampCounter();
    for (v = 0; v < 1000; v++)
    {
        for (u = 0; u < 1000; u++)
        {
            b = DeBruijnFirstBit(bbSpeed[u]);
        }
    }
    printf(" DeBruijnFirstBit: %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / (1000 * 1000));

    u64 = SystemReadTimeStampCounter();
    for (v = 0; v < 1000; v++)
    {
        for (u = 0; u < 1000; u++)
        {
            b = FirstBit(bbSpeed[u]);
        }
    }
    printf(" FirstBit:         %" COMPILER_LONGLONG_UNSIGNED_FORMAT 
           " cycles/op\n",
           (SystemReadTimeStampCounter() - u64) / (1000 * 1000));
#endif

    FOREACH_SQUARE(c)
    {
        if (!IS_ON_BOARD(c)) continue;
        
        if (BIT_NUMBER_TO_COOR(COOR_TO_BIT_NUMBER(c)) != c)
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, 
                      "bit number to coor / coor to bit number", 
                      NULL, 
                      NULL,
                      __FILE__, __LINE__);
        }

        if (SLOW_BIT_NUMBER_TO_COOR(COOR_TO_BIT_NUMBER(c)) != c)
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, 
                      "slow bit number to coor / coor to bit number", 
                      NULL,
                      NULL,
                      __FILE__, __LINE__);
        }
        
        if (BIT_NUMBER_TO_COOR(COOR_TO_BIT_NUMBER(c)) !=
            SLOW_BIT_NUMBER_TO_COOR(COOR_TO_BIT_NUMBER(c)))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, 
                      "bit number to coor / slow bit number to coor", 
                      NULL, 
                      NULL,
                      __FILE__, __LINE__);
        }
    }
    
    for (b = 1;
         b < 64;
         b++)
    {
        if (COOR_TO_BIT_NUMBER(BIT_NUMBER_TO_COOR(b)) != (b))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, 
                      "coor to bit number / bit number to coor", 
                      NULL, 
                      NULL,
                      __FILE__, __LINE__);
        }
    }
    
    bb = COOR_TO_BB(A4);
    bb |= COOR_TO_BB(A6);
    bb |= COOR_TO_BB(A8);
    bb |= COOR_TO_BB(B6);
    bb |= COOR_TO_BB(H2);
    if (CountBits(bb) != 5)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "CountBits", NULL, NULL,
                  __FILE__, __LINE__);
    }
    bb &= BBFILE[A];
    if (CountBits(bb) != 3)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "CountBits", NULL, NULL,
                  __FILE__, __LINE__);
    }
    b = 0;
    while(IS_ON_BOARD(c = CoorFromBitBoardRank8ToRank1(&bb)))
    {
        b++;
        if ((c != A4) && (c != A6) && (c != A8))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "CoorFromBitBoardRank8ToRank1", NULL, NULL,
                      __FILE__, __LINE__);
        }
    }

    if (b != 3)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "CoorFromBitBoardRank8ToRank1", NULL, NULL,
                  __FILE__, __LINE__);
    }

    bb = SLOWCOOR_TO_BB(A4) | SLOWCOOR_TO_BB(H2);
    bb |= SLOWCOOR_TO_BB(D4) | SLOWCOOR_TO_BB(F6) | SLOWCOOR_TO_BB(F4);
    if (CountBits(bb) != 5)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "CountBits", NULL, NULL,
                  __FILE__, __LINE__);
    }
    bb &= BBRANK[4];
    if (CountBits(bb) != 3)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "CountBits", NULL, NULL,
                  __FILE__, __LINE__);
    }
    b = 0;
    while(IS_ON_BOARD(c = CoorFromBitBoardRank1ToRank8(&bb)))
    {
        b++;
        if ((c != A4) && (c != D4) && (c != F4))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "CoorFromBitBoardRank1ToRank8", NULL, NULL,
                      __FILE__, __LINE__);
        }
    }

    if (b != 3)
    {
        UtilPanic(TESTCASE_FAILURE,
                  NULL, "CoorFromBitBoardRank1ToRank8", NULL, NULL,
                  __FILE__, __LINE__);
    }
}
#endif
