/**

Copyright (c) Scott Gasch

Module Name:

    board.c

Abstract:

    Chess-board related functions.

Author:

    Scott Gasch (scott.gasch@gmail.com) 15 Apr 2004

Revision History:

    $Id: board.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

void
SetRootToInitialPosition(void)
/**

Routine description:

    Sets the root position to the initial position.

Parameters:

    none

Return value:

    void

**/
{
#ifdef DEBUG
    POSITION *pos;
#endif

    (void)SetRootPosition(STARTING_POSITION_IN_FEN);
#ifdef DEBUG
    pos = GetRootPosition();

    //
    // Sanity check the sig system.  Note: if this changes then any
    // opening books need to be reconstructed.
    //
    ASSERT(pos->u64NonPawnSig == 0xd46f3e84d897a443ULL);
    ASSERT(pos->u64PawnSig == 0xa9341a54d7352d3cULL);

    ASSERT_ASM_ASSUMPTIONS;
#endif
}


CHAR *
DrawTextBoardFromPosition(POSITION *pos)
/**

Routine description:

    Draw a text-based representation of the board configuration in
    POSITION pos and returned it as a string.  Note: this function is
    _NOT_ thread safe.

Parameters:

    POSITION *pos : position to draw

Return value:

    static char * : my drawing

**/
{
    static char buf[MEDIUM_STRING_LEN_CHAR];
    COOR f, r;

    memset(buf, 0, sizeof(buf));
    strcpy(buf, "    0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07\n");
    strcat(buf, "   +---------------------------------------+\n");
    for (r = 8; r >= 1; r--)
    {
        sprintf(buf, "%s %u |", buf, r);

        for (f = A; f <= H; f++)
        {
            if (IS_WHITE_SQUARE_FR(f, r))
            {
                if (IS_EMPTY(pos->rgSquare[FILE_RANK_TO_COOR(f,r)].pPiece))
                {
                    sprintf(buf, "%s%s", buf, "    |");
                }
                else
                {
                    sprintf(buf, "%s %2s |", buf,
                        PieceAbbrev(pos->rgSquare[FILE_RANK_TO_COOR(f,r)].pPiece));
                }
            }
            else
            {
                if (IS_EMPTY(pos->rgSquare[FILE_RANK_TO_COOR(f,r)].pPiece))
                {
                    sprintf(buf, "%s%s", buf, "::::|");
                }
                else
                {
                    sprintf(buf, "%s:%2s:|", buf,
                        PieceAbbrev(pos->rgSquare[FILE_RANK_TO_COOR(f,r)].pPiece));
                }
            }
        }
        sprintf(buf, "%s 0x%x0\n", buf, (8 - r));
    }
    strcat(buf, "   +---------------------------------------+\n");
    strcat(buf, "     A    B    C    D    E    F    G    H\n\n");
    return(buf);
}


FLAG
VerifyPositionConsistency(POSITION *pos,
                          FLAG fContinue)
/**

Routine description:

    Verifies the internal consistency of a POSITION structure.  Looks
    for defects like bad piece lists, bad piece counts, bad material
    counts, etc...

Parameters:

    POSITION *pos : the position to verify

Return value:

    FLAG : TRUE if consistent, FALSE otherwise

**/
{
    static char *szExplainations[] = {
        "Pawn signature mismatch",
        "Main signature mismatch",
        "Invalid enpassant square",
        "Material balance for white != -Material balance for black",
        "White castled already and can still castle",
        "Black castled already and can still castle",
        "Pawn list points to non-pawn square on board",
        "Square pointed to by pawn list doesn't point back",
        "Piece list points to non-piece square on board",
        "Square pointed to by the non-piece list doesn't point back",
        "Non-king piece in 0th position of list",
        "Invalid piece sitting in non-pawn list entry",
        "Counted pawn material != expected material sum",
        "Counted piece material != expected material sum",
        "Total army material != expected material sum",
        "More white square bishops than total bishop count",
        "Strange bits flipped in board square",
        "More pawns on board than accounted for in pawn material",
        "More pieces on board than accounted for in piece material",
        "Extra pieces on board that are not accounted for",
        "Fifty move counter is too high",
    };
    ULONG u, v;
    COOR c;
    ULONG uPawnMaterial[2] = {0, 0};
    ULONG uPawnCount[2] = {0, 0};
    ULONG uNonPawnMaterial[2] = {0, 0};
    ULONG uNonPawnCount[2][7];
    ULONG uSigmaNonPawnCount[2] = {0, 0};
    ULONG uWhiteSqBishopCount[2] = {0, 0};
    UINT64 u64Computed;
    PIECE p;
    FLAG fRet = FALSE;
    ULONG uReason = (ULONG)-1;

    memset(uNonPawnCount, 0, sizeof(uNonPawnCount));
    u64Computed = ComputeSig(pos);
    if (pos->u64NonPawnSig != u64Computed)
    {
        uReason = 0;
        goto end;
    }

    u64Computed = ComputePawnSig(pos);
    if (pos->u64PawnSig != u64Computed)
    {
        uReason = 1;
        goto end;
    }

    if (!VALID_EP_SQUARE(pos->cEpSquare))
    {
        uReason = 2;
        goto end;
    }

    if (pos->iMaterialBalance[WHITE] != -pos->iMaterialBalance[BLACK])
    {
        uReason = 3;
        goto end;
    }

    if (pos->iMaterialBalance[WHITE] !=
        ((SCORE)(pos->uPawnMaterial[WHITE] + pos->uNonPawnMaterial[WHITE]) -
         (SCORE)(pos->uPawnMaterial[BLACK] + pos->uNonPawnMaterial[BLACK])))
    {
        uReason = 3;
        goto end;
    }

    FOREACH_COLOR(u)
    {
        //
        // If a side has already castled it shouldn't have legal castling
        // options.
        //
        if (TRUE == pos->fCastled[u])
        {
            if (u == WHITE)
            {
                if (pos->bvCastleInfo & WHITE_CAN_CASTLE)
                {
                    uReason = 4;
                    goto end;
                }
            }
            else
            {
                ASSERT(u == BLACK);
                if (pos->bvCastleInfo & BLACK_CAN_CASTLE)
                {
                    uReason = 5;
                    goto end;
                }
            }
        }

        //
        // Make sure the pawns are where they should be
        //
        for (v = 0; v < pos->uPawnCount[u]; v++)
        {
            c = pos->cPawns[u][v];
            if (IS_ON_BOARD(c))
            {
                if (pos->rgSquare[c].pPiece != ((PAWN << 1) | u))
                {
                    uReason = 6;
                    goto end;
                }

                if (pos->rgSquare[c].uIndex != v)
                {
                    uReason = 7;
                    goto end;
                }
                uPawnMaterial[u] += VALUE_PAWN;
                uPawnCount[u]++;
            }
        }

        //
        // Make sure the non-pawns are where they should be
        //
        for (v = 0; v < 16; v++)
        {
            c = pos->cNonPawns[u][v];
            if (v < pos->uNonPawnCount[u][0])
            {
                if (!IS_ON_BOARD(c))
                {
                    uReason = 8;
                    goto end;
                }
                else
                {
                    p = pos->rgSquare[c].pPiece;
                    if (pos->rgSquare[c].uIndex != v)
                    {
                        uReason = 9;
                        goto end;
                    }

                    //
                    // 0th spot must be a king
                    //
                    if ((v == 0) && (!IS_KING(p)))
                    {
                        uReason = 10;
                        goto end;
                    }

                    if ((!IS_VALID_PIECE(p)) || (PIECE_COLOR(p) != u))
                    {
                        uReason = 11;
                        goto end;
                    }
                    uNonPawnCount[u][PIECE_TYPE(p)]++;
                    uNonPawnMaterial[u] += PIECE_VALUE(p);
                    if ((IS_BISHOP(p)) &&
                        (IS_WHITE_SQUARE_COOR(c)))
                    {
                        uWhiteSqBishopCount[u]++;
                    }
                }
            }
            else
            {
                //
                // The first off-board is the last, the list must not
                // have gaps in it followed by good piece data again.
                //
                break;
            }
        }
    }

    FOREACH_COLOR(u)
    {
        //
        // Make sure the materials and counts match up with what we
        // got when walking the piece lists
        //
        if ((pos->uPawnMaterial[u] != uPawnMaterial[u]) ||
            (pos->uPawnCount[u] != uPawnCount[u]) ||
            (pos->uPawnMaterial[u] != pos->uPawnCount[u] * VALUE_PAWN))
        {
            uReason = 12;
            goto end;
        }

        for (v = KNIGHT; v <= KING; v++)
        {
            if (pos->uNonPawnCount[u][v] != uNonPawnCount[u][v])
            {
                uReason = 13;
                goto end;
            }
            uSigmaNonPawnCount[u] += pos->uNonPawnCount[u][v];
        }

        //
        // Note: the 0th spot in the array is the sum of all non pawns
        //
        if (uSigmaNonPawnCount[u] != pos->uNonPawnCount[u][0])
        {
            uReason = 14;
            goto end;
        }

        //
        // You can't have more white-square bishops than you have
        // bishops.
        //
        if (pos->uWhiteSqBishopCount[u] > pos->uNonPawnCount[u][BISHOP])
        {
            uReason = 15;
            goto end;
        }
    }

    //
    // Now walk the actual board and reduce the material counts we got
    // by walking the piece lists.  If everything is ok then the
    // material counts will end up at 0.  If there are pieces on the
    // board that are not on a list or pieces in a list that are not
    // on the board this will catch it.
    //
    FOREACH_SQUARE(c)
    {
        if (IS_ON_BOARD(c))
        {
            p = pos->rgSquare[c].pPiece;

            if (EMPTY != p)
            {
                //
                // Make sure none of these have strange bits flipped
                //
                if (p & 0xFFFFFFF0)
                {
                    uReason = 16;
                    goto end;
                }

                u = PIECE_COLOR(p);
                if (IS_PAWN(p))
                {
                    uPawnMaterial[u] -= VALUE_PAWN;
                    if (pos->rgSquare[c].uIndex > pos->uPawnCount[u])
                    {
                        uReason = 17;
                        goto end;
                    }
                }
                else
                {
                    uNonPawnMaterial[u] -= PIECE_VALUE(p);
                    if (pos->rgSquare[c].uIndex > pos->uNonPawnCount[u][0])
                    {
                        uReason = 18;
                        goto end;
                    }
                }
            }
        }
    }

    if ((uPawnMaterial[WHITE] + uPawnMaterial[BLACK] +
         uNonPawnMaterial[WHITE] + uNonPawnMaterial[BLACK]) != 0)
    {
        uReason = 19;
        goto end;
    }

//    if (pos->uFifty > 101)
//    {
//        uReason = 20;
//        goto end;
//    }

    //
    // Looks Ok
    //
    fRet = TRUE;

 end:
    if (FALSE == fRet)
    {
        if (FALSE == fContinue)
        {
            UtilPanic(INCONSISTENT_POSITION,
                      pos, szExplainations[uReason], NULL, NULL,
                      __FILE__, __LINE__);
            ASSERT(FALSE);
        }
    }
    return(fRet);
}



FLAG
PositionsAreEquivalent(POSITION *p1,
                       POSITION *p2)
/**

Routine description:

    Compare two positions and decide whether they are the same
    position.  Note: this code is meant to be accessed from test code
    or from non-speed critical codepaths.  Comparing their signatures
    should be sufficient to determine whether they are the same in
    production code.

Parameters:

    POSITION *p1 : first position to compare
    POSITION *p2 : second position to compare

Return value:

    FLAG : TRUE if they are the same position, FALSE otherwise

**/
{
    COOR c;
    ULONG u;

    if ((p1->u64NonPawnSig != p2->u64NonPawnSig) ||
        (p1->u64PawnSig != p2->u64PawnSig) ||
        (p1->uToMove != p2->uToMove) ||
        (p1->uFifty != p2->uFifty) ||
        (p1->bvCastleInfo != p2->bvCastleInfo) ||
        (p1->cEpSquare != p2->cEpSquare))
    {
        return(FALSE);
    }

    FOREACH_COLOR(u)
    {
        if ((p1->fCastled[u] != p2->fCastled[u]) ||
            (p1->uWhiteSqBishopCount[u] != p2->uWhiteSqBishopCount[u]))
        {
            return(FALSE);
        }
    }

    FOREACH_SQUARE(c)
    {
        if (!IS_ON_BOARD(c)) continue;

        if (p1->rgSquare[c].pPiece != p2->rgSquare[c].pPiece)
        {
            return(FALSE);
        }
    }

    VerifyPositionConsistency(p1, FALSE);
    VerifyPositionConsistency(p2, FALSE);
    return(TRUE);
}


char *
CastleInfoString(BITV bv)
/**

Routine description:

    Generate and returns a small string describing the possible
    castling choices in the bitvector passed in for use in a friandly
    display or in a FEN string representing a position.  This function
    is _NOT_ thread safe.

Parameters:

    BITV bv : the castling bitvector

Return value:

    char * : the string, either '-' signifying none possible or some
             combination of:

                 K = white kingside possible
                 Q = white queenside possibe
                 k = black kingside possible
                 q = black queenside possible

**/
{
    static char buf[5];
    char *p = buf;

    memset(buf, 0, sizeof(buf));
    ASSERT((bv & ~CASTLE_ALL_POSSIBLE) == 0);

    if (bv == 0)
    {
        *p++ = '-';
    }
    else
    {
        if (bv & CASTLE_WHITE_SHORT)
        {
            *p++ = 'K';
        }

        if (bv & CASTLE_WHITE_LONG)
        {
            *p++ = 'Q';
        }

        if (bv & CASTLE_BLACK_SHORT)
        {
            *p++ = 'k';
        }

        if (bv & CASTLE_BLACK_LONG)
        {
            *p++ = 'q';
        }
    }
    return(buf);
}


void
DumpPosition(POSITION *pos)
/**

Routine description:

    Dump an ASCII representation of POSITION pos on stdout.

Parameters:

    POSITION *pos

Return value:

    void

**/
{
    char *p;
#ifdef DEBUG
    ULONG u;
    if (!VerifyPositionConsistency(pos, TRUE))
    {
        Bug("===========================================================================\n"
            " WARNING: The following position seems to be inconsistent.  I'll do my best\n"
            " to render it but this may not be such a good idea...\n"
            "===========================================================================\n\n");
    }
#endif
    p = PositionToFen(pos);
    if (g_Options.fRunningUnderXboard)
    {
        Trace("; PositionToFen(pos): %s\n", p);
        goto end;
    }
    else
    {
        Trace("FEN: %s\n\n", p);
    }

#ifdef DEBUG
    Trace("--------------------------------------------------------------------------------\n"
          "PositionToFen(pos): %s\n\n"

          "POSITION (@%p):\n"
          "  +0x%03x u64NonPawnSig             : %"
              COMPILER_LONGLONG_HEX_FORMAT "\n"
          "  +0x%03x u64PawnSig                : %"
              COMPILER_LONGLONG_HEX_FORMAT "\n"
          "         u64NonPawnSig ^ u64PawnSig: %"
              COMPILER_LONGLONG_HEX_FORMAT "\n"
          "  +0x%03x pSquare[128]              : \n\n"
          "%s"
          "  +0x%03x uToMove                   : 0x%02x (%s)\n"
          "  +0x%03x uFifty                    : 0x%02x (%u moves)\n"
          "  +0x%03x fCastled[2]               : W:0x%02x         B:0x%02x\n"
          "  +0x%03x bvCastleInfo              : 0x%02x (%s)\n"
          "  +0x%03x cEpSquare                 : 0x%02x (%s)\n"
          "  +0x%03x cPawns[2][8]              : \n",
          p,
          pos,
          OFFSET_OF(u64PawnSig, POSITION), pos->u64PawnSig,
          OFFSET_OF(u64NonPawnSig, POSITION), pos->u64NonPawnSig,
          pos->u64NonPawnSig ^ pos->u64PawnSig,
          OFFSET_OF(rgSquare, POSITION), DrawTextBoardFromPosition(pos),
          OFFSET_OF(uToMove, POSITION), pos->uToMove, ColorToString(pos->uToMove),
          OFFSET_OF(uFifty, POSITION), pos->uFifty, pos->uFifty,
          OFFSET_OF(fCastled, POSITION), pos->fCastled[WHITE], pos->fCastled[BLACK],
          OFFSET_OF(bvCastleInfo, POSITION), pos->bvCastleInfo, CastleInfoString(pos->bvCastleInfo),
          OFFSET_OF(cEpSquare, POSITION), pos->cEpSquare, (IS_ON_BOARD(pos->cEpSquare) ? CoorToString(pos->cEpSquare) : "none"),
          OFFSET_OF(cPawns, POSITION));

    Trace("    W: ");
    for (u = 0; u < pos->uPawnCount[WHITE]; u++)
    {
        Trace("0x%02x[%s] ",
              pos->cPawns[WHITE][u],
              CoorToString(pos->cPawns[WHITE][u]));
        ASSERT(IS_ON_BOARD(pos->cPawns[WHITE][u]));
    }

    Trace("\n"
          "    B: ");
    for (u = 0; u < pos->uPawnCount[BLACK]; u++)
    {
        Trace("0x%02x[%s] ",
              pos->cPawns[BLACK][u],
              CoorToString(pos->cPawns[BLACK][u]));
        ASSERT(IS_ON_BOARD(pos->cPawns[BLACK][u]));
    }

    Trace("\n"
          "  +0x%03x uPawnMaterial[2]          : W:0x%02x (%u, pawn=%u)\n"
          "                                   : B:0x%02x (%u, pawn=%u)\n"
          "  +0x%03x uPawnCount[2]             : W:%1u pawns      B:%1u pawns\n",
          OFFSET_OF(uPawnMaterial, POSITION),
          pos->uPawnMaterial[WHITE], pos->uPawnMaterial[WHITE], VALUE_PAWN,
          pos->uPawnMaterial[BLACK], pos->uPawnMaterial[BLACK], VALUE_PAWN,
          OFFSET_OF(uPawnCount, POSITION),
          pos->uPawnCount[WHITE], pos->uPawnCount[BLACK]);

    Trace("  +0x%03x cNonPawns[2][16]          : \n"
          "    W: ", OFFSET_OF(cNonPawns, POSITION));
    for (u = 0; u < pos->uNonPawnCount[WHITE][0]; u++)
    {
        Trace("0x%02x[%s] ",
              pos->cNonPawns[WHITE][u],
              CoorToString(pos->cNonPawns[WHITE][u]));
        ASSERT(IS_ON_BOARD(pos->cNonPawns[WHITE][u]));
    }
    Trace("\n"
          "    B: ");
    for (u = 0; u < pos->uNonPawnCount[BLACK][0]; u++)
    {
        Trace("0x%02x[%s] ",
              pos->cNonPawns[BLACK][u],
              CoorToString(pos->cNonPawns[BLACK][u]));
        ASSERT(IS_ON_BOARD(pos->cNonPawns[BLACK][u]));
    }
    Trace("\n"
          "  +0x%03x uNonPawnMaterial[2]       : W: 0x%02x (%u, pawn=%u)\n"
          "                                   : B: 0x%02x (%u, pawn=%u)\n"
          "  +0x%03x uNonPawnCount[2][7]       : WHITE           BLACK\n"
          "                            KNIGHT : %u              %u\n"
          "                            BISHOP : %u              %u\n"
          "               WHITE SQUARE BISHOP : %u              %u\n"
          "                              ROOK : %u              %u\n"
          "                             QUEEN : %u              %u\n"
          "                               SUM : %u              %u\n",
          OFFSET_OF(uNonPawnMaterial, POSITION),
          pos->uNonPawnMaterial[WHITE], pos->uNonPawnMaterial[WHITE],
          VALUE_PAWN,
          pos->uNonPawnMaterial[BLACK], pos->uNonPawnMaterial[BLACK],
          VALUE_PAWN,
          OFFSET_OF(uNonPawnCount, POSITION),
          pos->uNonPawnCount[WHITE][KNIGHT], pos->uNonPawnCount[BLACK][KNIGHT],
          pos->uNonPawnCount[WHITE][BISHOP], pos->uNonPawnCount[BLACK][BISHOP],
          pos->uWhiteSqBishopCount[WHITE], pos->uWhiteSqBishopCount[BLACK],
          pos->uNonPawnCount[WHITE][ROOK], pos->uNonPawnCount[BLACK][ROOK],
          pos->uNonPawnCount[WHITE][QUEEN], pos->uNonPawnCount[BLACK][QUEEN],
          pos->uNonPawnCount[WHITE][0], pos->uNonPawnCount[BLACK][0]);
#else
    Trace("\n%s\n", DrawTextBoardFromPosition(pos));
#endif

 end:
    if (NULL != p)
    {
        SystemFreeMemory(p);
    }
}

//
// TODO: DumpBoard
//
