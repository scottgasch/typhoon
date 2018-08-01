/**

Copyright (c) Scott Gasch

Module Name:

    move.c

Abstract:

    Functions to make and unmake moves.

Author:

    Scott Gasch (scott.gasch@gmail.com) 09 May 2004

Revision History:

    $Id: move.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

void 
SlidePiece(POSITION *pos, COOR cFrom, COOR cTo)
/**

Routine description:

    Slide a piece (i.e. not a pawn) from cFrom to cTo.  This is much
    faster than LiftPiece / PlacePiece.

Parameters:

    POSITION *pos : the board
    COOR cFrom : square moving from
    COOR cTo : square moving to

Return value:

    void

**/
{
    register PIECE p;
    register ULONG c;
    ULONG uIndex;
    
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(IS_EMPTY(pos->rgSquare[cTo].pPiece));
#endif
    p = pos->rgSquare[cFrom].pPiece;
    pos->rgSquare[cFrom].pPiece = EMPTY;
    ASSERT(!IS_PAWN(p));
    ASSERT(IS_VALID_PIECE(p));
    uIndex = pos->rgSquare[cFrom].uIndex;
    c = GET_COLOR(p);
    ASSERT(IS_VALID_COLOR(c));
    ASSERT(pos->cNonPawns[c][uIndex] == cFrom);
    pos->cNonPawns[c][uIndex] = cTo;
    pos->u64NonPawnSig ^= g_u64SigSeeds[cFrom][PIECE_TYPE(p)][c];
    pos->u64NonPawnSig ^= g_u64SigSeeds[cTo][PIECE_TYPE(p)][c];
#ifdef DEBUG
    if (IS_BISHOP(p) && (IS_SQUARE_WHITE(cFrom)))
    {
        ASSERT(IS_SQUARE_WHITE(cTo));
    }
#endif
    pos->rgSquare[cTo].pPiece = p;
    pos->rgSquare[cTo].uIndex = uIndex;
#ifdef DEBUG
    pos->rgSquare[cFrom].uIndex = INVALID_PIECE_INDEX;
    VerifyPositionConsistency(pos, FALSE);
#endif
}


void 
SlidePawn(POSITION *pos, COOR cFrom, COOR cTo)
/**

Routine description:

    Slide a pawn (i.e. not a piece) from cFrom to cTo.  This is much
    faster than LiftPiece / PlacePiece.

Parameters:

    POSITION *pos : the board
    COOR cFrom : square moving from
    COOR cTo : square moving to

Return value:

    void

**/
{
    register PIECE p;
    register ULONG c;
    ULONG uIndex;
    
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(IS_EMPTY(pos->rgSquare[cTo].pPiece));
#endif
    p = pos->rgSquare[cFrom].pPiece;
    pos->rgSquare[cFrom].pPiece = EMPTY;
    ASSERT(IS_PAWN(p));
    ASSERT(IS_VALID_PIECE(p));
    uIndex = pos->rgSquare[cFrom].uIndex;
    c = GET_COLOR(p);
    ASSERT(IS_VALID_COLOR(c));
    ASSERT(pos->cPawns[c][uIndex] == cFrom);
    pos->cPawns[c][uIndex] = cTo;
    pos->u64PawnSig ^= g_u64PawnSigSeeds[cFrom][c];
    pos->u64PawnSig ^= g_u64PawnSigSeeds[cTo][c];
    pos->rgSquare[cTo].pPiece = p;
    pos->rgSquare[cTo].uIndex = uIndex;
#ifdef DEBUG
    pos->rgSquare[cFrom].uIndex = INVALID_PIECE_INDEX;
    VerifyPositionConsistency(pos, FALSE);
#endif
}


void 
SlidePieceWithoutSigs(POSITION *pos, COOR cFrom, COOR cTo)
/**

Routine description:

    Slide a piece (i.e. not a pawn) from cFrom to cTo.  This is way
    faster than LiftPiece / PlacePiece.  Also note that this version
    of the routine doesn't maintain the consistency of the position
    signatures and should therefore only be called from UnmakeMove.

Parameters:

    POSITION *pos : the board
    COOR cFrom : square moving from
    COOR cTo : square moving to

Return value:

    void

**/
{
    register PIECE p;
    register ULONG c;
    ULONG uIndex;
    
#ifdef DEBUG
    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(IS_EMPTY(pos->rgSquare[cTo].pPiece));
#endif
    p = pos->rgSquare[cFrom].pPiece;
    pos->rgSquare[cFrom].pPiece = EMPTY;
    ASSERT(IS_VALID_PIECE(p));
    ASSERT(!IS_PAWN(p));
    uIndex = pos->rgSquare[cFrom].uIndex;
    c = GET_COLOR(p);
    ASSERT(IS_VALID_COLOR(c));
    ASSERT(pos->cNonPawns[c][uIndex] == cFrom);
    pos->cNonPawns[c][uIndex] = cTo;
    pos->rgSquare[cTo].pPiece = p;
    pos->rgSquare[cTo].uIndex = uIndex;
#ifdef DEBUG
    pos->rgSquare[cFrom].uIndex = INVALID_PIECE_INDEX;
#endif
}


void 
SlidePawnWithoutSigs(POSITION *pos, COOR cFrom, COOR cTo)
/**

Routine description:

    Slide a pawn (i.e. not a piece) from cFrom to cTo.  This is way
    faster than LiftPiece / PlacePiece.  Also note that this version
    of the routine doesn't maintain the consistency of the position
    signatures and should therefore only be called from UnmakeMove.

Parameters:

    POSITION *pos : the board
    COOR cFrom : square moving from
    COOR cTo : square moving to

Return value:

    void

**/
{
    register PIECE p;
    register ULONG c;
    ULONG uIndex;
    
#ifdef DEBUG
    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(IS_EMPTY(pos->rgSquare[cTo].pPiece));
#endif
    p = pos->rgSquare[cFrom].pPiece;
    pos->rgSquare[cFrom].pPiece = EMPTY;
    ASSERT(IS_VALID_PIECE(p));
    ASSERT(IS_PAWN(p));
    uIndex = pos->rgSquare[cFrom].uIndex;
    c = GET_COLOR(p);
    ASSERT(IS_VALID_COLOR(c));
    ASSERT(pos->cPawns[c][uIndex] == cFrom);
    pos->cPawns[c][uIndex] = cTo;
    pos->rgSquare[cTo].pPiece = p;
    pos->rgSquare[cTo].uIndex = uIndex;
#ifdef DEBUG
    pos->rgSquare[cFrom].uIndex = INVALID_PIECE_INDEX;
#endif
}


PIECE 
LiftPiece(POSITION *pos, COOR cSquare)
/**

Routine description:

    Remove the piece at square cSquare from the board.  Update POSITION
    (piece lists, counts, material, etc...) accordingly.

Parameters:

    POSITION *pos,
    COOR cSquare

Return value:

    PIECE

**/
{
    register PIECE pLifted;
    ULONG color;
    register SCORE pv;
    ULONG uLastIndex;
    ULONG uIndex;
    COOR c;
    ULONG u;
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
    ASSERT(IS_ON_BOARD(cSquare));
#endif
    pLifted = pos->rgSquare[cSquare].pPiece;
    if (IS_EMPTY(pLifted))
    {
        return(pLifted);                        // ok to return an empty square
    }
    pos->rgSquare[cSquare].pPiece = EMPTY;
    ASSERT(IS_VALID_PIECE(pLifted));
    ASSERT(!IS_KING(pLifted));
    uIndex = pos->rgSquare[cSquare].uIndex;
#ifdef DEBUG
    ASSERT(IS_VALID_PIECE_INDEX(uIndex));
    pos->rgSquare[cSquare].uIndex = INVALID_PIECE_INDEX;
#endif
    color = GET_COLOR(pLifted);
    ASSERT(IS_VALID_COLOR(color));
    pv = PIECE_VALUE(pLifted);
    ASSERT(pv > 0);
    pos->iMaterialBalance[color] -= pv;
    pos->iMaterialBalance[FLIP(color)] += pv;
    ASSERT(pos->iMaterialBalance[WHITE] * -1 == pos->iMaterialBalance[BLACK]);
    
    if (IS_PAWN(pLifted))
    {
        ASSERT(pos->cPawns[color][uIndex] == cSquare);
        ASSERT(pv == VALUE_PAWN);
        
        pos->uPawnMaterial[color] -= pv;
        ASSERT(pos->uPawnMaterial[color] <= (7 * VALUE_PAWN));

        pos->u64PawnSig ^= g_u64PawnSigSeeds[cSquare][color];

        //
        // Remove this pawn from the pawn list.
        // 
        pos->uPawnCount[color]--;
        ASSERT(pos->uPawnCount[color] < 8);
        uLastIndex = pos->uPawnCount[color];
        
        //
        // Assume that the pawn being lifted is not the last pawn on
        // the list.  We can't have a hole in the list so swap the
        // last pawn on the list into this guy's spot.
        //
        c = pos->cPawns[color][uLastIndex];
        pos->rgSquare[c].uIndex = uIndex;
        pos->cPawns[color][uIndex] = c;
#ifdef DEBUG
        pos->cPawns[color][uLastIndex] = ILLEGAL_COOR;
#endif
    }
    else 
    {
        //
        // Piece is not a pawn
        //
        ASSERT(pos->cNonPawns[color][uIndex] == cSquare);
        ASSERT(pos->uNonPawnMaterial[color] >= VALUE_KING);
        ASSERT(pv > VALUE_PAWN);
        pos->uNonPawnMaterial[color] -= pv;
        ASSERT(pos->uNonPawnMaterial[color] <= VALUE_MAX_ARMY);
       
        //
        // Remove this piece from the list.
        // 
        pos->uNonPawnCount[color][0]--;
        ASSERT(pos->uNonPawnCount[color] > 0);
        uLastIndex = pos->uNonPawnCount[color][0]; // optimized...
        //
        // Assume that it is NOT the last piece in the list ... swap
        // the last piece into this one's spot.
        // 
        c = pos->cNonPawns[color][uLastIndex];
        pos->rgSquare[c].uIndex = uIndex;
        pos->cNonPawns[color][uIndex] = c;
#ifdef DEBUG
        pos->cNonPawns[color][uLastIndex] = ILLEGAL_COOR;
#endif

        //
        // Change per-piece counters.
        //
        u = PIECE_TYPE(pLifted);
        ASSERT((u >= KNIGHT) && (u < KING));
        pos->u64NonPawnSig ^= g_u64SigSeeds[cSquare][u][color];
        pos->uNonPawnCount[color][u]--;
        ASSERT(pos->uNonPawnCount[color][u] <= 9);
        pos->uWhiteSqBishopCount[color] -= (IS_BISHOP(pLifted) &
                                            IS_SQUARE_WHITE(cSquare));
        ASSERT(pos->uWhiteSqBishopCount[color] <= 9);
    }
    
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
#endif
    return(pLifted);
}


PIECE 
LiftPieceWithoutSigs(POSITION *pos, COOR cSquare)
/**

Routine description:

    Remove the piece at square cSquare from the board.  Update POSITION
    (piece lists, counts, material, etc...) accordingly.

    Note: LiftPieceWithoutSigs can _only_ be called from UnmakeMove.
    It is used to unmake a previously made move and therefore does
    not worry about updating position signatures.  UnmakeMove has
    the pre-move signature in a history list and will klobber the
    position signature soon anyway.
    
Parameters:

    POSITION *pos,
    COOR cSquare

Return value:

    PIECE

**/
{
    register PIECE pLifted;
    ULONG color;
    register SCORE pv;
    ULONG uLastIndex;
    ULONG uIndex;
    COOR c;
    ULONG u;

    ASSERT(IS_ON_BOARD(cSquare));
    pLifted = pos->rgSquare[cSquare].pPiece;
    if (IS_EMPTY(pLifted))
    {
        return(pLifted);                        // ok to return an empty square
    }
    pos->rgSquare[cSquare].pPiece = EMPTY;
    ASSERT(IS_VALID_PIECE(pLifted));
    ASSERT(!IS_KING(pLifted));
    
    uIndex = pos->rgSquare[cSquare].uIndex;
#ifdef DEBUG
    ASSERT(IS_VALID_PIECE_INDEX(uIndex));
    pos->rgSquare[cSquare].uIndex = INVALID_PIECE_INDEX;
#endif
    color = GET_COLOR(pLifted);
    ASSERT(IS_VALID_COLOR(color));
    pv = PIECE_VALUE(pLifted);
    ASSERT(pv > 0);
    pos->iMaterialBalance[color] -= pv;
    pos->iMaterialBalance[FLIP(color)] += pv;
    ASSERT(pos->iMaterialBalance[WHITE] * -1 == pos->iMaterialBalance[BLACK]);
    
    if (IS_PAWN(pLifted))
    {
        ASSERT(pos->cPawns[color][uIndex] == cSquare);
        ASSERT(pv == VALUE_PAWN);
        
        pos->uPawnMaterial[color] -= pv;
        ASSERT(pos->uPawnMaterial[color] <= (7 * VALUE_PAWN));
        
        //
        // Remove this pawn from the pawn list.
        // 
        pos->uPawnCount[color]--;
        ASSERT(pos->uPawnCount[color] < 8);
        uLastIndex = pos->uPawnCount[color];  // optimized...
        
        //
        // Assume that the pawn being lifted is not the last pawn on
        // the list.  We can't have a hole in the list so swap the
        // last pawn on the list into this guy's spot.
        //
        c = pos->cPawns[color][uLastIndex];
        pos->rgSquare[c].uIndex = uIndex;
        pos->cPawns[color][uIndex] = c;
#ifdef DEBUG
        pos->cPawns[color][uLastIndex] = ILLEGAL_COOR;
#endif
    }
    else 
    {
        //
        // Piece is not a pawn
        //
        ASSERT(pos->cNonPawns[color][uIndex] == cSquare);
        ASSERT(pos->uNonPawnMaterial[color] >= VALUE_KING);
        ASSERT(pv > VALUE_PAWN);
        pos->uNonPawnMaterial[color] -= pv;
        ASSERT(pos->uNonPawnMaterial[color] <= VALUE_MAX_ARMY);
       
        //
        // Remove this piece from the list.
        // 
        pos->uNonPawnCount[color][0]--;
        ASSERT(pos->uNonPawnCount[color] > 0);
        uLastIndex = pos->uNonPawnCount[color][0]; // optimized...
        
        //
        // Assume that it is NOT the last piece in the list ... swap
        // the last piece into this one's spot.
        // 
        c = pos->cNonPawns[color][uLastIndex];
        pos->rgSquare[c].uIndex = uIndex;
        pos->cNonPawns[color][uIndex] = c;
#ifdef DEBUG
        pos->cNonPawns[color][uLastIndex] = ILLEGAL_COOR;
#endif

        //
        // Change per-piece counters.
        //
        u = PIECE_TYPE(pLifted);
        ASSERT((u >= KNIGHT) && (u < KING));
        pos->uNonPawnCount[color][u]--;
        ASSERT(pos->uNonPawnCount[color][u] <= 9);
        pos->uWhiteSqBishopCount[color] -= (IS_BISHOP(pLifted) & 
                                            IS_SQUARE_WHITE(cSquare));
        ASSERT(pos->uWhiteSqBishopCount[color] <= 9);
    }
    
    return(pLifted);
}


void 
PlacePiece(POSITION *pos, COOR cSquare, PIECE pPiece)
/**

Routine description:

    Place a piece back on the board.  Link it into the right piece list.

Parameters:

    POSITION *pos,
    COOR cSquare,
    PIECE pPiece

Return value:

    void

**/
{
    register ULONG color = GET_COLOR(pPiece);
    register ULONG uIndex;
    register SCORE pv;
    ULONG u;
    
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
    ASSERT(IS_EMPTY(pos->rgSquare[cSquare].pPiece));
    ASSERT(!IS_KING(pPiece));
#endif
   
    pv = PIECE_VALUE(pPiece);
    pos->iMaterialBalance[color] += pv;
    pos->iMaterialBalance[FLIP(color)] -= pv;
    ASSERT(pos->iMaterialBalance[WHITE] * -1 == pos->iMaterialBalance[BLACK]);
    
    if (IS_PAWN(pPiece))
    {
        //
        // Piece is a pawn
        //
        ASSERT(pv == VALUE_PAWN);
        pos->uPawnMaterial[color] += pv;
        ASSERT(pos->uPawnMaterial[color] <= (8 * VALUE_PAWN));
        
        uIndex = pos->uPawnCount[color];
        ASSERT((uIndex >= 0) && (uIndex <= 7));
        pos->uPawnCount[color]++;
        ASSERT(pos->uPawnCount[color] <= 8);
        pos->cPawns[color][uIndex] = cSquare;
        pos->u64PawnSig ^= g_u64PawnSigSeeds[cSquare][color];
    }
    else
    {
        //
        // Piece is not a pawn
        //
        ASSERT(pv > VALUE_PAWN);
        pos->uNonPawnMaterial[color] += pv;
        ASSERT(pos->uNonPawnMaterial[color] <= VALUE_MAX_ARMY);
        uIndex = pos->uNonPawnCount[color][0];
        pos->uNonPawnCount[color][0]++;
        ASSERT(pos->uNonPawnCount[color][0] <= 16);
        pos->cNonPawns[color][uIndex] = cSquare;
        
        u = PIECE_TYPE(pPiece);
        ASSERT((u >= KNIGHT) && (u < KING));
        pos->u64NonPawnSig ^= g_u64SigSeeds[cSquare][u][color];
        pos->uNonPawnCount[color][u]++;
        ASSERT(pos->uNonPawnCount[color][u] <= 10);
        
        pos->uWhiteSqBishopCount[color] += (IS_BISHOP(pPiece) &
                                            IS_SQUARE_WHITE(cSquare));
        ASSERT(pos->uWhiteSqBishopCount[color] <= 10);
    }

    //
    // Place the piece on the board
    // 
    pos->rgSquare[cSquare].pPiece = pPiece;
    pos->rgSquare[cSquare].uIndex = uIndex;
    
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
#endif
}


void 
PlacePieceWithoutSigs(POSITION *pos, COOR cSquare, PIECE pPiece)
/**

Routine description:

    Place a piece back on the board.  Link it into the right piece list.

    Note: PlacePieceWithoutSigs can _only_ be called from UnmakeMove.
    It is used to unmake a previously made move and therefore does
    not worry about updating position signatures.  UnmakeMove has
    the pre-move signature in a history list and will klobber the
    position signature soon anyway.
    
Parameters:

    POSITION *pos,
    COOR cSquare,
    PIECE pPiece

Return value:

    void

**/
{
    register ULONG color = GET_COLOR(pPiece);
    register ULONG uIndex;
    register SCORE pv;
    ULONG u;
    
    ASSERT(IS_EMPTY(pos->rgSquare[cSquare].pPiece));
    ASSERT(!IS_KING(pPiece));
   
    pv = PIECE_VALUE(pPiece);
    pos->iMaterialBalance[color] += pv;
    pos->iMaterialBalance[FLIP(color)] -= pv;
    ASSERT(pos->iMaterialBalance[WHITE] * -1 == pos->iMaterialBalance[BLACK]);
    
    if (IS_PAWN(pPiece))
    {
        //
        // Piece is a pawn
        //
        ASSERT(pv == VALUE_PAWN);
        pos->uPawnMaterial[color] += pv;
        ASSERT(pos->uPawnMaterial[color] <= (8 * VALUE_PAWN));
        
        uIndex = pos->uPawnCount[color];
        ASSERT((uIndex >= 0) && (uIndex <= 7));
        pos->uPawnCount[color]++;
        ASSERT(pos->uPawnCount[color] <= 8);
        pos->cPawns[color][uIndex] = cSquare;
    }
    else
    {
        //
        // Piece is not a pawn
        //
        ASSERT(pv > VALUE_PAWN);
        pos->uNonPawnMaterial[color] += pv;
        ASSERT(pos->uNonPawnMaterial[color] <= VALUE_MAX_ARMY);
        uIndex = pos->uNonPawnCount[color][0];
        pos->uNonPawnCount[color][0]++;
        ASSERT(pos->uNonPawnCount[color][0] <= 16);
        pos->cNonPawns[color][uIndex] = cSquare;
        
        u = PIECE_TYPE(pPiece);
        ASSERT((u >= KNIGHT) && (u < KING));
        pos->uNonPawnCount[color][u]++;
        ASSERT(pos->uNonPawnCount[color][u] <= 10);
        
        pos->uWhiteSqBishopCount[color] += (IS_BISHOP(pPiece) &
                                            IS_SQUARE_WHITE(cSquare));
        ASSERT(pos->uWhiteSqBishopCount[color] <= 10);
    }

    //
    // Place the piece on the board
    // 
    pos->rgSquare[cSquare].pPiece = pPiece;
    pos->rgSquare[cSquare].uIndex = uIndex;
}


void 
SaveCurrentPositionHistoryInContext(SEARCHER_THREAD_CONTEXT *ctx,
                                    MOVE mv)
/**

Routine description:

    This routine is called just before MakeMove to record some
    information about the current board position to help unmake the
    move later.  It's also used to detect draws in the search.
    
    Note: when this function is called, ply has not been incremented.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    void

**/
{
    ULONG uPly = ctx->uPly;
    POSITION *pos = &(ctx->sPosition);
    PLY_INFO *pi = &ctx->sPlyInfo[uPly];
    
    //
    // Save data about the position we are leaving.
    //
    ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);
    ASSERT((pos->u64NonPawnSig & 1) == pos->uToMove);
    pi->u64NonPawnSig = pos->u64NonPawnSig;
    pi->u64PawnSig = pos->u64PawnSig;
    pi->u64Sig = (pos->u64NonPawnSig ^ pos->u64PawnSig);
    pi->uFifty = pos->uFifty;
    pi->fCastled[BLACK] = pos->fCastled[BLACK];
    pi->fCastled[WHITE] = pos->fCastled[WHITE];
    pi->uTotalNonPawns = (pos->uNonPawnCount[BLACK][0] + 
                          pos->uNonPawnCount[WHITE][0]);
    ASSERT(IS_VALID_FLAG(pos->fCastled[WHITE]) &&
           IS_VALID_FLAG(pos->fCastled[BLACK]));
    ASSERT((pos->bvCastleInfo & ~CASTLE_ALL_POSSIBLE) == 0);
    pi->bvCastleInfo = pos->bvCastleInfo;
    ASSERT(VALID_EP_SQUARE(pos->cEpSquare));
    pi->cEpSquare = pos->cEpSquare;
    (pi+1)->fInQsearch = pi->fInQsearch;
}


static ULONG g_uCastleMask[128] = 
{
    0x7, 0xF, 0xF, 0xF, 0x3, 0xF, 0xF, 0xB,        0,0,0,0,0,0,0,0,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,        0,0,0,0,0,0,0,0,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,        0,0,0,0,0,0,0,0,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,        0,0,0,0,0,0,0,0,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,        0,0,0,0,0,0,0,0,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,        0,0,0,0,0,0,0,0,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,        0,0,0,0,0,0,0,0,
    0xD, 0xF, 0xF, 0xF, 0xC, 0xF, 0xF, 0xE,        0,0,0,0,0,0,0,0
};


FLAG 
MakeMove(SEARCHER_THREAD_CONTEXT *ctx,
         MOVE mv)
/**

Routine description:

    Attempts to make MOVE mv on the POSITION in ctx.  Returns TRUE if
    the move was successful, FALSE if it was illegal.  If the move was
    successful, this function affects the POSITION (piece locations,
    piece lists, castling perms, en-passant square, signatures etc...)
    and also increments the ply in ctx.  These changes can be reverted
    at a later point by using UnmakeMove.
    
    Note: This function only does a very limited amount of legality
    checking in order to save time because it is designed to take
    moves from the move generator (which will only make illegal moves
    in certain, known ways).  If the move is coming from the user
    (and is therefore of dubious legality)) it should be passed to
    MakeUserMove, not MakeMove.  MakeUserMove verifies that it is a
    legal chess move in every way.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx
    MOVE mv

Return value:

    FLAG

**/
{
    POSITION *pos = &ctx->sPosition;
    PIECE pPiece;
    PIECE pCaptured;
    PIECE pPromoted;
    COOR c;
    COOR cRookFrom;
    COOR cRookTo;
    COOR cKing;
    PIECE xPawn;
    FLAG fMoveIsLegal = FALSE;
    static int iSign[2] = { -1, +1 };

#ifdef DEBUG
    cRookFrom = cRookTo = ILLEGAL_COOR;       // Satisfy overzealous compiler
    VerifyPositionConsistency(pos, FALSE);
    SanityCheckMove(pos, mv);
    ASSERT(!InCheck(pos, FLIP(pos->uToMove)));
    if (InCheck(pos, pos->uToMove))
    {
        ASSERT(IS_ESCAPING_CHECK(mv));
    }
#endif
    SaveCurrentPositionHistoryInContext(ctx, mv);
    if (mv.uMove == 0)
    {
        pos->uFifty++;
        goto clear_ep;
    }

    pPiece = mv.pMoved;
    pCaptured = mv.pCaptured;
    ASSERT(!IS_EMPTY(pPiece));
    ASSERT(pPiece == pos->rgSquare[mv.cFrom].pPiece);
    ASSERT(GET_COLOR(pPiece) == pos->uToMove);
    
    //
    // Is this a special move?  Special moves are: promotions, enpassant,
    // castling and double pawn moves.
    // 
    if (IS_SPECIAL_MOVE(mv))
    {
        if (IS_PAWN(pPiece))
        {
            pPromoted = mv.pPromoted;
            if (pPromoted)
            {
                ASSERT(!IS_PAWN(pPromoted));
                ASSERT(GET_COLOR(pPromoted) == GET_COLOR(pPiece));
                pPiece = LiftPiece(pos, mv.cFrom);
                pCaptured = LiftPiece(pos, mv.cTo);
                ASSERT(!pCaptured || (OPPOSITE_COLORS(pCaptured, pPiece)));
                PlacePiece(pos, mv.cTo, pPromoted);
                pos->uFifty = 0;
                goto update_castle_and_clear_ep;
            }
            
            //
            // Is this an enpassant capture?
            // 
            else if (pCaptured)
            {
                ASSERT(mv.cTo == pos->cEpSquare);
                pCaptured = LiftPiece(pos, mv.cTo +
                                      iSign[GET_COLOR(pPiece)] * 16);
                ASSERT(IS_PAWN(pCaptured));
                ASSERT(OPPOSITE_COLORS(pPiece, pCaptured));
                ASSERT(IS_EMPTY(pos->rgSquare[mv.cTo].pPiece));
                ASSERT(IS_PAWN(pPiece));
                SlidePawn(pos, mv.cFrom, mv.cTo);
                pos->uFifty = 0;
                goto clear_ep;
            }
            
            //
            // A special pawn move other than promotion and enpassant
            // capture must be a double initial jump.
            // 
            else
            {
                c = mv.cFrom;
                c += (iSign[GET_COLOR(pPiece)] * -16);
                ASSERT(RANK3(c) || RANK6(c));
                ASSERT(c + (iSign[GET_COLOR(pPiece)] * -16) == mv.cTo);
                ASSERT(IS_EMPTY(pos->rgSquare[c].pPiece));
                ASSERT(IS_EMPTY(pos->rgSquare[mv.cTo].pPiece));
                ASSERT(IS_PAWN(pPiece));
                SlidePawn(pos, mv.cFrom, mv.cTo);
                pos->uFifty = 0;
                
                //
                // Only change the position signature to reflect the
                // possible en-passant if there's an enemy who can
                // take advantage of it.
                //
                xPawn = BLACK_PAWN | FLIP(GET_COLOR(pPiece));
                if (((!IS_ON_BOARD(mv.cTo - 1)) ||
                     (pos->rgSquare[mv.cTo - 1].pPiece != xPawn)) &&
                    ((!IS_ON_BOARD(mv.cTo + 1)) ||
                     (pos->rgSquare[mv.cTo + 1].pPiece != xPawn)))
                {
                    goto clear_ep;
                }
                pos->u64NonPawnSig ^= g_u64EpSigSeeds[FILE(pos->cEpSquare)];
                ASSERT(VALID_EP_SQUARE(c));
                pos->cEpSquare = c;
                pos->u64NonPawnSig ^= g_u64EpSigSeeds[FILE(pos->cEpSquare)];
                goto update_board;
            }
        }
        else if (IS_KING(pPiece))
        {
            ASSERT(!IS_ESCAPING_CHECK(mv));
            
            // 
            // The only special king move is a castle.
            // 
            switch (mv.cTo)
            {
            case C1:
                ASSERT(pos->bvCastleInfo & CASTLE_WHITE_LONG);
                ASSERT(IS_EMPTY(pos->rgSquare[B1].pPiece) &&
                       IS_EMPTY(pos->rgSquare[C1].pPiece) &&
                       IS_EMPTY(pos->rgSquare[D1].pPiece));
                if ((IsAttacked(D1, pos, BLACK)) ||
                    (IsAttacked(C1, pos, BLACK))) 
                {
                    goto illegal;
                }
                cRookFrom = A1;
                cRookTo = D1;
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->bvCastleInfo &= ~(WHITE_CAN_CASTLE);
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->fCastled[WHITE] = TRUE;
                break;
            case G1:
                ASSERT(pos->bvCastleInfo & CASTLE_WHITE_SHORT);
                ASSERT(IS_EMPTY(pos->rgSquare[F1].pPiece) &&
                       IS_EMPTY(pos->rgSquare[G1].pPiece));
                if ((IsAttacked(F1, pos, BLACK)) ||
                    (IsAttacked(G1, pos, BLACK))) 
                {
                    goto illegal;
                }
                cRookFrom = H1;
                cRookTo = F1;
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->bvCastleInfo &= ~(WHITE_CAN_CASTLE);
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->fCastled[WHITE] = TRUE;
                break;
            case C8:
                ASSERT(pos->bvCastleInfo & CASTLE_BLACK_LONG);
                ASSERT(IS_EMPTY(pos->rgSquare[B8].pPiece) &&
                       IS_EMPTY(pos->rgSquare[C8].pPiece) &&
                       IS_EMPTY(pos->rgSquare[D8].pPiece));
                if ((IsAttacked(D8, pos, WHITE)) ||
                    (IsAttacked(C8, pos, WHITE))) 
                {
                    goto illegal;
                }
                cRookFrom = A8;
                cRookTo = D8;
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->bvCastleInfo &= ~(BLACK_CAN_CASTLE);
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->fCastled[BLACK] = TRUE;
                break;
            case G8:
                ASSERT(pos->bvCastleInfo & CASTLE_BLACK_SHORT);
                ASSERT(IS_EMPTY(pos->rgSquare[F8].pPiece) &&
                       IS_EMPTY(pos->rgSquare[G8].pPiece));
                if ((IsAttacked(F8, pos, WHITE)) ||
                    (IsAttacked(G8, pos, WHITE))) 
                {
                    goto illegal;
                }
                cRookFrom = H8;
                cRookTo = F8;
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->bvCastleInfo &= ~(BLACK_CAN_CASTLE);
                pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
                pos->fCastled[BLACK] = TRUE;
                break;
#ifdef DEBUG
            default:
                cRookFrom = cRookTo = ILLEGAL_COOR;
                ASSERT(FALSE);
                goto illegal;
#endif
            }
            ASSERT(IS_EMPTY(pos->rgSquare[mv.cTo].pPiece));
            ASSERT(IS_ROOK(pos->rgSquare[cRookFrom].pPiece));
            ASSERT(IS_EMPTY(pos->rgSquare[cRookTo].pPiece));
            SlidePiece(pos, mv.cFrom, mv.cTo);   // king
            SlidePiece(pos, cRookFrom, cRookTo); // rook
            pos->uFifty = 0;
            goto clear_ep;
        }
#ifdef DEBUG
        else
        {
            ASSERT(FALSE);
            goto illegal;
        }
#endif
    }
    
    //
    // Normal move or normal capture
    //
    pos->uFifty++;
    if (pCaptured)
    {
        pos->uFifty = 0;
        pCaptured = LiftPiece(pos, mv.cTo);
        ASSERT(!IS_KING(pCaptured));
        ASSERT(!IS_EMPTY(pCaptured));
        ASSERT(OPPOSITE_COLORS(pCaptured, pPiece));
    }
    if (IS_PAWN(pPiece)) 
    {
        pos->uFifty = 0;
        SlidePawn(pos, mv.cFrom, mv.cTo);
    }
    else 
    {
        SlidePiece(pos, mv.cFrom, mv.cTo);
    }
    
 update_castle_and_clear_ep:
    pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];
    pos->bvCastleInfo &= g_uCastleMask[mv.cFrom];
    pos->bvCastleInfo &= g_uCastleMask[mv.cTo];
    pos->u64NonPawnSig ^= g_u64CastleSigSeeds[pos->bvCastleInfo];

 clear_ep:
    if (pos->cEpSquare != ILLEGAL_COOR)
    {
        pos->u64NonPawnSig ^= g_u64EpSigSeeds[FILE(pos->cEpSquare)];
        pos->cEpSquare = ILLEGAL_COOR;
        pos->u64NonPawnSig ^= g_u64EpSigSeeds[FILE(pos->cEpSquare)];
    }

 update_board:
    // 
    // At this point we have made the move on the board (moved pieces,
    // updated castling permissions, updated 50 move count, updated
    // signature with respect to piece location etc...)
    // 
    // Now we switch the side on move and flip a bit in the signature
    // to indicate that it's the other side's turn now.  We also
    // record this position in the movelist (for later detection of
    // repetition of position draws) and increment either the ply or
    // move num.
    // 
    pos->u64NonPawnSig ^= 1;
    pos->uToMove = FLIP(pos->uToMove);
    ASSERT((pos->u64NonPawnSig & 1) == pos->uToMove);
    ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH - 1);
    ctx->sPlyInfo[ctx->uPly].mv = mv;
    ctx->uPly++;
    ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);
    
    //
    // Make sure the move doesn't leave his own king in check.
    //
    if (mv.uMove != 0)
    {
        cKing = pos->cNonPawns[FLIP(pos->uToMove)][0];
        ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));

        if (IS_KING(mv.pMoved) || IS_ESCAPING_CHECK(mv))
        {
            //
            // If this guy just moved his king OR he was in check
            // before making this move we need to do a full check
            // check now... this involves using the attack vector
            // table to test ALL pieces in the enemy army and see if
            // they are attacking the just-moved king.
            //
            ASSERT(GET_COLOR(mv.pMoved) == FLIP(pos->uToMove));
            if (InCheck(pos, GET_COLOR(mv.pMoved)))
            {
                goto takeback;
            }
        }

        //
        // else he did not move the king and he was not in check
        // before he made this move.  To determine if he is in check
        // now should be cheaper because all we have to consider is
        // whether the move he just made exposed his king to check.
        // 
        else
        {
            //
            // This call will also catch positions like:
            //
            // 3K4/8/8/8/1k1Pp1R1/8/8/8 w - d3 0 1
            //
            // ... where the black pawn capturing on d3 exposes his
            // own king to the white rook's horizontal attack.
            // Because the pawn captured en passant has already been
            // lifted.
            //
            if (IS_ON_BOARD(ExposesCheck(pos, mv.cFrom, cKing)))
            {
                ASSERT(InCheck(pos, GET_COLOR(mv.pMoved)));
                goto takeback;
            }
            
            //
            // This is a special case -- if the move was enpassant the
            // guy might have exposed his king to check by removing
            // the enemy pawn (i.e. exposes a diagonal attack on his
            // own king)
            // 
            if (IS_ENPASSANT(mv))
            {
                ASSERT(IS_SPECIAL_MOVE(mv));
                ASSERT(!IS_PROMOTION(mv));
                ASSERT(mv.pCaptured);
                ASSERT(IS_PAWN(mv.pMoved));
                ASSERT(IS_PAWN(mv.pCaptured));
                if (IS_ON_BOARD(ExposesCheck(pos,
                                             ((pos->uToMove == WHITE) ?
                                              (mv.cTo + 16) : 
                                              (mv.cTo - 16)), 
                                             cKing)))
                {
                    ASSERT(InCheck(pos, GET_COLOR(mv.pMoved)));
                    goto takeback;
                }
            }
        }
        ASSERT(IS_EMPTY(pos->rgSquare[mv.cFrom].pPiece));
        ASSERT(!IS_EMPTY(pos->rgSquare[mv.cTo].pPiece));
        ASSERT((pos->rgSquare[mv.cTo].pPiece == mv.pMoved) ||
               (pos->rgSquare[mv.cTo].pPiece == mv.pPromoted));
    }
    ASSERT(!InCheck(pos, GET_COLOR(mv.pMoved)));
    fMoveIsLegal = TRUE;
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
#endif
    //
    // Fall into illegal... but don't worry: we're legal!
    //

 illegal:
    return(fMoveIsLegal);
    
 takeback:
    //
    // If we goto here it means that the guy is trying to move into
    // check.  We didn't determine this until after we already made
    // the whole move so we can't just return false (illegal move)
    // from there.  First we have to unmake the move we just made.
    //
    ASSERT(InCheck(pos, FLIP(pos->uToMove)));
    UnmakeMove(ctx, mv);
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
#endif
    return(FALSE);
}


FLAG 
MakeUserMove(SEARCHER_THREAD_CONTEXT *ctx, 
             MOVE mvUser)
/**

Routine description:

    This function should be called to make a move of dubious quality.
    The engine calls MakeMove directly for moves it generated itself
    because these are _mostly_ legal.  The only exceptions being:

        1. moves that expose the friendly king to check
        2. castles through check

    This function is intended for moves from a user or ICS.  It
    screens these moves by first generating available moves at this
    ply and making sure it looks like one of them.  It returns 
    TRUE if the move is legal and was played, FALSE otherwise.

    Both moves made with MakeMove and MakeUserMove can be unmade
    with UnmakeMove.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    MOVE mvUser

Return value:

    FLAG

**/
{
    POSITION *pos = &ctx->sPosition;
    FLAG fInCheck = InCheck(pos, pos->uToMove);
    FLAG fRet = FALSE;
    MOVE mv;
    ULONG x;

    //
    // Ask the generator to give us a list of moves here
    //
    mv.uMove = 0;
    GenerateMoves(ctx, mv, (fInCheck ? GENERATE_ESCAPES : GENERATE_ALL_MOVES));
    for (x = ctx->sMoveStack.uBegin[ctx->uPly];
         x < ctx->sMoveStack.uEnd[ctx->uPly];
         x++)
    {
        mv = ctx->sMoveStack.mvf[x].mv;
        if (IS_SAME_MOVE(mvUser, mv))
        {
            if (TRUE == MakeMove(ctx, mv))
            {
                fRet = TRUE;
                goto end;
            }
        }
    }
    
 end:
    return(fRet);
}


void
UnmakeMove(SEARCHER_THREAD_CONTEXT *ctx, 
           MOVE mv)
/**

Routine description:

    Reverses a move made by MakeMove.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    MOVE mv

Return value:

    void

**/
{
    PIECE pPiece;
    PIECE pCaptured;
    PLY_INFO *pi;
    POSITION *pos = &(ctx->sPosition);
    static int iSign[2] = { -1, +1 };

#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
    ASSERT(IS_ON_BOARD(mv.cTo));
    ASSERT(IS_ON_BOARD(mv.cFrom));
#endif
    if (mv.uMove == 0) 
    {
        goto restore_saved_position_data;
    }
    
    pPiece = mv.pMoved;
    pCaptured = mv.pCaptured;
    if (IS_CASTLE(mv))
    {
        ASSERT(IS_KING(pPiece));
        ASSERT(IS_SPECIAL_MOVE(mv));
        
        switch (mv.cTo)
        {
            case C1:
                //
                // No need to call SlidePiece here, the signatures will
                // get reset to their pre-move state below
                //
                SlidePieceWithoutSigs(pos, D1, A1); // rook
                SlidePieceWithoutSigs(pos, C1, E1); // king
                ASSERT(IS_KING(pos->rgSquare[E1].pPiece));
                ASSERT(IS_ROOK(pos->rgSquare[A1].pPiece));
                break;
            case G1:
                SlidePieceWithoutSigs(pos, F1, H1); // rook
                SlidePieceWithoutSigs(pos, G1, E1); // king
                ASSERT(IS_KING(pos->rgSquare[E1].pPiece));
                ASSERT(IS_ROOK(pos->rgSquare[H1].pPiece));
                break;
            case C8:
                SlidePieceWithoutSigs(pos, D8, A8); // rook
                SlidePieceWithoutSigs(pos, C8, E8); // king
                ASSERT(IS_KING(pos->rgSquare[E8].pPiece));
                ASSERT(IS_ROOK(pos->rgSquare[A8].pPiece));
                break;
            case G8:
                SlidePieceWithoutSigs(pos, F8, H8); // rook
                SlidePieceWithoutSigs(pos, G8, E8); // king
                ASSERT(IS_KING(pos->rgSquare[E8].pPiece));
                ASSERT(IS_ROOK(pos->rgSquare[H8].pPiece));
                break;
#ifdef DEBUG
            default:
                ASSERT(FALSE);
                break;
#endif
        }
    }
    else
    {
        if (IS_PROMOTION(mv))
        {
            ASSERT(IS_SPECIAL_MOVE(mv));
            ASSERT(mv.pPromoted);
            
            //
            // LiftPiece/PlacePiece must be done here to maintain
            // piece list integrity.
            //
            (void)LiftPieceWithoutSigs(pos, mv.cTo);
            pPiece |= 2;
            pPiece &= 3;
            ASSERT(pPiece == mv.pMoved);
            PlacePieceWithoutSigs(pos, mv.cFrom, pPiece);
        }
        else
        {
            if (IS_PAWN(pPiece))
            {
                SlidePawnWithoutSigs(pos, mv.cTo, mv.cFrom);
            }
            else
            {
                SlidePieceWithoutSigs(pos, mv.cTo, mv.cFrom);
            }
        }
        
        if (pCaptured)
        {
            //
            // If this was an enpassant capture, put the pawn back to the
            // right square.
            // 
            if (IS_ENPASSANT(mv))
            {
                ASSERT(IS_SPECIAL_MOVE(mv));
                ASSERT(IS_PAWN(mv.pMoved));
                ASSERT(IS_PAWN(pCaptured));
                ASSERT(IS_ON_BOARD(mv.cTo) + 16);
                ASSERT(IS_ON_BOARD(mv.cTo) - 16);
                
                //
                // Resurrect the dead piece in the enpassant location 
                // where it died (not the same as where the killer moved).
                // 
                ASSERT(IS_EMPTY(pos->rgSquare[mv.cTo + 16 * 
                                       iSign[GET_COLOR(mv.pMoved)]].pPiece));
                PlacePieceWithoutSigs(pos, 
                                     mv.cTo + 16 * iSign[GET_COLOR(mv.pMoved)],
                                      pCaptured);
            }
            else
            {
                //
                // Normal capture... resurrect the dead piece where the
                // killer moved.
                // 
                ASSERT(IS_EMPTY(pos->rgSquare[mv.cTo].pPiece));
                PlacePieceWithoutSigs(pos, mv.cTo, pCaptured);
            }
        }
    }
    
 restore_saved_position_data:
    ctx->uPly--;
    ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);

    pi = &ctx->sPlyInfo[ctx->uPly];
    pos->u64NonPawnSig = pi->u64NonPawnSig;
    pos->u64PawnSig = pi->u64PawnSig;
    ASSERT((pos->u64NonPawnSig & WHITE) == FLIP(pos->uToMove));
    pos->uFifty = pi->uFifty;
    pos->fCastled[WHITE] = pi->fCastled[WHITE];
    pos->fCastled[BLACK] = pi->fCastled[BLACK];
    ASSERT((pi->bvCastleInfo & ~CASTLE_ALL_POSSIBLE) == 0);
    pos->bvCastleInfo = pi->bvCastleInfo;
    ASSERT(VALID_EP_SQUARE(pi->cEpSquare));
    ASSERT(VALID_EP_SQUARE(pos->cEpSquare));
    pos->cEpSquare = pi->cEpSquare;
    pos->uToMove = FLIP(pos->uToMove);
#ifdef DEBUG
    VerifyPositionConsistency(pos, FALSE);
#endif
}


static char *
_MoveFlagsToString(BITV bv)
/**

Routine description:

    Support routing for DumpMove.  Translates bvFlags component of a
    MOVE into a nice string.  Not thread safe.

Parameters:

    BITV bv

Return value:

    char

**/
{
    static char buf[5];
    char *p = buf;
    
    if (bv & MOVE_FLAG_SPECIAL)
    {
        *p++ = 'S';
    }
    if (bv & MOVE_FLAG_ESCAPING_CHECK)
    {
        *p++ = 'E';
    }
    if (bv & MOVE_FLAG_CHECKING)
    {
        *p++ = '+';
    }
    if (bv & MOVE_FLAG_KILLERMATE)
    {
        *p++ = '!';
    }
    *p++ = '\0';
    return(buf);
}


void 
DumpMove(ULONG u)
/**

Routine description:

    Dump a move on stdout.  This function takes a ULONG instead of a
    MOVE becasue both gdb and msdev do not like to pass unions to
    functions invoked at debugtime.

Parameters:

    ULONG u

Return value:

    void

**/
{
    MOVE mv;
    
    mv.uMove = u;
    Trace("--------------------------------------------------------------------------------\n"
          "MOVE:\n"
          "  +0x000 uMove                     : 0x%x\n"
          "  +0x000 cFrom      <pos 0, 8 bits>: 0x%x (%s)\n",
          mv.uMove,
          mv.cFrom, CoorToString(mv.cFrom));
    Trace("  +0x001 cTo        <pos 0, 8 bits>: 0x%x (%s)\n"
          "  +0x002 pMoved     <pos 0, 4 Bits>: 0x%x (%s)\n",
          mv.cTo, CoorToString(mv.cTo),
          mv.pMoved, PieceAbbrev(mv.pMoved));
    Trace("  +0x002 pCaptured  <pos 4, 4 Bits>: 0x%x (%s)\n",
          mv.pCaptured, PieceAbbrev(mv.pCaptured));
    Trace("  +0x003 pPromoted  <pos 0, 4 Bits>: 0x%x (%s)\n",
          mv.pPromoted, PieceAbbrev(mv.pPromoted));
    Trace("  +0x003 bvFlags    <pos 4, 4 Bits>: 0x%x (%s)\n", 
          mv.bvFlags, _MoveFlagsToString(mv.bvFlags));
}
