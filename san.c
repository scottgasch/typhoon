/**

Copyright (c) Scott Gasch

Module Name:

    san.c 

Abstract:

    Code to support Standard Algebraic Notation parsing.  SAN is a
    format for expressing moves: e.g. "Nd4".

    See also ics.c.  

Author:

    Scott Gasch (scott.gasch@gmail.com) 13 May 2004

Revision History:

    $Id: san.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

CHAR g_cPieces[] = { 'P', 'N', 'B', 'R', 'Q', 'K', '\0' };

static MOVE 
_ParseCastleSan(CHAR *szCapturedMove, POSITION *pos)
/**

Routine description:

    Parse a SAN-format castle move string.

Parameters:

    CHAR *szCapturedMove,
    POSITION *pos

Return value:

    static MOVE

**/
{
    MOVE mv = {0};
    
    ASSERT(IS_VALID_COLOR(pos->uToMove));
    if ((!STRCMPI(szCapturedMove, "OO")) ||
        (!STRCMPI(szCapturedMove, "00")))
    {
        switch(pos->uToMove)
        {
            case WHITE:
                if ((pos->rgSquare[E1].pPiece != WHITE_KING) ||
                    (InCheck(pos, WHITE)))
                {
                    goto end;
                }

                if (pos->bvCastleInfo & CASTLE_WHITE_SHORT) 
                {
                    ASSERT(pos->rgSquare[H1].pPiece == WHITE_ROOK);
                    if (!IS_EMPTY(pos->rgSquare[F1].pPiece) ||
                        !IS_EMPTY(pos->rgSquare[G1].pPiece) ||
                        IsAttacked(F1, pos, BLACK) ||
                        IsAttacked(G1, pos, BLACK))
                    {
                        goto end;
                    }
                    mv.uMove = 
                        MAKE_MOVE(E1, G1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL);
                    goto end;
                }
                break;
                
            case BLACK:
                if ((pos->rgSquare[E8].pPiece != BLACK_KING) ||
                    (InCheck(pos, BLACK)))
                {
                    goto end;
                }

                if (pos->bvCastleInfo & CASTLE_BLACK_SHORT)
                {
                    ASSERT(pos->rgSquare[H8].pPiece == BLACK_ROOK);
                    if (!IS_EMPTY(pos->rgSquare[F8].pPiece) ||
                        !IS_EMPTY(pos->rgSquare[G8].pPiece) ||
                        IsAttacked(F8, pos, WHITE) ||
                        IsAttacked(G8, pos, WHITE))
                    {
                        goto end;
                    }
                    mv.uMove = 
                        MAKE_MOVE(E8, G8, BLACK_KING, 0, 0, MOVE_FLAG_SPECIAL);
                    goto end;
                }
                break;
#ifdef DEBUG
            default:
                UtilPanic(SHOULD_NOT_GET_HERE,
                          NULL, NULL, NULL, NULL, 
                          __FILE__, __LINE__);
                goto end;
#endif
        }
    }
    else if ((!STRCMPI(szCapturedMove, "OOO")) ||
             (!STRCMPI(szCapturedMove, "000")))
    {
        switch(pos->uToMove)
        {
            case WHITE:
                if ((pos->rgSquare[E1].pPiece != WHITE_KING) ||
                    (InCheck(pos, WHITE)))
                {
                    goto end;
                }

                if (pos->bvCastleInfo & CASTLE_WHITE_LONG) 
                {
                    ASSERT(pos->rgSquare[A1].pPiece == WHITE_ROOK);
                    if (!IS_EMPTY(pos->rgSquare[D1].pPiece) ||
                        !IS_EMPTY(pos->rgSquare[C1].pPiece) ||
                        !IS_EMPTY(pos->rgSquare[B1].pPiece) ||
                        IsAttacked(D1, pos, BLACK) ||
                        IsAttacked(C1, pos, BLACK))
                    {
                        goto end;
                    }
                    mv.uMove = 
                        MAKE_MOVE(E1, C1, WHITE_KING, 0, 0, MOVE_FLAG_SPECIAL);
                    goto end;
                }
                break;
                
            case BLACK:
                if ((pos->rgSquare[E8].pPiece != BLACK_KING) ||
                    (InCheck(pos, BLACK)))
                {
                    goto end;
                }

                if (pos->bvCastleInfo & CASTLE_BLACK_LONG) 
                {
                    ASSERT(pos->rgSquare[A8].pPiece == BLACK_ROOK);
                    if (!IS_EMPTY(pos->rgSquare[D8].pPiece) ||
                        !IS_EMPTY(pos->rgSquare[C8].pPiece) ||
                        !IS_EMPTY(pos->rgSquare[B8].pPiece) ||
                        IsAttacked(D8, pos, WHITE) ||
                        IsAttacked(C8, pos, WHITE))
                    {
                        goto end;
                    }
                    mv.uMove = 
                        MAKE_MOVE(E8, C8, BLACK_KING, 0, 0, MOVE_FLAG_SPECIAL);
                    goto end;
                }
                break;
                
#ifdef DEBUG
            default:
                UtilPanic(SHOULD_NOT_GET_HERE,
                          NULL, NULL, NULL, NULL, 
                          __FILE__, __LINE__);
                goto end;
#endif
        }
    }
    
 end:
    return(mv);
}

static MOVE 
_ParseNormalSan(CHAR *szCapturedMove, POSITION *pos)
/**

Routine description:

    Parse a "normal" SAN move.  Note: these SAN string should begin
    with a piece code (or have pawn implied).  This parser relies on
    the fact that the piece code is uppercase.  This is in order to
    distinguish between file b and Bishop.

Parameters:

    CHAR *szCapturedMove,
    POSITION *pos

Return value:

    static MOVE

**/
{
    static LIGHTWEIGHT_SEARCHER_CONTEXT ctx;
    CHAR *p, *q;
    PIECE pPieceType = PAWN;
    ULONG u;
    COOR cFileRestr = 99;
    COOR cRankRestr = 99;
    COOR cFrom = ILLEGAL_COOR;
    COOR cTo = ILLEGAL_COOR;
    PIECE pMoved;
    PIECE pCaptured;
    PIECE pPromoted = 0;
    FLAG fSpecial = FALSE;
    MOVE mv;
    ULONG uNumMatches = 0;
    
    p = szCapturedMove;
   
    //
    // Is the first thing a piece code?  If not PAWN is implied.
    //
    if (isupper(*p))
    {
        if (NULL != (q = strchr(g_cPieces, *p)))
        {
            pPieceType = (PIECE)((BYTE *)q - (BYTE *)g_cPieces);
            pPieceType++;
            ASSERT(IS_VALID_PIECE(pPieceType));
        }
        p++;
    }
    
    //
    // Is the next thing a board coor?  If not it should be a rank or
    // file restrictor.
    // 
    while ((*p != '\0') && 
           (FALSE == LooksLikeCoor(p)))
    {
        if (LooksLikeFile(*p))
        {
            cFileRestr = toupper(*p) - 'A';
        }
        else if (LooksLikeRank(*p))
        {
            cRankRestr = *p - '0';
        }
        else
        {
            goto illegal;
        }
        p++;
    }
    if (*p == '\0') goto illegal;
    
    //
    // We got the piece code taken care of and any rank/file
    // restrictions done with.  See if we just have two coors left.
    // 
    if ((strlen(p) > 3) &&
        LooksLikeCoor(p) && 
        LooksLikeCoor(p + 2))
    {
        cFrom = FILE_RANK_TO_COOR((toupper(*p) - 'A'),
                                  *(p+1) - '0');
        cTo = FILE_RANK_TO_COOR((toupper(*(p+2)) - 'A'),
                                *(p+3) - '0');
        if (!IS_ON_BOARD(cFrom) || !IS_ON_BOARD(cTo))
        {
            goto illegal;
        }
        p += 4;
        goto found_coors;
    }
    else if ((strlen(p) > 1) &&
             LooksLikeCoor(p))
    {
        cTo = FILE_RANK_TO_COOR(toupper(*p) - 'A',
                                *(p+1) - '0');
        if (!IS_ON_BOARD(cTo))
        {
            goto illegal;
        }
        p += 2;
    }
    
    //
    // We need to find out what piece can move to cTo
    //
    if (!IS_ON_BOARD(cFrom))
    {
        if (!IS_ON_BOARD(cTo))
        {
            goto illegal;
        }

        //
        // Handle promotion
        //
        switch (*(p))
        {
            case 'N':
                pPromoted = BLACK_KNIGHT | pos->uToMove;
                fSpecial = MOVE_FLAG_SPECIAL;
                break;
            case 'B':
                pPromoted = BLACK_BISHOP | pos->uToMove;
                fSpecial = MOVE_FLAG_SPECIAL;
                break;
            case 'R':
                pPromoted = BLACK_ROOK | pos->uToMove;
                fSpecial = MOVE_FLAG_SPECIAL;
                break;
            case 'Q':
                pPromoted = BLACK_QUEEN | pos->uToMove;
                fSpecial = MOVE_FLAG_SPECIAL;
                break;
            default:
                break;
        }
        
        InitializeLightweightSearcherContext(pos, &ctx);
        mv.uMove = 0;
        GenerateMoves((SEARCHER_THREAD_CONTEXT *)&ctx,
                      mv,
                      (InCheck(pos, pos->uToMove) ? GENERATE_ESCAPES :
                                                    GENERATE_ALL_MOVES));
        for (u = ctx.sMoveStack.uBegin[0];
             u < ctx.sMoveStack.uEnd[0];
             u++)
        {
            mv = ctx.sMoveStack.mvf[u].mv;
            pMoved = mv.pMoved;
            if (PIECE_TYPE(pMoved) == pPieceType)
            {
                if ((99 != cFileRestr) && 
                    (FILE(mv.cFrom) != cFileRestr)) continue;
                if ((99 != cRankRestr) && 
                    (RANK(mv.cFrom) != cRankRestr)) continue;

                if ((mv.cTo == cTo) &&
                    (mv.pPromoted == pPromoted))
                {
                    if (TRUE == MakeMove((SEARCHER_THREAD_CONTEXT *)&ctx, mv))
                    {
                        UnmakeMove((SEARCHER_THREAD_CONTEXT *)&ctx, mv);
                        cFrom = mv.cFrom;
                        cTo = mv.cTo;
                        uNumMatches++;
                    }
                }
            }
        }
        
        //
        // Note: 4 moves match for promotions
        //
        if (uNumMatches == 1)
        {
            goto found_coors;
        }
    }
    goto illegal;

 found_coors:
    pMoved = pos->rgSquare[cFrom].pPiece;
    if (IS_EMPTY(pMoved))
    {
        goto illegal;
    }
    pCaptured = pos->rgSquare[cTo].pPiece;
    if (!IS_EMPTY(pCaptured) &&
        !OPPOSITE_COLORS(pMoved, pCaptured))
    {
        goto illegal;
    }

    //
    // Handle enpassant
    // 
    if ((cTo == pos->cEpSquare) && (IS_PAWN(pMoved)))
    {
        if (WHITE == pos->uToMove)
        {
            pCaptured = pos->rgSquare[cTo + 16].pPiece;
            if (pCaptured != BLACK_PAWN) goto illegal;
        }
        else
        {
            pCaptured = pos->rgSquare[cTo - 16].pPiece;
            if (pCaptured != WHITE_PAWN) goto illegal;
        }
        fSpecial = MOVE_FLAG_SPECIAL;
    }
    
    //
    // Handle double jump
    // 
    if (IS_PAWN(pMoved) && DISTANCE(cFrom, cTo) > 1)
    {
        fSpecial = MOVE_FLAG_SPECIAL;
    }
    
    mv.uMove = MAKE_MOVE(cFrom, cTo, pMoved, pCaptured, pPromoted, fSpecial);
    if (FALSE == SanityCheckMove(pos, mv)) 
    {
        goto illegal;
    }
    return(mv);
    
 illegal:
    mv.uMove = 0;
    return(mv);
}


MOVE 
ParseMoveSan(CHAR *szInput, 
             POSITION *pos)
/**

Routine description:

    Given a move string in SAN format and a current board position,
    parse the move and make it into a valid MOVE structure.

Parameters:

    CHAR *szInput,
    POSITION *pos

Return value:

    MOVE

**/
{
    CHAR szCapturedMove[SMALL_STRING_LEN_CHAR];
    MOVE mv;

    mv.uMove = 0;
    memset(szCapturedMove, 0, sizeof(szCapturedMove));
    strncpy(szCapturedMove, StripMove(szInput), SMALL_STRING_LEN_CHAR - 1);

    if (MOVE_SAN != LooksLikeMove(szCapturedMove)) 
    {
        return(mv);                           // fail
    }

    mv = _ParseCastleSan(szCapturedMove, pos);
    if (mv.uMove != 0)
    {
        goto end;
    }
    
    mv = _ParseNormalSan(szCapturedMove, pos);
    if (mv.uMove != 0)
    {
        goto end;
    }
    
    return(mv);                               // fail

 end:
    if (InCheck(pos, pos->uToMove))
    {
        mv.bvFlags |= MOVE_FLAG_ESCAPING_CHECK;
    }
    return(mv);
}


CHAR *
MoveToSan(MOVE mv, POSITION *pos)
/**

Routine description:

    Given a move and a current board position, convert the MOVE
    structure into a SAN text string.

Parameters:

    MOVE mv,
    POSITION *pos

Return value:

    CHAR

**/
{
    static char szTextMove[10];
    char *p;
    ULONG u = 0;
    MOVE x;
    
    ASSERT(IS_ON_BOARD(mv.cFrom));
    ASSERT(IS_ON_BOARD(mv.cTo));
    if (0 == mv.uMove)
    {
        return(NULL);
    }
    
    do
    {
        p = szTextMove;
        memset(szTextMove, 0, sizeof(szTextMove));

        //
        // Handle castles.
        //
        if (IS_KING(mv.pMoved) && (IS_SPECIAL_MOVE(mv)))
        {
            if ((mv.cTo == G1) || (mv.cTo == G8))
            {
                strcpy(szTextMove, "O-O");
                if (IS_CHECKING_MOVE(mv))
                {
                    *p++ = '+';
                }
                goto end;
            }
            else if ((mv.cTo == C1) || (mv.cTo == C8))
            {
                strcpy(szTextMove, "O-O-O");
                if (IS_CHECKING_MOVE(mv))
                {
                    *p++ = '+';
                }
                goto end;
            }
            ASSERT(FALSE);
            strcpy(szTextMove, "ILLEGAL");
            goto end;
        }
    
        if (!IS_PAWN(mv.pMoved))
        {
            *p++ = g_cPieces[(PIECE_TYPE(mv.pMoved)) - 1];
            if (u == 1)
            {
                *p++ = FILE(mv.cFrom) + 'a';
            }
            else if (u == 2)
            {
                *p++ = RANK(mv.cFrom) + '0';
            }
            else if (u == 3)
            {
                *p++ = FILE(mv.cFrom) + 'a';
                *p++ = RANK(mv.cFrom) + '0';
            }
        }                
        else
        {
            if (u > 0)
            {
                *p++ = FILE(mv.cFrom) + 'a';
            }
        }
        
        if (mv.pCaptured)
        {
            if (IS_PAWN(mv.pMoved))
            {
                *p++ = FILE(mv.cFrom) + 'a';
            }
            *p++ = 'x';
        }
        *p++ = FILE(mv.cTo) + 'a';
        *p++ = RANK(mv.cTo) + '0';

        if (mv.pPromoted)
        {
            *p++ = '=';
            *p++ = g_cPieces[(PIECE_TYPE(mv.pPromoted)) - 1];
        }
        
        u++;
        if (u >= 4)
        {
            strcpy(szTextMove, "ILLEGAL");
            goto end;
        }

        if (IS_CHECKING_MOVE(mv))
        {
            *p++ = '+';
        }
        x = ParseMoveSan(szTextMove, pos);
    }
    while ((mv.cTo != x.cTo) ||
           (mv.cFrom != x.cFrom));

end:
    return(szTextMove);    
}
