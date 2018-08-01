/**

Copyright (c) Scott Gasch

Module Name:

    testgenerate.c

Abstract:

    Test the move generator.

Author:

    Scott Gasch (scott.gasch@gmail.com) 8 April 2005

Revision History:

    $Id: testsup.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

FLAG
IsBoardLegal(POSITION *pos)
{
    COOR c;
    
    for (c = A1; c <= H1; c++)
    {
        if (!IS_EMPTY(pos->rgSquare[c].pPiece))
        {
            if (IS_PAWN(pos->rgSquare[c].pPiece))
            {
                return(FALSE);
            }
        }
    }
    
    for (c = A8; c <= H8; c++)
    {
        if (!IS_EMPTY(pos->rgSquare[c].pPiece))
        {
            if (IS_PAWN(pos->rgSquare[c].pPiece))
            {
                return(FALSE);
            }
        }
    }
    
    if (InCheck(pos, WHITE) &&
        InCheck(pos, BLACK))
    {
        return(FALSE);
    }

    if (InCheck(pos, FLIP(pos->uToMove)))
    {
        return(FALSE);
    }
    return(TRUE);
}


void
GenerateRandomLegalPosition(POSITION *pos)
{
    COOR c, c1;
    ULONG x, uIndex;
    ULONG uColor;
    PIECE p;
    ULONG u;

    do
    {
        //
        // Clear the board
        //
        memset(pos, 0, sizeof(POSITION));
        for (u = 0; u < ARRAY_SIZE(pos->cPawns[WHITE]); u++)
        {
            pos->cPawns[WHITE][u] = pos->cPawns[BLACK][u] = ILLEGAL_COOR;
        }
        
        for (u = 0; u < ARRAY_SIZE(pos->cNonPawns[WHITE]); u++)
        {
            pos->cNonPawns[WHITE][u] = pos->cNonPawns[BLACK][u] = ILLEGAL_COOR;
        }
        
        //
        // Place both kings legally
        //
        c = RANDOM_COOR;
        ASSERT(IS_ON_BOARD(c));
        pos->rgSquare[c].pPiece = WHITE_KING;
        ASSERT(pos->cNonPawns[WHITE][0] == ILLEGAL_COOR);
        pos->cNonPawns[WHITE][0] = c;
        pos->uNonPawnCount[WHITE][KING] = 1;
        pos->uNonPawnCount[WHITE][0] = 1;
        pos->uNonPawnMaterial[WHITE] = VALUE_KING;
        
        do
        {
            c1 = RANDOM_COOR;
        }
        while(DISTANCE(c, c1) < 2);
        ASSERT(IS_ON_BOARD(c1));
        pos->rgSquare[c1].pPiece = BLACK_KING;
        ASSERT(pos->cNonPawns[BLACK][0] == ILLEGAL_COOR);
        pos->cNonPawns[BLACK][0] = c1;
        pos->uNonPawnCount[BLACK][KING] = 1;
        pos->uNonPawnCount[BLACK][0] = 1;
        pos->uNonPawnMaterial[BLACK] = VALUE_KING;
        
        //
        // Place the rest of the armies
        //
        FOREACH_COLOR(uColor)
        {
            ASSERT(IS_VALID_COLOR(uColor));

            for (x = 0;
                 x < (ULONG)(rand() % 15 + 1);
                 x++)
            {
                //
                // Pick a random empty square
                //
                do
                {
                    c = RANDOM_COOR;
                }
                while((pos->rgSquare[c].pPiece) || (RANK1(c) || RANK8(c)));

                do
                {
                    p = RANDOM_PIECE;
                    if (WHITE == uColor)
                    {
                        p |= WHITE;
                    }
                    else
                    {
                        p &= ~WHITE;
                    }
                    ASSERT(GET_COLOR(p) == uColor);
                    
                    if (IS_PAWN(p) &&
                        (pos->uPawnCount[uColor] < 8) &&
                        (!RANK1(c)) && 
                        (!RANK8(c)))
                    {
                        //
                        // The pawn list is not ordered at all, add
                        // the new pawn's location to the list and
                        // point from the pawn to the list.
                        //
                        pos->cPawns[uColor][pos->uPawnCount[uColor]] = c;
                        pos->rgSquare[c].uIndex = pos->uPawnCount[uColor];
                        pos->rgSquare[c].pPiece = p;
                        pos->uPawnCount[uColor]++;
                        pos->uPawnMaterial[uColor] += VALUE_PAWN;
                        break;
                    }
                    else if (!IS_KING(p) &&
                             (IS_VALID_PIECE(p)))
                    {
                        uIndex = pos->uNonPawnCount[uColor][0];
                        pos->cNonPawns[uColor][uIndex] = c;
                        pos->uNonPawnCount[uColor][0]++;
                        pos->uNonPawnCount[uColor][PIECE_TYPE(p)]++;
                        pos->rgSquare[c].pPiece = p;
                        pos->rgSquare[c].uIndex = uIndex;
                        pos->uNonPawnMaterial[uColor] += PIECE_VALUE(p);
                        if (IS_BISHOP(p))
                        {
                            if (IS_WHITE_SQUARE_COOR(c))
                            {
                                pos->uWhiteSqBishopCount[uColor]++;
                            }
                        }
                        break;
                    }
                }
                while(1);

            } // next piece

        } // next color
        ASSERT(pos->uNonPawnCount[WHITE][KING] == 1);
        ASSERT(pos->uNonPawnCount[BLACK][KING] == 1);
        //pos->uNonPawnCount[WHITE][0]--;
        //pos->uNonPawnCount[BLACK][0]--;

        //
        // Do the rest of the position
        //
        pos->iMaterialBalance[WHITE] =
            ((SCORE)(pos->uNonPawnMaterial[WHITE] +
                     pos->uPawnMaterial[WHITE]) -
             (SCORE)(pos->uNonPawnMaterial[BLACK] +
                     pos->uPawnMaterial[BLACK]));
        pos->iMaterialBalance[BLACK] = -pos->iMaterialBalance[WHITE];
        pos->uToMove = RANDOM_COLOR;
        pos->uFifty = rand() % 100;
        if (pos->rgSquare[E1].pPiece == WHITE_KING)
        {
            if (pos->rgSquare[A1].pPiece == WHITE_ROOK)
            {
                pos->bvCastleInfo |= CASTLE_WHITE_SHORT;
            }
            if (pos->rgSquare[H1].pPiece == WHITE_ROOK)
            {
                pos->bvCastleInfo |= CASTLE_WHITE_LONG;
            }
        }
        if (pos->rgSquare[E8].pPiece == BLACK_KING)
        {
            if (pos->rgSquare[A8].pPiece == BLACK_ROOK)
            {
                pos->bvCastleInfo |= CASTLE_BLACK_SHORT;
            }
            if (pos->rgSquare[H8].pPiece == BLACK_ROOK)
            {
                pos->bvCastleInfo |= CASTLE_BLACK_LONG;
            }
        }
        pos->u64NonPawnSig = ComputeSig(pos);
        pos->u64PawnSig = ComputePawnSig(pos);

        //
        // See if it's legal
        //
        if (VerifyPositionConsistency(pos, TRUE) &&
            !InCheck(pos, FLIP(pos->uToMove)))
        {
            break;
        }
    }
    while(1);
}


#define SYM_SQ(c)                  ((7 - (((c) & 0xF0) >> 4)) << 4) \
                                       | (7 - ((c) & 0x0F))
void
GenerateRandomLegalSymetricPosition(POSITION *pos)
{
    CHAR szFen[256];
    CHAR *p;
    ULONG uPieceCount[7];
    COOR c, xc;
    PIECE pPiece = KING;
    PIECE pBoard[128];
    ULONG x, y;

    do
    {
        memset(pBoard, 0, sizeof(pBoard));
        memset(uPieceCount, 0, sizeof(uPieceCount));

        y = (ULONG)((rand() % 15) + 1);
        for (x = 0;
             x < y;
             x++)
        {
            //
            // Pick a piece type
            //
            do
            {
                pPiece = (rand() % 5) + 1;
                switch(pPiece)
                {
                    case PAWN:
                        if (uPieceCount[PAWN] < 8)
                        {
                            uPieceCount[PAWN]++;
                        }
                        else
                        {
                            pPiece = 0;
                        }
                        break;
                    case KNIGHT:
                    case BISHOP:
                    case ROOK:
                    case QUEEN:
                        if (uPieceCount[pPiece] < 2)
                        {
                            uPieceCount[pPiece]++;
                        }
                        else
                        {
                            if (uPieceCount[PAWN] < 8)
                            {
                                uPieceCount[PAWN]++;
                                pPiece = PAWN;
                            }
                        }
                        break;
                    default:
                        pPiece = 0;
                        break;
                }
            }
            while(pPiece == 0);
            
            //
            // Pick a location
            //
            do
            {
                c = RANDOM_COOR;
            }
            while (!IS_ON_BOARD(c) ||
                   (pBoard[c] != 0) ||
                   ((pPiece == PAWN) && ((RANK(c) == 1) || (RANK(c) == 8))));
            xc = SYM_SQ(c);
            ASSERT(IS_ON_BOARD(xc));
            ASSERT(IS_EMPTY(pBoard[xc]));
            
            pBoard[c] = (pPiece << 1) | WHITE;
            pBoard[xc] = (pPiece << 1) | BLACK;
            pPiece = 0;
        }
        
        do
        {
            c = RANDOM_COOR;
        }
        while (!IS_ON_BOARD(c) ||
               (pBoard[c] != 0));
        xc = SYM_SQ(c);
        ASSERT(IS_ON_BOARD(xc));
        ASSERT(IS_EMPTY(pBoard[xc]));
        pBoard[c] = WHITE_KING;
        pBoard[xc] = BLACK_KING;
        
        memset(szFen, 0, sizeof(szFen));
        p = szFen;
        for (x = 0; x < 120; x++)
        {
            if ((x % 15 == 0) && 
                (x != 120) && 
                (x != 0)) *p++ = '/';
            
            if (!IS_ON_BOARD(x)) continue;
            if (0 == pBoard[x]) *p++ = '1';
            else *p++ = *(PieceAbbrev(pBoard[x]) + 1);
        }
        strcat(p, " w - - 0 0");
        FenToPosition(pos, szFen);
    }
    while(!IsBoardLegal(pos));
}
