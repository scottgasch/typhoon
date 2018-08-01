/**

Copyright (c) Scott Gasch

Module Name:

    sig.c

Abstract:

    Position signature code... the algorithm is a Zobrist hash scheme.
    Every piece has a different 64-bit "seed" for every different
    square.  Plus some non-piece/location features of a position (such
    as legal castling rights or the en passant location) have 64-bit
    "seed" keys.  The position's signature is the xor sum of all these
    seeds.  A position's pawn hash signature is the xor sum of all
    pawn-related seeds.  These signatures, once computed, can be
    incrementally maintained by just xoring off the seed of a moving
    piece's from location and xoring on the seed of its to location.
    These signatures are used as the basis for the pawn hash and main
    transposition table code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 09 May 2004

Revision History:
   
    $Id: sig.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

UINT64 g_u64SigSeeds[128][7][2];
UINT64 g_u64PawnSigSeeds[128][2];
UINT64 g_u64CastleSigSeeds[16];
UINT64 g_u64EpSigSeeds[9];

void 
InitializeSigSystem(void)
/**

Routine description:

    Populate the arrays of signature keys (see above) used to generate
    normal and pawn signatures for POSITIONs.

Parameters:

    void

Return value:

    void

**/
{
    ULONG x, y, z;
    UINT uChecksum = 0;
    ULONG uRandom;
    
    memset(g_u64SigSeeds, 0, sizeof(g_u64SigSeeds));
    memset(g_u64PawnSigSeeds, 0, sizeof(g_u64PawnSigSeeds));
    memset(g_u64CastleSigSeeds, 0, sizeof(g_u64CastleSigSeeds));
    memset(g_u64EpSigSeeds, 0, sizeof(g_u64EpSigSeeds));
    seedMT(2222);

    for (x = A; x <= H+1; x++)
    {
        //
        // Do en-passant sig
        //
        uRandom = randomMT();
        uChecksum ^= uRandom;
        g_u64EpSigSeeds[x] = (UINT64)uRandom << 32;
        uRandom = randomMT();
        uChecksum ^= uRandom;
        g_u64EpSigSeeds[x] |= uRandom;
        g_u64EpSigSeeds[x] &= ~1;
    }

    //
    // Foreach square on the board.
    //
    for (x = 0; x < 128; x++)
    {      
        //
        // For both colors
        // 
        FOREACH_COLOR(z)
        {
            //
            // Do the pawn sigs
            //
            uRandom = randomMT();
            uChecksum ^= uRandom;
            g_u64PawnSigSeeds[x][z] = (UINT64)uRandom << 32;
            
            uRandom = randomMT();
            uChecksum ^= uRandom;
            g_u64PawnSigSeeds[x][z] |= uRandom;
            g_u64PawnSigSeeds[x][z] &= ~1;
            
            //
            // Do the piece sigs
            // 
            for (y = 0; y < 7; y++)
            {
                uRandom = randomMT();
                uChecksum ^= uRandom;
                g_u64SigSeeds[x][y][z] = (UINT64)uRandom << 32;

                uRandom = randomMT();
                uChecksum ^= uRandom;
                g_u64SigSeeds[x][y][z] |= uRandom;
                g_u64SigSeeds[x][y][z] &= ~1;
            }
        } 
    }                        
    
    //
    // For all castle permissions
    // 
    for (x = 0; x < 16; x++)
    {
        uRandom = randomMT();
        uChecksum ^= uRandom;
        g_u64CastleSigSeeds[x] = (UINT64)uRandom << 32;

        uRandom = randomMT();
        uChecksum ^= uRandom;
        g_u64CastleSigSeeds[x] |= uRandom;
        g_u64CastleSigSeeds[x] &= ~1;
    }

    //
    // Make sure we got what we expected.
    //
    if (uChecksum != 0xac19ab2b)
    {
        UtilPanic(DETECTED_INCORRECT_INITIALIZATION,
                  NULL,
                  "signature system",
                  (void *)uChecksum,
                  (void *)0xac19ab2b, 
                  __FILE__, __LINE__);
    }
} 


UINT64 
ComputePawnSig(POSITION *pos)
/**

Routine description:

    Given a board, recompute the pawn signature from scratch.

Parameters:

    POSITION *pos

Return value:

    UINT64

**/
{
    register COOR c;
    PIECE p;
    UINT64 u64Sum = 0;

    FOREACH_SQUARE(c)
    {
        if (!IS_ON_BOARD(c)) continue;
        
        p = pos->rgSquare[c].pPiece;
        if (IS_PAWN(p))
        {
            u64Sum ^= g_u64PawnSigSeeds[c][GET_COLOR(p)];
        }
    }
    return(u64Sum);
}


UINT64 
ComputeSig(POSITION *pos)
/**

Routine description:

    Given a board, recompute the main signature from scratch.

Parameters:

    POSITION *pos

Return value:

    UINT64

**/
{
    register COOR c;
    UINT64 u64Sum = 0;
    PIECE p;
    
    FOREACH_SQUARE(c)
    {
        if (!IS_ON_BOARD(c)) continue;
        
        p = pos->rgSquare[c].pPiece;

        if (!IS_EMPTY(p) && !IS_PAWN(p))
        {
            ASSERT(IS_VALID_PIECE(p));
            u64Sum ^= g_u64SigSeeds[c][PIECE_TYPE(p)][GET_COLOR(p)];
        }
    }

    //
    // Update the signature to reflect the castle permissions and ep 
    // square at this point.
    //
    u64Sum ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
    u64Sum ^= g_u64EpSigSeeds[FILE(pos->cEpSquare)];
    
    //
    // Update the low order bit to reflect the side to move.
    //
    if (pos->uToMove == WHITE)
    {
        u64Sum |= 1;
    }
    else
    {
        u64Sum &= ~1;
    }
    ASSERT((u64Sum & 1) == pos->uToMove);

    return(u64Sum);
}
