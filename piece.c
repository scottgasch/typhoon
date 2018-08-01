/**

Copyright (c) Scott Gasch

Module Name:

    piece.c

Abstract:

    Chess piece related code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 14 Apr 2004

Revision History:

    $Id: piece.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

PIECE_DATA g_PieceData[] =
{
    // piece value  piece value / 100     invert value   piece name
    { 0,            0,                    0,             "NONE" },
    { VALUE_PAWN,   (VALUE_PAWN / 100),   INVERT_PAWN,   "PAWN" },
    { VALUE_KNIGHT, (VALUE_KNIGHT / 100), INVERT_KNIGHT, "KNIGHT" },
    { VALUE_BISHOP, (VALUE_BISHOP / 100), INVERT_BISHOP, "BISHOP" },
    { VALUE_ROOK,   (VALUE_ROOK / 100),   INVERT_ROOK,   "ROOK" },
    { VALUE_QUEEN,  (VALUE_QUEEN / 100),  INVERT_QUEEN,  "QUEEN" },
    { VALUE_KING,   100,                  INVERT_KING,   "KING" },
    { 0,            0,                    0,             "NONE" }
};


CHAR *
PieceAbbrev(PIECE p) 
{
    static char _PieceAbbrev[] =
    {
        '-', 'p', 'n', 'b', 'r', 'q', 'k', '\0'
    };
    static char buf[3];

    if (!IS_VALID_PIECE(p))
    {
        buf[0] = buf[1] = '?';
        buf[2] = 0;
        return(buf);
    }

    buf[1] = _PieceAbbrev[PIECE_TYPE(p)];
    if (PIECE_COLOR(p) == WHITE) 
    {
        buf[0] = '.'; 
        buf[1] = (char)toupper(buf[1]);
    }
    else 
    {
        buf[0] = '*';
    }
    buf[2] = 0;
    
    return(buf);
}

ULONG
PieceValueOver100(PIECE p)
{
    return(PIECE_VALUE_OVER_100(p));
}

ULONG
PieceValue(PIECE p)
{
    return(PIECE_VALUE(p));
}

ULONG
PieceInvertedValue(PIECE p)
{
    return(INVERTED_PIECE_VALUE(p));
}
