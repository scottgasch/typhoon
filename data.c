/**

Copyright (c) Scott Gasch

Module Name:

    data.c

Abstract:

    Large data structures and the code that creates/verifies them.

Author:

    Scott Gasch (scott.gasch@gmail.com) 10 May 2004

Revision History:

    $Id: data.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

// Distance between two squares lookup table
ULONG g_uDistance[256];
ULONG *g_pDistance = &(g_uDistance[128]);

// Who controls the square lookup table
CHAR g_SwapTable[14][32][32];

// Vector (bitvector indicating which pieces can move between squares) and
// Delta (direction to travel to get from square a to square b) lookup table.
VECTOR_DELTA g_VectorDelta[256];
VECTOR_DELTA *g_pVectorDelta = &(g_VectorDelta[128]);

// How many generated moves should we bother to sort
ULONG g_uSearchSortLimits[MAX_PLY_PER_SEARCH];

// Hardcoded move patterns to terminate PVs with
MOVE NULLMOVE = {0};
MOVE HASHMOVE = {0x11118888};
MOVE RECOGNMOVE = {0x22228888};
MOVE DRAWMOVE = {0x33338888};
MOVE MATEMOVE = {0x44448888};

// Is a square white?
FLAG g_fIsWhiteSquare[128];

#ifdef DEBUG
COOR 
DistanceBetweenSquares(COOR a, COOR b)
{
    int i = (int)a - (int)b + 128;
    int j = (int)b - (int)a + 128;
    ASSERT(IS_ON_BOARD(a));
    ASSERT(IS_ON_BOARD(b));
    ASSERT((i >= 0) && (i < 256));
    ASSERT(g_uDistance[i] == REAL_DISTANCE(a, b));
    ASSERT(g_pDistance[a - b] == g_uDistance[i]);
    ASSERT(g_pDistance[b - a] == g_uDistance[i]);
    ASSERT(g_uDistance[j] == REAL_DISTANCE(a, b));
    return(REAL_DISTANCE(a, b));
}

#define VALID_VECTOR(x) \
    (((x) == 0) || \
     ((x) == (1 << KNIGHT)) || \
     ((x) == ( (1 << BISHOP) | (1 << QUEEN))) || \
     ((x) == ( (1 << ROOK) | (1 << QUEEN))) || \
     ((x) == ( (1 << BISHOP) | (1 << QUEEN) | (1 << KING))) || \
     ((x) == ( (1 << ROOK) | (1 << QUEEN) | (1 << KING))) || \
     ((x) == ( (1 << BISHOP) | (1 << QUEEN) | (1 << KING) | (1 << PAWN))))


#define VALID_INDEX(x) \
    (((x) >= -128) && ((x) <= 127))

ULONG
CheckVectorWithIndex(int i, ULONG uColor) 
{
	ULONG u;
    ASSERT(VALID_INDEX(i));
    ASSERT(IS_VALID_COLOR(uColor));
    u = g_pVectorDelta[i].iVector[uColor];
    ASSERT(VALID_VECTOR(u));
    ASSERT(u == g_VectorDelta[i + 128].iVector[uColor]);
    ASSERT(&(g_pVectorDelta[i]) == &(g_VectorDelta[i + 128]));
    return(u);
}

#define VALID_DELTA(x) \
    (((x) == 0) || \
     ((x) == +1) || ((x) == -1) || \
     ((x) == +16) || ((x) == -16) || \
     ((x) == +15) || ((x) == -15) || \
     ((x) == +17) || ((x) == -17))

int
DirectionBetweenSquaresWithIndex(int i) 
{
    int iDir;
    ASSERT(VALID_INDEX(i));
    iDir = g_pVectorDelta[i].iDelta;
    ASSERT(iDir == g_VectorDelta[i + 128].iDelta);
    ASSERT(&(g_pVectorDelta[i]) == &(g_VectorDelta[i + 128]));
    ASSERT(VALID_DELTA(iDir));
    ASSERT(iDir == -g_pVectorDelta[i].iNegDelta);
    return(iDir);
}

int 
DirectionBetweenSquaresFromTo(COOR from, COOR to) 
{
    int i = (int)from - (int)to;
    return DirectionBetweenSquaresWithIndex(i);
}

int 
NegativeDirectionBetweenSquaresWithIndex(int i)
{
    int iDir;
    ASSERT(VALID_INDEX(i));
    iDir = g_pVectorDelta[i].iNegDelta;
    ASSERT(iDir == g_VectorDelta[i + 128].iNegDelta);
    ASSERT(&(g_pVectorDelta[i]) == &(g_VectorDelta[i + 128]));
    ASSERT(VALID_DELTA(iDir));
    ASSERT(iDir == -g_pVectorDelta[i].iDelta);
    return(iDir);
}
    
int
DirectionBetweenSquares(COOR cFrom, COOR cTo)
{
    int i = (int)cFrom - (int)cTo;
    return(DirectionBetweenSquaresWithIndex(i));
}

FLAG 
IsSquareWhite(COOR c) 
{
	BITBOARD bb;
    FLAG f = IS_WHITE_SQUARE_COOR(c);
    ASSERT(IS_ON_BOARD(c));
    ASSERT(f == g_fIsWhiteSquare[c]);
    bb = COOR_TO_BB(c);
    ASSERT(f == ((bb & BBWHITESQ) != 0));
    ASSERT(f == ((bb & BBBLACKSQ) == 0));
    return(f);
}
#endif

void
VerifyVectorDelta(void)
{
    int iIndex;
    int iChecksum = 0;
    
    for (iIndex = 0;
         iIndex < 256;
         iIndex++)
    {
        ASSERT(VALID_VECTOR(g_VectorDelta[iIndex].iVector[BLACK]));
        ASSERT(VALID_VECTOR(g_VectorDelta[iIndex].iVector[WHITE]));
        ASSERT(VALID_DELTA(g_VectorDelta[iIndex].iDelta));
        ASSERT(VALID_DELTA(g_VectorDelta[iIndex].iNegDelta));
        ASSERT(-g_VectorDelta[iIndex].iDelta == 
               g_VectorDelta[iIndex].iNegDelta);
        iChecksum += g_VectorDelta[iIndex].iVector[BLACK] * iIndex;
        iChecksum += g_VectorDelta[iIndex].iVector[WHITE] * iIndex;
        iChecksum += g_VectorDelta[iIndex].iDelta * iIndex;
    }
    
    if (iChecksum != 0xb1b58)
    {
        UtilPanic(DETECTED_INCORRECT_INITIALIZATION,
                  NULL,
                  "vector/delta",
                  (void *)iChecksum,
                  (void *)0xb1b58, 
                  __FILE__, __LINE__);
    }
}

void
InitializeSearchDepthArray(void)
{
    ULONG x;

    for (x = 0;
         x < ARRAY_LENGTH(g_uSearchSortLimits);
         x++)
    {
        g_uSearchSortLimits[x] = 5;
    }
    g_uSearchSortLimits[0] = 0;
    g_uSearchSortLimits[1] = 17;
    g_uSearchSortLimits[2] = 12;
    g_uSearchSortLimits[3] = 9;
    g_uSearchSortLimits[4] = 7;
    g_uSearchSortLimits[5] = 6;
}

#ifdef DEBUG
ULONG 
GetSearchSortLimit(ULONG uPly)
{
    ASSERT(uPly > 0);
    ASSERT(uPly < MAX_PLY_PER_SEARCH);
    ASSERT(g_uSearchSortLimits[uPly] != 0);
    return(g_uSearchSortLimits[uPly]);
}
#endif

void
InitializeWhiteSquaresTable(void) 
{
    COOR c;

    FOREACH_SQUARE(c)
    {
        g_fIsWhiteSquare[c] = FALSE;
        if (!IS_ON_BOARD(c)) 
        {
            continue;
        }
        g_fIsWhiteSquare[c] = IS_WHITE_SQUARE_COOR(c);
    }
}

void
InitializeVectorDeltaTable(void)
{
    COOR cStart;
    COOR cNew;
    COOR cRay;
    int iIndex;
    PIECE p;
    int iDir;
    ULONG u;
    static const BYTE _LOCATIONS[] = 
    {
        D4, A5, A3, G3, C1, B5, H4, E5, E3, A6, C5, A5, A5, 
        G5, A6, D3, D2, A5, E1, E1, A6, H4, B2, D1, D2, E5 
    };
    static int iNumDirs[7] =    { 0, 0, 8, 4, 4, 8, 8 };
    static int iDelta[7][8] =   { {   0,   0,   0,   0,   0,   0,   0,   0 },
                                  {   0,   0,   0,   0,   0,   0,   0,   0 },
                                  { -33, -31, -18, -14, +14, +18, +31, +33 },
                                  { -17, -15, +15, +17,   0,   0,   0,   0 },
                                  { -16,  -1,  +1, +16,   0,   0,   0,   0 },
                                  { -17, -16, -15,  -1,  +1, +15, +16, +17 },
                                  { -17, -16, -15,  -1,  +1, +15, +16, +17 } };

    memset(g_VectorDelta, 0, sizeof(g_VectorDelta));
    FOREACH_SQUARE(cStart)
    {
        if (!IS_ON_BOARD(cStart)) continue;
        for (u = 0; u < ARRAY_LENGTH(_LOCATIONS); u++) 
        {
            if (cStart == _LOCATIONS[u]) 
            {
                iIndex = 0;
            }
        }
        
        //
        // Do the pawn bits
        //
        iIndex = (int)cStart - ((int)cStart - 17) + 128;
        g_VectorDelta[iIndex].iVector[WHITE] |= (1 << PAWN);
        iIndex = (int)cStart - ((int)cStart - 15) + 128;
        g_VectorDelta[iIndex].iVector[WHITE] |= (1 << PAWN);
        
        iIndex = (int)cStart - ((int)cStart + 17) + 128;
        g_VectorDelta[iIndex].iVector[BLACK] |= (1 << PAWN);
        iIndex = (int)cStart - ((int)cStart + 15) + 128;
        g_VectorDelta[iIndex].iVector[BLACK] |= (1 << PAWN);
        
        //
        // Do the piece bits
        //
        for (p = KNIGHT; p <= KING; p++)
        {
            for (iDir = 0; 
                 iDir < iNumDirs[p]; 
                 iDir++)
            {
                cRay = cStart;
                cNew = cRay + iDelta[p][iDir];
                while (IS_ON_BOARD(cNew))
                {
                    iIndex = (int)cStart - (int)cNew + 128;
                    
                    //
                    // Fill in the vector
                    //
                    g_VectorDelta[iIndex].iVector[WHITE] |= (1 << p);
                    g_VectorDelta[iIndex].iVector[BLACK] |= (1 << p);
                    
                    //
                    // Fill in the delta
                    //
                    if (FILE(cStart) == FILE(cNew))
                    {
                        if (cStart < cNew)
                        {
                            g_VectorDelta[iIndex].iDelta = 16;
                            g_VectorDelta[iIndex].iNegDelta = -16;
                        }
                        else
                        {
                            g_VectorDelta[iIndex].iDelta = -16;
                            g_VectorDelta[iIndex].iNegDelta = 16;
                        }                            
                    }
                    else if (RANK(cStart) == RANK(cNew))
                    {
                        if (cStart < cNew)
                        {
                            g_VectorDelta[iIndex].iDelta = 1;
                            g_VectorDelta[iIndex].iNegDelta = -1;
                        }
                        else
                        {
                            g_VectorDelta[iIndex].iDelta = -1;
                            g_VectorDelta[iIndex].iNegDelta = 1;
                        }
                    }
                    else if ((cStart % 15) == (cNew % 15))
                    {
                        if (cStart < cNew)
                        {
                            g_VectorDelta[iIndex].iDelta = +15;
                            g_VectorDelta[iIndex].iNegDelta = -15;
                        }
                        else
                        {
                            g_VectorDelta[iIndex].iDelta = -15;
                            g_VectorDelta[iIndex].iNegDelta = 15;
                        }
                    }
                    else if ((cStart % 17) == (cNew % 17))
                    {
                        if (cStart < cNew)
                        {
                            g_VectorDelta[iIndex].iDelta = +17;
                            g_VectorDelta[iIndex].iNegDelta = -17;
                        }
                        else
                        {
                            g_VectorDelta[iIndex].iDelta = -17;
                            g_VectorDelta[iIndex].iNegDelta = 17;
                        }
                    }
                    cNew += iDelta[p][iDir];
                    
                    if ((KING == p) || (KNIGHT == p))
                    {
                        break;
                    }
                }
            }
        }
    }
    
    (void)VerifyVectorDelta();
#ifdef OSX
    //
    // nasm under OSX/macho has a nasty bug that causes the addresses
    // of extern symbols to be screwed up.  I only use two extern data
    // structs in the x86.asm code: the vector delta table and the
    // piece data table.  My workaround to the nasm bug is to copy
    // these tables into a symbol in the asm module and access them
    // locally on OSX.
    //
    extern VECTOR_DELTA *g_NasmVectorDelta;
    extern PIECE_DATA *g_NasmPieceData;

    memcpy(&g_NasmVectorDelta, &g_VectorDelta, sizeof(g_VectorDelta));
    memcpy(&g_NasmPieceData, &g_PieceData, sizeof(g_PieceData));
#endif
}

//
// TODO: fix this to use attack counts also
//
void
InitializeSwapTable(void)
{
    ULONG GET_HIGH_BIT[32] = 
    {
        0, // 00000 = 0
        1, // 00001 = 1
        2, // 00010 = 2
        2, // 00011 = 3
        4, // 00100 = 4
        4, // 00101 = 5
        4, // 00110 = 6 
        4, // 00111 = 7
        8, // 01000 = 8 
        8, // 01001 = 9
        8, // 01010 = 10
        8, // 01011 = 11
        8, // 01100 = 12
        8, // 01101 = 13
        8, // 01110 = 14 
        8, // 01111 = 15
       16, // 10000 = 16
       16, // 10001 = 17
       16, // 10010 = 18
       16, // 10011 = 19
       16, // 10100 = 20
       16, // 10101 = 21
       16, // 10110 = 22
       16, // 10111 = 23
       16, // 11000 = 24
       16, // 11001 = 25
       16, // 11010 = 26
       16, // 11011 = 27
       16, // 11100 = 28
       16, // 11101 = 29 
       16, // 11110 = 30
       16  // 11111 = 31
    };
    ULONG uOnMove;
    PIECE p;
    ULONG uAtStake;
    ULONG uBlack;
    ULONG uWhite;
    ULONG uOld;
    ULONG uGains[2];
    ULONG uAttacks[2];
    INT iDiff;
    
    for (p = 0; p <= WHITE_KING; p++)
    {
        if (p > 1)
        {
            for (uBlack = 0; uBlack < 32; uBlack++)
            {
                for (uWhite = 0; uWhite < 32; uWhite++)
                {
                    uGains[BLACK] = uGains[WHITE] = 0;
                    uAttacks[BLACK] = uBlack;
                    uAttacks[WHITE] = uWhite;
                    uAtStake = PIECE_VALUE(p);
                    uOnMove = WHITE;
                    if (GET_COLOR(p) == WHITE)
                    {
                        uOnMove = BLACK;
                    }

                    //
                    // Ok, if the side on move has an attack, play it 
                    // and give them credit for some plunder.
                    //
                    while(uAttacks[uOnMove])
                    {
                        uOld = uAtStake;
                        uGains[uOnMove] += uAtStake;
                        uAtStake = GET_HIGH_BIT[uAttacks[uOnMove]];
                        if (uAtStake == 16)
                        {
                            uAtStake = VALUE_PAWN;
                        }
                        else if (uAtStake == 8)
                        {
                            uAtStake = VALUE_BISHOP;
                        }
                        else if (uAtStake == 4)
                        {
                            uAtStake = VALUE_ROOK;
                        }
                        else if (uAtStake == 2)
                        {
                            uAtStake = VALUE_QUEEN;
                        }
                        else if (uAtStake == 1)
                        {
                            //
                            // Don't let them move into check
                            //
                            if (uAttacks[FLIP(uOnMove)])
                            {
                                uGains[uOnMove] -= uOld;
                                uAtStake = 0;
                            }
                            break;
                        }
                        else
                        {
                            UtilPanic(SHOULD_NOT_GET_HERE,
                                      NULL, NULL, NULL, NULL,
                                      __FILE__, __LINE__);
                        }
                        uAttacks[uOnMove] &= ~GET_HIGH_BIT[uAttacks[uOnMove]];
                        uOnMove = FLIP(uOnMove);
                    }
                    
                    //
                    // The side on move doesn't have an attack... does
                    // the side not on move have an attack?  If so they
                    // get a chance to assert some more control.
                    //
                    if (uAttacks[FLIP(uOnMove)])
                    {
                        uGains[FLIP(uOnMove)] += 100;
                    }

                    if (uGains[BLACK] > uGains[WHITE])
                    {
                        iDiff = uGains[BLACK] - uGains[WHITE];
                        iDiff /= 100;
						iDiff += (iDiff == 0);
                        g_SwapTable[p][uWhite][uBlack] = MIN(+127, iDiff);
                        g_SwapTable[p][uWhite][uBlack] *= -1;
                    }
                    else if (uGains[WHITE] > uGains[BLACK])
                    {
                        iDiff = uGains[WHITE] - uGains[BLACK];
                        iDiff /= 100;
						iDiff += (iDiff == 0);
                        g_SwapTable[p][uWhite][uBlack] = MIN(+127, iDiff);
                    }
                    else
                    {
                        g_SwapTable[p][uWhite][uBlack] = 0;
                    }
                }
            }
        }
        else                                  // no piece sitting there
        {
            for (uBlack = 0; uBlack < 32; uBlack++)
            {
                for (uWhite = 0; uWhite < 32; uWhite++)
                {
                    if (uBlack > uWhite)
                    {
                        g_SwapTable[p][uWhite][uBlack] = -1;
                    }
                    else if (uWhite > uBlack)
                    {
                        g_SwapTable[p][uWhite][uBlack] = +1;
                    }
                    else
                    {
                        g_SwapTable[p][uWhite][uBlack] = 0;
                    }
                }
            }
        }            
    }
}

void
InitializeDistanceTable(void) 
{
    COOR x, y;
    int i;
#ifdef DEBUG
	int j;
#endif

    for (x = 0; x < 128; x++) 
    {
        if (!IS_ON_BOARD(x)) continue;
        for (y = 0; y < 128; y++) 
        {
            if (!IS_ON_BOARD(y)) continue;
            i = (int)x - (int)y;
            i += 128;
            g_uDistance[i] = REAL_DISTANCE(x, y);
            ASSERT(g_uDistance[i] >= 0);
            ASSERT(g_uDistance[i] <= 7);
        }
    }

#ifdef DEBUG
    for (x = 0; x < 128; x++) 
    {
        if (!IS_ON_BOARD(x)) continue;
        for (y = 0; y < 128; y++) 
        {
            if (!IS_ON_BOARD(y)) continue;
            i = (int)x - (int)y + 128;
            ASSERT(g_uDistance[i] == REAL_DISTANCE(x, y));
            ASSERT(g_pDistance[x - y] == g_uDistance[i]);
            ASSERT(&(g_pDistance[x - y]) == &(g_uDistance[i]));
            j = (int)y - (int)x + 128;
            ASSERT(g_uDistance[j] == REAL_DISTANCE(x, y));
            ASSERT(g_pDistance[y - x] == g_uDistance[j]);
            ASSERT(&(g_pDistance[y - x]) == &(g_uDistance[j]));
        }
    }
#endif
}
