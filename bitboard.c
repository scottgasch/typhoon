/**

Copyright (c) Scott Gasch

Module Name:

    bitboard.c

Abstract:

    Routines dealing with bitboards.  If you build with the
    testbitboard.c code enabled it will give you a rough benchmark of
    these routines' speeds.  Here are some typical results (from a
    1.5ghz Athlon):

        SLOWCOOR_TO_BB: 10 cycles/op
        COOR_TO_BB:     5 cycles/op
        SlowCountBits:  82 cycles/op
        CountBits:      2 cycles/op
        SlowLastBit:    158 cycles/op
        LastBit:        2 cycles/op
        SlowFirstBit:   22 cycles/op
        FirstBit:       2 cycles/op

    As you can see the inline assembly language code is quite a bit
    faster so use it if you can.  And if, on the off chance, you are
    porting this code to another hardware platform, it might be worth
    your time to research what your instruction set has in the way of
    bit twiddling opcodes.

Author:

    Scott Gasch (scott.gasch@gmail.com) 19 Jun 2004

Revision History:

    $Id: bitboard.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"


//
// Mapping from square -> bit
//
BITBOARD BBSQUARE[64] =
{
    0x1ULL,                                   // 1
    0x2ULL,                                   // 2
    0x4ULL,                                   // 3
    0x8ULL,                                   // 4
    0x10ULL,                                  // 5
    0x20ULL,                                  // 6
    0x40ULL,                                  // 7
    0x80ULL,                                  // 8
    0x100ULL,                                 // 9
    0x200ULL,                                 // 10
    0x400ULL,                                 // 11
    0x800ULL,                                 // 12
    0x1000ULL,                                // 13
    0x2000ULL,                                // 14
    0x4000ULL,                                // 15
    0x8000ULL,                                // 16
    0x10000ULL,                               // 17
    0x20000ULL,                               // 18
    0x40000ULL,                               // 19
    0x80000ULL,                               // 20
    0x100000ULL,                              // 21
    0x200000ULL,                              // 22
    0x400000ULL,                              // 23
    0x800000ULL,                              // 24
    0x1000000ULL,                             // 25
    0x2000000ULL,                             // 26
    0x4000000ULL,                             // 27
    0x8000000ULL,                             // 28
    0x10000000ULL,                            // 29
    0x20000000ULL,                            // 30
    0x40000000ULL,                            // 31
    0x80000000ULL,                            // 32
//--------------------------------------------------
    0x100000000ULL,                           // 33
    0x200000000ULL,                           // 34
    0x400000000ULL,                           // 35
    0x800000000ULL,                           // 36
    0x1000000000ULL,                          // 37
    0x2000000000ULL,                          // 38
    0x4000000000ULL,                          // 39
    0x8000000000ULL,                          // 40
    0x10000000000ULL,                         // 41
    0x20000000000ULL,                         // 42
    0x40000000000ULL,                         // 43
    0x80000000000ULL,                         // 44
    0x100000000000ULL,                        // 45
    0x200000000000ULL,                        // 46
    0x400000000000ULL,                        // 47
    0x800000000000ULL,                        // 48
    0x1000000000000ULL,                       // 49
    0x2000000000000ULL,                       // 50
    0x4000000000000ULL,                       // 51
    0x8000000000000ULL,                       // 52
    0x10000000000000ULL,                      // 53
    0x20000000000000ULL,                      // 54
    0x40000000000000ULL,                      // 55
    0x80000000000000ULL,                      // 56
    0x100000000000000ULL,                     // 57
    0x200000000000000ULL,                     // 58
    0x400000000000000ULL,                     // 59
    0x800000000000000ULL,                     // 60
    0x1000000000000000ULL,                    // 61
    0x2000000000000000ULL,                    // 62
    0x4000000000000000ULL,                    // 63
    0x8000000000000000ULL,                    // 64
};



//
// The white colored squares on the board
//
BITBOARD BBWHITESQ = (
    SLOWCOOR_TO_BB(A2) | SLOWCOOR_TO_BB(A4) | SLOWCOOR_TO_BB(A6) | 
    SLOWCOOR_TO_BB(A8) | SLOWCOOR_TO_BB(B1) | SLOWCOOR_TO_BB(B3) | 
    SLOWCOOR_TO_BB(B5) | SLOWCOOR_TO_BB(B7) | SLOWCOOR_TO_BB(C2) | 
    SLOWCOOR_TO_BB(C4) | SLOWCOOR_TO_BB(C6) | SLOWCOOR_TO_BB(C8) |
    SLOWCOOR_TO_BB(D1) | SLOWCOOR_TO_BB(D3) | SLOWCOOR_TO_BB(D5) | 
    SLOWCOOR_TO_BB(D7) | SLOWCOOR_TO_BB(E2) | SLOWCOOR_TO_BB(E4) | 
    SLOWCOOR_TO_BB(E6) | SLOWCOOR_TO_BB(E8) | SLOWCOOR_TO_BB(F1) | 
    SLOWCOOR_TO_BB(F3) | SLOWCOOR_TO_BB(F5) | SLOWCOOR_TO_BB(F7) |
    SLOWCOOR_TO_BB(G2) | SLOWCOOR_TO_BB(G4) | SLOWCOOR_TO_BB(G6) | 
    SLOWCOOR_TO_BB(G8) | SLOWCOOR_TO_BB(H1) | SLOWCOOR_TO_BB(H3) | 
    SLOWCOOR_TO_BB(H5) | SLOWCOOR_TO_BB(H7)
    );

//
// The black colored squares on the board
//
BITBOARD BBBLACKSQ = ~(
    SLOWCOOR_TO_BB(A2) | SLOWCOOR_TO_BB(A4) | SLOWCOOR_TO_BB(A6) | 
    SLOWCOOR_TO_BB(A8) | SLOWCOOR_TO_BB(B1) | SLOWCOOR_TO_BB(B3) | 
    SLOWCOOR_TO_BB(B5) | SLOWCOOR_TO_BB(B7) | SLOWCOOR_TO_BB(C2) | 
    SLOWCOOR_TO_BB(C4) | SLOWCOOR_TO_BB(C6) | SLOWCOOR_TO_BB(C8) |
    SLOWCOOR_TO_BB(D1) | SLOWCOOR_TO_BB(D3) | SLOWCOOR_TO_BB(D5) | 
    SLOWCOOR_TO_BB(D7) | SLOWCOOR_TO_BB(E2) | SLOWCOOR_TO_BB(E4) | 
    SLOWCOOR_TO_BB(E6) | SLOWCOOR_TO_BB(E8) | SLOWCOOR_TO_BB(F1) | 
    SLOWCOOR_TO_BB(F3) | SLOWCOOR_TO_BB(F5) | SLOWCOOR_TO_BB(F7) |
    SLOWCOOR_TO_BB(G2) | SLOWCOOR_TO_BB(G4) | SLOWCOOR_TO_BB(G6) | 
    SLOWCOOR_TO_BB(G8) | SLOWCOOR_TO_BB(H1) | SLOWCOOR_TO_BB(H3) | 
    SLOWCOOR_TO_BB(H5) | SLOWCOOR_TO_BB(H7)
    );


//
// A way to select one file
//
BITBOARD BBFILE[8] = {
    BBFILEA,
    BBFILEB,
    BBFILEC,
    BBFILED,
    BBFILEE,
    BBFILEF,
    BBFILEG,
    BBFILEH
};

//
// A way to select one rank
//
BITBOARD BBRANK[9] = {
    0,
    BBRANK11,
    BBRANK22,
    BBRANK33,
    BBRANK44,
    BBRANK55,
    BBRANK66,
    BBRANK77,
    BBRANK88
};

//
// Two files: A and H
//
BITBOARD BBROOK_PAWNS = BBFILEA | BBFILEH;


ULONG CDECL
SlowCountBits(BITBOARD bb)
/**

Routine description:

    Strictly-C implementation of bit counting.  How many bits are
    asserted in a particular bitboard?

Parameters:

    BITBOARD bb : the bitboard to count

Return value:

    ULONG : number of bits set in bb

**/
{
    ULONG uCount = 0;
    while(bb)
    {
        uCount++;
        bb &= (bb - 1);
    }
    return(uCount);
}


static const int foldedTable[] = {
    63,30, 3,32,59,14,11,33,
    60,24,50, 9,55,19,21,34,
    61,29, 2,53,51,23,41,18,
    56,28, 1,43,46,27, 0,35,
    62,31,58, 4, 5,49,54, 6,
    15,52,12,40, 7,42,45,16,
    25,57,48,13,10,39, 8,44,
    20,47,38,22,17,37,36,26,
};

ULONG CDECL 
DeBruijnFirstBit(BITBOARD bb) 
{
	int folded;
    if (bb == 0ULL) return(0);
    bb ^= (bb - 1);
    folded = ((int)bb) ^ ((int)(bb >> 32));
    return foldedTable[(folded * 0x78291ACF) >> 26] + 1;
}      

ULONG CDECL
SlowFirstBit(BITBOARD bb)
/**

Routine description:

    Strictly-C implementation of "find the number of the first (lowest
    order) asserted bit in the bitboard".

Parameters:

    BITBOARD bb : the bitboard to test

Return value:

    ULONG : the number of the first bit from the low order side of the
            bb that is asserted.  The lowest order bit is #1.

**/
{
    static ULONG uTable[16] = 
    { // 0000  0001  0010  0011  0100  0101  0110  0111 
         0,    1,    2,    1,    3,    1,    2,    1,
      // 1000  1001  1010  1011  1100  1101  1110  1111
         4,    1,    2,    1,    3,    1,    2,    1 };
    ULONG uShifts = 0;
    ULONG u;
    
    while(bb)
    {
        u = (ULONG)bb & 0xF;
        if (0 != u)
        {
            return(uTable[u] + (uShifts * 4));
        }
        bb >>= 4;
        uShifts++;
    }
    return(0);
}


ULONG CDECL
SlowLastBit(BITBOARD bb)
/**

Routine description:

    Strictly-C implementation of "find the number of the last (highest
    order) bit asserted in the bitboard".

    Note: On every system benchmarked this code sucked.  Using the x86 
    bsr instruction is way faster on modern x86 family processors.

Parameters:

    BITBOARD bb : the bitboard to test

Return value:

    ULONG : the number of the first bit from the high order side of bb
            that is asserted.  The highest order bit is #64.

**/
{
    static ULONG uTable[16] = 
    { // 0000  0001  0010  0011  0100  0101  0110  0111 
            0,    1,    2,    2,    3,    3,    3,    3,
      // 1000  1001  1010  1011  1100  1101  1110  1111
            4,    4,    4,    4,    4,    4,    4,    4 };
    ULONG uShifts = 15;
    ULONG u;
    
    while(bb)
    {
        u = (ULONG)((bb & 0xF000000000000000ULL) >> 60);
        if (0 != u)
        {
            return(uTable[u] + (uShifts * 4));
        }
        bb <<= 4;
        uShifts--;
        ASSERT(uShifts < 15);
    }
    return(0);
}

COOR 
CoorFromBitBoardRank8ToRank1(BITBOARD *pbb)
/**

Routine description:

    Return the square cooresponding to the first asserted bit in the
    bitboard or ILLEGAL_COOR if there are no bits asserted in the
    bitboard.  If a valid COOR is returned, clear that bit in the
    bitboard.

Parameters:

    BITBOARD *pbb

Return value:

    COOR

**/
{
    COOR c = ILLEGAL_COOR;
    ULONG uFirstBit = FirstBit(*pbb);

    ASSERT(uFirstBit == SlowFirstBit(*pbb));
    if (0 != uFirstBit)
    {
        uFirstBit--; // bit 1 is 1 << 0
        c = BIT_NUMBER_TO_COOR(uFirstBit);
        ASSERT(c == SLOW_BIT_NUMBER_TO_COOR(uFirstBit));
        ASSERT(IS_ON_BOARD(c));
        *pbb &= (*pbb - 1);
    }
    return(c);
}


COOR 
CoorFromBitBoardRank1ToRank8(BITBOARD *pbb)
/**

Routine description:

    Return the square cooresponding to the last asserted bit in the
    bitboard or ILLEGAL_COOR if there are no bits asserted in the
    bitboard.  If a valid COOR is returned, clear that bit in the
    bitboard.

Parameters:

    BITBOARD *pbb

Return value:

    COOR

**/
{
    COOR c;
    ULONG uLastBit = LastBit(*pbb);
    ASSERT(SlowLastBit(*pbb) == uLastBit);
    
    c = ILLEGAL_COOR;
    if (0 != uLastBit)
    {
        ASSERT(*pbb);
        uLastBit--;
        *pbb &= (*pbb - 1);
        c = BIT_NUMBER_TO_COOR(uLastBit);
        ASSERT(c == SLOW_BIT_NUMBER_TO_COOR(uLastBit));
        ASSERT(IS_ON_BOARD(c));
    }
    return(c);
}
