/**

Copyright (c) Scott Gasch

Module Name:

    generate.c

Abstract:

    This module contains two move generators: a pseudo legal move
    generator and a pseudo legal escaping move generator.  The former
    is called in positions where side on move is not in check and the
    latter is called when the side on move is in check.  Note: both of
    these generators produce pseudo-legal moves.  The normal move
    generator is pretty bad: it does not bother to see if moves expose
    their own king to check or if castles pass through check
    etc... Instead it relies on MakeMove to throw out any illegal
    moves it generates.  The escape generator is better in that it
    produces less illegal moves.  But it is still possible to generate
    some with it.  An example: it will generate replies to check that
    may include capturing moves that are illegal because the capturing
    piece is pinned to the (already in check) king.  Both generators
    produce a set of moves that contains the true legal set of moves in
    a position as a subset; if a move is legal it will be produced.

    This module also contains some code to flag moves as checking
    moves and score moves after they are generated.

Author:

    Scott Gasch (scott.gasch@gmail.com) 11 May 2004

Revision History:

    $Id: generate.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"
#include "psqt.h"

//
// Note: this order is important because of how EvalQueen works
//
const INT g_iQKDeltas[] = {
    -17, -16, -15, -1, +1, +17, +15, +16, 0 };

static void
_FindUnblockedSquares(IN MOVE_STACK *pStack,
                      IN POSITION *pos)
/**

Routine description:

    Given a position, find, count and mark all squares that are
    unblocked for the king of the side not on move.  For example:

       +---+---+---+---+- - -    The squares marked with *'s in this
       |***| K |***|*Q*|         diagram are unblocked for the K.  As
       +---+---+---+---+- - -    you can see, there are nine of them.
       |*P*|***|***|   |
       +---+---+---+---+- - -    Note the piece at the end of an unblocked
       |   |***|   |*P*|         ray from the king is on an unblocked
       +---+---+---+---+- - -    square (and its color doesn't matter)
       |   |*R*|   |   |
       +---+---+---+---+- - -    This stuff is used to flag checking moves
       |   |   |   |   |         as they are generated.
       .   .   .   .   .

    Also note: this is one of the most called routines in the engine;
    speed is of the essence here.

Parameters:

    MOVE_STACK *pStack : move stack pointer
    POSITION *pos : position pointer

Return value:

    void

**/
{
    register ULONG u;
    COOR cKing = pos->cNonPawns[FLIP(pos->uToMove)][0];
    COOR c;
    ULONG uPly = pStack->uPly;
#ifdef DEBUG
    PIECE pKing = pos->rgSquare[cKing].pPiece;
    ULONG uCount = 0;
    COOR cx;

    ASSERT(IS_ON_BOARD(cKing));
    ASSERT(IS_KING(pKing));
    ASSERT(GET_COLOR(pKing) != pos->uToMove);
#endif

    //
    // Adjust the unblocked key value for this ply...
    //
    pStack->uUnblockedKeyValue[uPly]++;
#ifdef DEBUG
    FOREACH_SQUARE(c)
    {
        if (!IS_ON_BOARD(c)) continue;
        ASSERT(pStack->sUnblocked[uPly][c].uKey !=
               pStack->uUnblockedKeyValue[uPly]);
    }
#endif

    u = 0;
    ASSERT(g_iQKDeltas[u] != 0);
    do
    {
        c = cKing + g_iQKDeltas[u];
        while(IS_ON_BOARD(c))
        {
            pStack->sUnblocked[uPly][c].uKey =
                pStack->uUnblockedKeyValue[uPly];
            pStack->sUnblocked[uPly][c].iPointer = -1 * g_iQKDeltas[u];
#ifdef DEBUG
            ASSERT(-g_iQKDeltas[u] == DIRECTION_BETWEEN_SQUARES(c,cKing));
            ASSERT(pStack->sUnblocked[uPly][c].iPointer != 0);
            cx = c;
            do
            {
                cx += pStack->sUnblocked[uPly][c].iPointer;
            }
            while(IS_ON_BOARD(cx) && (IS_EMPTY(pos->rgSquare[cx].pPiece)));
            ASSERT(cx == cKing);
            uCount++;
#endif
            if (!IS_EMPTY(pos->rgSquare[c].pPiece))
            {
                break;
            }
            c += g_iQKDeltas[u];
        }
        u++;
    }
    while(g_iQKDeltas[u] != 0);
    ASSERT(uCount > 2);
    ASSERT(uCount < 28);
}

#define SQUARE_IS_UNBLOCKED(c) (uUnblocked == kp[(c)].uKey)
#define DIR_FROM_SQ_TO_KING(c) (kp[(c)].iPointer)

FLAG
WouldGiveCheck(IN SEARCHER_THREAD_CONTEXT *ctx,
               IN MOVE mv)
/**

Routine description:

    Determine whether a move is a checking move or not.  For this
    function to work properly _FindUnblockedSquares must have been
    called at the start of generation.

    Note: this is one of the most called codepaths in the engine,
    speed is of the essence here.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board
    MOVE mv : the move under consideration

Return value:

    FLAG : TRUE if the move is checking, FALSE otherwise

**/
{
    //
    // TODO: consider adding a hash table to see if sig+mv would
    // give check.  Is this worth it?
    //
    MOVE_STACK *pStack = &(ctx->sMoveStack);
    POSITION *pos = &(ctx->sPosition);
    COOR cTo = mv.cTo;
    COOR cFrom = mv.cFrom;
    PIECE pPiece = mv.pMoved;
    COOR xKing = pos->cNonPawns[FLIP(GET_COLOR(pPiece))][0];
    KEY_POINTER *kp = &(pStack->sUnblocked[ctx->uPly][0]);
    ULONG uUnblocked = pStack->uUnblockedKeyValue[ctx->uPly];
    COOR cEpSquare;
    int iDelta;

    ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);
    ASSERT(PositionsAreEquivalent(&(pStack->board[ctx->uPly]), pos));
    ASSERT(mv.uMove);
    ASSERT(IS_KING(pos->rgSquare[xKing].pPiece));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(pPiece);

    //
    // Phase 1: Does this move directly attack the king?
    //

    //
    // You can't attack one king directly with the other except by
    // castling (in which case the rook is doing the attack).
    //
    if (IS_KING(pPiece))
    {
        if (!IS_SPECIAL_MOVE(mv))
        {
            goto exposes;
        }

        ASSERT((mv.cFrom == E8) || (mv.cFrom == E1));
        pPiece &= ~4;
        ASSERT(!IS_KING(pPiece));
        ASSERT(IS_ROOK(pPiece));
        ASSERT(pPiece == (BLACK_ROOK | GET_COLOR(pPiece)));

        switch(cTo)
        {
            case C1:
                cTo = D1;
                if (SQUARE_IS_UNBLOCKED(E1) &&
                    (DIR_FROM_SQ_TO_KING(E1) == 1))
                {
                    ASSERT(RANK1(xKing));
                    return(MOVE_FLAG_CHECKING);
                }
                break;
            case G1:
                cTo = F1;
                if (SQUARE_IS_UNBLOCKED(E1) &&
                    (DIR_FROM_SQ_TO_KING(E1) == -1))
                {
                    ASSERT(RANK1(xKing));
                    return(MOVE_FLAG_CHECKING);
                }
                break;
            case C8:
                cTo = D8;
                if (SQUARE_IS_UNBLOCKED(E8) &&
                    (DIR_FROM_SQ_TO_KING(E8) == 1))
                {
                    ASSERT(RANK8(xKing));
                    return(MOVE_FLAG_CHECKING);
                }
                break;
            case G8:
                cTo = F8;
                if (SQUARE_IS_UNBLOCKED(E8) &&
                    (DIR_FROM_SQ_TO_KING(E8) == -1))
                {
                    ASSERT(RANK8(xKing));
                    return(MOVE_FLAG_CHECKING);
                }
                break;
#ifdef DEBUG
            default:
                UtilPanic(SHOULD_NOT_GET_HERE,
                          NULL, NULL, NULL, NULL,
                          __FILE__, __LINE__);
                break;
#endif
        }
    }

    //
    // Ok, either its not a king or its a castle move and we are thinking
    // about the rook now.
    //

    //
    // Check the attack vector table
    //
    ASSERT(!IS_KING(pPiece));
    iDelta = 0;
    if (mv.pPromoted)
    {
        ASSERT(IS_PAWN(pPiece));
        ASSERT(GET_COLOR(pPiece) == GET_COLOR(mv.pPromoted));
        pPiece = mv.pPromoted;
        ASSERT(!IS_PAWN(pPiece));
        iDelta = cFrom - cTo;
        ASSERT(iDelta != 0);
    }

    //
    // Does pPiece attack xKing directly if there are not pieces in
    // the way?
    //
    if (0 != (CHECK_VECTOR_WITH_INDEX((int)cTo - (int)xKing,
                                      GET_COLOR(pPiece)) &
              (1 << PIECE_TYPE(pPiece))))
    {
        //
        // We don't toggle knight squares as unblocked... so if the sq
        // is unblocked then piece must not be a knight.  Note: this
        // logical OR replaced with bitwise equivalent for speed.
        //
        if ((SQUARE_IS_UNBLOCKED(cTo)) | (IS_KNIGHT(pPiece)))
        {
#ifdef DEBUG
            if (SQUARE_IS_UNBLOCKED(cTo))
            {
                ASSERT(!IS_KNIGHT(pPiece));
            }
            else
            {
                ASSERT(IS_KNIGHT(pPiece));
            }
#endif
            return(MOVE_FLAG_CHECKING);
        }
        ASSERT(!IS_PAWN(pPiece));

        //
        // Special case: if this is a pawn that just promoted, and the
        // from square was unblocked, this pawn in its new state can
        // check the king.
        //
        if ((DIR_FROM_SQ_TO_KING(cFrom) == iDelta) &&
            (SQUARE_IS_UNBLOCKED(cFrom)))
        {
            return(MOVE_FLAG_CHECKING);
        }
    }

 exposes:
    //
    // We now know that the move itself does not directly attack the
    // enemy king.  We will now see if that move exposes check to the
    // enemy king.
    //

    //
    // A move cannot expose check directly if its from square is not
    // an unblocked square.  But if it is unblocked, we will have to
    // scan behind the piece to see if there is some attacker.
    //
    if (SQUARE_IS_UNBLOCKED(cFrom))
    {
        //
        // Piece moves towards the king on the same ray?  Cannot be
        // check.  Piece moves away from the king on the same ray?
        // Cannot be check.
        //
        if (DIRECTION_BETWEEN_SQUARES(cTo, xKing) !=
            DIR_FROM_SQ_TO_KING(cFrom))
        {
            if (IS_ON_BOARD(FasterExposesCheck(pos, cFrom, xKing)))
            {
                ASSERT(IS_ON_BOARD(ExposesCheck(pos, cFrom, xKing)));
                return(MOVE_FLAG_CHECKING);
            }
        }
    }

    //
    // There is one last special case.  If this move was enpassant
    // it's possible that the removal of the enemy pawn has exposed
    // the enemy king to check.
    //
    // rnb2bnr/ppp3pp/4k3/1B1qPpB1/4p1Q1/8/PPP2PPP/RN2K1NR w  - f6 0 0
    // rn5r/pp4pp/8/5qk1/1b2pP2/4K3/PPP3PP/RN4NR b  - f3 0 0
    //
    if (IS_ENPASSANT(mv))
    {
        ASSERT(IS_PAWN(pPiece));
        ASSERT(VALID_EP_SQUARE(cTo));
        ASSERT(cTo == pos->cEpSquare);
        cEpSquare = cTo + 16 * g_iBehind[(GET_COLOR(pPiece))];
#ifdef DEBUG
        if (GET_COLOR(pPiece))
        {
            ASSERT(cEpSquare == (cTo + 16));
        }
        else
        {
            ASSERT(cEpSquare == (cTo - 16));
        }
#endif
        ASSERT(IS_PAWN(pos->rgSquare[cEpSquare].pPiece));
        ASSERT(OPPOSITE_COLORS(pPiece, pos->rgSquare[cEpSquare].pPiece));

        //
        // Logical OR replaced with bitwise for speed.
        //
        if (((SQUARE_IS_UNBLOCKED(cEpSquare)) |
             (SQUARE_IS_UNBLOCKED(cFrom))) &&
            (IS_ON_BOARD(ExposesCheckEp(pos, cEpSquare, cFrom, cTo, xKing))))
        {
            return(MOVE_FLAG_CHECKING);
        }
    }

    //
    // No direct check + no exposed check = no check.
    //
    return(0);
}


static void FORCEINLINE
_AddNormalMove(IN MOVE_STACK *pStack,
               IN POSITION *pos,
               IN COOR cFrom,
               IN COOR cTo,
               IN PIECE pCap)
/**

Routine description:

    Adds a move from the generator functions to the stack of generated
    moves.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cFrom : the move's from square
    COOR cTo : the move's to square
    PIECE pCap : what was captured (if anything)

Return value:

    static void FORCEINLINE

**/
{
    PIECE pMoved = pos->rgSquare[cFrom].pPiece;
    ULONG uPly = pStack->uPly;
    MOVE_STACK_MOVE_VALUE_FLAGS *pMvf;

    ASSERT(pMoved);
    ASSERT(GET_COLOR(pMoved) == pos->uToMove);
    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));

    pMvf = &(pStack->mvf[pStack->uEnd[uPly]]);
    pMvf->mv.uMove = MAKE_MOVE_WITH_NO_PROM_OR_FLAGS(cFrom, cTo, pMoved, pCap);
    pStack->uEnd[uPly]++;

    ASSERT(pMvf->mv.uMove == MAKE_MOVE(cFrom, cTo, pMoved, pCap, 0, 0));
    ASSERT(SanityCheckMove(pos, pMvf->mv));
}


static void FORCEINLINE
_AddEnPassant(IN MOVE_STACK *pStack,
              IN POSITION *pos,
              IN COOR cFrom,
              IN COOR cTo)
/**

Routine description:

    Add an en passant pawn capture to the move stack.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cFrom : the from square
    COOR cTo : the to square (and en passant square)

Return value:

    static void

**/
{
    PIECE pMoved = BLACK_PAWN | pos->uToMove;
    PIECE pCaptured = FLIP(pMoved);
    ULONG uPly = pStack->uPly;
    MOVE_STACK_MOVE_VALUE_FLAGS *pMvf;

    ASSERT(pCaptured == (BLACK_PAWN | (FLIP(pos->uToMove))));
    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(RANK4(cFrom) || RANK5(cFrom));
    ASSERT(VALID_EP_SQUARE(cTo));
    ASSERT(IS_PAWN(pos->rgSquare[cFrom].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cFrom].pPiece) == pos->uToMove);

    pMvf = &(pStack->mvf[pStack->uEnd[uPly]]);
    pMvf->mv.uMove =
        MAKE_MOVE(cFrom, cTo, pMoved, pCaptured, 0, MOVE_FLAG_SPECIAL);
    pStack->uEnd[uPly]++;

    ASSERT(SanityCheckMove(pos, pMvf->mv));
}


static void FORCEINLINE
_AddCastle(IN MOVE_STACK *pStack,
           IN POSITION *pos,
           IN COOR cFrom,
           IN COOR cTo)
/**

Routine description:

    Add a castle move to the move stack.

Parameters:

    MOVE_STACK *pStack,
    POSITION *pos,
    COOR cFrom,
    COOR cTo

Return value:

    static void FORCEINLINE

**/
{
    PIECE pMoved = BLACK_KING | pos->uToMove;
    ULONG uPly = pStack->uPly;
    MOVE_STACK_MOVE_VALUE_FLAGS *pMvf;

    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT((E1 == cFrom) || (E8 == cFrom));
    ASSERT(IS_KING(pos->rgSquare[cFrom].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cFrom].pPiece) == pos->uToMove);

    pMvf = &(pStack->mvf[pStack->uEnd[uPly]]);
    pMvf->mv.uMove = MAKE_MOVE(cFrom, cTo, pMoved, 0, 0, MOVE_FLAG_SPECIAL);
    pStack->uEnd[uPly]++;

    ASSERT(SanityCheckMove(pos, pMvf->mv));
}


static void FORCEINLINE
_AddPromote(IN MOVE_STACK *pStack,
            IN POSITION *pos,
            IN COOR cFrom,
            IN COOR cTo)
/**

Routine description:

    Adds a pawn promotion (set of) moves to the move stack.  This can
    be a capture-promotion or non-capture promotion.  This function
    pushes several moves onto the move stack, one for each possible
    promotion target.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cFrom : the move's from square
    COOR cTo : the move's to square

Return value:

    static void

**/
{
    PIECE pMoved = BLACK_PAWN | pos->uToMove;
    PIECE pCaptured = pos->rgSquare[cTo].pPiece;
    ULONG uPly = pStack->uPly;
    MOVE_STACK_MOVE_VALUE_FLAGS *pMvf;
    ULONG u;

#define ARRAY_LENGTH_TARG (4)
    const static PIECE pTarg[ARRAY_LENGTH_TARG] = { BLACK_QUEEN,
                                                    BLACK_ROOK,
                                                    BLACK_BISHOP,
                                                    BLACK_KNIGHT };

    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(RANK1(cTo) || RANK8(cTo));
    ASSERT(RANK7(cFrom) || RANK2(cFrom));
    ASSERT(ARRAY_LENGTH(pTarg) == ARRAY_LENGTH_TARG);
    ASSERT(IS_PAWN(pos->rgSquare[cFrom].pPiece));

    for (u = 0; u < ARRAY_LENGTH_TARG; u++)
    {
        pMvf = &(pStack->mvf[pStack->uEnd[uPly]]);
        pMvf->mv.uMove = MAKE_MOVE(cFrom, cTo, pMoved, pCaptured,
                                   pTarg[u] | pos->uToMove, MOVE_FLAG_SPECIAL);
        pStack->uEnd[uPly]++;
        ASSERT(SanityCheckMove(pos, pMvf->mv));
    }
#undef ARRAY_LENGTH_TARG
}


static void FORCEINLINE
_AddDoubleJump(IN MOVE_STACK *pStack,
               IN POSITION *pos,
               IN COOR cFrom,
               IN COOR cTo)
/**

Routine description:

    This function adds a pawn double jump to the move stack.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cFrom : the move's from square
    COOR cTo : the move's to square

Return value:

    static void

**/
{
    PIECE pMoved = BLACK_PAWN | pos->uToMove;
    ULONG uPly = pStack->uPly;
    MOVE_STACK_MOVE_VALUE_FLAGS *pMvf;

    ASSERT(IS_ON_BOARD(cFrom));
    ASSERT(IS_ON_BOARD(cTo));
    ASSERT(RANK2(cFrom) || RANK7(cFrom));
    ASSERT(RANK4(cTo) || RANK5(cTo));
    ASSERT(IS_PAWN(pos->rgSquare[cFrom].pPiece));

    pMvf = &(pStack->mvf[pStack->uEnd[uPly]]);
    pMvf->mv.uMove = MAKE_MOVE(cFrom, cTo, pMoved, 0, 0, MOVE_FLAG_SPECIAL);
    pStack->uEnd[uPly]++;

    ASSERT(SanityCheckMove(pos, pMvf->mv));
}


const INT g_iNDeltas[] = {
    -33, -31,  -18, -14, +14, +18, +31, +33,  0 };

void
GenerateKnight(IN MOVE_STACK *pStack,
               IN POSITION *pos,
               IN COOR cKnight)
/**

Routine description:

    This function is called by GenerateMoves' JumpTable.  Its job
    is to generate and push pseudo-legal knight moves for the
    knight sitting on square cKnight.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cKnight : the knight's location

Return value:

    void

**/
{
    COOR c;
    ULONG u = 0;
    PIECE p;

    ASSERT(IS_KNIGHT(pos->rgSquare[cKnight].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cKnight].pPiece) == pos->uToMove);
    ASSERT(g_iNDeltas[u] != 0);

    do
    {
        c = cKnight + g_iNDeltas[u];
        u++;
        if (IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;

            //
            // This logical OR replaced with bitwise OR; the effect is
            // the same the the bitwise is marginally faster.
            //
            if (IS_EMPTY(p) | (OPPOSITE_COLORS(p, pos->uToMove)))
            {
                _AddNormalMove(pStack, pos, cKnight, c, p);
            }
        }
    }
    while(0 != g_iNDeltas[u]);
}

void
GenerateWhiteKnight(IN MOVE_STACK *pStack,
                    IN POSITION *pos,
                    IN COOR cKnight)
{
    COOR c;
    ULONG u = 0;
    PIECE p;

    ASSERT(IS_KNIGHT(pos->rgSquare[cKnight].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cKnight].pPiece) == pos->uToMove);
    ASSERT(g_iNDeltas[u] != 0);

    do
    {
        c = cKnight + g_iNDeltas[u];
        u++;
        if (IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;

            //
            // Note: this test covers empty squares also -- it is just
            // testing that the low order bit (color bit) of p is zero
            // which is true for black pieces and empty squares.  This
            // is done for speed over readability.
            //
            if (GET_COLOR(p) == BLACK)
            {
                ASSERT(IS_EMPTY(p) || (GET_COLOR(p) == BLACK));
                _AddNormalMove(pStack, pos, cKnight, c, p);
            }
        }
    }
    while(0 != g_iNDeltas[u]);
}

//
// These logical AND/ORs replaced with bitwise AND/OR; the effect is
// the same the the bitwise is marginally faster.
//
#define BLOCKS_THE_CHECK(sq) \
    ((iAttackDelta != 0) & \
     (iAttackDelta == DIRECTION_BETWEEN_SQUARES(cAttacker, (sq))) & \
     (((cAttacker > cKing) & ((sq) > cKing)) | \
      ((cAttacker < cKing) & ((sq) < cKing))))

void
SaveMeKnight(IN MOVE_STACK *pStack,
             IN POSITION *pos,
             IN COOR cKnight,
             IN COOR cKing,
             IN COOR cAttacker)
/**

Routine description:

    This routine is called by GenerateEscapes' JumpTable.  Its job is
    to generate moves by the knight on square cKnight that alleviate
    check to the king on square cKing.  The (lone) piece attacking the
    king is on square cAttacker.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cKnight : the location of the knight
    COOR cKing : the location of the friendly king
    COOR cAttacker : the location of the checking piece

Return value:

    void

**/
{
    COOR c, cExposed;
    int iAttackDelta = DIRECTION_BETWEEN_SQUARES(cAttacker, cKing);
    ULONG u = 0;
    PIECE p;

    ASSERT(InCheck(pos, pos->uToMove));
    ASSERT(IS_KNIGHT(pos->rgSquare[cKnight].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cKnight].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cKnight].pPiece) ==
           GET_COLOR(pos->rgSquare[cKing].pPiece));
    ASSERT(OPPOSITE_COLORS(pos->rgSquare[cAttacker].pPiece,
                           pos->rgSquare[cKing].pPiece));
    ASSERT(g_iNDeltas[u] != 0);

    do
    {
        c = cKnight + g_iNDeltas[u];
        u++;
        if (IS_ON_BOARD(c))
        {
            if ((c == cAttacker) || BLOCKS_THE_CHECK(c))
            {
                cExposed = ExposesCheck(pos, cKnight, cKing);
                if (!IS_ON_BOARD(cExposed) || (cExposed == c))
                {
                    p = pos->rgSquare[c].pPiece;
                    ASSERT(!p ||
                           OPPOSITE_COLORS(p, pos->rgSquare[cKnight].pPiece));
                    ASSERT(!p || (c == cAttacker));
                    _AddNormalMove(pStack, pos, cKnight, c, p);
                }
            }
        }
    }
    while(0 != g_iNDeltas[u]);
}


const INT g_iBDeltas[] = {
    -17, -15, +15, +17, 0 };

void
GenerateBishop(IN MOVE_STACK *pStack,
               IN POSITION *pos,
               IN COOR cBishop)
/**

Routine description:

    This routine is called by GenerateMoves' JumpTable to generate
    pseudo-legal bishop moves by the piece at cBishop.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cBishop : the bishop location

Return value:

    void

**/
{
    COOR c;
    PIECE p;

    ASSERT(IS_BISHOP(pos->rgSquare[cBishop].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cBishop].pPiece) == pos->uToMove);

    //
    // Note: manually unrolled loop
    //
    c = cBishop - 17;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cBishop, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cBishop, c, p);
            }
            break;
        }
        c += -17;
    }

    c = cBishop - 15;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cBishop, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cBishop, c, p);
            }
            break;
        }
        c += -15;
    }

    c = cBishop + 15;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cBishop, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cBishop, c, p);
            }
            break;
        }
        c += 15;
    }

    c = cBishop + 17;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cBishop, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cBishop, c, p);
            }
            break;
        }
        c += 17;
    }
}


void
SaveMeBishop(IN MOVE_STACK *pStack,
             IN POSITION *pos,
             IN COOR cBishop,
             IN COOR cKing,
             IN COOR cAttacker)
/**

Routine description:

    This routine is called by GenerateEscapes' JumpTable in order to
    generate moves by the bishop at cBishop to alleviate check on its
    friendly king (at cKing) which is being checked by a piece at
    cAttacker.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cBishop : the bishop's location
    COOR cKing : the friendly king's location
    COOR cAttacker : the checking piece's location

Return value:

    void

**/
{
    int iAttackDelta = DIRECTION_BETWEEN_SQUARES(cAttacker, cKing);
    COOR c;
    COOR cExposed;
    ULONG u = 0;
    PIECE p;

    ASSERT(InCheck(pos, pos->uToMove));
    ASSERT(IS_BISHOP(pos->rgSquare[cBishop].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cBishop].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cBishop].pPiece) ==
           GET_COLOR(pos->rgSquare[cKing].pPiece));
    ASSERT(OPPOSITE_COLORS(pos->rgSquare[cAttacker].pPiece,
                           pos->rgSquare[cKing].pPiece));
    ASSERT(g_iBDeltas[u] != 0);

    do
    {
        c = cBishop + g_iBDeltas[u];
        while(IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;
            if ((c == cAttacker) || BLOCKS_THE_CHECK(c))
            {
                ASSERT(!p ||
                       OPPOSITE_COLORS(p, pos->rgSquare[cBishop].pPiece));
                ASSERT(!p || (c == cAttacker));
                cExposed = ExposesCheck(pos, cBishop, cKing);
                if (!IS_ON_BOARD(cExposed) || (cExposed == c))
                {
                    _AddNormalMove(pStack, pos, cBishop, c, p);
                    break;
                }
            }
            else if (!IS_EMPTY(p))
            {
                break;
            }
            c += g_iBDeltas[u];
        }
        u++;
    }
    while(0 != g_iBDeltas[u]);
}


const INT g_iRDeltas[] = {
    -1, +1, +16, -16, 0 };


void
GenerateRook(IN MOVE_STACK *pStack,
             IN POSITION *pos,
             IN COOR cRook)
/**

Routine description:

    This routine is called by GenerateMoves' JumpTable in order to
    generate pseudo-legal moves for the rook at position cRook.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cRook : the rook's location

Return value:

    void

**/
{
    COOR c;
    PIECE p;

    ASSERT(IS_ROOK(pos->rgSquare[cRook].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cRook].pPiece) == pos->uToMove);

    //
    // Note: manually unrolled loop
    //
    c = cRook - 1;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cRook, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cRook, c, p);
            }
            break;
        }
        c += -1;
    }

    c = cRook + 1;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cRook, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cRook, c, p);
            }
            break;
        }
        c += 1;
    }

    c = cRook + 16;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cRook, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cRook, c, p);
            }
            break;
        }
        c += 16;
    }

    c = cRook - 16;
    while(IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (IS_EMPTY(p))
        {
            _AddNormalMove(pStack, pos, cRook, c, 0);
        }
        else
        {
            if (OPPOSITE_COLORS(p, pos->uToMove))
            {
                _AddNormalMove(pStack, pos, cRook, c, p);
            }
            break;
        }
        c += -16;
    }
}

void
SaveMeRook(IN MOVE_STACK *pStack,
           IN POSITION *pos,
           IN COOR cRook,
           IN COOR cKing,
           IN COOR cAttacker)
/**

Routine description:

    This routine is called by GenerateEscapes' JumpTable in order to
    generate moves by the rook at location cRook that alleviate check
    to the king at cKing.  The lone checking piece is at cAttacker.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cRook : the rook's location
    COOR cKing : the king's location
    COOR cAttacker : the checker's location

Return value:

    void

**/
{
    int iAttackDelta = DIRECTION_BETWEEN_SQUARES(cAttacker, cKing);
    COOR c;
    COOR cExposed;
    ULONG u = 0;
    PIECE p;

    ASSERT(InCheck(pos, pos->uToMove));
    ASSERT(IS_ROOK(pos->rgSquare[cRook].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cRook].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cRook].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cRook].pPiece) ==
           GET_COLOR(pos->rgSquare[cKing].pPiece));
    ASSERT(OPPOSITE_COLORS(pos->rgSquare[cAttacker].pPiece,
                           pos->rgSquare[cKing].pPiece));
    ASSERT(g_iRDeltas[u] != 0);

    do
    {
        c = cRook + g_iRDeltas[u];
        while(IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;
            if ((c == cAttacker) || BLOCKS_THE_CHECK(c))
            {
                ASSERT(!p ||
                       OPPOSITE_COLORS(p, pos->rgSquare[cRook].pPiece));
                ASSERT(!p || (c == cAttacker));
                cExposed = ExposesCheck(pos, cRook, cKing);
                if (!IS_ON_BOARD(cExposed) || (cExposed == c))
                {
                    _AddNormalMove(pStack, pos, cRook, c, p);
                    break;
                }
            }
            else if (!IS_EMPTY(p))
            {
                break;
            }
            c += g_iRDeltas[u];
        }
        u++;
    }
    while(0 != g_iRDeltas[u]);
}


void
GenerateQueen(IN MOVE_STACK *pStack,
              IN POSITION *pos,
              IN COOR cQueen)
/**

Routine description:

    This routine is called by GenerateMoves' JumpTable in order to
    generate pseudo-legal moves for the queen at position cQueen.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cQueen : the queen's location

Return value:

    void

**/
{
    COOR c;
    ULONG u = 0;
    PIECE p;

    ASSERT(IS_QUEEN(pos->rgSquare[cQueen].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cQueen].pPiece) == pos->uToMove);
    ASSERT(g_iQKDeltas[u] != 0);

    do
    {
        c = cQueen + g_iQKDeltas[u];
        while(IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;
            if (IS_EMPTY(p))
            {
                _AddNormalMove(pStack, pos, cQueen, c, 0);
            }
            else
            {
                if (OPPOSITE_COLORS(p, pos->uToMove))
                {
                    _AddNormalMove(pStack, pos, cQueen, c, p);
                }
                break;
            }
            c += g_iQKDeltas[u];
        }
        u++;
    }
    while(0 != g_iQKDeltas[u]);
}


void
SaveMeQueen(IN MOVE_STACK *pStack,
            IN POSITION *pos,
            IN COOR cQueen,
            IN COOR cKing,
            IN COOR cAttacker)
/**

Routine description:

    This routine is called by code in GenerateEscapes in order to
    generate moves by the queen at square cQueen that alleviate check
    on the friendly king at square cKing.  The lone enemy piece that
    is checking the king is sitting on square cAttacker.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cQueen : the queen's location
    COOR cKing : the friendly king's location
    COOR cAttacker : the attacker's location

Return value:

    void

**/
{
    COOR c;
    COOR cExposed;
    int iAttackDelta = DIRECTION_BETWEEN_SQUARES(cAttacker, cKing);
    ULONG u = 0;
    PIECE p;

    ASSERT(InCheck(pos, pos->uToMove));
    ASSERT(IS_QUEEN(pos->rgSquare[cQueen].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cQueen].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cQueen].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cQueen].pPiece) ==
           GET_COLOR(pos->rgSquare[cKing].pPiece));
    ASSERT(OPPOSITE_COLORS(pos->rgSquare[cAttacker].pPiece,
                           pos->rgSquare[cKing].pPiece));
    ASSERT(g_iQKDeltas[u] != 0);

    do
    {
        c = cQueen + g_iQKDeltas[u];
        while(IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;
            if ((c == cAttacker) || BLOCKS_THE_CHECK(c))
            {
                ASSERT(!p ||
                       OPPOSITE_COLORS(p, pos->rgSquare[cQueen].pPiece));
                ASSERT(!p || (c == cAttacker));
                cExposed = ExposesCheck(pos, cQueen, cKing);
                if (!IS_ON_BOARD(cExposed) || (cExposed == c))
                {
                    _AddNormalMove(pStack, pos, cQueen, c, p);
                    break;
                }
            }
            else if (!IS_EMPTY(p))
            {
                break;
            }
            c += g_iQKDeltas[u];
        }
        u++;
    }
    while(0 != g_iQKDeltas[u]);
}

void
GenerateBlackKing(IN MOVE_STACK *pStack,
                  IN POSITION *pos,
                  IN COOR cKing)
/**

Routine description:

    This function is called by GenerateMoves in order to generate
    pseudo-legal moves for the black king at square cKing.  Note: this
    function often generates illegal castling moves and relies on the
    MakeMove code to decide about the legality of the castle.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cKing : the king location

Return value:

    void

**/
{
    COOR c;
    ULONG u = 0;
    PIECE p;

    ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cKing].pPiece) == pos->uToMove);
    ASSERT(g_iQKDeltas[u] != 0);

    do
    {
        c = cKing + g_iQKDeltas[u];
        u++;
        if (IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;

            //
            // This logical OR replaced with bitwise OR; the effect is
            // the same the the bitwise is marginally faster.
            //
            if (IS_EMPTY(p) | (GET_COLOR(p)))
            {
                _AddNormalMove(pStack, pos, cKing, c, p);
            }
        }
    }
    while(0 != g_iQKDeltas[u]);

    if ((pos->bvCastleInfo & BLACK_CAN_CASTLE) == 0) return;
#ifdef DEBUG
    p = pos->rgSquare[E8].pPiece;
    ASSERT(IS_KING(p));
    ASSERT(cKing == E8);
#endif

    //
    // Note: no castling through check is enforced at the time the
    // move is played.  This is a "pseudo-legal" move when generated.
    //
    if ((pos->bvCastleInfo & CASTLE_BLACK_SHORT) &&
        (IS_EMPTY(pos->rgSquare[G8].pPiece)) &&
        (IS_EMPTY(pos->rgSquare[F8].pPiece)))
    {
        ASSERT(pos->rgSquare[H8].pPiece == BLACK_ROOK);
        _AddCastle(pStack, pos, E8, G8);
    }

    if ((pos->bvCastleInfo & CASTLE_BLACK_LONG) &&
        (IS_EMPTY(pos->rgSquare[C8].pPiece)) &&
        (IS_EMPTY(pos->rgSquare[D8].pPiece)) &&
        (IS_EMPTY(pos->rgSquare[B8].pPiece)))
    {
        ASSERT(pos->rgSquare[A8].pPiece == BLACK_ROOK);
        _AddCastle(pStack, pos, E8, C8);
    }
}

//
// N.B. There is no SaveYourselfBlackKing or SaveYourselfWhiteKing
// because this functionality is built into phase 1 of
// GenerateEscapes.
//

void
GenerateWhiteKing(IN MOVE_STACK *pStack,
                  IN POSITION *pos,
                  IN COOR cKing)
/**

Routine description:

    This function is called by GenerateMoves in order to generate
    pseudo-legal king moves by the white king at square cKing.  Note:
    it often generates illegal castling moves and relies on the code
    in MakeMove to determine the legality of castles.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cKing : the king location

Return value:

    void

**/
{
    COOR c;
    ULONG u = 0;
    PIECE p;

    ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cKing].pPiece) == pos->uToMove);
    ASSERT(g_iQKDeltas[u] != 0);

    do
    {
        c = cKing + g_iQKDeltas[u];
        u++;
        if (IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;

            //
            // Note: this test covers empty squares also -- it is just
            // testing that the low order bit (color bit) of p is zero
            // which is true for black pieces and empty squares.  This
            // is done for speed over readability.
            //
            if (GET_COLOR(p) == BLACK)
            {
                ASSERT(IS_EMPTY(p) || (GET_COLOR(p) == BLACK));
                _AddNormalMove(pStack, pos, cKing, c, p);
            }
        }
    }
    while(0 != g_iQKDeltas[u]);

    if ((pos->bvCastleInfo & WHITE_CAN_CASTLE) == 0) return;
#ifdef DEBUG
    p = pos->rgSquare[E1].pPiece;
    ASSERT(IS_KING(p));
    ASSERT(cKing == E1);
#endif

    //
    // Note: no castling through check is enforced at the time the
    // move is played.  This is a "pseudo-legal" move when generated.
    //
    if ((pos->bvCastleInfo & CASTLE_WHITE_SHORT) &&
        (IS_EMPTY(pos->rgSquare[G1].pPiece)) &&
        (IS_EMPTY(pos->rgSquare[F1].pPiece)))
    {
        ASSERT(pos->rgSquare[H1].pPiece == WHITE_ROOK);
        _AddCastle(pStack, pos, E1, G1);
    }

    if ((pos->bvCastleInfo & CASTLE_WHITE_LONG) &&
        (IS_EMPTY(pos->rgSquare[C1].pPiece)) &&
        (IS_EMPTY(pos->rgSquare[B1].pPiece)) &&
        (IS_EMPTY(pos->rgSquare[D1].pPiece)))
    {
        ASSERT(pos->rgSquare[A1].pPiece == WHITE_ROOK);
        _AddCastle(pStack, pos, E1, C1);
    }
}


void
GenerateWhitePawn(IN MOVE_STACK *pStack,
                  IN POSITION *pos,
                  IN COOR cPawn)
/**

Routine description:

    This function is called by GenerateMoves in order to generate
    pseudo-legal pawn moves by the pawn at cPawn.  It handles
    en passant captures, double jumps, promotions, etc...

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cPawn : the pawn's location

Return value:

    void

**/
{
    ULONG uRank = RANK(cPawn);
    PIECE p;
    COOR cTo;

    ASSERT(IS_PAWN(pos->rgSquare[cPawn].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == pos->uToMove);
    ASSERT((RANK(cPawn) >= 2) && (RANK(cPawn) <= 7));

    if (uRank != 7)
    {
        cTo = cPawn - 16;
        if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            _AddNormalMove(pStack, pos, cPawn, cTo, 0);
            if (uRank == 2)
            {
                cTo = cPawn - 32;
                if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
                {
                    _AddDoubleJump(pStack, pos, cPawn, cTo);
                }
            }
        }
        cTo = cPawn - 15;
        if (IS_ON_BOARD(cTo))
        {
            if (cTo == pos->cEpSquare)
            {
                _AddEnPassant(pStack, pos, cPawn, cTo);
            }
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p) == BLACK))
            {
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }
        cTo = cPawn - 17;
        if (IS_ON_BOARD(cTo))
        {
            if (cTo == pos->cEpSquare)
            {
                _AddEnPassant(pStack, pos, cPawn, cTo);
            }
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p) == BLACK))
            {
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }
    }
    else
    {
        ASSERT(RANK7(cPawn));
        cTo = cPawn - 16;
        if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            _AddPromote(pStack, pos, cPawn, cTo);
        }
        cTo = cPawn - 15;
        if (IS_ON_BOARD(cTo))
        {
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p) == BLACK))
            {
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }
        cTo = cPawn - 17;
        if (IS_ON_BOARD(cTo))
        {
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p) == BLACK))
            {
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }
    }
}


void
SaveMeWhitePawn(IN MOVE_STACK *pStack,
                IN POSITION *pos,
                IN COOR cPawn,
                IN COOR cKing,
                IN COOR cAttacker)
/**

Routine description:

    This function is called by the GenerateEscapes code in order to
    ask for a pawn's help in alleviating check on the king at square
    cKing.  The lone attacker to said king is on square cAttacker.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cPawn : the pawn's location
    COOR cKing : the friendly king's location
    COOR cAttacker : the location of the lone checker to the king

Return value:

    void

**/
{
    ULONG uRank = RANK(cPawn);
    PIECE p;
    COOR cTo;
    COOR cExposed;
    int iAttackDelta = DIRECTION_BETWEEN_SQUARES(cAttacker, cKing);

    ASSERT(InCheck(pos, pos->uToMove));
    ASSERT(IS_PAWN(pos->rgSquare[cPawn].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == WHITE);
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) ==
           GET_COLOR(pos->rgSquare[cKing].pPiece));
    ASSERT(OPPOSITE_COLORS(pos->rgSquare[cAttacker].pPiece,
                           pos->rgSquare[cKing].pPiece));
    ASSERT((RANK(cPawn) >= 2) && (RANK(cPawn) <= 7));

    if (uRank != 7)
    {
        cTo = cPawn - 16;
        if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            if (BLOCKS_THE_CHECK(cTo) &&
                !IS_ON_BOARD(ExposesCheck(pos, cPawn, cKing)))
            {
                    _AddNormalMove(pStack, pos, cPawn, cTo, 0);
            }

            if (uRank == 2)
            {
                cTo = cPawn - 32;
                if (IS_EMPTY(pos->rgSquare[cTo].pPiece) &&
                    BLOCKS_THE_CHECK(cTo) &&
                    !IS_ON_BOARD(ExposesCheck(pos, cPawn, cKing)))
                {
                    _AddDoubleJump(pStack, pos, cPawn, cTo);
                }
            }
        }

        cTo = cPawn - 15;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == BLACK);
                ASSERT(IS_ON_BOARD(cTo));
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }

        //
        // N.B. There is no possible way to block check with an
        // en-passant capture because by definition the last move made
        // by the other side was the pawn double jump.  So only handle
        // if the double jumping enemy pawn is the attacker and we can
        // kill it en passant.
        //
        if ((cTo == pos->cEpSquare) && (cAttacker == cTo + 16))
        {
            _AddEnPassant(pStack, pos, cPawn, cTo);
        }

        cTo = cPawn - 17;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == BLACK);
                ASSERT(IS_ON_BOARD(cTo));
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }
        if ((cTo == pos->cEpSquare) && (cAttacker == cTo + 16))
        {
            _AddEnPassant(pStack, pos, cPawn, cTo);
        }
    }
    else
    {
        ASSERT(RANK7(cPawn));
        cTo = cPawn - 16;
        if (BLOCKS_THE_CHECK(cTo) && IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            _AddPromote(pStack, pos, cPawn, cTo);
        }

        cTo = cPawn - 15;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == BLACK);
                ASSERT(IS_ON_BOARD(cTo));
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }

        cTo = cPawn - 17;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == BLACK);
                ASSERT(IS_ON_BOARD(cTo));
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }
    }
}


void
GenerateBlackPawn(IN MOVE_STACK *pStack,
                  IN POSITION *pos,
                  IN COOR cPawn)
/**

Routine description:

    This code is called by GenerateMoves in order to handle move
    generation for the black pawn on square cPawn.  It handles
    en passant captures, double jumps, and promotion.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the position
    COOR cPawn : the pawn location

Return value:

    void

**/
{
    ULONG uRank = RANK(cPawn);
    PIECE p;
    COOR cTo;

    ASSERT(IS_PAWN(pos->rgSquare[cPawn].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == pos->uToMove);
    ASSERT((RANK(cPawn) >= 2) && (RANK(cPawn) <= 7));

    if (uRank != 2)
    {
        cTo = cPawn + 16;
        if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            _AddNormalMove(pStack, pos, cPawn, cTo, 0);
            if (uRank == 7)
            {
                cTo = cPawn + 32;
                if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
                {
                    _AddDoubleJump(pStack, pos, cPawn, cTo);
                }
            }
        }
        cTo = cPawn + 15;
        if (IS_ON_BOARD(cTo))
        {
            if (cTo == pos->cEpSquare)
            {
                _AddEnPassant(pStack, pos, cPawn, cTo);
            }
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p)))
            {
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }
        cTo = cPawn + 17;
        if (IS_ON_BOARD(cTo))
        {
            if (cTo == pos->cEpSquare)
            {
                _AddEnPassant(pStack, pos, cPawn, cTo);
            }
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p)))
            {
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }
    }
    else
    {
        ASSERT(RANK2(cPawn));
        cTo = cPawn + 16;
        if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            _AddPromote(pStack, pos, cPawn, cTo);
        }
        cTo = cPawn + 15;
        if (IS_ON_BOARD(cTo))
        {
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p)))
            {
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }
        cTo = cPawn + 17;
        if (IS_ON_BOARD(cTo))
        {
            p = pos->rgSquare[cTo].pPiece;

            //
            // This logical AND replaced with bitwise AND; the effect
            // is the same the the bitwise is marginally faster.
            //
            if (!IS_EMPTY(p) & (GET_COLOR(p)))
            {
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }
    }
}


void
SaveMeBlackPawn(IN MOVE_STACK *pStack,
                IN POSITION *pos,
                IN COOR cPawn,
                IN COOR cKing,
                IN COOR cAttacker)
/**

Routine description:

    This code is called to ask for a black pawn's help in alleviating
    check to its king.  The pawn is on square cPawn.  The friendly
    king is on square cKing.  The lone enemy piece attacking it is on
    square cAttacker.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position
    COOR cPawn : the pawn location
    COOR cKing : the friendly king location
    COOR cAttacker : the lone checker location

Return value:

    void

**/
{
    ULONG uRank = RANK(cPawn);
    PIECE p;
    COOR cTo;
    COOR cExposed;
    int iAttackDelta = DIRECTION_BETWEEN_SQUARES(cAttacker, cKing);

    ASSERT(InCheck(pos, pos->uToMove));
    ASSERT(IS_PAWN(pos->rgSquare[cPawn].pPiece));
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == BLACK);
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) == pos->uToMove);
    ASSERT(GET_COLOR(pos->rgSquare[cPawn].pPiece) ==
           GET_COLOR(pos->rgSquare[cKing].pPiece));
    ASSERT(OPPOSITE_COLORS(pos->rgSquare[cAttacker].pPiece,
                           pos->rgSquare[cKing].pPiece));
    ASSERT((RANK(cPawn) >= 2) && (RANK(cPawn) <= 7));

    if (uRank != 2)
    {
        cTo = cPawn + 16;
        if (IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            if (BLOCKS_THE_CHECK(cTo) &&
                !IS_ON_BOARD(ExposesCheck(pos, cPawn, cKing)))
            {
                _AddNormalMove(pStack, pos, cPawn, cTo, 0);
            }

            if (uRank == 7)
            {
                cTo = cPawn + 32;
                if (IS_EMPTY(pos->rgSquare[cTo].pPiece) &&
                    BLOCKS_THE_CHECK(cTo) &&
                    !IS_ON_BOARD(ExposesCheck(pos, cPawn, cKing)))
                {
                    _AddDoubleJump(pStack, pos, cPawn, cTo);
                }
            }
        }

        cTo = cPawn + 15;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == WHITE);
                ASSERT(IS_ON_BOARD(cTo));
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }

        //
        // N.B. There is no possible way to block check with an
        // en-passant capture because by definition the last move made
        // by the other side was the pawn double jump.  So only handle
        // if the double jumping enemy pawn is the attacker and we can
        // kill it en passant.
        //
        if ((cTo == pos->cEpSquare) &&
            (cAttacker == cTo - 16))
        {
            _AddEnPassant(pStack, pos, cPawn, cTo);
        }

        cTo = cPawn + 17;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == WHITE);
                ASSERT(IS_ON_BOARD(cTo));
                _AddNormalMove(pStack, pos, cPawn, cTo, p);
            }
        }
        if ((cTo == pos->cEpSquare) &&
            (cAttacker == cTo - 16))
        {
            _AddEnPassant(pStack, pos, cPawn, cTo);
        }
    }
    else
    {
        ASSERT(RANK2(cPawn));
        cTo = cPawn + 16;
        if (BLOCKS_THE_CHECK(cTo) &&
            IS_EMPTY(pos->rgSquare[cTo].pPiece))
        {
            _AddPromote(pStack, pos, cPawn, cTo);
        }

        cTo = cPawn + 15;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == WHITE);
                ASSERT(IS_ON_BOARD(cTo));
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }

        cTo = cPawn + 17;
        if (cTo == cAttacker)
        {
            cExposed = ExposesCheck(pos, cPawn, cKing);
            if (!IS_ON_BOARD(cExposed) || (cExposed == cAttacker))
            {
                p = pos->rgSquare[cTo].pPiece;
                ASSERT(!IS_EMPTY(p));
                ASSERT(GET_COLOR(p) == WHITE);
                ASSERT(IS_ON_BOARD(cTo));
                _AddPromote(pStack, pos, cPawn, cTo);
            }
        }
    }
}


void
InvalidGenerator(IN UNUSED MOVE_STACK *pStack,
                 IN UNUSED POSITION *pos,
                 IN UNUSED COOR cPawn)
/**

Routine description:

    If you'd like to make a call, please hang up and try your call
    again...

Parameters:

    IN UNUSED MOVE_STACK *pStack,
    IN UNUSED POSITION *pos,
    IN UNUSED COOR cPawn

Return value:

    void

**/
{
    UtilPanic(SHOULD_NOT_GET_HERE,
              NULL, NULL, NULL, NULL,
              __FILE__, __LINE__);
}


void
InvalidSaveMe(IN UNUSED MOVE_STACK *pStack,
              IN UNUSED POSITION *pos,
              IN UNUSED COOR cPawn,
              IN UNUSED COOR cKing,
              IN UNUSED COOR cAttacker)
/**

Routine description:

    ...if you need help, please dial the operator.

Parameters:

    IN UNUSED MOVE_STACK *pStack,
    IN UNUSED POSITION *pos,
    IN UNUSED COOR cPawn,
    IN UNUSED COOR cKing,
    IN UNUSED COOR cAttacker

Return value:

    void

**/
{
    UtilPanic(SHOULD_NOT_GET_HERE,
              NULL, NULL, NULL, NULL,
              __FILE__, __LINE__);
}


static void
_GenerateAllMoves(IN MOVE_STACK *pStack,
                  IN POSITION *pos)
/**

Routine description:

    This code generates all pseudo-legal moves in a position.  Please
    see notes at the top of this module for details.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board position

Return value:

    static void

**/
{
    COOR c;
    static void (*JumpTable[])(MOVE_STACK *, POSITION *, COOR) =
    {
        InvalidGenerator,                     // EMPTY
        InvalidGenerator,                     // EMPTY | WHITE
        InvalidGenerator,                     // 2  (BLACK_PAWN)
        InvalidGenerator,                     // 3  (WHITE_PAWN)
        GenerateKnight,                       // 4  (BLACK_KNIGHT)
        GenerateWhiteKnight,                  // 5  (WHITE_KNIGHT)
        GenerateBishop,                       // 6  (BLACK_BISHOP)
        GenerateBishop,                       // 7  (WHITE_BISHOP)
        GenerateRook,                         // 8  (BLACK_ROOK)
        GenerateRook,                         // 9  (WHITE_ROOK)
        GenerateQueen,                        // 10 (BLACK_QUEEN)
        GenerateQueen,                        // 11 (WHITE_QUEEN)
        GenerateBlackKing,                    // 12 (BLACK_KING)
        GenerateWhiteKing                     // 13 (WHITE_KING)
    };
    ULONG u;
#ifdef DEBUG
    PIECE p;
#endif

    for(u = pos->uNonPawnCount[pos->uToMove][0] - 1;
        u != (ULONG)-1;
        u--)
    {
        c = pos->cNonPawns[pos->uToMove][u];
#ifdef DEBUG
        ASSERT(IS_ON_BOARD(c));
        p = pos->rgSquare[c].pPiece;
        ASSERT(!IS_EMPTY(p));
        ASSERT(!IS_PAWN(p));
        ASSERT(GET_COLOR(p) == pos->uToMove);
#endif
        (JumpTable[pos->rgSquare[c].pPiece])(pStack, pos, c);
    }

    if (pos->uToMove == BLACK) 
    {
        for(u = 0; u < pos->uPawnCount[BLACK]; u++)
        {
            c = pos->cPawns[BLACK][u];
#ifdef DEBUG
            ASSERT(IS_ON_BOARD(c));
            p = pos->rgSquare[c].pPiece;
            ASSERT(!IS_EMPTY(p));
            ASSERT(IS_PAWN(p));
            ASSERT(GET_COLOR(p) == pos->uToMove);
#endif
            GenerateBlackPawn(pStack, pos, c);
        }
    } else {
        ASSERT(pos->uToMove == WHITE);
        for(u = 0; u < pos->uPawnCount[WHITE]; u++) 
        {
            c = pos->cPawns[WHITE][u];
#ifdef DEBUG
            ASSERT(IS_ON_BOARD(c));
            p = pos->rgSquare[c].pPiece;
            ASSERT(!IS_EMPTY(p));
            ASSERT(IS_PAWN(p));
            ASSERT(GET_COLOR(p) == pos->uToMove);
#endif
            GenerateWhitePawn(pStack, pos, c);
        }
    }
}



static ULONG
_GenerateEscapes(IN MOVE_STACK *pStack,
                 IN POSITION *pos)
/**

Routine description:

    This function is called when side to move is in check in order to
    generate pseudo-legal escapes from check.  All legal escapes from
    check are a subset of the moves returned by this function.  In
    general it's pretty good; the only bug I know about is that it
    will generate a capture that alleviates check where the capturing
    piece is pinned to the king (and thus the capture is illegal).
    The purpose of this function is to allow the search to detect when
    there is one legal reply to check and possibly extend.

Parameters:

    MOVE_STACK *pStack : the move stack
    POSITION *pos : the board

Return value:

    ULONG : the number of pieces checking the king

**/
{
    ULONG u;
    ULONG v;
    COOR c;
    COOR cDefender;
    PIECE p;
    PIECE pKing;
    COOR cKing = pos->cNonPawns[pos->uToMove][0];
    int iIndex;
    SEE_LIST rgCheckers;
    int iDelta;
    ULONG uReturn = 0;
    static void (*JumpTable[])
        (MOVE_STACK *, POSITION *, COOR, COOR, COOR) =
    {
        InvalidSaveMe,                        // EMPTY
        InvalidSaveMe,                        // EMPTY | WHITE
        InvalidSaveMe,                        // 2  (BLACK_PAWN)
        InvalidSaveMe,                        // 3  (WHITE_PAWN)
        SaveMeKnight,                         // 4  (BLACK_KNIGHT)
        SaveMeKnight,                         // 5  (WHITE_KNIGHT)
        SaveMeBishop,                         // 6  (BLACK_BISHOP)
        SaveMeBishop,                         // 7  (WHITE_BISHOP)
        SaveMeRook,                           // 8  (BLACK_ROOK)
        SaveMeRook,                           // 9  (WHITE_ROOK)
        SaveMeQueen,                          // 10 (BLACK_QUEEN)
        SaveMeQueen,                          // 11 (WHITE_QUEEN)
        InvalidSaveMe,                        // kings already considered
        InvalidSaveMe                         // kings already considered
    };

    ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));
    ASSERT(TRUE == InCheck(pos, pos->uToMove));

    //
    // Use the SEE function GetAttacks to find out how many pieces are
    // giving check and from whence.  Note: we don't sort rgCheckers
    // here; maybe it would be worth trying.  In any event it
    // shouldn't be affected by treating the list as a minheap
    // vs. as a sorted list.
    //
    GetAttacks(&rgCheckers, pos, cKing, FLIP(pos->uToMove));
    ASSERT(rgCheckers.uCount > 0);
    ASSERT(rgCheckers.uCount < 17);
    uReturn = rgCheckers.uCount;

    //
    // Phase I: See if the king can flee or take the checking piece.
    //
    pKing = pos->rgSquare[cKing].pPiece;
    ASSERT(GET_COLOR(pKing) == pos->uToMove);
    ASSERT(IS_KING(pKing));
    ASSERT(OPPOSITE_COLORS(pKing, rgCheckers.data[0].pPiece));
    u = 0;
    while(0 != g_iQKDeltas[u])
    {
        c = cKing + g_iQKDeltas[u];
        u++;
        if (IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;

            //
            // This logical OR replaced with bitwise OR; the effect is
            // the same the the bitwise is marginally faster.
            //
            if (IS_EMPTY(p) | (OPPOSITE_COLORS(p, pKing)))
            {
                if (FALSE == IsAttacked(c, pos, FLIP(pos->uToMove)))
                {
                    //
                    // So we know the destination square is not under
                    // attack right now... but that could be because
                    // the present (pre-move) king position "blocks"
                    // the attack.  Detect this.
                    //
                    for (v = 0;
                         v < rgCheckers.uCount;
                         v++)
                    {
                        ASSERT(OPPOSITE_COLORS(pKing,
                                               rgCheckers.data[v].pPiece));
                        if (!IS_PAWN(rgCheckers.data[v].pPiece) &&
                            !IS_KNIGHT(rgCheckers.data[v].pPiece))
                        {
                            iIndex = (int)rgCheckers.data[v].cLoc -
                                     (int)cKing;
                            iDelta = CHECK_DELTA_WITH_INDEX(iIndex);
                            ASSERT(iDelta);

                            iIndex = (int)rgCheckers.data[v].cLoc -
                                     (int)c;
                            if (iDelta == CHECK_DELTA_WITH_INDEX(iIndex))
                            {
                                goto loop;
                            }
                        }
                    }
                    ASSERT(v == rgCheckers.uCount);

                    //
                    // If we get here the square is not under attack
                    // and it does not contain our own piece.  It's
                    // either empty or contains an enemy piece.
                    //
                    ASSERT(!p || OPPOSITE_COLORS(pKing, p));
                    _AddNormalMove(pStack, pos, cKing, c, p);
                    uReturn += 0x00010000;
                }
            }
        }
 loop:
        ;
    }

    //
    // N.B. If there is more than one piece checking the king then
    // there is no way to escape check by blocking check with another
    // piece or capturing the offending piece.  The two attackers
    // cannot be on the same rank, file or diagonal because this would
    // depend on the previous position being illegal.
    //
    if (rgCheckers.uCount > 1)
    {
#ifdef DEBUG
        //
        // Note: the above comment is correct but this assertion fails
        // on positions that we made up to run TestSearch.  Disabled
        // for now.
        //
#if 0
        iDelta = DIRECTION_BETWEEN_SQUARES(rgCheckers.data[0].cLoc, cKing);
        for (v = 1; v < rgCheckers.uCount; v++)
        {
            if (DIRECTION_BETWEEN_SQUARES(rgCheckers.data[v].cLoc, cKing) !=
                iDelta)
            {
                break;
            }
        }
        ASSERT(v != rgCheckers.uCount);
#endif
#endif
        return(uReturn);
    }

    //
    // Phase 2: If we get here there is a lone attacker causing check.
    // See if the other pieces can block the check or take the
    // checking piece.
    //
    c = rgCheckers.data[0].cLoc;
    for (u = 1;                               // don't consider the king
         u < pos->uNonPawnCount[pos->uToMove][0];
         u++)
    {
        cDefender = pos->cNonPawns[pos->uToMove][u];
        ASSERT(IS_ON_BOARD(cDefender));
        p = pos->rgSquare[cDefender].pPiece;
        ASSERT(p && !IS_PAWN(p));
        ASSERT(GET_COLOR(p) == pos->uToMove);

        (JumpTable[p])(pStack, pos, cDefender, cKing, c);
    }

    // Consider all pawns too
    if (pos->uToMove == BLACK) 
    {
        for (u = 0; u < pos->uPawnCount[BLACK]; u++)
        {
            cDefender = pos->cPawns[BLACK][u];
#ifdef DEBUG
            ASSERT(IS_ON_BOARD(cDefender));
            p = pos->rgSquare[cDefender].pPiece;
            ASSERT(p && IS_PAWN(p));
            ASSERT(GET_COLOR(p) == pos->uToMove);
#endif
            SaveMeBlackPawn(pStack, pos, cDefender, cKing, c);
        }
    } else {
        ASSERT(pos->uToMove == WHITE);
        for (u = 0; u < pos->uPawnCount[WHITE]; u++) 
        {
            cDefender = pos->cPawns[WHITE][u];
#ifdef DEBUG
            ASSERT(IS_ON_BOARD(cDefender));
            p = pos->rgSquare[cDefender].pPiece;
            ASSERT(p && IS_PAWN(p));
            ASSERT(GET_COLOR(p) == pos->uToMove);
#endif
            SaveMeWhitePawn(pStack, pos, cDefender, cKing, c);
        }
    }
    return(uReturn);
}


typedef struct _PRECOMP_KILLERS
{
    MOVE mv;
    ULONG uBonus;
}
PRECOMP_KILLERS;

static void
_ScoreAllMoves(IN MOVE_STACK *pStack,
               IN SEARCHER_THREAD_CONTEXT *ctx,
               IN MOVE mvHash)
/**

Routine description:

    We have just generated all the moves, now score them.  See comments
    inline below about order.

Parameters:

    IN MOVE_STACK *pStack,
    IN SEARCHER_THREAD_CONTEXT *ctx,
    IN MOVE mvHash,

Return value:

    static void

**/
{
    ULONG uPly = ctx->uPly;
    POSITION *pos = &ctx->sPosition;
    ULONG u;
    MOVE mv;
    SCORE s;
    MOVE_STACK_MOVE_VALUE_FLAGS mvf;
    ULONG uHashMoveLoc = (ULONG)-1;
    ULONG uColor = pos->uToMove;
    PRECOMP_KILLERS sKillers[4];
    COOR cEnprise = GetEnprisePiece(pos, uColor);

    //
    // We have generated all moves here.  We also know that we are not
    // escaping from check.  There can be a hash move -- if there is
    // then the search has already tried it and we should not generate
    // it again.
    //
    // White has 218 legal moves in this position:
    // 3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1
    //

    //
    // Pre-populate killer/bonuses
    //
    sKillers[0].mv = ctx->mvKiller[uPly][0];
    sKillers[0].uBonus = FIRST_KILLER;
    sKillers[1].mv.uMove = sKillers[3].mv.uMove = 0;
    sKillers[2].mv = ctx->mvKiller[uPly][1];
    sKillers[2].uBonus = THIRD_KILLER;
    if (uPly > 1)
    {
        sKillers[1].mv = ctx->mvKiller[uPly - 2][0];
        sKillers[1].uBonus = SECOND_KILLER;
        sKillers[3].mv = ctx->mvKiller[uPly - 2][1];
        sKillers[3].uBonus = FOURTH_KILLER;
    }
    sKillers[0].uBonus |=
        (SORT_THESE_FIRST * (IS_KILLERMATE_MOVE(sKillers[0].mv) != 0));
    sKillers[1].uBonus |=
        (SORT_THESE_FIRST * (IS_KILLERMATE_MOVE(sKillers[1].mv) != 0));
    sKillers[2].uBonus |=
        (SORT_THESE_FIRST * (IS_KILLERMATE_MOVE(sKillers[2].mv) != 0));
    sKillers[3].uBonus |=
        (SORT_THESE_FIRST * (IS_KILLERMATE_MOVE(sKillers[3].mv) != 0));

    //
    // Score moves
    //
    ASSERT(MOVE_COUNT(ctx, uPly) <= MAX_MOVES_PER_PLY);
    for (u = pStack->uBegin[uPly];
         u < pStack->uEnd[uPly];
         u++)
    {
        mv = pStack->mvf[u].mv;
        ASSERT(mv.uMove);
        ASSERT(GET_COLOR(mv.pMoved) == uColor);
        if (!IS_SAME_MOVE(mv, mvHash))
        {
#ifdef DEBUG
            pStack->mvf[u].iValue = -MAX_INT;
            pStack->mvf[u].bvFlags = 0;
#endif
            //
            // 1. Hash move (already handled by search, all we have to
            //               do here is not generate it)
            // 2. Killer mate move, if one exists
            // 3. Winning captures & promotions (MVV/LVA to tiebreak)
            // 4. Even captures & promotions -- >= SORT_THESE_FIRST
            // 5. Killer move 1-4
            // 6. "good" moves (moves away from danger etc...)
            // 7. Other non capture moves (using dynamic move ordering scheme)
            // 8. Losing captures -- <0
            //

            //
            // Captures and promotions, use SEE
            //
            if (IS_CAPTURE_OR_PROMOTION(mv))
            {
                ASSERT((mv.pCaptured) || (mv.pPromoted));
                s = PIECE_VALUE(mv.pCaptured) -
                    PIECE_VALUE(mv.pMoved);
                if ((s <= 0) || mv.pPromoted)
                {
                    s = SEE(pos, mv);
                }

                if (s >= 0)
                {
                    s += PIECE_VALUE_OVER_100(mv.pCaptured) + 120;
                    s += PIECE_VALUE_OVER_100(mv.pPromoted);
                    s -= PIECE_VALUE_OVER_100(mv.pMoved);
                    s += SORT_THESE_FIRST;

                    //
                    // IDEA: bonus for capturing opponent's last moved
                    // piece to encourage quicker fail highs?
                    //
                    ASSERT(s >= SORT_THESE_FIRST);
                    ASSERT(s > 0);
                    ASSERT((s & STRIP_OFF_FLAGS) < VALUE_KING);
                }
#ifdef DEBUG
                else
                {
                    ASSERT((s & STRIP_OFF_FLAGS) > -VALUE_KING);
                    s -= VALUE_KING;
                    ASSERT(s < 0);
                }
#endif
            }

            //
            // Non-captures/promotes use history/killer
            //
            else
            {
                ASSERT((!mv.pCaptured) && (!mv.pPromoted));
                s = g_iPSQT[mv.pMoved][mv.cTo]; // 0..1000
                s |= (IS_SAME_MOVE(sKillers[0].mv, mv) * sKillers[0].uBonus);
                s |= (IS_SAME_MOVE(sKillers[1].mv, mv) * sKillers[1].uBonus);
                s |= (IS_SAME_MOVE(sKillers[2].mv, mv) * sKillers[2].uBonus);
                s |= (IS_SAME_MOVE(sKillers[3].mv, mv) * sKillers[3].uBonus);
                s |= ((GOOD_MOVE + PIECE_VALUE(mv.pMoved) / 2) * 
                      (mv.cFrom == cEnprise));
                ASSERT(s >= 0);
            }
            pStack->mvf[u].iValue = s;
            ASSERT(pStack->mvf[u].iValue != -MAX_INT);
        }
        else
        {
            ASSERT(uHashMoveLoc == (ULONG)-1);
            uHashMoveLoc = u;
#ifdef DEBUG
            ASSERT((mv.cFrom == mvHash.cFrom) && (mv.cTo == mvHash.cTo));
            pStack->mvf[u].bvFlags |= MVF_MOVE_SEARCHED;
#endif
        }

    } // next move

    //
    // If we generated the hash move, stick it at the end and pretend
    // we didn't... it was already considered by the search and we
    // should not generate it again.
    //
    if (uHashMoveLoc != (ULONG)-1)
    {
        ASSERT(MOVE_COUNT(ctx, uPly) >= 1);
        ASSERT(uHashMoveLoc >= pStack->uBegin[uPly]);
        ASSERT(uHashMoveLoc < pStack->uEnd[uPly]);
        mvf = pStack->mvf[uHashMoveLoc];
        pStack->mvf[uHashMoveLoc] = pStack->mvf[pStack->uEnd[uPly] - 1];
#ifdef DEBUG
        pStack->mvf[pStack->uEnd[uPly] - 1] = mvf;
#endif
        pStack->uEnd[uPly]--;
    }
}



static void
_ScoreAllEscapes(IN MOVE_STACK *pStack,
                 IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN MOVE mvHash)
/**

Routine description:

    We have just generated all the moves, now score them.  See comments
    inline below about order.

Parameters:

    IN MOVE_STACK *pStack,
    IN SEARCHER_THREAD_CONTEXT *ctx,
    IN MOVE mvHash,

Return value:

    static void

**/
{
    ULONG uPly = ctx->uPly;
    POSITION *pos = &ctx->sPosition;
    ULONG u;
    MOVE mv;
    SCORE s;
    MOVE_STACK_MOVE_VALUE_FLAGS mvf;
    ULONG uHashMoveLoc = (ULONG)-1;
    COOR c;
    ULONG v;
    PIECE p;
    static SCORE _bonus[2][7] = {
        { 0, 2000, 1250, 1500, 1100, 850,  0 }, // friend
        { 0, 1600,  100,  100,   50,  -1, -1 }, // foe
    };

    //
    // We have generated legal escapes from check here.  There can be
    // a hash move -- if there is then the search has already tried it
    // and we should not generate it again.
    //

    ASSERT(MOVE_COUNT(ctx, uPly) <= MAX_MOVES_PER_PLY);
    for (u = pStack->uBegin[uPly];
         u < pStack->uEnd[uPly];
         u++)
    {
        mv = pStack->mvf[u].mv;
        ASSERT(mv.uMove);
        ASSERT(GET_COLOR(mv.pMoved) == pos->uToMove);
        if (!IS_SAME_MOVE(mv, mvHash))
        {
#ifdef DEBUG
            pStack->mvf[u].iValue = -MAX_INT;
            pStack->mvf[u].bvFlags = 0;
#endif
            pStack->mvf[u].mv.bvFlags |= MOVE_FLAG_ESCAPING_CHECK;

            //
            // 1. Hash move (already handled by search, all we have to
            //               do here is not generate it)
            // 2. Winning captures by pieces other than the checked king
            // 3. Winning captures by the king
            // 4. Even captures by pieces other than the king
            // 5. Killer escapes
            // 6. Moves that block the check and are even
            // 7. King moves
            // 8. Losing captures / blocks
            //

            //
            // Captures and promotions, use SEE
            //
            if (IS_CAPTURE_OR_PROMOTION(mv))
            {
                ASSERT((mv.pCaptured) || (mv.pPromoted));
                s = PIECE_VALUE(mv.pCaptured) -
                    PIECE_VALUE(mv.pMoved);
                if ((s <= 0) || mv.pPromoted)
                {
                    s = SEE(pos, mv);
                }
                
                if (s > 0)
                {
                    // Winning captures: take with pieces before with king
                    s += PIECE_VALUE_OVER_100(mv.pPromoted);
                    s -= PIECE_VALUE_OVER_100(mv.pMoved);
                    s >>= IS_KING(mv.pMoved);
                    s |= SORT_THESE_FIRST;
                    ASSERT(s >= SORT_THESE_FIRST);
                    ASSERT(s > 0);
                    ASSERT((s & STRIP_OFF_FLAGS) < VALUE_KING);
                } 
                else if (s == 0) 
                {
                    // Even capture -- can't be capturing w/ king
                    ASSERT(!IS_KING(mv.pMoved));
                    s += PIECE_VALUE_OVER_100(mv.pPromoted) + 20;
                    s -= PIECE_VALUE_OVER_100(mv.pMoved);
                    s |= FIRST_KILLER;
                    ASSERT(s > 0);
                }
#ifdef DEBUG
                else
                {
                    // Losing capture
                    ASSERT(!IS_KING(mv.pMoved));
                    ASSERT((s & STRIP_OFF_FLAGS) > -VALUE_KING);
                    s -= VALUE_KING;
                    ASSERT(s < 0);
                }
#endif
            }

            //
            // Non-captures/promotes
            //
            else
            {
                ASSERT((!mv.pCaptured) && (!mv.pPromoted));
                if (IS_SAME_MOVE(mv, ctx->mvKillerEscapes[uPly][0]) ||
                    IS_SAME_MOVE(mv, ctx->mvKillerEscapes[uPly][1]))
                {
                    s = SECOND_KILLER;
                    ASSERT(s >= 0);
                }
                else
                {
                    s = g_iPSQT[mv.pMoved][mv.cTo]; // 0..1000
                    if (!IS_KING(mv.pMoved)) 
                    {
                        // 0..2400
                        s += (VALUE_QUEEN - PIECE_VALUE(mv.pMoved)) * 4;
                        if (SEE(pos, mv) >= 0)
                        {
                            // Non-losing block of check
                            ASSERT(SEE(pos, mv) == 0);
                            s |= THIRD_KILLER;
                            ASSERT(s >= 0);
                        }
                        else 
                        {
                            // Losing block of check
                            s -= VALUE_KING;
                            ASSERT(s < 0);
                        }
                    } 
                    else
                    {
                        // If the king is fleeing, encourage a spot next
                        // to friendly pieces.
                        v = 0;
                        do {
                            c = mv.cTo + g_iQKDeltas[v++];
                            if (IS_ON_BOARD(c)) 
                            {
                                p = pos->rgSquare[c].pPiece;
                                s += _bonus[OPPOSITE_COLORS(pos->uToMove, p)]
                                           [PIECE_TYPE(p)];
                                ASSERT(_bonus[OPPOSITE_COLORS(pos->uToMove, p)]
                                       [PIECE_TYPE(p)] >= 0);
                            }
                        } 
                        while(g_iQKDeltas[v] != 0);
                    }
                }
            }
            pStack->mvf[u].iValue = s;
            ASSERT(pStack->mvf[u].iValue != -MAX_INT);
        }
        else
        {
            ASSERT(uHashMoveLoc == (ULONG)-1);
            uHashMoveLoc = u;
#ifdef DEBUG
            ASSERT((mv.cFrom == mvHash.cFrom) && (mv.cTo == mvHash.cTo));
            pStack->mvf[u].bvFlags |= MVF_MOVE_SEARCHED;
#endif
        }
    } // next move

    //
    // If we generated the hash move, stick it at the end and pretend
    // we didn't... it was already considered by the search and we
    // should not generate it again.
    //
    if (uHashMoveLoc != (ULONG)-1)
    {
        ASSERT(MOVE_COUNT(ctx, uPly) >= 1);
        ASSERT(uHashMoveLoc >= pStack->uBegin[uPly]);
        ASSERT(uHashMoveLoc < pStack->uEnd[uPly]);
        mvf = pStack->mvf[uHashMoveLoc];
        pStack->mvf[uHashMoveLoc] = pStack->mvf[pStack->uEnd[uPly] - 1];
#ifdef DEBUG
        pStack->mvf[pStack->uEnd[uPly] - 1] = mvf;
#endif
        pStack->uEnd[uPly]--;
    }
}



static void
_ScoreQSearchMovesInclChecks(IN MOVE_STACK *pStack,
                             IN SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    We have just generated all the moves.  We were called from the
    Qsearch so:

       1. Unlike _ScoreAllMoves we don't have to think about a hash move
       2. QSearch only cares about winning/even captures/promotes and
          checking moves.

    Like _ScoreSearchMoves, this function takes "hints" from the
    searcher context about what squares are in danger and tries to
    order moves away from those squares sooner.  Unlike
    _ScoreSearchMoves this function also attempts to capture enemies
    that are in danger sooner.

Parameters:

    IN MOVE_STACK *pStack,
    IN SEARCHER_THREAD_CONTEXT *ctx

Return value:

    static void

**/
{
    register POSITION *pos = &ctx->sPosition;
    PLY_INFO *pi = &ctx->sPlyInfo[ctx->uPly];
    ULONG uPly = ctx->uPly;
    ULONG u, uColor = pos->uToMove;
    MOVE mv;
    MOVE mvLast = (pi-1)->mv;
    SCORE s;
    COOR cEnprise = GetEnprisePiece(pos, uColor);

    //
    // We have generate all moves or just legal escapes from check
    // here.  There can be a hash move -- if there is then the search
    // has already tried it and we should not generate it again.
    //
    // White has 218 legal moves in this position:
    // 3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1
    //
    ASSERT(MOVE_COUNT(ctx, uPly) <= MAX_MOVES_PER_PLY);
    ASSERT(uPly >= 1);

    for (u = pStack->uBegin[uPly];
         u < pStack->uEnd[uPly];
         u++)
    {
        mv = pStack->mvf[u].mv;
        ASSERT(GET_COLOR(mv.pMoved) == uColor);
#ifdef DEBUG
        pStack->mvf[u].iValue = -MAX_INT;
        pStack->mvf[u].bvFlags = 0;
#endif
        pStack->mvf[u].mv.bvFlags |= WouldGiveCheck(ctx, mv);

        //
        // 1. There can't be a hash move so don't worry about it
        // 2. Winning captures & promotions
        // 3. Even captures & promotions -- >= SORT_THESE_FIRST
        // 4. Checking non-capture moves
        // 5. Losing captures that check -- > 0
        // 6. Everything else is ignored -- <= 0
        //

        //
        // Captures and promotions, use SEE
        //
        if (IS_CAPTURE_OR_PROMOTION(mv))
        {
            ASSERT((mv.pCaptured) || (mv.pPromoted));
            s = PIECE_VALUE(mv.pCaptured) -
                PIECE_VALUE(mv.pMoved);
            if ((s <= 0) || mv.pPromoted)
            {
                s = SEE(pos, mv);
            }

            if (s >= 0)
            {
                //
                // Winning / even captures / promotes should be >=
                // SORT_THESE_FIRST.
                //
                s += PIECE_VALUE_OVER_100(mv.pCaptured) + 120;
                s += PIECE_VALUE_OVER_100(mv.pPromoted);
                s -= PIECE_VALUE_OVER_100(mv.pMoved);
                s += SORT_THESE_FIRST;

                //
                // Bonus if it checks too
                //
                s += (pStack->mvf[u].mv.bvFlags & MOVE_FLAG_CHECKING) * 4;

                //
                // Bonus for capturing last moved enemy piece.
                //
                s += (mv.cTo == mvLast.cTo) * 64;
                ASSERT(s >= SORT_THESE_FIRST);
                ASSERT(s > 0);
            }
            else
            {
                //
                // Make losing captures into either zero scores or
                // slightly positive if they check.  Qsearch is going
                // to bail as soon as it sees the first zero so no
                // need to keep real SEE scores on these.
                //
                s = (pStack->mvf[u].mv.bvFlags & MOVE_FLAG_CHECKING);
                ASSERT(s >= 0);
            }
        }

        //
        // Non-captures/promotes -- we don't care unless they check
        //
        else
        {
            s = (pStack->mvf[u].mv.bvFlags & MOVE_FLAG_CHECKING) * 2;
            ASSERT(s >= 0);
            ASSERT(s < SORT_THESE_FIRST);
        }

        //
        // If this move moves a piece that was in danger, consider it
        // sooner.
        //
        if (s > 0)
        {
            ASSERT(IS_CAPTURE_OR_PROMOTION(pStack->mvf[u].mv) ||
                   IS_CHECKING_MOVE(pStack->mvf[u].mv));
            s += (mv.cFrom == cEnprise) * (PIECE_VALUE(mv.pMoved) / 4);
            ASSERT(s > 0);
        }
        pStack->mvf[u].iValue = s;
        ASSERT(pStack->mvf[u].iValue != -MAX_INT);

    } // next move
}


static void
_ScoreQSearchMovesNoChecks(IN MOVE_STACK *pStack,
                           IN SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    We have just generated all the moves.  We were called from the
    Qsearch so:

       1. Unlike _ScoreAllMoves we don't have to think about a hash move
       2. QSearch only cares about winning/even captures/promotes.  At
          this point it is so deep that it doesn't even care about
          checks.

    Like _ScoreSearchMoves, this function takes "hints" from the
    searcher context about what squares are in danger and tries to
    order moves away from those squares sooner.  Unlike
    _ScoreSearchMoves this function also attempts to capture enemies
    that are in danger sooner.

Parameters:

    IN MOVE_STACK *pStack,
    IN SEARCHER_THREAD_CONTEXT *ctx

Return value:

    static void

**/
{
    register POSITION *pos = &(ctx->sPosition);
    ULONG uPly = ctx->uPly;
    ULONG u, uColor;
    MOVE mv;
    SCORE s;

    //
    // We have generate all moves or just legal escapes from check
    // here.  There can be a hash move -- if there is then the search
    // has already tried it and we should not generate it again.
    //
    // White has 218 legal moves in this position:
    // 3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1
    //
    ASSERT(MOVE_COUNT(ctx, uPly) <= MAX_MOVES_PER_PLY);

    uColor = pos->uToMove;
    for (u = pStack->uBegin[uPly];
         u < pStack->uEnd[uPly];
         u++)
    {
        mv = pStack->mvf[u].mv;
        ASSERT(GET_COLOR(mv.pMoved) == uColor);
#ifdef DEBUG
        pStack->mvf[u].iValue = -MAX_INT;
        pStack->mvf[u].bvFlags = 0;
#endif
        //
        // 1. There can't be a hash move so don't worry about it
        // 2. Winning captures & promotions
        // 3. Even captures & promotions -- >= SORT_THESE_FIRST
        // 4. Everything else is ignored -- <= 0
        //

        //
        // Captures and promotions, use SEE
        //
        s = 0;
        if (IS_CAPTURE_OR_PROMOTION(mv))
        {
            ASSERT((mv.pCaptured) || (mv.pPromoted));
            s = PIECE_VALUE(mv.pCaptured) -
                PIECE_VALUE(mv.pMoved);
            //
            // TODO: how does this handle positions like K *p *k ?
            //
            if (s <= 0)
            {
                s = SEE(pos, mv);
            }

            if (s >= 0)
            {
                //
                // Winning / even captures / promotes should be >=
                // SORT_THESE_FIRST.
                //
                s += PIECE_VALUE_OVER_100(mv.pCaptured) + 120;
                s += PIECE_VALUE_OVER_100(mv.pPromoted);
                s -= PIECE_VALUE_OVER_100(mv.pMoved);
                s += SORT_THESE_FIRST;

                //
                // IDEA: bonus for capturing an enprise enemy piece?
                //
                ASSERT(s >= SORT_THESE_FIRST);
                ASSERT(s > 0);
            }
        }
        pStack->mvf[u].iValue = s;
        ASSERT(pStack->mvf[u].iValue != -MAX_INT);

    } // next move
}



void
GenerateMoves(IN SEARCHER_THREAD_CONTEXT *ctx,
              IN MOVE mvHash,
              IN ULONG uType)
/**

Routine description:

    This is the entrypoint to the move generator.  It invokes the
    requested generation code which push moves onto the move stack in
    ctx.  Once the moves are generated this code optionally will score
    the moves.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : a searcher thread's context

        IN: ctx->sPosition.uDangerCount and ctx->sPosition.cDanger
            data is used to tweak move scoring.  This data MUST be
            consistent with the state of the current board.

       OUT: ctx->sMoveStack.uBegin[ctx->uPly] is set,
            ctx->sMoveStack.uEnd[ctx->uPly] is set,
            ctx->sMoveStack.mvf[begin..end] are populated

    MOVE mvHash : the hash move to not generate

    ULONG uType : what kind of moves to generate

        GENERATE_ALL_MOVES : pseudo-legal generation of all moves;
            scored by _ScoreAllMoves.  Return value is count of moves
            generated.

        GENERATE_ESCAPE: called when stm in check, generate evasions
            scored by _ScoreAllMoves.  Return value is special code;
            see inline comment below.

        GENERATE_CAPTURES_PROMS_CHECKS: caps, promotions, and checks
            scored by _ScoreQSearchMoves.  Return value is count of
            moves generated.

        GENERATE_CAPTURES_PROMS: caps, promotions.  Skip the checks.

        GENERATE_DONT_SCORE: pseudo-legal generation of all moves;
            not scored to save time.  Return value always zero.

Return value:

    void

**/
{
    register ULONG uPly = ctx->uPly;
    MOVE_STACK *pStack = &ctx->sMoveStack;
    POSITION *pos = &ctx->sPosition;
    ULONG uReturn;
#ifdef DEBUG
    POSITION board;
    ULONG u;
    MOVE mv;
    PIECE p;

    ASSERT((uPly >= 0) && (uPly < MAX_PLY_PER_SEARCH));
    memcpy(&board, pos, sizeof(POSITION));
    memcpy(&(pStack->board[uPly]), pos, sizeof(POSITION));

    //
    // Make sure the hash move makes sense (if provided)
    //
    if (mvHash.uMove)
    {
        p = pos->rgSquare[mvHash.cFrom].pPiece;
        ASSERT(p);
        ASSERT(GET_COLOR(p) == pos->uToMove);
        p = pos->rgSquare[mvHash.cTo].pPiece;
        ASSERT(!p || OPPOSITE_COLORS(p, pos->uToMove));
    }
#endif
    pStack->uPly = uPly;
    pStack->uEnd[uPly] = pStack->uBegin[uPly];
    pStack->sGenFlags[uPly].uAllGenFlags = 0;
    
    switch(uType)
    {
        case GENERATE_CAPTURES_PROMS_CHECKS:
            ASSERT(!InCheck(pos, pos->uToMove));
            ASSERT(mvHash.uMove == 0);
            _FindUnblockedSquares(pStack, pos);
            _GenerateAllMoves(pStack, pos);
            _ScoreQSearchMovesInclChecks(pStack, ctx);
            break;
        case GENERATE_CAPTURES_PROMS:
            ASSERT(!InCheck(pos, pos->uToMove));
            ASSERT(mvHash.uMove == 0);
            _FindUnblockedSquares(pStack, pos);
            _GenerateAllMoves(pStack, pos);
            _ScoreQSearchMovesNoChecks(pStack, ctx);
            break;
        case GENERATE_ALL_MOVES:
            ASSERT(!InCheck(pos, pos->uToMove));
            _FindUnblockedSquares(pStack, pos);
            _GenerateAllMoves(pStack, pos);
            _ScoreAllMoves(pStack, ctx, mvHash);
            uReturn = MOVE_COUNT(ctx, uPly);
            break;
        case GENERATE_ESCAPES:
            ASSERT(InCheck(pos, pos->uToMove));
            _FindUnblockedSquares(pStack, pos);
            uReturn = _GenerateEscapes(pStack, pos);
            ASSERT(uReturn);
            pStack->sGenFlags[uPly].uKingMoveCount = uReturn >> 16;
            pStack->sGenFlags[uPly].uCheckingPieces = uReturn & 0xFF;
            _ScoreAllEscapes(pStack, ctx, mvHash);
            break;
            
        // Note: this is just for plytest / seetest.  Just generate
        // moves, do not score them.
        case GENERATE_DONT_SCORE:
            if (InCheck(pos, pos->uToMove))
            {
                (void)_GenerateEscapes(pStack, pos);
                for (uReturn = pStack->uBegin[uPly];
                     uReturn < pStack->uEnd[uPly];
                     uReturn++)
                {
                    pStack->mvf[uReturn].mv.bvFlags |=
                        MOVE_FLAG_ESCAPING_CHECK;
                }
            }
            else
            {
                (void)_GenerateAllMoves(pStack, pos);
            }
            goto end;
#ifdef DEBUG
        default:
            uReturn = 0;
            ASSERT(FALSE);
#endif
    }

#ifdef DEBUG
    //
    // Sanity check the move list we just generated...
    //
    ASSERT(MOVE_COUNT(ctx, uPly) >= 0);
    for (u = pStack->uBegin[uPly];
         u < pStack->uEnd[uPly];
         u++)
    {
        mv = pStack->mvf[u].mv;
        ASSERT(!IS_SAME_MOVE(mv, mvHash));
        SanityCheckMove(pos, mv);

        if (WouldGiveCheck(ctx, mv))
        {
            if (MakeMove(ctx, mv))
            {
                ASSERT(ctx->uPly == (uPly + 1));
                ASSERT(InCheck(pos, pos->uToMove));
                UnmakeMove(ctx, mv);

                //
                // Note: this is so that DEBUG and RELEASE builds search
                // the same trees.  Without this the MakeMove/UnmakeMove
                // pair here can affect the order of the pieces in the
                // POSITION piece lists which in turn affects the order
                // in which moves are generated down the road.
                //
                memcpy(&ctx->sPosition, &board, sizeof(POSITION));
            }
        }
        else
        {
            if (MakeMove(ctx, mv))
            {
                ASSERT(ctx->uPly == (uPly + 1));
                ASSERT(!InCheck(pos, pos->uToMove));
                UnmakeMove(ctx, mv);

                //
                // Note: this is so that DEBUG and RELEASE builds search
                // the same trees.  Without this the MakeMove/UnmakeMove
                // pair here can affect the order of the pieces in the
                // POSITION piece lists which in turn affects the order
                // in which moves are generated down the road.
                //
                memcpy(&ctx->sPosition, &board, sizeof(POSITION));
            }
        }
    }
    ASSERT(PositionsAreEquivalent(&board, pos));
#endif
 end:
    pStack->sGenFlags[uPly].uMoveCount = MOVE_COUNT(ctx, uPly);
    pStack->uBegin[uPly + 1] = pStack->uEnd[uPly];
}
