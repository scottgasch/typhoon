/**

Copyright (c) Scott Gasch

Module Name:

    recogn.c

Abstract:

    This code is based on an article by Ernst A. Heinz: "Efficient
    Interior-Node Recognition" * ICCA Journal Volume 21, No. 3, pp
    156-167 (also "Scalable Search in Computer Chess" pp 65-81).  This
    code also borrows ideas from Thorsten Greiner's AMY chess program.
    
Author:

    Scott Gasch (scott.gasch@gmail.com) 16 Oct 2005

Revision History:

**/

#include "chess.h"

extern ULONG g_uIterateDepth;
static COOR QUEENING_SQUARE_BY_COLOR_FILE[2][8] = 
{ 
    { A1, B1, C1, D1, E1, F1, G1, H1 },
    { A8, B8, C8, D8, E8, F8, G8, H8 } 
};

#define RECOGN_INDEX(w, b) \
    (((b) | (w)) + (32 * ((w) && (b))))

typedef ULONG RECOGNIZER(SEARCHER_THREAD_CONTEXT *ctx, SCORE *piScore);

static RECOGNIZER *g_pRecognizers[64];
static BITV g_bvRecognizerAvailable[32];

static ULONG 
_MakeMaterialSig(IN FLAG fPawn, 
                 IN FLAG fKnight,
                 IN FLAG fBishop,
                 IN FLAG fRook, 
                 IN FLAG fQueen) 
/**

Routine description:

    Make a material signature ULONG out of flags representing the
    presence of pawns, knights, bishops, rooks, queens on the board.

Parameters:

    FLAG fPawn,
    FLAG fKnight,
    FLAG fBishop,
    FLAG fRook,
    FLAG fQueen

Return value:

    static ULONG

**/
{
    ULONG x;
    
    ASSERT(IS_VALID_FLAG(fPawn));
    ASSERT(IS_VALID_FLAG(fKnight));
    ASSERT(IS_VALID_FLAG(fBishop));
    ASSERT(IS_VALID_FLAG(fRook));
    ASSERT(IS_VALID_FLAG(fQueen));

    x = fPawn | (fKnight << 1) | (fBishop << 2) | (fRook << 3) | (fQueen << 4);
    
    ASSERT((0 <= x) && (x <= 31));
    return(x);
}


#ifdef DEBUG


static FLAG
_TablebasesSaySideWins(IN SEARCHER_THREAD_CONTEXT *ctx, 
                       IN ULONG uSide)
{
    SCORE iScore;
    if (TRUE == ProbeEGTB(ctx, &iScore))
    {
        if (ctx->sPosition.uToMove == uSide) 
        {
            return iScore > 0;
        } 
        else 
        {
            return iScore < 0;
        }
    } 
    return TRUE;
}

static FLAG
_TablebasesSayDraw(IN SEARCHER_THREAD_CONTEXT *ctx) 
{
    SCORE iScore;
    if (TRUE == ProbeEGTB(ctx, &iScore)) 
    {
        return iScore == 0;
    }
    return TRUE;
}

static FLAG
_TablebasesSayDrawOrWin(IN SEARCHER_THREAD_CONTEXT *ctx, 
                        IN ULONG uSide) 
{
    SCORE iScore;
    if (TRUE == ProbeEGTB(ctx, &iScore)) 
    {
        return ((iScore == 0) || 
                ((iScore > 0) && (ctx->sPosition.uToMove == uSide)) ||
                ((iScore < 0) && (ctx->sPosition.uToMove != uSide)));
    }
    return TRUE;
}


static FLAG 
_SanityCheckRecognizers(IN SEARCHER_THREAD_CONTEXT *ctx, 
                        IN SCORE iScore, 
                        IN ULONG uVal) {
    ULONG uToMove = ctx->sPosition.uToMove;
    switch(uVal) {
        case UNRECOGNIZED:
            return TRUE;
        case RECOGN_EXACT:
            if (iScore == 0) {
                return _TablebasesSayDraw(ctx);
            } else if (iScore > 0) {
                return _TablebasesSaySideWins(ctx, uToMove);
            } else {
                ASSERT(iScore < 0);
                return _TablebasesSaySideWins(ctx, !uToMove);
            }
        case RECOGN_LOWER:
            if (iScore == 0) {
                return _TablebasesSayDrawOrWin(ctx, uToMove);
            } else if (iScore > 0) {
                return _TablebasesSaySideWins(ctx, uToMove);
            } else {
                ASSERT(iScore < 0);
                return _TablebasesSaySideWins(ctx, !uToMove);
            }
        case RECOGN_UPPER:
            if (iScore == 0) {
                return _TablebasesSayDrawOrWin(ctx, !uToMove);
            } else if (iScore > 0) {
                return _TablebasesSayDrawOrWin(ctx, !uToMove);
            } else {
                ASSERT(iScore < 0);
                return _TablebasesSaySideWins(ctx, !uToMove);
            }
        default:
			ASSERT(FALSE);
			return FALSE;
    }
}

static FLAG 
_NothingBut(IN POSITION *pos, 
            IN PIECE p, 
            IN ULONG uColor)
/**

Routine description:

    Support routine to assert that nothing but the piece bits
    enumerated in parameter p are present in the position pos for side
    uColor.

Parameters:

    POSITION *pos,
    PIECE p,
    ULONG uColor

Return value:

    #ifdef DEBUG
static FLAG

**/
{
    static PIECE q[] = { KNIGHT, BISHOP, ROOK, QUEEN };
    ULONG u;
   
    if (!(p & PAWN))
    {
        if (pos->uPawnCount[uColor] > 0) return(FALSE);
    }

    for (u = 0; u < ARRAY_LENGTH(q); u++)
    {
        if (!(p & q[u]))
        {
            if (pos->uNonPawnCount[uColor][q[u]] > 0) return(FALSE);
        }
    }
    return(TRUE);
}
#endif

static ULONG 
_RecognizeKK(IN SEARCHER_THREAD_CONTEXT *ctx, 
             IN OUT SCORE *piScore)
/**

Routine description:

    Encode the knowledge that two lone kings on the board are a draw.

Parameters:

    POSITION *pos,
    SCORE *piScore

Return value:

    static ULONG

**/
{
    *piScore = 0;
    return(RECOGN_EXACT);
}

static ULONG 
_RecognizeKBK(IN SEARCHER_THREAD_CONTEXT *ctx, 
              IN OUT SCORE *piScore)
/**

Routine description:

    Attempt to recognize KB*KB+ positions.

Parameters:

    POSITION *pos,
    SCORE *piScore

Return value:

    static ULONG

**/
{
    ULONG uStrong;
    ULONG uBishops;
    COOR cWeakKing;
    ULONG u;
    ULONG uAdjacent;
    POSITION *pos = &ctx->sPosition;
    
    ASSERT((pos->uNonPawnCount[WHITE][0] <= 3) &&
           (pos->uNonPawnCount[BLACK][0] <= 3));
    ASSERT(_NothingBut(pos, BISHOP, WHITE));
    ASSERT(_NothingBut(pos, BISHOP, BLACK));

    //
    // Recognize KBKB as a draw unless there's a cornered king (in
    // which case it may be a mate-in-1)
    // 
    if ((pos->uNonPawnCount[WHITE][0] == 2) &&
        (pos->uNonPawnCount[BLACK][0] == 2))
    {
        if ((CORNER_DISTANCE(pos->cNonPawns[WHITE][0]) != 0) &&
            (CORNER_DISTANCE(pos->cNonPawns[BLACK][0]) != 0))
        {
            *piScore = 0;
            return(RECOGN_EXACT);
        }
    }
    
    //
    // Otherwise we want to deal with KB+ vs lone K.  KBKBB etc are
    // too hard to recognize.
    // 
    if ((pos->uNonPawnCount[WHITE][0] != 1) &&
        (pos->uNonPawnCount[BLACK][0] != 1))
    {
        return(UNRECOGNIZED);
    }
    
    //
    // If we get here then one side has no pieces (except the king).
    // 
    uStrong = BLACK;
    if (pos->uNonPawnCount[WHITE][0] > 1)
    {
        uStrong = WHITE;
    }
    ASSERT(pos->uNonPawnCount[uStrong][0] > 1);
    ASSERT(pos->uNonPawnCount[FLIP(uStrong)][0] == 1);

    //
    // KB vs K is a draw, KB+ vs K is still a draw if all bishops are the
    // same color.
    // 
    uBishops = pos->uNonPawnCount[uStrong][BISHOP];
    if ((uBishops == 1) ||
        (pos->uWhiteSqBishopCount[uStrong] == 0) ||
        (pos->uWhiteSqBishopCount[uStrong] == uBishops))
    {
        *piScore = 0;
        return(RECOGN_EXACT);
    }
    
    //
    // If we get here the strong side has more than one bishop and has
    // at least one bishop on each color.
    // 

    //
    // If the weak king is next to a strong side piece, fail to
    // recognize since the weak king may take the bishop with the
    // move.  Note: we allow the weak king to be adjacent to up to one
    // enemy bishop as long as it's the strong side's turn to move.
    // 
    cWeakKing = pos->cNonPawns[FLIP(uStrong)][0];
    ASSERT(DISTANCE(cWeakKing, pos->cNonPawns[uStrong][0]) > 1);
    uAdjacent = 0;
    for (u = 1; u < pos->uNonPawnCount[uStrong][0]; u++)
    {
        uAdjacent += DISTANCE(cWeakKing, pos->cNonPawns[uStrong][u] == 1);
    }
    if (uAdjacent > 1)
    {
        return(UNRECOGNIZED);
    }

    //
    // If it's the weak side's turn and the strong king is close
    // enough that the weak side may be stalemated, fail to recognize.
    //
    if (pos->uToMove != uStrong)
    {
        ASSERT(pos->uToMove == FLIP(uStrong));
        if (uAdjacent == 1)
        {
            return(UNRECOGNIZED);
        }
        
        ASSERT(IS_ON_BOARD(pos->cNonPawns[uStrong][0]));
        ASSERT(IS_KING(pos->rgSquare[pos->cNonPawns[uStrong][0]].pPiece));
        u = DISTANCE(cWeakKing, pos->cNonPawns[uStrong][0]);
        ASSERT(u != 1);
        if ((u == 2) && (ON_EDGE(cWeakKing)))
        {
            return(UNRECOGNIZED);
        }
    }
    
    //
    // This is a recognized win for the strong side.  Compute a score
    // that encourages cornering the weak king and making progress
    // towards a checkmate.
    // 
    *piScore = (pos->iMaterialBalance[uStrong] + VALUE_QUEEN - 
                (u * 16) - (CORNER_DISTANCE(cWeakKing) * 32));
    ASSERT(IS_VALID_SCORE(*piScore));
    if (pos->uToMove != uStrong)
    {
        *piScore *= -1;
        return(RECOGN_UPPER);
    }
    return(RECOGN_LOWER);
}

static ULONG 
_RecognizeKNK(IN SEARCHER_THREAD_CONTEXT *ctx, 
              IN OUT SCORE *piScore)
/**

Routine description:

    Recognize KN*KN+ positions.

Parameters:

    POSITION *pos,
    SCORE *piScore

Return value:

    static ULONG

**/
{
    POSITION *pos = &ctx->sPosition;
    ULONG uStrong;

    ASSERT((pos->uNonPawnCount[WHITE][0] <= 3) &&
           (pos->uNonPawnCount[BLACK][0] <= 3));
    ASSERT(_NothingBut(pos, KNIGHT, WHITE));
    ASSERT(_NothingBut(pos, KNIGHT, BLACK));
    
    //
    // KNKN is a draw unless someone has a K in the corner (in which case,
    // with the friend knight in the way, there's a possible mate)
    // 
    if ((pos->uNonPawnCount[WHITE][0] == 2) &&
        (pos->uNonPawnCount[BLACK][0] == 2))
    {
        if ((CORNER_DISTANCE(pos->cNonPawns[WHITE][0]) != 0) &&
            (CORNER_DISTANCE(pos->cNonPawns[BLACK][0]) != 0))
        {
            *piScore = 0;
            return(RECOGN_EXACT);
        }
        return(UNRECOGNIZED);
    }
    
    //
    // KNNKN etc... unrecognized.  Heinz says "exceptional wins possible for
    // any side by mates in seven or less moves."  TODO: add this knowledge.
    // 
    if ((pos->uNonPawnCount[WHITE][0] != 1) ||
        (pos->uNonPawnCount[BLACK][0] != 1))
    {
        return(UNRECOGNIZED);
    }
    
    //
    // If we get here somebody has no pieces (except a lone king).
    // 
    uStrong = WHITE;
    if (pos->uNonPawnCount[BLACK][0] > 1)
    {
        uStrong = BLACK;
    }
    ASSERT(pos->uNonPawnCount[uStrong][0] > 1);
    ASSERT(pos->uNonPawnCount[FLIP(uStrong)][0] == 1);

    //
    // Call KNNK a draw unless the weak K is on the edge of the board
    // where a checkmate is possible (e.g. 8/8/8/8/8/N2N4/4K3/2k5 b - -)
    // Everything else in here is a draw.
    //
    ASSERT(pos->uNonPawnCount[uStrong][0] < 4);
    if (ON_EDGE(pos->cNonPawns[FLIP(uStrong)][0])) 
    {
        return(UNRECOGNIZED);
    }
    *piScore = 0;
    return(RECOGN_EXACT);
}


static ULONG 
_RecognizeKBNK(IN SEARCHER_THREAD_CONTEXT *ctx, 
               IN OUT SCORE *piScore)
/**

Routine description:

Parameters:

    POSITION *pos,
    SCORE *piScore

Return value:

    static ULONG

**/
{
    ULONG uStrong;
    COOR cWeakKing;
    COOR cKnight;
    ULONG u;
    ULONG uDist;
    ULONG uAdjacent;
    POSITION *pos = &ctx->sPosition;

    ASSERT((pos->uNonPawnCount[WHITE][0] <= 3) &&
           (pos->uNonPawnCount[BLACK][0] <= 3));
    ASSERT(_NothingBut(pos, BISHOP | KNIGHT, WHITE));
    ASSERT(_NothingBut(pos, BISHOP | KNIGHT, BLACK));
    
    if ((pos->uNonPawnCount[WHITE][0] > 1) &&
        (pos->uNonPawnCount[BLACK][0] > 1))
    {
        //
        // Do not recognize stuff like KNNKB or KNKBB etc...
        // 
        if (pos->uNonPawnCount[WHITE][0] + pos->uNonPawnCount[BLACK][0] > 4)
        {
            return(UNRECOGNIZED);
        }
        
        //
        // This is KNKB; unless someone's king is on the edge,
        // recognize a draw.
        // 
        ASSERT((pos->uNonPawnCount[WHITE][0] == 2) &&
               (pos->uNonPawnCount[BLACK][0] == 2));
        if (ON_EDGE(pos->cNonPawns[WHITE][0]) ||
            ON_EDGE(pos->cNonPawns[BLACK][0]))
        {
            return(UNRECOGNIZED);
        }
        *piScore = 0;
        return(RECOGN_EXACT);
    }

    //
    // If we get here we are in a KBNK endgame.
    // 
    uStrong = WHITE;
    if (pos->uNonPawnCount[BLACK][0] > 1)
    {
        uStrong = BLACK;
    }
    ASSERT(pos->uNonPawnCount[uStrong][0] > 1);
    ASSERT(pos->uNonPawnCount[FLIP(uStrong)][0] == 1);
    cWeakKing = pos->cNonPawns[FLIP(uStrong)][0];
    ASSERT(IS_ON_BOARD(cWeakKing));
    ASSERT(IS_KING(pos->rgSquare[cWeakKing].pPiece));

    //
    // Don't recognize anything where the N is in a corner near the
    // lone king.  It's possible for the lone king to trap it there
    // thus drawing the game.
    //
    cKnight = pos->cNonPawns[uStrong][KNIGHT];
    if ((CORNER_DISTANCE(cKnight) == 0) &&
        (DISTANCE(cWeakKing, cKnight) == 1))
    {
        return(UNRECOGNIZED);
    }
    
    //
    // Don't recognize anything if the weak king is next to a strong side's
    // piece.
    // 
    uAdjacent = 0;
    for (u = 1; u < pos->uNonPawnCount[uStrong][0]; u++)
    {
        uAdjacent = (DISTANCE(pos->cNonPawns[uStrong][u], cWeakKing) == 1);
    }
    if (!uAdjacent) 
    {
        return(UNRECOGNIZED);
    }

    //
    // Don't recognize if the two kings are close enough to each other
    // that there might be a stalemate if the weak side is on move and
    // on the edge.
    // 
    if (pos->uToMove != uStrong)
    {
        ASSERT(pos->uToMove == FLIP(uStrong));
        if (uAdjacent == 1)
        {
            return(UNRECOGNIZED);
        }

        ASSERT(IS_ON_BOARD(pos->cNonPawns[uStrong][0]));
        ASSERT(IS_KING(pos->rgSquare[pos->cNonPawns[uStrong][0]].pPiece));
        u = DISTANCE(cWeakKing, pos->cNonPawns[uStrong][0]);
        ASSERT(u != 1);
        if ((u == 2) && (ON_EDGE(cWeakKing)))
        {
            return(UNRECOGNIZED);
        }
    }

    //
    // Calculate a score that grabs the search's attention and makes
    // progress towards driving the weak king to the correct corner to
    // mate him.
    // 
    if (pos->uWhiteSqBishopCount[uStrong] > 0)
    {
        uDist = WHITE_CORNER_DISTANCE(cWeakKing);
    }
    else
    {
        uDist = BLACK_CORNER_DISTANCE(cWeakKing);
    }
    ASSERT((0 <= uDist) && (uDist <= 7));
    
    *piScore = (pos->iMaterialBalance[uStrong] + (7 * VALUE_PAWN)
                - (uDist * 32) - (u * 16));
    ASSERT(IS_VALID_SCORE(*piScore));
    if (pos->uToMove != uStrong)
    {
        *piScore *= -1;
        return(RECOGN_UPPER);
    }
    return(RECOGN_LOWER);
}


static ULONG 
_RecognizeKNKP(IN SEARCHER_THREAD_CONTEXT *ctx, 
               IN OUT SCORE *piScore)
/**

Routine description:

    Recognize KN+KP+ positions.

Parameters:

    POSITION *pos,
    SCORE *piScore

Return value:

    static ULONG

**/
{
    ULONG uStrong;
    POSITION *pos = &ctx->sPosition;
    
    ASSERT((pos->uNonPawnCount[WHITE][0] <= 3) &&
           (pos->uNonPawnCount[BLACK][0] <= 3));
    ASSERT(_NothingBut(pos, PAWN | KNIGHT, WHITE));
    ASSERT(_NothingBut(pos, PAWN | KNIGHT, BLACK));

    //
    // Call the side with knight(s) "strong"
    //
    uStrong = WHITE;
    if (pos->uNonPawnCount[BLACK][0] > 1)
    {
        uStrong = BLACK;
    }
    ASSERT(pos->uNonPawnCount[uStrong][0] > 1);
    ASSERT(pos->uNonPawnCount[FLIP(uStrong)][0] == 1);

    //
    // Don't recognize KNNKP or KNKP with K on edge
    // 
    if ((pos->uNonPawnCount[uStrong][KNIGHT] > 2) ||
        (ON_EDGE(pos->cNonPawns[FLIP(uStrong)][0])))
    {
        return(UNRECOGNIZED);
    }

    //
    // This is at least a draw for the side with the pawn(s) and at
    // best a draw for the side with the knight(s)
    // 
    *piScore = 0;
    if (pos->uToMove == uStrong)
    {
        return(RECOGN_UPPER);
    }
    return(RECOGN_LOWER);
}


static ULONG
_RecognizeKBKP(IN SEARCHER_THREAD_CONTEXT *ctx, 
               IN OUT SCORE *piScore)
/**

Routine description:

    Recognize KB+P*KP+ positions including knowledge about "wrong color
    bishop(s)".

Parameters:

    POSITION *pos,
    SCORE *piScore

Return value:

    static ULONG

**/
{
    ULONG uStrong;
    BITBOARD bb;
    ULONG u;
    COOR c;
    COOR cWeakKing;
    POSITION *pos = &ctx->sPosition;

    ASSERT((pos->uNonPawnCount[WHITE][0] <= 3) &&
           (pos->uNonPawnCount[BLACK][0] <= 3));
    ASSERT(_NothingBut(pos, PAWN | BISHOP, WHITE));
    ASSERT(_NothingBut(pos, PAWN | BISHOP, BLACK));

    //
    // Call the side with bishop(s) "strong"
    //
    uStrong = WHITE;
    if (pos->uNonPawnCount[BLACK][BISHOP] > 0)
    {
        uStrong = BLACK;
    }
    ASSERT(pos->uNonPawnCount[uStrong][0] > 1);
    ASSERT(pos->uNonPawnCount[FLIP(uStrong)][0] == 1);

    cWeakKing = pos->cNonPawns[FLIP(uStrong)][0];
    ASSERT(IS_ON_BOARD(cWeakKing));
    ASSERT(IS_KING(pos->rgSquare[cWeakKing].pPiece));

    //
    // Construct a strong side bitboard of pawn locations
    // 
    bb = 0ULL;
    for (u = 0; u < pos->uPawnCount[uStrong]; u++)
    {
        c = pos->cPawns[uStrong][u];
        ASSERT(IS_ON_BOARD(c));
        bb |= COOR_TO_BB(c);
    }
    
    if ((pos->uNonPawnCount[BLACK][0] + pos->uPawnCount[BLACK] > 1) &&
        (pos->uNonPawnCount[WHITE][0] + pos->uPawnCount[WHITE] > 1))
    {
        //
        // Neither side has a lone king.  This is either KBKP+ or
        // KBP+KP+.
        // 
        if (pos->uPawnCount[uStrong] > 0)
        {
            //
            // Strong side can maybe take an adjacent pawn and survive the
            // bad bishop.
            // 
            if (uStrong == pos->uToMove)
            {
                return(UNRECOGNIZED);
            }
            
            //
            // Make sure the strong side has the right color bishop
            // for his pawns.
            //
            if (uStrong == WHITE)
            {
                if (!(bb & ~BBFILE[A]) &&
                    (pos->uWhiteSqBishopCount[WHITE] == 0) &&
                    (DISTANCE(cWeakKing, A8) <= 1))
                {
                    goto at_best_draw_for_strong;
                }
                
                if (!(bb & ~BBFILE[H]) &&
                    (pos->uWhiteSqBishopCount[WHITE] == 
                     pos->uNonPawnCount[WHITE][BISHOP]) &&
                    (DISTANCE(cWeakKing, H8) <= 1))
                {
                    goto at_best_draw_for_strong;
                }
            }
            else
            {
                if (!(bb & ~BBFILE[A]) &&
                    (pos->uWhiteSqBishopCount[BLACK] == 
                     pos->uNonPawnCount[BLACK][BISHOP]) &&
                    (DISTANCE(cWeakKing, A1) <= 1))
                {
                    goto at_best_draw_for_strong;
                }
                
                if (!(bb & ~BBFILE[H]) &&
                    (pos->uWhiteSqBishopCount[BLACK] == 0) &&
                    (DISTANCE(cWeakKing, H1) <= 1))
                {
                    goto at_best_draw_for_strong;
                }
            }
            return(UNRECOGNIZED);
        }
        else
        {
            //
            // Strong side has no pawns... there's a mate-in-1 here if
            // the weak king is stuck in the corner.
            //
            if ((pos->uNonPawnCount[uStrong][BISHOP] > 1) ||
                (CORNER_DISTANCE(cWeakKing) == 0))
            {
                return(UNRECOGNIZED);
            }
            goto at_best_draw_for_strong;
        }
    } 
    else 
    {
        //
        // KBPK: make sure the bishop is the right color.  This time
        // there is no need to check for on-move.
        // 
        ASSERT(pos->uNonPawnCount[FLIP(uStrong)][0] == 1);
        ASSERT(pos->uNonPawnCount[uStrong][0] > 1);

        if (uStrong == WHITE)
        {
            if (!(bb & ~BBFILE[A]) &&
                (pos->uWhiteSqBishopCount[WHITE] == 0) &&
                (DISTANCE(cWeakKing, A8) <= 1))
            {
                goto draw;
            }
            if (!(bb & ~BBFILE[H]) &&
                (pos->uWhiteSqBishopCount[WHITE] ==
                 pos->uNonPawnCount[WHITE][BISHOP]) &&
                (DISTANCE(cWeakKing, H8) <= 1))
            {
                goto draw;
            }
        } 
        else 
        {
            if (!(bb & ~BBFILE[A]) &&
                (pos->uWhiteSqBishopCount[BLACK] == 
                 pos->uNonPawnCount[BLACK][BISHOP]) &&
                (DISTANCE(cWeakKing, A1) <= 1))
            {
                goto draw;
            }
            if (!(bb & ~BBFILE[H]) &&
                (pos->uWhiteSqBishopCount[BLACK] == 0) &&
                (DISTANCE(cWeakKing, H1) <= 1))
            {
                goto draw;
            }
        }
        return(UNRECOGNIZED);
    }
#ifdef DEBUG
    UtilPanic(SHOULD_NOT_GET_HERE, 
              NULL, NULL, NULL, NULL,
              __FILE__, __LINE__);
#endif

 at_best_draw_for_strong:
    *piScore = 0;
    if (pos->uToMove == uStrong)
    {
        return(RECOGN_UPPER);
    }
    return(RECOGN_LOWER);

 draw:
    *piScore = 0;
    return(RECOGN_EXACT);
}

static void
_GetPassersCriticalSquares(IN ULONG uColor,
                           IN COOR cPawn, 
                           IN OUT COOR *cSquare)
/**

Routine description:

    In an effort to classify KPK positions I read some stuff in
    "Pandolfini's Endgame Course" about critical squares.  A critical
    square of a passed pawn is one that, if occupied by the friendly
    king, is sufficient to queen that pawn by force.  This code is a
    helper routing for the KPK recognizer that returns the critical
    square given a pawn's location for color uColor.

    Note: rook pawns only have one critical square so this code
    returns the same square three times in this case.

Parameters:

    ULONG uColor,
    COOR cPawn,
    COOR *cSquare

Return value:

    static void

**/
{
    static COOR cCriticalSquare[2][128] = 
    {
        {    
            0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,   0,0,0,0,0,0,0,0,
            0x61, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x66,   0,0,0,0,0,0,0,0,
            0x61, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x66,   0,0,0,0,0,0,0,0,
            0x61, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x66,   0,0,0,0,0,0,0,0,
            0x61, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x66,   0,0,0,0,0,0,0,0,
            0x61, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x66,   0,0,0,0,0,0,0,0,
            0x61, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x66,   0,0,0,0,0,0,0,0,
            0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,   0,0,0,0,0,0,0,0,
        },
        {    
            0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,   0,0,0,0,0,0,0,0,
            0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x16,   0,0,0,0,0,0,0,0,
            0x11, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x16,   0,0,0,0,0,0,0,0,
            0x11, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x16,   0,0,0,0,0,0,0,0,
            0x11, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x16,   0,0,0,0,0,0,0,0,
            0x11, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x16,   0,0,0,0,0,0,0,0,
            0x11, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x16,   0,0,0,0,0,0,0,0,
            0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,   0,0,0,0,0,0,0,0,
        } 
    };
    ULONG uFile = FILE(cPawn);

    ASSERT(IS_VALID_COLOR(uColor));
    ASSERT(IS_ON_BOARD(cPawn));
    ASSERT((RANK(cPawn) != 1) && (RANK(cPawn) != 8));
    
    if ((uFile == A) || (uFile == H))
    {
        cSquare[0] = cCriticalSquare[uColor][cPawn];
        cSquare[1] = cCriticalSquare[uColor][cPawn];
        cSquare[2] = cCriticalSquare[uColor][cPawn];
        goto end;
    }
    cSquare[1] = cCriticalSquare[uColor][cPawn];
    cSquare[0] = cSquare[1] - 1;
    cSquare[2] = cSquare[1] + 1;
    
 end:
    ASSERT(cSquare[0] != 0);
    ASSERT(cSquare[1] != 0);
    ASSERT(cSquare[2] != 0);
    ASSERT(IS_ON_BOARD(cSquare[0]));
    ASSERT(IS_ON_BOARD(cSquare[1]));
    ASSERT(IS_ON_BOARD(cSquare[2]));
}


static ULONG 
_RecognizeKPK(IN SEARCHER_THREAD_CONTEXT *ctx, 
              IN OUT SCORE *piScore)
/**

Routine description:

    Attempt to recognize KP+KP* positions.

Parameters:

    POSITION *pos,
    SCORE *piScore

Return value:

    static ULONG

**/
{
    ULONG uStrong;
    ULONG uWeak;
    COOR cPawn, cQueen;
    COOR cCritical[3];
    ULONG uDist[2];
    ULONG u;
    PAWN_HASH_ENTRY *pHash;
    POSITION *pos = &ctx->sPosition;

    ASSERT(pos->uNonPawnCount[WHITE][0] == 1);
    ASSERT(pos->uNonPawnCount[BLACK][0] == 1);
    ASSERT(_NothingBut(pos, PAWN, WHITE));
    ASSERT(_NothingBut(pos, PAWN, BLACK));

    //
    // Look to see if one side has a winning passed pawn
    //
    pHash = PawnHashLookup(ctx);
    ASSERT(pHash != NULL);
    if (pHash->u64Key == pos->u64PawnSig)
    {
        pos->iScore[BLACK] = pos->iScore[WHITE] = 0;
        if (TRUE == EvalPasserRaces(pos, pHash))
        {
            //
            // Someone wins.
            //
            ASSERT((pos->iScore[BLACK] == 0) || (pos->iScore[WHITE] == 0));
            ASSERT((pos->iScore[BLACK] != 0) || (pos->iScore[WHITE] != 0));
            if (pos->iScore[BLACK] > 0)
            {
                ASSERT(pos->iScore[WHITE] == 0);
                *piScore = (pos->iMaterialBalance[BLACK] + 2 * VALUE_PAWN +
                            pos->iScore[BLACK]);
                ASSERT(IS_VALID_SCORE(*piScore));
                if (pos->uToMove == WHITE)
                {
                    *piScore *= -1;
                    return(RECOGN_UPPER);
                }
                return(RECOGN_LOWER);
            }
            else
            {
                ASSERT(pos->iScore[WHITE] > 0);
                *piScore = (pos->iMaterialBalance[WHITE] + 2 * VALUE_PAWN +
                            pos->iScore[WHITE]);
                ASSERT(IS_VALID_SCORE(*piScore));
                if (pos->uToMove == BLACK)
                {
                    *piScore *= -1;
                    return(RECOGN_UPPER);
                }
                return(RECOGN_LOWER);
            }
        }
    }

    //
    // No one has a winning passer or we did not find the hash entry.
    //
    if ((pos->uPawnCount[WHITE] > 0) && (pos->uPawnCount[BLACK] > 0))
    {
        return(UNRECOGNIZED);
    }

    //
    // If we get here then no one has a winning passer and one side
    // has only a lone king -- no pawns.  Do some more "sophisticated"
    // analysis looking for a won passer.  Also: this is at best a
    // draw for the weak side.
    //
    uStrong = WHITE;
    if (pos->uPawnCount[WHITE] == 0)
    {
        uStrong = BLACK;
    }
    ASSERT(pos->uPawnCount[uStrong] > 0);
    uWeak = FLIP(uStrong);
    ASSERT(pos->uPawnCount[uWeak] == 0);
        
    if (pos->uPawnCount[uStrong] > 1)
    {
        *piScore = 0;
        if (pos->uToMove == uStrong)
        {
            return(RECOGN_LOWER);
        }
        return(RECOGN_UPPER);
    }

    //
    // The side with pawns has only one pawn, do some more
    // sophisticated analysis here to spot winning KPK
    // configurations earlier by using "critical squares"
    // 
    ASSERT(pos->uPawnCount[uStrong] == 1);
    cPawn = pos->cPawns[uStrong][0];
    ASSERT(IS_ON_BOARD(cPawn));
    ASSERT(IS_PAWN(pos->rgSquare[cPawn].pPiece));
            
    //
    // Step 1: the strong king must be closer to the pawn than
    // the weak king.
    //
    uDist[uStrong] = DISTANCE(pos->cNonPawns[uStrong][0], cPawn);
    ASSERT((1 <= uDist[uStrong]) && (uDist[uStrong] <= 7));
    uDist[uWeak] = DISTANCE(pos->cNonPawns[uWeak][0], cPawn);
    ASSERT((1 <= uDist[uWeak]) && (uDist[uWeak] <= 7));
    if (uDist[uStrong] <= uDist[uWeak])
    {
        //
        // Step 2: the strong king must be closer to one of the
        // passer's critical squares than the weak king.
        //
        _GetPassersCriticalSquares(uStrong, cPawn, cCritical);
        for (u = 0; u < 3; u++)
        {
            uDist[uStrong] = DISTANCE(pos->cNonPawns[uStrong][0], 
                                      cCritical[u]);
            ASSERT((0 <= uDist[uStrong]) && (uDist[uStrong] <= 7));
            uDist[uWeak] = DISTANCE(pos->cNonPawns[uWeak][0], 
                                    cCritical[u]);
            
            //
            // Assume if the weak side is on move he will move
            // towards the critical square.  Also assume that
            // in so doing he will increase the path that the
            // strong king must take to get to the critical
            // square.
            //
            if (uDist[uWeak] > 0) {
                uDist[uWeak] -= (pos->uToMove == uWeak);
            }
            if (uDist[uStrong] > 0) {
                uDist[uStrong] += (pos->uToMove == uWeak) * 2;
            }
            ASSERT((0 <= uDist[uWeak]) && (uDist[uWeak] <= 7));
            if (uDist[uStrong] < uDist[uWeak])
            {
                cQueen = 
                    QUEENING_SQUARE_BY_COLOR_FILE[uStrong][FILE(cPawn)];
                *piScore = (pos->iMaterialBalance[uStrong] +
                            VALUE_QUEEN + (2 * VALUE_PAWN) -
                            (RANK_DISTANCE(cPawn, cQueen) * 32));
                if (pos->uToMove == uWeak)
                {
                    *piScore *= -1;
                    return(RECOGN_UPPER);
                }
                return(RECOGN_LOWER);
            }
        }
    }

    //
    // If we get here then the weak king is as close to or
    // closer to the lone passer than the strong king -or- the
    // weak king is as close to or closer to the passer's
    // critical squares than the strong king.  This may still
    // be won for the strong side but we don't know -- the
    // logic is ugly.  This is still at best drawn for the
    // weak side though.
    //
    *piScore = 0;
    if (pos->uToMove == uStrong)
    {
        return(RECOGN_LOWER);
    }
    return(RECOGN_UPPER);
}


static void 
_NewRecognizer(IN RECOGNIZER *pFunct, 
               IN ULONG uWhiteSig, 
               IN ULONG uBlackSig)
/**

Routine description:

    Register a recognizer function.

Parameters:

    RECOGNIZER *pFunct,
    ULONG uWhiteSig,
    ULONG uBlackSig

Return value:

    static void

**/
{
    g_bvRecognizerAvailable[uWhiteSig] |= (1 << uBlackSig);
    g_bvRecognizerAvailable[uBlackSig] |= (1 << uWhiteSig);
    g_pRecognizers[RECOGN_INDEX(uWhiteSig, uBlackSig)] = pFunct;
}

void 
InitializeInteriorNodeRecognizers(void)
/**

Routine description:

    Init this recognizer module

Parameters:

    void

Return value:

    void

**/
{
    memset(g_pRecognizers, 0, sizeof(g_pRecognizers));
    memset(g_bvRecognizerAvailable, 0, sizeof(g_bvRecognizerAvailable));

    // KK                           P  N  B  R  Q
    _NewRecognizer(_RecognizeKK,
                   _MakeMaterialSig(0, 0, 0, 0, 0),
                   _MakeMaterialSig(0, 0, 0, 0, 0));

    // KB+K                         P  N  B  R  Q    
    _NewRecognizer(_RecognizeKBK,
                   _MakeMaterialSig(0, 0, 1, 0, 0),
                   _MakeMaterialSig(0, 0, 0, 0, 0));

    // KB+KB+                       P  N  B  R  Q
    _NewRecognizer(_RecognizeKBK,
                   _MakeMaterialSig(0, 0, 1, 0, 0),
                   _MakeMaterialSig(0, 0, 1, 0, 0));
    
    // KN+K                         P  N  B  R  Q    
    _NewRecognizer(_RecognizeKNK,
                   _MakeMaterialSig(0, 1, 0, 0, 0), 
                   _MakeMaterialSig(0, 0, 0, 0, 0));

    // KN+KN+                       P  N  B  R  Q        
    _NewRecognizer(_RecognizeKNK, 
                   _MakeMaterialSig(0, 1, 0, 0, 0), 
                   _MakeMaterialSig(0, 1, 0, 0, 0));

    // KN+KB+                       P  N  B  R  Q    
    _NewRecognizer(_RecognizeKBNK, 
                   _MakeMaterialSig(0, 1, 0, 0, 0), 
                   _MakeMaterialSig(0, 0, 1, 0, 0));

    // KN+B+K                       P  N  B  R  Q    
    _NewRecognizer(_RecognizeKBNK, 
                   _MakeMaterialSig(0, 1, 1, 0, 0), 
                   _MakeMaterialSig(0, 0, 0, 0, 0));

    // KN+KP+                       P  N  B  R  Q    
    _NewRecognizer(_RecognizeKNKP, 
                   _MakeMaterialSig(1, 0, 0, 0, 0), 
                   _MakeMaterialSig(0, 1, 0, 0, 0));

    // KB+KP+                       P  N  B  R  Q    
    _NewRecognizer(_RecognizeKBKP, 
                   _MakeMaterialSig(1, 0, 0, 0, 0), 
                   _MakeMaterialSig(0, 0, 1, 0, 0));
    
    // KP+B+KP+                     P  N  B  R  Q    
    _NewRecognizer(_RecognizeKBKP, 
                   _MakeMaterialSig(1, 0, 1, 0, 0), 
                   _MakeMaterialSig(1, 0, 0, 0, 0));
    

    // KP+B+K                       P  N  B  R  Q
    _NewRecognizer(_RecognizeKBKP, 
                   _MakeMaterialSig(1, 0, 1, 0, 0), 
                   _MakeMaterialSig(0, 0, 0, 0, 0));

    // KP+K                         P  N  B  R  Q    
    _NewRecognizer(_RecognizeKPK, 
                   _MakeMaterialSig(0, 0, 0, 0, 0), 
                   _MakeMaterialSig(1, 0, 0, 0, 0));

    // KP+KP+                       P  N  B  R  Q
    _NewRecognizer(_RecognizeKPK, 
                   _MakeMaterialSig(1, 0, 0, 0, 0), 
                   _MakeMaterialSig(1, 0, 0, 0, 0));
}


ULONG 
RecognLookup(IN SEARCHER_THREAD_CONTEXT *ctx,
             IN OUT SCORE *piScore,
             IN FLAG fProbeEGTB)
/**

Routine description:

    This is the interface to the recogn module; it is called from
    Search and Qsearch.  Its job is to: 1. check high speed internal
    node recognizer functions if a recognizable material signature is
    present on the board and 2. probe on-disk EGTB files if no useful
    recognizer result is found.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    SCORE *piScore,
    FLAG fProbeEGTB

Return value:

    ULONG

**/
{
    ULONG uVal;
    POSITION *pos = &ctx->sPosition;
    ULONG uSig[2];
    RECOGNIZER *pRec;
    SCORE iScore;

    //
    // Try interior node recognizers
    // 
    if ((pos->uNonPawnCount[WHITE][0] <= 3) &&
        (pos->uNonPawnCount[BLACK][0] <= 3))
    {
        uSig[BLACK] = _MakeMaterialSig((pos->uPawnCount[BLACK] > 0),
                                       (pos->uNonPawnCount[BLACK][KNIGHT] > 0),
                                       (pos->uNonPawnCount[BLACK][BISHOP] > 0),
                                       (pos->uNonPawnCount[BLACK][ROOK] > 0),
                                       (pos->uNonPawnCount[BLACK][QUEEN] > 0));
        uSig[WHITE] = _MakeMaterialSig((pos->uPawnCount[WHITE] > 0),
                                       (pos->uNonPawnCount[WHITE][KNIGHT] > 0),
                                       (pos->uNonPawnCount[WHITE][BISHOP] > 0),
                                       (pos->uNonPawnCount[WHITE][ROOK] > 0),
                                       (pos->uNonPawnCount[WHITE][QUEEN] > 0));
        pRec = g_pRecognizers[RECOGN_INDEX(uSig[WHITE], uSig[BLACK])];
        if (NULL != pRec)
        {
            if (g_bvRecognizerAvailable[uSig[WHITE]] & (1 << uSig[BLACK]))
            {
                uVal = pRec(ctx, &iScore);
                if (UNRECOGNIZED != uVal)
                {
                    *piScore = iScore;
                    ASSERT((-NMATE < iScore) && (iScore < +NMATE));
                    ASSERT(_SanityCheckRecognizers(ctx, iScore, uVal));
                    return(uVal);
                }
            }
        }
    }

    //
    // Try EGTB probe as long as some conditions are met
    // 
    if ((FALSE != fProbeEGTB) &&
        ((pos->uNonPawnCount[WHITE][0] + pos->uNonPawnCount[BLACK][0] +
          pos->uPawnCount[WHITE] + pos->uPawnCount[BLACK]) <= 5))
    {
        if (TRUE == ProbeEGTB(ctx, &iScore))
        {
            //
            // We have an exact score from EGTB but if it's a mate in N
            // we still have to adjust it for the present ply. 
            //
            if (iScore >= +NMATE) {
                iScore -= ctx->uPly;
            }
            else if (iScore <= -NMATE) {
                iScore += ctx->uPly;
            }
#ifdef DEBUG
            else 
            {
                ASSERT(iScore == 0); // iDrawValue[side]
            }
#endif
            *piScore = iScore;
            return(RECOGN_EXACT);
        }
    }
    return(UNRECOGNIZED);
}
