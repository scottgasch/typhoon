/**

Copyright (c) Scott Gasch

Module Name:

    see.c

Abstract:

    Static exchange evaluator and support code.  See also x86.asm.

Author:

    Scott Gasch (scott.gasch@gmail.com) 11 Jun 2004

Revision History:

    $Id: see.c 348 2008-01-05 07:11:36Z scott $

**/

#include "chess.h"

#define ADD_ATTACKER(p, c, v) \
    pList->data[pList->uCount].pPiece = (p); \
    pList->data[pList->uCount].cLoc = (c);   \
    pList->data[pList->uCount].uVal = (v);   \
    pList->uCount++;

void CDECL
SlowGetAttacks(IN OUT SEE_LIST *pList,
               IN POSITION *pos,
               IN COOR cSquare,
               IN ULONG uSide)
/*++

Routine description:

    SlowGetAttacks is the C version of GetAttacks; it should be
    identical to the GetAttacks code in x86.asm.  The job of the
    function is, given a position, square and side, to populate the
    SEE_LIST with the locations and types of enemy pieces attacking
    the square.

Parameters:

    SEE_LIST *pList : list to populate
    POSITION *pos : the board
    COOR cSquare : square in question
    ULONG uSide : side we are looking for attacks from

Return value:

    void

--*/
{
    register ULONG x;
    PIECE p;
    COOR c;
    int iIndex;
    COOR cBlockIndex;
    int iDelta;
    static PIECE pPawn[2] = { BLACK_PAWN, WHITE_PAWN };
    static int iSeeDelta[2] = { -17, +15 };

#ifdef DEBUG
    ASSERT(IS_ON_BOARD(cSquare));
    ASSERT(IS_VALID_COLOR(uSide));
    VerifyPositionConsistency(pos, FALSE);
#endif
    pList->uCount = 0;

    //
    // Check for pawns attacking cSquare
    //
    c = cSquare + (iSeeDelta[uSide]);
    if (IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (p == pPawn[uSide])
        {
            //
            // N.B. Don't use ADD_ATTACKER here because we know we're
            // at element zero.
            //
            pList->data[0].pPiece = p;
            pList->data[0].cLoc = c;
            pList->data[0].uVal = VALUE_PAWN;
            pList->uCount = 1;
        }
    }

    c += 2;
    if (IS_ON_BOARD(c))
    {
        p = pos->rgSquare[c].pPiece;
        if (p == pPawn[uSide])
        {
            ADD_ATTACKER(p, c, VALUE_PAWN);
        }
    }

    //
    // Check for pieces attacking cSquare
    //
    for (x = pos->uNonPawnCount[uSide][0] - 1;
         x != (ULONG)-1;
         x--)
    {
        c = pos->cNonPawns[uSide][x];
        ASSERT(IS_ON_BOARD(c));

        p = pos->rgSquare[c].pPiece;
        ASSERT(p && !IS_PAWN(p));
        ASSERT(GET_COLOR(p) == uSide);

        iIndex = (int)c - (int)cSquare;
        if (0 == (CHECK_VECTOR_WITH_INDEX(iIndex, GET_COLOR(p)) &
                  (1 << PIECE_TYPE(p))))
        {
            continue;
        }

        if (IS_KNIGHT_OR_KING(p))
        {
            ASSERT(IS_KNIGHT(p) || IS_KING(p));
            ADD_ATTACKER(p, c, PIECE_VALUE(p));
            continue;
        }

        //
        // Check to see if there is a piece in the path from cSquare
        // to c that blocks the attack.
        //
        iDelta = NEG_DELTA_WITH_INDEX(iIndex);
        ASSERT(iDelta == -1 * CHECK_DELTA_WITH_INDEX(iIndex));
        ASSERT(iDelta != 0);
        for (cBlockIndex = cSquare + iDelta;
             cBlockIndex != c;
             cBlockIndex += iDelta)
        {
            if (!IS_EMPTY(pos->rgSquare[cBlockIndex].pPiece))
            {
                goto done;
            }
        }

        //
        // Nothing in the way.
        //
        ADD_ATTACKER(p, c, PIECE_VALUE(p));

 done:
        ;
    }
}

#ifdef SEE_HEAPS
//
// SEE_HEAPS works great in principle but makes MinLegalPiece
// inaccurate if the top of the heap is not a legal move (i.e. the
// piece is pinned etc...)  When tested the result of this was minimal
// and the use of a heap instead of a sorted list sped up the SEE
// code.
//
#define PARENT(x)      ((x - 1) / 2)
#define LEFT_CHILD(x)  (((x) * 2) + 1)
#define RIGHT_CHILD(x) (((x) * 2) + 2)

#ifdef DEBUG
static FLAG
_IsValidHeap(IN SEE_LIST *p)
/**

Routine description:

    Is the minheap in SEE_LIST p->data[] valid (i.e. does it satisfy
    the minheap property?)

Parameters:

    SEE_LIST *p : SEE_LIST to check

Return value:

    static FLAG : TRUE if valid, FALSE otherwise

**/
{
    ULONG u, l, r;

    for (u = 0; u < p->uCount; u++)
    {
        l = LEFT_CHILD(u);
        if ((l < p->uCount) && (p->data[l].uVal < p->data[u].uVal))
        {
            return(FALSE);
        }

        r = RIGHT_CHILD(u);
        if ((r < p->uCount) && (p->data[r].uVal < p->data[u].uVal))
        {
            return(FALSE);
        }
    }
    return(TRUE);
}
#endif // DEBUG


static void
_PushDown(IN OUT SEE_LIST *p,
          IN ULONG u)
/**

Routine description:

    Take heap node number u and compare its value with the value of
    its children.  Swap smallest value into position u and continue
    the push-down process if warranted.

Parameters:

    SEE_LIST *p : SEE_LIST/minheap
    ULONG u : node number in consideration

Return value:

    static void

**/
{
    ULONG l = LEFT_CHILD(u);
    ULONG r;
    ULONG uSmallest;
    SEE_THREESOME temp;

    //
    // The heap is a complete tree -- if there's no left child of this
    // node then there's no right child either.  If this is a leaf node
    // then our work is done.
    //
    ASSERT(p->uCount > 0);
    if (l >= p->uCount)
    {
        return;
    }
    ASSERT(PARENT(l) == u);

    //
    // Otherwise, find the smallest of u, l and r.
    //
    r = l + 1;
    ASSERT(r == RIGHT_CHILD(u));
    ASSERT(PARENT(r) == u);
    ASSERT((r != u) && (r != l) && (l != u));
    uSmallest = u;
    if (p->data[l].uVal < p->data[u].uVal)
    {
        uSmallest = l;
    }
    if ((r < p->uCount) && (p->data[r].uVal < p->data[uSmallest].uVal))
    {
        uSmallest = r;
    }

    //
    // If it's anything other than u, swap them and continue to push.
    //
    if (uSmallest != u)
    {
        ASSERT((uSmallest == l) || (uSmallest == r));
        temp = p->data[uSmallest];
        p->data[uSmallest] = p->data[u];
        p->data[u] = temp;
        _PushDown(p, uSmallest);
    }
}

static void
_BuildHeap(IN OUT SEE_LIST *p)
/**

Routine description:

    Convert a random array into a minheap.  Start at the first
    internal node and push it down into place.  Continue to work
    backwards until we get to the root (index 0).

Parameters:

    SEE_LIST *p : list to heapify

Return value:

    static void

**/
{
    int iStart = (p->uCount / 2) - 1;
    int i;

    ASSERT(iStart < (int)p->uCount);
    for (i = iStart; i > -1; i--)
    {
        ASSERT(i >= 0);
        _PushDown(p, (ULONG)i);
    }
    ASSERT(_IsValidHeap(p));
}


static void
_BubbleUp(IN OUT SEE_LIST *p, IN ULONG u)
/**

Routine description:

    Take a node and bubble it up into place to re-create a minheap.

Parameters:

    SEE_LIST *p : heap
    ULONG u : index of node to bubble up

Return value:

    static void

**/
{
    ULONG uParent;
    SEE_THREESOME temp;

    ASSERT(p->uCount > 0);
    if (u == 0) return;

    uParent = PARENT(u);
    ASSERT((LEFT_CHILD(uParent) == u) ||
           (RIGHT_CHILD(uParent) == u));
    ASSERT(uParent < u);
    if (p->data[uParent].uVal > p->data[u].uVal)
    {
        temp = p->data[uParent];
        p->data[uParent] = p->data[u];
        p->data[u] = temp;
        _BubbleUp(p, uParent);
    }
}

#else // !SEE_HEAPS

static void
_SortList(IN OUT SEE_LIST *pList)
/**

Routine description:

    Sort the SEE list by piece value with a selection sort.

Parameters:

    SEE_LIST *pList

Return value:

    static void

**/
{
    ULONG x;
    ULONG y;
    register ULONG uMaxVal;
    register ULONG uMaxLoc;
    SEE_THREESOME sTemp;

    for (x = 0;
         x < pList->uCount;
         x++)
    {
        //
        // Assume index X is the largest value item in the list
        //
        uMaxVal = pList->data[x].uVal;
        uMaxLoc = x;

        //
        // Look for others with a larger value
        //
        for (y = x + 1;
             y < pList->uCount;
             y++)
        {
            if (pList->data[y].uVal > uMaxVal)
            {
                uMaxVal = pList->data[y].uVal;
                uMaxLoc = y;
            }
        }

        sTemp = pList->data[uMaxLoc];
        pList->data[uMaxLoc] = pList->data[x];
        pList->data[x] = sTemp;
    }
}

#endif

static void INLINE
_RemoveItem(IN OUT SEE_LIST *pList,
            IN ULONG x)
/**

Routine description:

    Delete item x from the list.

Parameters:

    SEE_LIST *pList,
    ULONG x

Return value:

    static void INLINE

**/
{
#ifndef SEE_HEAPS
    ULONG y;
#endif

    ASSERT(x < pList->uCount);
    ASSERT(pList->uCount > 0);

#ifdef SEE_HEAPS
    //
    // Swap item x with the last thing on the heap and then push the
    // node back down.
    //
    if (x != (pList->uCount - 1))
    {
        pList->data[x] = pList->data[pList->uCount - 1];
        pList->uCount--;
        _PushDown(pList, 0);
    }
    else
    {
        pList->uCount--;
    }

#else // !SEE_HEAPS

    //
    // If X is not the last thing in the list we will have to ripple
    // shift to close the hole.  This is rare.
    //
    for (y = x + 1;
         y < pList->uCount;
         y++)
    {
        pList->data[y - 1] = pList->data[y];
    }
    pList->uCount--;

#endif // SEE_HEAPS
}


static void
_ClearPieceByLocation(IN OUT SEE_LIST *pList,
                      IN COOR cLoc)
/**

Routine description:

    Find a piece on the SEE list at COOR cLoc and delete it.

Parameters:

    SEE_LIST *pList,
    COOR cLoc

Return value:

    static void

**/
{
    ULONG x = 0;

    while (x < pList->uCount)
    {
        if (pList->data[x].cLoc == cLoc)
        {
            _RemoveItem(pList, x);
            return;
        }
        x++;
    }

    //
    // This can happen if, for example, the SEE move was a passed pawn push
    //
}


#ifdef SEE_HEAPS
static PIECE
_MinLegalPiece(IN POSITION *pos,
               IN ULONG uColor,
               IN SEE_LIST *pList,
               IN SEE_LIST *pOther,
               IN COOR *pc,
               IN COOR cIgnore)
/**

Routine description:

    Return the piece from the SEE list with the lowest value that is
    not pinned to its own king.  Because we are storing the SEE_LISTS
    as heaps, this is only an approximate value in the event that the
    real min value piece is indeed pinned to its own king.  I
    considered sorting the list in this case but it seems like (in
    tests) this inaccuracy really has little or no impact on the
    search tree size.  The speedup with heaps instead of sorted lists
    seems worth the price.

Parameters:

    POSITION *pos : the board
    ULONG uColor  : the color on move
    SEE_LIST *pList : the list we're selecting from
    SEE_LIST *pOther : the other side's list
    COOR *pc,
    COOR cIgnore

Return value:

    static PIECE

**/
{
    COOR cKing;
    PIECE p;
    register ULONG x;
    COOR c;

    //
    // The list is a minheap with the min value piece at index 0.
    //
    for (x = 0;
         x < pList->uCount;
         x++)
    {
        p = pList->data[x].pPiece;

        //
        // If this piece is the king, then no need to see if the move
        // exposes the king to check.. just play the move as long as
        // it's legal (i.e. no defenders to the square)
        //
        if (IS_KING(p))
        {
            if (pOther->uCount == 0)
            {
                *pc = pList->data[0].cLoc;

                //
                // Note: if p is a king and we allow them to play it then
                // by definition the other side has nothing to counter
                // with...  otherwise we'd be moving into check here.  So
                // even if we had other pieces that were pinned to the
                // king, empty out the list because we're done in the next
                // SEE loop.
                //
                pList->uCount = 0;
                return(p);
            }
        }
        else
        {
            //
            // Otherwise... consider pins.  If the least valuable
            // defender of the square cannot move because doing so
            // would exposes his king to check, skip it and try the
            // next least valuable.
            //
            cKing = pos->cNonPawns[uColor][0];
            c = ExposesCheck(pos, pList->data[x].cLoc, cKing);

            //
            // cIgnore is the coordinate of the last piece the other
            // side "moved" in this capture sequence.  This is a hack
            // to ignore pins based on a piece that has already moved
            // in the computation but is already on the board.  This
            // of course does not work for positions where the piece
            // you expose check to was "moved" two turns ago but these
            // are pretty rare.
            //
            if (!IS_ON_BOARD(c) || (c == cIgnore))
            {
                *pc = pList->data[x].cLoc;
                _RemoveItem(pList, x);
                return(p);
            }
        }
    }

    //
    // There are no legal pieces to move to square
    //
    return(0);
}

#else // !SEE_HEAPS

static PIECE
_MinLegalPiece(IN POSITION *pos,
               IN ULONG uColor,
               IN SEE_LIST *pList,
               IN SEE_LIST *pOther,
               IN COOR *pc,
               IN COOR cIgnore)
/**

Routine description:

    Return the piece from the SEE list with the lowest value that is
    not pinned to its own king.

Parameters:

    POSITION *pos : the board
    ULONG uColor  : the color on move
    SEE_LIST *pList : the list we're selecting from
    SEE_LIST *pOther : the other side's list
    COOR *pc,
    COOR cIgnore

Return value:

    static PIECE

**/
{
    COOR cKing;
    PIECE p;
    register ULONG x;
    COOR c;

    //
    // The list is sorted from most valuable (index 0) to least valuable
    // (index N).  Begin at the least valuable and work up.
    //
    for (x = pList->uCount - 1;
         x != (ULONG)-1;
         x--)
    {
        p = pList->data[x].pPiece;
        //
        // If this piece is the king, then no need to see if the move
        // exposes the king to check.. just play the move as long as 
        // it's legal (i.e. no defenders to the square)
        //
        if ((IS_KING(p)) && (pOther->uCount == 0))
        {
            ASSERT(x == 0);
            *pc = pList->data[0].cLoc;
            pList->uCount = 0;
            return(p);
        }
        else
        {
            //
            // Otherwise... consider pins.  If the least valuable
            // defender of the square cannot move because doing so
            // would exposes his king to check, skip it and try the
            // next least valuable.
            //
            cKing = pos->cNonPawns[uColor][0];
            c = ExposesCheck(pos, pList->data[x].cLoc, cKing);

            //
            // cIgnore is the coordinate of the last piece the other
            // side "moved" in this capture sequence.  This is a hack
            // to ignore pins based on a piece that has already moved
            // in the computation but is already on the board.  This
            // of course does not work for positions where the piece
            // you expose check to was "moved" two turns ago but these
            // are pretty rare.
            //
            if (!IS_ON_BOARD(c) || (c == cIgnore))
            {
                *pc = pList->data[x].cLoc;
                _RemoveItem(pList, x);
                return(p);
            }
        }
    }

    //
    // There are no legal pieces to move to square
    //
    return(0);
}
#endif // SEE_HEAPS

static void _AddXRays(IN POSITION *pos,
                      IN INT iAttackerColor,
                      IN COOR cTarget,
                      IN COOR cObstacle,
                      IN OUT SEE_LIST *pAttacks,
                      IN OUT SEE_LIST *pDefends)
/**

Routine description:

    We just "moved" a piece in the SEE sequence... add any xray
    attacks that it exposed to the SEE list to be take part as the
    sequence plays out.

Parameters:

    POSITION *pos,
    INT iAttackerColor,
    COOR cTarget,
    COOR cObstacle,
    SEE_LIST *pAttacks,
    SEE_LIST *pDefends

Return value:

    static void

**/
{
    int iDelta;
    COOR cIndex;
    PIECE xPiece;
    int iIndex;

    iIndex = (int)cTarget - (int)cObstacle;

    //
    // If there is no way for a queen sitting on the target square to
    // reach the obsticle square then there is no discovered attack.
    // (This could happen, for instance, if a knight captured.  It
    // can't uncover a new attack on the square where it took.
    //
    if (0 == (CHECK_VECTOR_WITH_INDEX(iIndex, BLACK) & (1 << QUEEN)))
    {
        return;
    }

    //
    // The squares are on the same rank, file or diagonal.  iDelta is
    // the way to move from target towards obstacle.
    //
    iDelta = CHECK_DELTA_WITH_INDEX(iIndex);

    //
    // Search for a piece that moves the right way to attack the target
    // square starting at the obstacle square + delta.
    //
    for (cIndex = cObstacle + iDelta;
         IS_ON_BOARD(cIndex);
         cIndex += iDelta)
    {
        xPiece = pos->rgSquare[cIndex].pPiece;
        if (!IS_EMPTY(xPiece))
        {
            //
            // Does it move the right way to hit cTarget?  TODO: can this
            // be optimized?  Remember pawns though...
            //
            if (0 != (CHECK_VECTOR_WITH_INDEX((int)cIndex - (int)cTarget,
                                              GET_COLOR(xPiece)) &
                      (1 << PIECE_TYPE(xPiece))))
            {
                //
                // Add this attacker to the proper SEE_LIST
                //
                if (GET_COLOR(xPiece) == iAttackerColor)
                {
                    pAttacks->data[pAttacks->uCount].pPiece = xPiece;
                    pAttacks->data[pAttacks->uCount].cLoc = cIndex;
                    pAttacks->data[pAttacks->uCount].uVal =
                        PIECE_VALUE(xPiece);
                    pAttacks->uCount++;
                    ASSERT(pAttacks->uCount > 0);
#ifdef SEE_HEAPS
                    _BubbleUp(pAttacks, pAttacks->uCount - 1);
#else
                    _SortList(pAttacks);
#endif
                }
                else
                {
                    pDefends->data[pDefends->uCount].pPiece = xPiece;
                    pDefends->data[pDefends->uCount].cLoc = cIndex;
                    pDefends->data[pDefends->uCount].uVal =
                        PIECE_VALUE(xPiece);
                    pDefends->uCount++;
                    ASSERT(pDefends->uCount > 0);
#ifdef SEE_HEAPS
                    _BubbleUp(pDefends, pDefends->uCount - 1);
#else
                    _SortList(pDefends);
#endif
                }
            }
            return;
        }
    }
}


SCORE
SEE(IN POSITION *pos,
    IN MOVE mv)
/**

Routine description:

    Given a board and a move on the board, estimate the value of the
    move by considering the friend/enemy pieces that attack the move's
    destination square.

Parameters:

    POSITION *pos,
    MOVE mv

Return value:

    SCORE : the estimate of the move's score

**/
{
    SEE_LIST rgPieces[2];
    PIECE pPiece;
    ULONG uInPeril;
    SCORE rgiList[32];
    ULONG uListIndex;
    ULONG uWhoseTurn = GET_COLOR(mv.pMoved);
    ULONG uOrig = uWhoseTurn;
    int iSign = 1;
    ULONG uPromValue;
    ULONG uVal;
    COOR cFrom = mv.cFrom;
    static FLAG _Table[2][3] = {
        // a<b    a==b   a>b
        {  FALSE, FALSE,  TRUE   },  // uVal==0
        {  TRUE,  FALSE,  FALSE  },  // uVal==1
    };

#ifdef DEBUG
    ASSERT(mv.uMove != 0);
    memset(rgiList, 0xFF, sizeof(rgiList));
    memset(rgPieces, 0xFF, sizeof(rgPieces));
#endif

    //
    // Create a sorted list of pieces attacking and defending the
    // square.
    //
    GetAttacks(&(rgPieces[uWhoseTurn]), pos, mv.cTo, uWhoseTurn);
    GetAttacks(&(rgPieces[FLIP(uWhoseTurn)]), pos, mv.cTo, FLIP(uWhoseTurn));
#ifdef SEE_HEAPS
    _BuildHeap(&(rgPieces[FLIP(uWhoseTurn)]));
    _BuildHeap(&(rgPieces[uWhoseTurn]));
#else
    _SortList(&(rgPieces[FLIP(uWhoseTurn)]));
    _SortList(&(rgPieces[uWhoseTurn]));
#endif

    //
    // Play the first move -- TODO: the first move may be illegal...
    // fix this?
    //
    rgiList[0] = (PIECE_VALUE(mv.pCaptured) +
                  PIECE_VALUE(mv.pPromoted));
    uInPeril = (PIECE_VALUE(mv.pMoved) +
                PIECE_VALUE(mv.pPromoted));
    uListIndex = 1;

    _ClearPieceByLocation(&(rgPieces[uWhoseTurn]), mv.cFrom);
    _AddXRays(pos,
              uWhoseTurn,
              mv.cTo,
              mv.cFrom,
              &(rgPieces[uWhoseTurn]),
              &(rgPieces[FLIP(uWhoseTurn)]));

    //
    // Play moves 2..n
    //
    do
    {
        //
        // Other side's turn now...
        //
        uWhoseTurn = FLIP(uWhoseTurn);
        iSign = -iSign;
        pPiece = _MinLegalPiece(pos,
                                uWhoseTurn,
                                &(rgPieces[uWhoseTurn]),
                                &(rgPieces[FLIP(uWhoseTurn)]),
                                &cFrom,
                                cFrom);
        if (0 == pPiece) break;               // no legal piece
        //
        // If this is a pawn capturing and it ends on the queening
        // rank, set uPromValue appropriately.  Bitwise operators are
        // correct here, done for speed and branch removal; this loop
        // is pretty heavily used.
        //
        uPromValue = IS_PAWN(pPiece) & (RANK1(mv.cTo) | RANK8(mv.cTo));
        uPromValue *= VALUE_QUEEN;
        ASSERT((uPromValue == 0) || (uPromValue == VALUE_QUEEN));

        ASSERT(uListIndex != 0);
        rgiList[uListIndex] = rgiList[uListIndex - 1] +
            iSign * (uInPeril + uPromValue);
        uListIndex++;
        ASSERT(uListIndex < ARRAY_LENGTH(rgiList));
        uInPeril = PIECE_VALUE(pPiece) + uPromValue;

        _AddXRays(pos,
                  GET_COLOR(mv.pMoved),
                  mv.cTo,
                  cFrom,
                  &(rgPieces[uOrig]),        // These must be rgAttacks and
                  &(rgPieces[FLIP(uOrig)])); // rgDefends.  Not based on tomove
    }
    while(1);

    //
    // The swaplist is now complete but we still must consider that either
    // side has the option of not taking (not continuing the exchange).
    //
    ASSERT(uListIndex >= 1);
    uListIndex--;

    while (uListIndex > 0)
    {
        uVal = (uListIndex & 1);
        iSign = ((rgiList[uListIndex] > rgiList[uListIndex - 1]) -
                 (rgiList[uListIndex] < rgiList[uListIndex - 1])) + 1;
        ASSERT((iSign >= 0) && (iSign <= 2));
        if (TRUE == _Table[uVal][iSign])
        {
            rgiList[uListIndex - 1] = rgiList[uListIndex];
        }
        uListIndex--;
    }

#ifdef TEST_BROKEN
    iSign = DebugSEE(pos, mv);

    if ((rgiList[0] != iSign) &&
        (iSign != INVALID_SCORE))
    {
        DumpPosition(pos);
        DumpMove(mv.uMove);
        Trace("Real SEE says: %d\n"
              "Test SEE says: %d\n",
              rgiList[0], iSign);
        UtilPanic(TESTCASE_FAILURE,
                  NULL,
                  "See mismatch",
                  rgiList[0], 
                  iSign,
                  __FILE__, __LINE__);
    }
#endif
    return(rgiList[0]);
}
