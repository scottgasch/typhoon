/**

Copyright (c) Scott Gasch

Module Name:

    movesup.c

Abstract:

    Functions to support MakeMove and UnmakeMove along with some other
    miscellanious move support functions.

Author:

    Scott Gasch (scott.gasch@gmail.com) 13 May 2004

Revision History:

    $Id: movesup.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

COOR 
FasterExposesCheck(POSITION *pos,
                   COOR cRemove,
                   COOR cLocation)
/**

Routine description:

    This is just like ExposesCheck (see below) but it does not bother
    checking whether its possible to expose check with the piece at
    cRemove because we already know it is.

Parameters:

    POSITION *pos,
    COOR cRemove,
    COOR cLocation

Return value:

    COOR

**/
{
    register int iDelta;
    register COOR cBlockIndex;
    int iIndex;
    PIECE xPiece;
    ULONG uColor = GET_COLOR(pos->rgSquare[cLocation].pPiece);

    ASSERT(IS_KING(pos->rgSquare[cLocation].pPiece));
    iIndex = (int)cLocation - (int)cRemove;

    ASSERT(IS_ON_BOARD(cRemove));
    ASSERT(IS_ON_BOARD(cLocation));
    ASSERT(!IS_EMPTY(pos->rgSquare[cLocation].pPiece));
    ASSERT(0 != (CHECK_VECTOR_WITH_INDEX(iIndex, BLACK) & (1 << QUEEN)));
  
    iDelta = CHECK_DELTA_WITH_INDEX(iIndex);
    ASSERT(iDelta != 0);
    
    for (cBlockIndex = cRemove + iDelta;
         IS_ON_BOARD(cBlockIndex);
         cBlockIndex += iDelta)
    {
        ASSERT(cBlockIndex != cRemove);
        ASSERT(cBlockIndex != cLocation);
        xPiece = pos->rgSquare[cBlockIndex].pPiece;
        if (!IS_EMPTY(xPiece))
        {
            //
            // Is it an enemy?
            // 
            if (OPPOSITE_COLORS(xPiece, uColor))
            {
                //
                // Does it move the right way to attack?
                // 
                iIndex = (int)cBlockIndex - (int)cLocation;
                if (0 != (CHECK_VECTOR_WITH_INDEX(iIndex, WHITE) &
                          (1 << (PIECE_TYPE(xPiece)))))
                {
                    return(cBlockIndex);
                }
            }
            
            //
            // Either we found another friendly piece or an enemy piece
            // that was unable to reach us.  Either way we are safe.
            // 
            return(ILLEGAL_COOR);
        }
    }
    
    //
    // We fell off the board without seeing another piece.
    // 
    return(ILLEGAL_COOR);
}



COOR 
ExposesCheck(POSITION *pos, 
             COOR cRemove, 
             COOR cLocation)
/**

Routine description:

    Test if removing the piece at cRemove exposes the piece at
    cLocation to attack by the other side.  Note: there must be a
    piece at cLocation because the color of that piece is used to
    determine what side is "checking" and what side is "being
    checked".

Parameters:

    POSITION *pos : the board
    COOR cRemove : the square where a piece hypothetically removed from
    COOR cLocation : the square where the attackee is sitting

Return value:

    COOR : the location of an attacker piece or 0x88 (!IS_ON_BOARD) if 
           the removal of cRemove does not expose check.

**/
{
    register int iDelta;
    register COOR cBlockIndex;
    int iIndex;
    PIECE xPiece;
    
    ASSERT(IS_ON_BOARD(cRemove));
    ASSERT(IS_ON_BOARD(cLocation));
    ASSERT(!IS_EMPTY(pos->rgSquare[cLocation].pPiece));
    
    iIndex = (int)cLocation - (int)cRemove;
    
    //
    // If there is no way for a queen sitting at the square removed to
    // reach the square we are testing (i.e. the two squares are not
    // on the same rank, file, or diagonal) then there is no way
    // removing it can expose cLocation to check.
    //
    if (0 == (CHECK_VECTOR_WITH_INDEX(iIndex, BLACK) & (1 << QUEEN)))
    {
        return(ILLEGAL_COOR);
    }
    iDelta = CHECK_DELTA_WITH_INDEX(iIndex);
    for (cBlockIndex = cLocation + iDelta; 
         IS_ON_BOARD(cBlockIndex);
         cBlockIndex += iDelta)
    {
        if (cBlockIndex == cRemove) 
        {
            continue;
        }
        xPiece = pos->rgSquare[cBlockIndex].pPiece;
        if (!IS_EMPTY(xPiece))
        {
            //
            // Is it an enemy?
            // 
            if (OPPOSITE_COLORS(xPiece, pos->rgSquare[cLocation].pPiece))
            {
                //
                // Does it move the right way to attack?
                // 
                iIndex = (int)cBlockIndex - (int)cLocation;
                if (0 != (CHECK_VECTOR_WITH_INDEX(iIndex, GET_COLOR(xPiece)) &
                          (1 << (PIECE_TYPE(xPiece)))))
                {
                    return(cBlockIndex);
                }
            }
            
            //
            // Either we found another friendly piece or an enemy piece
            // that was unable to reach us.  Either way we are safe.
            // 
            return(ILLEGAL_COOR);
        }
    }
    
    //
    // We fell off the board without seeing another piece.
    // 
    return(ILLEGAL_COOR);
}


COOR 
ExposesCheckEp(POSITION *pos, 
               COOR cTest, 
               COOR cIgnore, 
               COOR cBlock, 
               COOR cKing) 
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR cTest : The square from whence the attack comes
    COOR cIgnore : Ignore this square, the pawn moved
    COOR cBlock : This square is where the pawn moved to and now blocks
    COOR cKing : The square under attack

Return value:

    COOR

**/
{
    register int iDelta;
    register COOR cBlockIndex;
    int iIndex;
    PIECE xPiece;

    ASSERT(IS_ON_BOARD(cTest));
    ASSERT(IS_ON_BOARD(cIgnore));
    ASSERT(IS_ON_BOARD(cBlock));
    ASSERT(IS_ON_BOARD(cKing));

    iIndex = (int)cKing - (int)cTest;

    //
    // If there is no way for a queen sitting at the square removed to
    // reach the square we are testing (i.e. the two squares are not
    // on the same rank, file, or diagonal) then no problemo.
    //
    if (0 == (CHECK_VECTOR_WITH_INDEX(iIndex, BLACK) & (1 << QUEEN))) 
    {
        return(ILLEGAL_COOR);
    }
  
    iDelta = CHECK_DELTA_WITH_INDEX(iIndex);
    for (cBlockIndex = cKing + iDelta; 
         IS_ON_BOARD(cBlockIndex);
         cBlockIndex += iDelta)
    {
        if (cBlockIndex == cTest) continue;
        if (cBlockIndex == cIgnore) continue;
        if (cBlockIndex == cBlock) return(ILLEGAL_COOR);

        xPiece = pos->rgSquare[cBlockIndex].pPiece;
        if (!IS_EMPTY(xPiece))
        {
            if (OPPOSITE_COLORS(xPiece, pos->rgSquare[cKing].pPiece))
            {
                //
                // Does it move the right way to attack?
                // 
                iIndex = (int)cBlockIndex - (int)cKing;
                if (0 != (CHECK_VECTOR_WITH_INDEX(iIndex, GET_COLOR(xPiece)) &
                          (1 << (PIECE_TYPE(xPiece)))))
                {
                    return(cBlockIndex);
                }
            }

            //
            // Either we found another friendly piece or an enemy piece
            // that was unable to reach us.  Either way we are safe.
            // 
            return(ILLEGAL_COOR);
        }
    }
    
    //
    // We fell off the board without seeing another piece.
    // 
    return(ILLEGAL_COOR);
}


FLAG 
IsAttacked(COOR cTest, POSITION *pos, ULONG uSide)
/**

Routine description:

    Determine whether the square cTest is attacked by the side uSide.

Parameters:

    COOR cTest : the square we want to determine if is under attack
    POSITION *pos : the board
    ULONG uSide : the side we want to see if is attacking cTest

Return value:

    FLAG : TRUE if uSide attacks cTest, FALSE otherwise

**/
{
    ULONG u;
    PIECE pPiece;
    COOR cLocation;
    COOR cBlockIndex;
    int iIndex;
    int iDelta;
    static int iPawnLoc[2] = { -17, +15 };
    static PIECE pPawn[2] = { BLACK_PAWN, WHITE_PAWN };
    
    ASSERT(IS_ON_BOARD(cTest));
    ASSERT(IS_VALID_COLOR(uSide));
    ASSERT((pos->uNonPawnCount[uSide][0] - 1) < 16);
    
    //
    // Begin at the end because the king is the 0th item in the list and 
    // we want to consider king moves last.
    // 
    for (u = pos->uNonPawnCount[uSide][0] - 1;
         u != (ULONG)-1;
         u--)
    {
        cLocation = pos->cNonPawns[uSide][u];
        ASSERT(IS_ON_BOARD(cLocation));
        pPiece = pos->rgSquare[cLocation].pPiece;
        ASSERT(!IS_EMPTY(pPiece));
        ASSERT(!IS_PAWN(pPiece));
        ASSERT(GET_COLOR(pPiece) == uSide);
        iIndex = (int)cLocation - (int)cTest;

        if (!(CHECK_VECTOR_WITH_INDEX(iIndex, uSide) &
              (1 << (PIECE_TYPE(pPiece)))))
        {
            continue;
        }
        
        //
        // These pieces do not slide, we don't need to look for
        // blockers.  If they can get there then there is nothing
        // we can do to stop them.
        //
        if (IS_KNIGHT(pPiece) || IS_KING(pPiece))
        {
            return(TRUE);
        }

        //
        // Well, here we have a sliding piece (bishop, rook or queen)
        // that is on the same line (file, rank or diagonal) as the
        // cTest.  We now have to see if the attacker (pPiece) is
        // blocked or is free to attack cTest.
        //
        iDelta = NEG_DELTA_WITH_INDEX(iIndex);
        for (cBlockIndex = cTest + iDelta;
             cBlockIndex != cLocation;
             cBlockIndex += iDelta)
        {
            if (!IS_EMPTY(pos->rgSquare[cBlockIndex].pPiece))
            {
                break;
            }
        }
        if (cBlockIndex == cLocation)
        {
            return(TRUE);
        }
    }

    //
    // Check the locations a pawn could attack cTest from.
    // 
    cLocation = cTest + iPawnLoc[uSide];
    if (IS_ON_BOARD(cLocation) && 
        (pos->rgSquare[cLocation].pPiece == pPawn[uSide]))
    {
        return(TRUE);
    }
    
    cLocation += 2;
    if (IS_ON_BOARD(cLocation) && 
        (pos->rgSquare[cLocation].pPiece == pPawn[uSide]))
    {
        return(TRUE);
    }
    return(FALSE);
}


FLAG 
InCheck(POSITION *pos, ULONG uSide)
/**

Routine description:

    Determine if a side is in check or not.

Parameters:

    POSITION *pos : the board
    ULONG uSide : the side we want to determine if is in check

Return value:

    FLAG : TRUE if side is in check, FALSE otherwise.

**/
{
    COOR cKingLoc;
#ifdef DEBUG
    PIECE pKing;
#endif

    ASSERT(IS_VALID_COLOR(uSide));
    
    cKingLoc = pos->cNonPawns[uSide][0];
#ifdef DEBUG
    pKing = pos->rgSquare[cKingLoc].pPiece;
    ASSERT(IS_KING(pKing));
    ASSERT(IS_VALID_COLOR(uSide));
    ASSERT(GET_COLOR(pKing) == uSide);
#endif
    return(IsAttacked(cKingLoc, pos, FLIP(uSide)));
}


static FLAG 
_SanityCheckPawnMove(POSITION *pos, MOVE mv)
/**

Routine description:

Parameters:

    POSITION *pos,
    MOVE mv

Return value:

    FLAG

**/
{
    PIECE p = mv.pMoved;
    COOR cFrom = mv.cFrom;
    COOR cTo = mv.cTo;

    ASSERT(mv.uMove);
    ASSERT(IS_PAWN(p));

    //
    // General sanity checking...
    // 
    if (!IS_ON_BOARD(cFrom) || !IS_ON_BOARD(cTo)) 
    {
        return(FALSE);
    }
    
    //
    // Sanity check promotions.
    // 
    if (mv.pPromoted)
    {
        if (GET_COLOR(p) == WHITE)
        {
            if ((RANK(cTo) != 8) && (RANK(cFrom) != 7)) return(FALSE);
        }
        else
        {
            if ((RANK(cTo) != 1) && (RANK(cFrom) != 2)) return(FALSE);
        }
        
        if (!IS_KNIGHT(mv.pPromoted) &&
            !IS_BISHOP(mv.pPromoted) &&
            !IS_ROOK(mv.pPromoted) &&
            !IS_QUEEN(mv.pPromoted))
        {
            return(FALSE);
        } 
    }
    
    //
    // Handle capture moves.
    // 
    if (mv.pCaptured)
    {
        //
        // Must be capturing something
        // 
        if (IS_SPECIAL_MOVE(mv) && (!mv.pPromoted))
        {
            //
            // en passant
            // 
            if (!IS_EMPTY(pos->rgSquare[cTo].pPiece)) return(FALSE);
        }
        else
        {
            //
            // normal capture
            // 
            if (IS_EMPTY(pos->rgSquare[cTo].pPiece)) return(FALSE);
            if (!OPPOSITE_COLORS(mv.pMoved, mv.pCaptured)) return(FALSE);
        }

        //
        // Must be in the capturing position.
        // 
        if (WHITE == GET_COLOR(p))
        {
            if (((cFrom - 17) != cTo) && ((cFrom - 15) != cTo)) return(FALSE);
        }
        else
        {
            if (((cFrom + 17) != cTo) && ((cFrom + 15) != cTo)) return(FALSE);
        }
        
        if ((IS_SPECIAL_MOVE(mv)) && 
            (!mv.pPromoted) &&
            (pos->cEpSquare != cTo)) return(FALSE);
        return(TRUE);
    }
    else
    {
        // 
        // If the pawn is not capturing, the TO square better be empty.
        // 
        if (!IS_EMPTY(pos->rgSquare[cTo].pPiece)) return(FALSE);
        
        // 
        // A non-capturing pawn can only push forward.  One or two squares
        // based on whether or not its the initial move.
        //
        if (WHITE == GET_COLOR(p))
        {
            if (cTo == cFrom - 16) return(TRUE);
            if ((cTo == cFrom - 32) && 
                (RANK2(cFrom)) &&
                (IS_EMPTY(pos->rgSquare[cFrom - 16].pPiece))) return(TRUE);
        }
        else
        {
            if (cTo == cFrom + 16) return(TRUE);
            if ((cTo == cFrom + 32) &&
                (RANK7(cFrom)) &&
                (IS_EMPTY(pos->rgSquare[cFrom + 16].pPiece))) return(TRUE);
        }
    }
    return(FALSE);
}


static FLAG
_SanityCheckPieceMove(POSITION *pos, MOVE mv)
/**

Routine description:

Parameters:

    POSITION *pos,
    MOVE mv

Return value:

    static

**/
{
    PIECE p = mv.pMoved;
    PIECE pPieceType = p >> 1;
    int iIndex;
    COOR cFrom = mv.cFrom;
    COOR cTo = mv.cTo;
    COOR cBlock;
    COOR cAttack;
    COOR cKing;
    int iDelta;
    
    ASSERT(!IS_PAWN(p));

    //
    // Handle castling
    // 
    if (IS_CASTLE(mv))
    {
        ASSERT(IS_KING(mv.pMoved));
        ASSERT(IS_SPECIAL_MOVE(mv));
        return(TRUE);
    }
    
    //
    // Make sure the piece moves in the way the move describes.
    // 
    iIndex = (int)cFrom - (int)cTo;
    if (0 == (CHECK_VECTOR_WITH_INDEX(iIndex, BLACK) &
              (1 << pPieceType)))
    {
        return(FALSE);
    }
    
    //
    // If we get here the piece can move in the way described by the
    // move but we still have to check to see if there are any other
    // pieces in the way if the piece moved is not a king or knight
    // (pawns handled in their own routine, see above).
    // 
    if ((pPieceType == QUEEN) ||
        (pPieceType == ROOK) ||
        (pPieceType == BISHOP))
    {
        iDelta = CHECK_DELTA_WITH_INDEX(iIndex);
        for (cBlock = cFrom + iDelta;
             cBlock != cTo; 
             cBlock += iDelta)
        {
            if (!IS_EMPTY(pos->rgSquare[cBlock].pPiece))
            {
                break;
            }
        }
        if (cBlock != cTo) return(FALSE);
    }
    
    //
    // Ok the piece can move the way they asked and there are no other
    // pieces in the way... but it cannot expose its own king to check
    // by doing so!
    //
    cKing = pos->cNonPawns[pos->uToMove][0];
    cAttack = ExposesCheck(pos, 
                           cFrom, 
                           cKing);
    if ((ILLEGAL_COOR != cAttack) && (cTo != cAttack))
    {
        //
        // Ok if we are here then removing this piece from the from square
        // does expose the king to check and the move does not capture the
        // checking piece.  But it's still ok as long as the TO location
        // still blocks the checking piece.
        // 
        iIndex = cAttack - cKing;
        iDelta = CHECK_DELTA_WITH_INDEX(iIndex);
        for (cBlock = cAttack + iDelta;
             cBlock != cKing;
             cBlock += iDelta)
        {
            if ((cBlock == cTo) ||
                !IS_EMPTY(pos->rgSquare[cBlock].pPiece)) break;
        }
        if (cBlock == cKing) return(FALSE);
    }

    //
    // Legal!
    // 
    return(TRUE);
}


FLAG 
SanityCheckMove(POSITION *pos, MOVE mv)
/**

Routine description:

    Sanity check a move -- Note: Not a strict chess legality checker!

Parameters:

    POSITION *pos,
    MOVE mv

Return value:

    FLAG

**/
{
    register PIECE p = mv.pMoved;
    
    if (0 == mv.uMove)
    {
        return(FALSE);
    }
    else if (mv.cTo == mv.cFrom)
    {
        return(FALSE);
    }
    else if (IS_EMPTY(pos->rgSquare[mv.cFrom].pPiece))
    {
        return(FALSE);
    }
    else if (GET_COLOR(mv.pMoved) != pos->uToMove)
    {
        return(FALSE);
    }
    else if ((!IS_EMPTY(pos->rgSquare[mv.cTo].pPiece)) &&
             (0 == mv.pCaptured))
    {
        return(FALSE);
    }
    
    if (!IS_SPECIAL_MOVE(mv))
    {
        if (mv.pCaptured)
        {
            if (IS_EMPTY(pos->rgSquare[mv.cTo].pPiece))
            {
                return(FALSE);
            }

            if (((pos->rgSquare[mv.cTo].pPiece) != mv.pCaptured) ||
                (!OPPOSITE_COLORS(mv.pCaptured, mv.pMoved)))
            {
                return(FALSE);
            }
        }
        
        if (!IS_EMPTY(pos->rgSquare[mv.cTo].pPiece) &&
            !mv.pCaptured)
        {
            return(FALSE);
        }
    }
    else
    {
        if (!IS_PAWN(p) && !IS_KING(p))
        {
            return(FALSE);
        }
    }
       
    if (IS_PAWN(p))
    {
        return(_SanityCheckPawnMove(pos, mv));
    }
    else
    {
        return(_SanityCheckPieceMove(pos, mv));
    }
}


FLAG 
LooksLikeFile(CHAR c)
/**

Routine description:

    Does some char look like a file (i.e. 'a' <= char <= 'h')

Parameters:

    CHAR c

Return value:

    FLAG

**/
{
    if ((toupper(c) >= 'A') &&
        (toupper(c) <= 'H'))
    {
        return(TRUE);
    }
    return(FALSE);
}

FLAG LooksLikeRank(CHAR c)
/**

Routine description:

    Does char look like a coor rank (i.e. '1' <= char <= '8')

Parameters:

    CHAR c

Return value:

    FLAG

**/
{
    if (((c - '0') >= 1) &&
        ((c - '0') <= 8))
    {
        return(TRUE);
    }
    return(FALSE);
}
                            

FLAG 
LooksLikeCoor(char *szData)
/**

Routine description:

    Do the first two chars in szData look like a file/rank?

Parameters:

    char *szData

Return value:

    FLAG

**/
{
    char f = szData[0];
    char r = szData[1];
    
    if (LooksLikeFile(f) &&
        LooksLikeRank(r))
    {
        return(TRUE);
    }
    return(FALSE);
}


CHAR *
StripMove(CHAR *szMove)
/**

Routine description:

    Strip decoration punctuation marks out of a move.

Parameters:

    CHAR *szMove

Return value:

    CHAR

**/
{
    CHAR cIgnore[] = "?!x+#-=\r\n";            // chars stripped from szMove
    static CHAR szStripped[10];
    ULONG y;
    CHAR *p;
    
    memset(szStripped, 0, sizeof(szStripped));
    p = szMove;
    y = 0;
    while (*p != '\0')
    {
        if (NULL == strchr(cIgnore, *p))
        {
            szStripped[y++] = *p;
            if (y >= 8) break;
        }
        p++;
    }
    return(szStripped);
}

ULONG LooksLikeMove(char *szData)
/**

Routine description:

    Does some char pointer point at some text that looks like a move?
    If so, does the move look like SAN or ICS style?

Parameters:

    char *szData

Return value:

    ULONG

**/
{
    CHAR *szStripped = StripMove(szData);
    ULONG u;
    static CHAR cPieces[] = { 'P', 'N', 'B', 'R', 'Q', 'K', '\0' };
    
    //
    // A (stripped) move must be at least two characters long even in
    // SAN.
    // 
    if ((strlen(szStripped) < 2) || (strlen(szStripped) > 7))
    {
        return(NOT_MOVE);
    }

    //
    // O-O, O-O-O
    //
    if (!STRNCMPI(szStripped, "OOO", 3) ||
        !STRNCMPI(szStripped, "OO", 2) ||
        !STRNCMPI(szStripped, "00", 2) ||      // duh
        !STRNCMPI(szStripped, "000", 3))
    {
        return(MOVE_SAN);
    }
    
    //
    // COOR COOR (d2d4)
    //
    if (LooksLikeCoor(szStripped) &&
        LooksLikeCoor(szStripped + 2))
    {
        return(MOVE_ICS);
    }
    
    //
    // COOR (d4)
    // 
    if (LooksLikeCoor(szStripped) &&
        strlen(szStripped) == 2)
    {
        return(MOVE_SAN);
    }
    
    //
    // RANK COOR
    // 
    if ((LooksLikeRank(szStripped[0]) ||
         LooksLikeFile(szStripped[0])) &&
        LooksLikeCoor(&szStripped[1]))
    {
        return(MOVE_SAN);
    }
    
    //
    // PIECE COOR COOR
    // PIECE COOR
    // PIECE FILE COOR
    // PIECE RANK COOR
    // 
    for (u = 0; u < ARRAY_LENGTH(cPieces); u++)
    {
        if (szStripped[0] == cPieces[u])    // must be uppercase!
        {
            if (LooksLikeCoor(&szStripped[1])) 
            {
                return(MOVE_SAN);
            }
            if (LooksLikeFile(szStripped[1]) &&
                LooksLikeCoor(&szStripped[2]))
            {
                return(MOVE_SAN);
            }
            if (LooksLikeRank(szStripped[1]) &&
                LooksLikeCoor(&szStripped[2]))
            {
                return(MOVE_SAN);
            }
        }
    }

    //
    // (COORD)=(PIECE)
    // 
    if ((strlen(szStripped) == 4) &&
        LooksLikeCoor(szStripped) &&
        szStripped[2] == '=')
    {
        if (NULL != strchr(cPieces, szStripped[3]))
        {
            return(MOVE_SAN);
        }
    }

    //
    // (COOR)(PIECE)
    //
    if ((strlen(szStripped) == 3) &&
        LooksLikeCoor(szStripped) &&
        (NULL != strchr(cPieces, szStripped[2])))
    {
        return(MOVE_SAN);
    }

    return(NOT_MOVE);
}


void FASTCALL 
SelectBestWithHistory(SEARCHER_THREAD_CONTEXT *ctx,
                      ULONG u)
/**

Routine description:

    Pick the best (i.e. move with the highest "score" assigned to it
    at generation time) that has not been played yet this ply and move
    it to the front of the move list to be played next.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG u

Return value:

    void FASTCALL

**/
{
    register ULONG v;
    register ULONG uEnd = ctx->sMoveStack.uEnd[ctx->uPly];
    register SCORE iBestVal;
    ULONG uLoc;
    SCORE iVal;
    MOVE mv;
    MOVE_STACK_MOVE_VALUE_FLAGS mvfTemp;
    
    ASSERT(ctx->sMoveStack.uBegin[ctx->uPly] <= uEnd);
    ASSERT(u >= ctx->sMoveStack.uBegin[ctx->uPly]);
    ASSERT(u < uEnd);
    
    //
    // Linear search from u..ctx->sMoveStack.uEnd[ctx->uPly] for the
    // move with the best value.
    //
    iBestVal = ctx->sMoveStack.mvf[u].iValue;
    mv = ctx->sMoveStack.mvf[u].mv;
    if (!IS_CAPTURE_OR_PROMOTION(mv))
    {
        iBestVal += g_HistoryCounters[mv.pMoved][mv.cTo];
    }
    uLoc = u;
    
    for (v = u + 1; v < uEnd; v++)
    {
        iVal = ctx->sMoveStack.mvf[v].iValue;
        mv = ctx->sMoveStack.mvf[v].mv;
        if (!IS_CAPTURE_OR_PROMOTION(mv))
        {
            iVal += g_HistoryCounters[mv.pMoved][mv.cTo];
        }
        if (iVal > iBestVal)
        {
            iBestVal = iVal;
            uLoc = v;
        }
    }

    //
    // Note: the if here slows down the code, just swap em.
    //
    mvfTemp = ctx->sMoveStack.mvf[u];
    ctx->sMoveStack.mvf[u] = ctx->sMoveStack.mvf[uLoc];
    ctx->sMoveStack.mvf[uLoc] = mvfTemp;
}


void FASTCALL 
SelectBestNoHistory(SEARCHER_THREAD_CONTEXT *ctx,
                    ULONG u)
/**

Routine description:

    Pick the best (i.e. move with the highest "score" assigned to it
    at generation time) that has not been played yet this ply and move
    it to the front of the move list to be played next.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG u

Return value:

    void FASTCALL

**/
{
    register ULONG v;
    register ULONG uEnd = ctx->sMoveStack.uEnd[ctx->uPly];
    register SCORE iBestVal;
    ULONG uLoc;
    SCORE iVal;
    MOVE_STACK_MOVE_VALUE_FLAGS mvfTemp;
    
    ASSERT(ctx->sMoveStack.uBegin[ctx->uPly] <= uEnd);
    ASSERT(u >= ctx->sMoveStack.uBegin[ctx->uPly]);
    ASSERT(u < uEnd);
    
    //
    // Linear search from u..ctx->sMoveStack.uEnd[ctx->uPly] for the
    // move with the best value.
    //
    iBestVal = ctx->sMoveStack.mvf[u].iValue;
    uLoc = u;
    for (v = u + 1; v < uEnd; v++)
    {
        iVal = ctx->sMoveStack.mvf[v].iValue;
        if (iVal > iBestVal)
        {
            iBestVal = iVal;
            uLoc = v;
        }
    }

    //
    // Note: the if here slows down the code, just swap em.
    //
    mvfTemp = ctx->sMoveStack.mvf[u];
    ctx->sMoveStack.mvf[u] = ctx->sMoveStack.mvf[uLoc];
    ctx->sMoveStack.mvf[uLoc] = mvfTemp;
}

void FASTCALL 
SelectMoveAtRoot(SEARCHER_THREAD_CONTEXT *ctx,
                 ULONG u)
/**

Routine description:

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG u

Return value:

    void FASTCALL

**/
{
    ULONG v = u;
    ULONG uEnd = ctx->sMoveStack.uEnd[ctx->uPly];
    SCORE iBestVal = -INFINITY;
    ULONG uLoc = v;
    SCORE iVal;
    MOVE_STACK_MOVE_VALUE_FLAGS mvfTemp;
    
    ASSERT(ctx->sMoveStack.uBegin[ctx->uPly] <= uEnd);
    ASSERT(u >= ctx->sMoveStack.uBegin[ctx->uPly]);
    ASSERT(u < uEnd);
    ASSERT(MOVE_COUNT(ctx, ctx->uPly) >= 1);
    
    //
    // Find the first move that we have not already searched.  It will
    // provide our initial iBestVal.
    //
    do
    {
        if (!(ctx->sMoveStack.mvf[v].bvFlags & MVF_MOVE_SEARCHED))
        {
            iBestVal = ctx->sMoveStack.mvf[v].iValue;
            uLoc = v;
            break;
        }
        v++;
    }
    while(v < uEnd);
    if (v >= uEnd) return;

    //
    // Search the rest of the moves that have not yet been tried by
    // RootSearch and find the one with the best value.
    //
    for (v = uLoc + 1; v < uEnd; v++)
    {
        if (!(ctx->sMoveStack.mvf[v].bvFlags & MVF_MOVE_SEARCHED))
        {
            iVal = ctx->sMoveStack.mvf[v].iValue;
            if (iVal > iBestVal)
            {
                iBestVal = iVal;
                uLoc = v;
            }
        }
    }

    //
    // Move the best move we found into position u.
    //
    mvfTemp = ctx->sMoveStack.mvf[u];
    ctx->sMoveStack.mvf[u] = ctx->sMoveStack.mvf[uLoc];
    ctx->sMoveStack.mvf[uLoc] = mvfTemp;
}


static UINT64 g_uPerftNodeCount;
static UINT64 g_uPerftTotalNodes;
static UINT64 g_uPerftGenerates;

void 
Perft(SEARCHER_THREAD_CONTEXT *ctx,
      ULONG uDepth)
{
    MOVE mv;
    ULONG u;
    ULONG uPly;

    g_uPerftTotalNodes += 1;
    ASSERT(uDepth < MAX_PLY_PER_SEARCH);
    if (uDepth == 0) 
    {
        g_uPerftNodeCount += 1;
        return;
    }
    
    mv.uMove = 0;
    g_uPerftGenerates += 1;
    GenerateMoves(ctx, mv, GENERATE_DONT_SCORE);
    
    uPly = ctx->uPly;
    for (u = ctx->sMoveStack.uBegin[uPly];
         u < ctx->sMoveStack.uEnd[uPly];
         u++)
    {
        mv = ctx->sMoveStack.mvf[u].mv;
        if (MakeMove(ctx, mv))
        {
            Perft(ctx, uDepth - 1);
            UnmakeMove(ctx, mv);
        }
    }
}

COMMAND(PerftCommand)
/**

Routine description:

    This function implements the 'perft' engine command.  I have no
    idea what 'perft' means but some people use this command to 
    benchmark the speed of the move generator code.  Note: the way
    this is implemented here does nothing whatsoever with the move
    scoring code (i.e. the SEE)

    Usage:
    
        perft <depth>

Parameters:

    The COMMAND macro hides four arguments from the input parser:

        CHAR *szInput : the full line of input
        ULONG argc    : number of argument chunks
        CHAR *argv[]  : array of ptrs to each argument chunk
        POSITION *pos : a POSITION pointer to operate on

Return value:

    void

**/
{
    LIGHTWEIGHT_SEARCHER_CONTEXT ctx;
    ULONG u;
    ULONG uDepth;
    double dBegin, dTime;
    double dNps;

    if (argc < 2) 
    {
        Trace("Usage: perft <required_depth>\n");
        return;
    }
    uDepth = atoi(argv[1]);
    if (uDepth >= MAX_DEPTH_PER_SEARCH) 
    {
        Trace("Error: invalid depth.\n");
        return;
    }

    InitializeLightweightSearcherContext(pos, &ctx);
    
    g_uPerftTotalNodes = g_uPerftGenerates = 0ULL;
    dBegin = SystemTimeStamp();
    for (u = 1; u <= uDepth; u++) 
    {
        g_uPerftNodeCount = 0ULL;
        Perft((SEARCHER_THREAD_CONTEXT *)&ctx, u);
        Trace("%u. %" COMPILER_LONGLONG_UNSIGNED_FORMAT " node%s, "
                  "%" COMPILER_LONGLONG_UNSIGNED_FORMAT " generate%s.\n",
              u, 
              g_uPerftNodeCount, 
              (g_uPerftNodeCount > 1) ? "s" : "",
              g_uPerftGenerates,
              (g_uPerftGenerates > 1) ? "s" : "");
    }
    dTime = SystemTimeStamp() - dBegin;
    dNps = (double)g_uPerftTotalNodes;
    dNps /= dTime;
    Trace("%" COMPILER_LONGLONG_UNSIGNED_FORMAT " total nodes, "
          "%" COMPILER_LONGLONG_UNSIGNED_FORMAT " total generates "
          "in %6.2f seconds.\n", 
          g_uPerftTotalNodes, 
          g_uPerftGenerates,
          dTime);
    Trace("That's approx %.0f moves/sec\n", dNps);
}
