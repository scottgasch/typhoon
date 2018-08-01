/**

Copyright (c) Scott Gasch

Module Name:

    ics.c

Abstract:

    Routines to convert internal move structs into ICS (d2d4 style)
    move strings and vice versa.

    See also san.c.

Author:

    Scott Gasch (scott.gasch@gmail.com) 16 May 2004

Revision History:

    $Id: ics.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"


CHAR *
MoveToIcs(MOVE mv)
/**

Routine description:

    Convert a MOVE into an ICS ("d2d4" style) string.  Note: not
    thread safe.

Parameters:

    MOVE mv : the move to convert

Return value:

    CHAR *

**/
{
    static char szTextMove[10];
    char *p = szTextMove;

    if (0 == mv.uMove)
    {
        return(NULL);
    }
    memset(szTextMove, 0, sizeof(szTextMove));

    ASSERT(IS_ON_BOARD(mv.cFrom));
    ASSERT(IS_ON_BOARD(mv.cTo));
    *p++ = FILE(mv.cFrom) + 'a';
    *p++ = RANK(mv.cFrom) + '0';
    *p++ = FILE(mv.cTo) + 'a';
    *p++ = RANK(mv.cTo) + '0';
    
    if (mv.pPromoted)
    {
        if (IS_QUEEN(mv.pPromoted))
        {
            *p++ = 'Q';
        }
        else if (IS_ROOK(mv.pPromoted))
        {
            *p++ = 'R';
        }
        else if (IS_BISHOP(mv.pPromoted))
        {
            *p++ = 'B';
        }
        else if (IS_KNIGHT(mv.pPromoted))
        {
            *p++ = 'N';
        }
        else
        {
            ASSERT(FALSE);
            return(NULL);
        }
    }
    *p = 0;
    return(szTextMove);
}             


MOVE 
ParseMoveIcs(CHAR *szInput, POSITION *pos)
/**

Routine description:

    Parse a move in ICS format (i.e. "d2d4") and create a MOVE struct.

Parameters:

    CHAR *szInput,
    POSITION *pos

Return value:

    MOVE

**/
{
    MOVE mv;
    COOR cFrom, cTo;
    PIECE pMoved, pCaptured, pProm;
    CHAR *szMoveText = StripMove(szInput);
    static CHAR *szPieces = "nbrq";
    CHAR *p;
    
    mv.uMove = 0;
    if (InCheck(pos, pos->uToMove))
    {
        mv.bvFlags |= MOVE_FLAG_ESCAPING_CHECK;
    }

    cFrom = FILE_RANK_TO_COOR((tolower(szMoveText[0]) - 'a'),
                              (tolower(szMoveText[1]) - '0'));
    cTo = FILE_RANK_TO_COOR((tolower(szMoveText[2]) - 'a'),
                            (tolower(szMoveText[3]) - '0'));
    
    //
    // check for castling moves in o-o style notation.
    //
    if ((!STRCMPI(szMoveText, "OO")) ||
        (!STRCMPI(szMoveText, "00")) ||
        (((E1 == cFrom) && (G1 == cTo)) && 
         (pos->rgSquare[E1].pPiece == WHITE_KING)) ||
        (((E8 == cFrom) && (G8 == cTo)) &&
         (pos->rgSquare[E8].pPiece == BLACK_KING)))
    {
        if (pos->uToMove == WHITE) 
        {
            if (pos->bvCastleInfo & CASTLE_WHITE_SHORT) 
            {
                ASSERT(pos->rgSquare[E1].pPiece == WHITE_KING);
                mv.uMove = 
                    MAKE_MOVE(E1, G1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL);
            }
        } 
        else // black to move
        {
            if (pos->bvCastleInfo & CASTLE_BLACK_SHORT) 
            {
                ASSERT(pos->rgSquare[E8].pPiece == BLACK_KING);
                mv.uMove = 
                    MAKE_MOVE(E8, G8, BLACK_KING, 0, 0, MOVE_FLAG_SPECIAL);
            }
        }
        goto done;
    } 
    else if ((!STRCMPI(szMoveText, "OOO")) ||
             (!STRCMPI(szMoveText, "000")) ||
             (((E1 == cFrom) && (C1 == cTo)) &&
              (pos->rgSquare[E1].pPiece == WHITE_KING)) ||
             (((E8 == cFrom) && (C8 == cTo)) &&
              (pos->rgSquare[E8].pPiece == BLACK_KING)))
    {
        if (pos->uToMove == WHITE) 
        {
            if (pos->bvCastleInfo & CASTLE_WHITE_LONG) 
            {
                ASSERT(pos->rgSquare[E1].pPiece == WHITE_KING);
                mv.uMove = 
                    MAKE_MOVE(E1, C1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL);
            }
        } 
        else // black to move
        {
            if (pos->bvCastleInfo & CASTLE_BLACK_LONG) 
            {
                ASSERT(pos->rgSquare[E8].pPiece == BLACK_KING);
                mv.uMove = 
                    MAKE_MOVE(E8, C8, BLACK_KING, 0, 0, MOVE_FLAG_SPECIAL);
            }
        }
        goto done;
    }
    
    if ((!IS_ON_BOARD(cFrom)) ||
        (!IS_ON_BOARD(cTo)))
    {
        goto illegal;
    }

    pMoved = pos->rgSquare[cFrom].pPiece;
    if ((IS_EMPTY(pMoved)) || (GET_COLOR(pMoved) != pos->uToMove))
    {
        goto illegal;
    }

    pCaptured = pos->rgSquare[cTo].pPiece;
    if ((pCaptured != 0) && (GET_COLOR(pCaptured) == pos->uToMove))
    {
        goto illegal;
    }
    
    mv.cFrom = cFrom;
    mv.cTo = cTo;
    mv.pMoved = pMoved;
    mv.pCaptured = pCaptured;

    if (IS_PAWN(pMoved))
    {
        //
        // Promotion
        //
        if ((szMoveText[4] != 0) && (szMoveText[4] != '\n')) 
        {
            if (IS_PAWN(mv.pMoved))
            {
                if (((BLACK == GET_COLOR(mv.pMoved)) && (RANK(cTo) != 1)) ||
                    ((WHITE == GET_COLOR(mv.pMoved)) && (RANK(cTo) != 8)))
                {
                    goto illegal;
                }
            }
            p = strchr(szPieces, tolower(szMoveText[4]));
            if (NULL == p)
            {
                goto illegal;
            }
            pProm = (PIECE)((BYTE *)p - (BYTE *)szPieces);
            pProm += 2;
            pProm <<= 1;
            pProm |= pos->uToMove;
            mv.pPromoted = pProm;
            mv.bvFlags = MOVE_FLAG_SPECIAL;
        }
        
        //
        // Handle en passant captures.
        //
        if (cTo == pos->cEpSquare)
        {
            mv.bvFlags = MOVE_FLAG_SPECIAL;
            if (WHITE == pos->uToMove)
            {
                mv.pCaptured = pos->rgSquare[cTo + 0x10].pPiece;
                ASSERT(mv.pCaptured == BLACK_PAWN);
            }
            else
            {
                mv.pCaptured = pos->rgSquare[cTo - 0x10].pPiece;
                ASSERT(mv.pCaptured == WHITE_PAWN);
            }
        }

        //
        // Handle double jumps.
        //
        if (abs(RANK(cTo) - RANK(cFrom)) > 1)
        {
            mv.bvFlags = MOVE_FLAG_SPECIAL;
        }
    }
    
 done:
    return(mv);
    
 illegal:
    mv.uMove = 0;
    return(mv);
}  
