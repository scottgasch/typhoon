/**

Copyright (c) Scott Gasch

Module Name:

    search.c

Abstract:

    Recursive chess tree searching.  See also split.c.

    "A type 1 node is also called a PV node.  The root of the tree is
    a type-1 node, and the *first* successor of a type-1 node is a
    type-1 node also.  A type-1 node must have all branches examined,
    but it is unique in that we don't know anything about alpha and
    beta yet, because we haven't searched the first move to establish
    them.

    A type 2 node is either (a) a successor of any type-3 node, or,
    (b) it's any successor (other than the first) of a type-1 node.
    With perfect move ordering, the first branch at a type-2 node will
    "refute" the move made at the previous ply via the alpha/beta
    algorithm.  This node requires good move ordering, because you
    have to find a move good enough that your opponent would not play
    the move he chose that led to this position.  If you try a poor
    move first, it won't produce a cutoff, and you have to search
    another move (or more) until you find the "good" move that would
    make him not play his move.

    A type-3 node follows a type-2 node.  Here, you have to try every
    move at your disposal.  Since your opponent (at the previous ply)
    has played a "strong" move (supposedly the "best" move) you are
    going to have to try every move you have in an effort to refute
    this move.  None will do so (unless your opponent tried some poor
    move first due to incorrect move ordering).  Here, move ordering
    is not worth the trouble, since the ordering won't let you avoid
    searching some moves.  Of course, with the transposition /
    refutation table, ordering might help you get more "hits" if your
    table is not large enough...

    As you can see, at type-1 nodes you have to do good move ordering
    to choose that "1" move (or to choose that one out of a very few)
    that is good enough to cause a cutoff, while avoiding choosing
    those that are no good.  At a type-1 node, the same thing applies.
    If you don't pick the best move first (take the moves at the root
    for example) you will search an inferior move, establish alpha or
    beta incorrectly, and thereby increase the size of the total tree
    by a *substantial* amount.

    By the way, some authors call type-1 nodes "PV" nodes, type-2
    nodes "CUT" nodes, and type-3 nodes "ALL" nodes.  These make it
    easier to read, but, unfortunately, I "cut my teeth" on the
    Knuth/Moore paper and think in terms of type 1,2,3."

                                                 --Bob Hyatt, r.g.c.c

Author:

    Scott Gasch (scott.gasch@gmail.com) 21 May 2004

Revision History:

    $Id: search.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

extern ULONG g_uIterateDepth;
extern FLAG g_fCanSplit[MAX_PLY_PER_SEARCH];

#define TRY_HASH_MOVE         (0)
#define GENERATE_MOVES        (1)
#define PREPARE_TO_TRY_MOVES  (2)
#define TRY_GENERATED_MOVES   (3)

#ifdef DEBUG
#define VERIFY_HASH_HIT                                \
    ASSERT(IS_VALID_SCORE(iScore));                    \
    ASSERT(((ULONG)pHash->uDepth << 4) >= uDepth);     \
    switch (pHash->bvFlags & HASH_FLAG_VALID_BOUNDS)   \
    {                                                  \
        case HASH_FLAG_LOWER:                          \
            ASSERT(iScore >= iBeta);                   \
            ASSERT(iScore > -NMATE);                   \
            ASSERT(iScore <= +NMATE);                  \
            break;                                     \
        case HASH_FLAG_UPPER:                          \
            ASSERT(iScore <= iAlpha);                  \
            ASSERT(iScore < +NMATE);                   \
            ASSERT(iScore >= -NMATE);                  \
            break;                                     \
        case HASH_FLAG_EXACT:                          \
            ASSERT((-NMATE <= iScore) &&               \
                   (iScore <= +NMATE));                \
            break;                                     \
        default:                                       \
            ASSERT(FALSE);                             \
    }
#else
#define VERIFY_HASH_HIT
#endif

/**

Routine description:

    This is the full-width portion of the main chess tree search.  In
    general, its job is to ask the move generator to make a list of
    all the moves possible at the board position in ctx, to make each
    move in turn, and to search each resulting position recursively.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : the context to search in
    SCORE iAlpha : the lower bound of the interesting score window
    SCORE iBeta : the upper bound of the interesting score window
    ULONG uDepth : the depth remaining before QSearch is invoked

Return value:

    SCORE : a score

    Also affects the transposition table, searcher context, and just
    about every other large data structure in the engine...

**/
SCORE FASTCALL
Search(IN SEARCHER_THREAD_CONTEXT *ctx,
       IN SCORE iAlpha,
       IN SCORE iBeta,
       IN ULONG uDepth)
{
    POSITION *pos = &ctx->sPosition;
    PLY_INFO *pi = &ctx->sPlyInfo[ctx->uPly];
    CUMULATIVE_SEARCH_FLAGS *pf = &ctx->sSearchFlags;
    MOVE mvLast = (pi-1)->mv;
    SCORE iBestScore = -INFINITY;
    SCORE iInitialAlpha;
    SCORE iRoughEval;
    MOVE mv, mvBest, mvHash;
    ULONG x = 0;
    SCORE iScore;
    INT iOrigExtend = 0;
    INT iExtend;
    ULONG uNextDepth;
    ULONG uLegalMoves = 0;
    HASH_ENTRY *pHash;
    FLAG fThreat;
    FLAG fSkipNull;
    FLAG fIsDraw;
    ULONG uStage = TRY_HASH_MOVE;
    ULONG u;
    ULONG uFutilityMargin = 0;
#ifdef DEBUG
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT(ctx->uPly > 0);
    ASSERT((mvLast.uMove != 0) || (pf->fAvoidNullmove == TRUE));
    ASSERT(IS_VALID_FLAG(pf->fAvoidNullmove));
    ASSERT(IS_VALID_FLAG(pf->fVerifyNullmove));
    memcpy(&pi->sPosition, pos, sizeof(POSITION));
#endif

    // Jump directly to Qsearch if remaining depth is low enough.
    // This is the only place Qsearch is entered.
    if (uDepth < ONE_PLY)
    {
        pf->fCouldStandPat[BLACK] = pf->fCouldStandPat[WHITE] = FALSE;
        pf->uQsearchNodes = pf->uQsearchDepth = 0;
        pf->uQsearchCheckDepth = QPLIES_OF_NON_CAPTURE_CHECKS;
        pi->fInQsearch = TRUE;
        iBestScore = QSearch(ctx, iAlpha, iBeta);
        ASSERT(pf->uQsearchNodes < 20000);
        ASSERT(pf->uQsearchDepth == 0);
        goto end;
    }
    pi->fInQsearch = FALSE;

    // Common initialization code (which may cause a cutoff or change the
    // bounds or decide that we need to stop searching now).
    if (TRUE == CommonSearchInit(ctx,
                                 &iAlpha,
                                 &iBeta,
                                 &iBestScore))
    {
        goto end;
    }
    DTEnterNode(ctx, uDepth, FALSE, iAlpha, iBeta);
    iInitialAlpha = iAlpha;
    ASSERT((IS_CHECKING_MOVE(mvLast) && (TRUE == pi->fInCheck)) ||
           (!IS_CHECKING_MOVE(mvLast) && (FALSE == pi->fInCheck)));

    // Prepare next depth for nullmove and hashtable lookup
    uNextDepth = uDepth - SelectNullmoveRFactor(ctx, uDepth) - ONE_PLY;
    if (uNextDepth > MAX_DEPTH_PER_SEARCH) uNextDepth = 0;

    // Check the transposition table.  This may give us a cutoff
    // without doing any work if we have previously stored the score
    // of this search in the hash.  It also may set mvHash even if it
    // can't give us a cutoff.  It also may set fSkipNull (see below)
    // based on uNextDepth to inform us that a nullmove search is
    // unlikely to succeed here.
    mvHash.uMove = 0;
    pHash = HashLookup(ctx,
                       uDepth,
                       uNextDepth,
                       iAlpha,
                       iBeta,
                       &fThreat,
                       &fSkipNull,
                       &mvHash,
                       &iScore);
    if (NULL != pHash)
    {
        VERIFY_HASH_HIT;
        u = pHash->bvFlags & HASH_FLAG_VALID_BOUNDS;
        if (0 != mvHash.uMove)
        {
            // This is an idea posted by Dieter Brusser on CCC:  If we
            // get a hash hit that leads to a draw then only accept it
            // if it has a score of zero, allows us to fail high when
            // a draw would also have allowed a fail high, or allows a
            // fail low when a draw would also have allowed a fail
            // low.
            VERIFY(MakeMove(ctx, mvHash));
            fIsDraw = IsDraw(ctx);
            UnmakeMove(ctx, mvHash);
            if ((FALSE == fIsDraw) || (iScore == 0) ||
               ((u == HASH_FLAG_LOWER) && (iScore >= iBeta) && (0 >= iBeta)) ||
               ((u == HASH_FLAG_UPPER) && (iScore <= iAlpha) && (0 <= iAlpha)))
            {
                if ((iAlpha < iScore) && (iScore < iBeta))
                {
                    UpdatePV(ctx, HASHMOVE);
                }
                iBestScore = iScore;
                goto end;
            }
        }
        else
        {
            // The hash move is empty.  Either this is an upper bound
            // in which case we have no best move since the node that
            // generated it was a fail low -or- this is a lower bound
            // recorded after a null move search.  In the latter case
            // we only accept the cutoff if we are considering null
            // moves at this node too.
            ASSERT(u != HASH_FLAG_EXACT);
            if ((HASH_FLAG_UPPER == u) || (FALSE == pf->fAvoidNullmove))
            {
                ASSERT(((u == HASH_FLAG_UPPER) && (iScore <= iAlpha)) ||
                       ((u == HASH_FLAG_LOWER) && (iScore >= iBeta)));
                iBestScore = iScore;
                goto end;
            }
        }
    }

    // Probe interior node recognizers; allow probes of ondisk EGTB files
    // if it looks like we can get hit.
    switch(RecognLookup(ctx, &iScore, ctx->uPly <= (g_uIterateDepth / 2)))
    {
        case UNRECOGNIZED:
            break;
        case RECOGN_EXACT:
        case RECOGN_EGTB:
            if ((iAlpha < iScore) && (iScore < iBeta))
            {
                UpdatePV(ctx, RECOGNMOVE);
            }
            iBestScore = iScore;
            goto end;
        case RECOGN_LOWER:
            if (iScore >= iBeta)
            {
                iBestScore = iScore;
                goto end;
            }
            break;
        case RECOGN_UPPER:
            if (iScore <= iAlpha)
            {
                iBestScore = iScore;
                goto end;
            }
            break;
#ifdef DEBUG
        default:
            ASSERT(FALSE);
#endif
    }

    // Maybe do nullmove pruning
    iRoughEval = GetRoughEvalScore(ctx, iAlpha, iBeta, FALSE);
    GENERATE_NO_MOVES;
    if (!fSkipNull &&
        !fThreat &&
        WeShouldTryNullmovePruning(ctx,
                                   iAlpha,
                                   iBeta,
                                   iRoughEval,
                                   uNextDepth))
    {
        if (TryNullmovePruning(ctx,
                               &fThreat,
                               iAlpha,
                               iBeta,
                               uNextDepth,
                               &iOrigExtend,
                               &iScore))
        {
            if (iScore > iBeta) {
                StoreLowerBound(mvHash, pos, iScore, uDepth, FALSE);
            }
            iBestScore = iScore; // TODO: try just beta here
            goto end;
        }
    }
    
    // Maybe increment positional extension level b/c of nullmove search
    // or hash table results.
    if (fThreat)
    {
        iOrigExtend += THREE_QUARTERS_PLY;
        INC(ctx->sCounters.extension.uMateThreat);
    }
       
    // Main search loop, try moves under this position.  Before we get
    // into the move loop, save the extensions merited by this
    // position in the tree (pre-move) and the original search flags.
    // Also clear the avoid null bit in the search flags -- we were
    // either told to avoid it or not but there is no need to avoid it
    // for the rest of the line...
    mvBest.uMove = 0;
    pf->fAvoidNullmove = FALSE;
    do
    {
        ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));

        // Becase we want to try the hash move before generating any
        // moves (in case it fails high and we can avoid the work) we
        // have this ugly crazy looking switch statement...
        switch(uStage)
        {
            case TRY_HASH_MOVE:
                uStage++;
                x = 0;
                ASSERT(iBestScore == -INFINITY);
                ASSERT(uLegalMoves == 0);
                if (mvHash.uMove != 0)
                {
                    mv = mvHash;
                    break;
                }
                // else fall through

            case GENERATE_MOVES:
                ASSERT(ctx->uPly > 0);
                ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
                x = ctx->sMoveStack.uBegin[ctx->uPly];
                uStage++;
                if (IS_CHECKING_MOVE(mvLast))
                {
                    ASSERT(InCheck(pos, pos->uToMove));
                    GenerateMoves(ctx, mvHash, GENERATE_ESCAPES);
                    if (MOVE_COUNT(ctx, ctx->uPly))
                    {
                        if (NUM_CHECKING_PIECES(ctx, ctx->uPly) > 1)
                        {
                            iOrigExtend += QUARTER_PLY;
                            INC(ctx->sCounters.extension.uMultiCheck);
                        } else if (NUM_KING_MOVES(ctx, ctx->uPly) == 0) {
                            iOrigExtend += QUARTER_PLY;
                            INC(ctx->sCounters.extension.uNoLegalKingMoves);
                        }
                    }
                }
                else
                {
                    ASSERT(!InCheck(pos, pos->uToMove));
                    GenerateMoves(ctx, mvHash, GENERATE_ALL_MOVES);
                }
                // fall through

            case PREPARE_TO_TRY_MOVES:
                ASSERT(x == ctx->sMoveStack.uBegin[ctx->uPly]);
                ASSERT((uLegalMoves == 0) ||
                       ((uLegalMoves == 1) && (mvHash.uMove)));
#ifdef DO_IID
                if (MOVE_COUNT(ctx, ctx->uPly))
                {
                    SelectBestNoHistory(ctx, x);

                    // EXPERIMENT: If we got no best move from the
                    // hash table and the best move we got from the
                    // generator looks crappy (i.e. is not a winning
                    // or even capture/promotion) then rescore the
                    // moves we generated at this ply using a
                    // shallower search.  "Internal Iterative
                    // Deepening" or something like it.
                    if ((iAlpha + 1 != iBeta) &&
                        (mvHash.uMove == 0) &&
                        (ctx->sMoveStack.mvf[x].iValue < SORT_THESE_FIRST) &&
                        (uDepth >= FOUR_PLY))
                    {
                        ASSERT(uDepth >= (IID_R_FACTOR + ONE_PLY));
                        ASSERT(ctx->sSearchFlags.fAvoidNullmove == FALSE);
                        ctx->sSearchFlags.fAvoidNullmove = TRUE;
                        RescoreMovesViaSearch(ctx, uDepth, iAlpha, iBeta);
                        ctx->sSearchFlags.fAvoidNullmove = FALSE;
                    }
                }
#endif
                // This is very similar to Ernst Heinz's "extended
                // futility pruning" except that it uses the added
                // dynamic criteria of "ValueOfMaterialInTrouble"
                // condition as a safety net.  Note: before we actually
                // prune away moves we will also make sure there is
                // no per-move extension.
                ASSERT(!uFutilityMargin);
                if ((iRoughEval + VALUE_ROOK <= iAlpha) &&
                    (uDepth <= TWO_PLY) &&
                    (ctx->uPly >= 2) &&
                    (iOrigExtend == 0) &&
                    (ctx->sPlyInfo[ctx->uPly - 2].iExtensionAmount <= 0) &&
                    (ValueOfMaterialInTroubleDespiteMove(pos, pos->uToMove)))
                {
                    uFutilityMargin = (iAlpha - iRoughEval) / 2;
                    ASSERT(uFutilityMargin);
                }
                uStage++;
                ASSERT(x == ctx->sMoveStack.uBegin[ctx->uPly]);
                // fall through

            case TRY_GENERATED_MOVES:
                if (x < ctx->sMoveStack.uEnd[ctx->uPly]) 
                {
                    ASSERT(x >= ctx->sMoveStack.uBegin[ctx->uPly]);
                    if (uLegalMoves < SEARCH_SORT_LIMIT(ctx->uPly))
                    {
                        SelectBestWithHistory(ctx, x);
                    }
                    mv = ctx->sMoveStack.mvf[x].mv;
#ifdef DEBUG
                    ASSERT(0 == (ctx->sMoveStack.mvf[x].bvFlags &
                                 MVF_MOVE_SEARCHED));
                    ctx->sMoveStack.mvf[x].bvFlags |= MVF_MOVE_SEARCHED;
#endif
                    mv.bvFlags |= WouldGiveCheck(ctx, mv);

                    // Note: x is the index of the NEXT move to be
                    // considered, this move's index is (x-1).
                    x++;
                    break;
                }
                // else fall through

            default:
                goto no_more_moves;
        }
        ASSERT(mv.uMove);
        ASSERT(SanityCheckMove(pos, mv));

#ifdef MP
        // Can we search the remaining moves in parallel?
        ASSERT((uDepth / ONE_PLY - 1) >= 0);
        ASSERT((uDepth / ONE_PLY - 1) < MAX_PLY_PER_SEARCH);
        if (((uLegalMoves >= 2)) &&
            (0 != g_uNumHelpersAvailable) &&
            (0 == uFutilityMargin) &&
            (TRUE == g_fCanSplit[uDepth / ONE_PLY - 1]) &&
            (MOVE_COUNT(ctx, ctx->uPly) > 3))
        {
            ASSERT(pf->fAvoidNullmove == FALSE);
            ASSERT(uStage == TRY_GENERATED_MOVES);
            ASSERT(x != 0);
            ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
            ASSERT(iBestScore <= iAlpha);
            ctx->sMoveStack.mvf[x-1].bvFlags &= ~MVF_MOVE_SEARCHED;
            iScore = StartParallelSearch(ctx,
                                         &iAlpha,
                                         iBeta,
                                         &iBestScore,
                                         &mvBest,
                                         (x - 1),
                                         iOrigExtend,
                                         uDepth);
            ASSERT(iAlpha < iBeta);
            ASSERT((IS_SAME_MOVE(pi->PV[ctx->uPly], mvBest)) ||
                   (iScore <= iAlpha) || (iScore >= iBeta));
            ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
#ifdef DEBUG
            VerifyPositionConsistency(pos, FALSE);
#endif
            if (IS_VALID_SCORE(iScore))
            {
                pi->mvBest = mvBest;
                goto no_more_moves;
            }
            else
            {
                ASSERT(WE_SHOULD_STOP_SEARCHING);
                iBestScore = iScore;
                goto end;
            }
            ASSERT(FALSE);
        }
#endif

        if (TRUE == MakeMove(ctx, mv))
        {
            uLegalMoves++;
            ASSERT((IS_CHECKING_MOVE(mv) && InCheck(pos, pos->uToMove)) ||
                   (!IS_CHECKING_MOVE(mv) && !InCheck(pos, pos->uToMove)));

            // Compute per-move extension (as opposed to per-position
            // extensions which are represented by iOrigExtend).
            iExtend = iOrigExtend;
            ComputeMoveExtension(ctx,
                                 iAlpha,
                                 iBeta,
                                 (x - 1),     // Note: x==0 if doing mvHash
                                 iRoughEval,
                                 uDepth,
                                 &iExtend);

            // Decide whether or not to do history pruning
            if (TRUE == WeShouldDoHistoryPruning(iRoughEval,
                                                 iAlpha,
                                                 iBeta,
                                                 ctx,
                                                 uDepth,
                                                 uLegalMoves,
                                                 mv,
                                                 (x - 1), // Note: x==0 if hash
                                                 iExtend))
            {
                ASSERT(iExtend == 0);
                iExtend = -ONE_PLY;
                pi->iExtensionAmount = -ONE_PLY;
            }

            // Maybe even "futility prune" this move away.
            if ((x != 0) &&
                (uLegalMoves > 1) &&
                (uFutilityMargin) &&
                (ComputeMoveScore(ctx, mv, (x - 1)) < uFutilityMargin) &&
                (iExtend <= 0) &&
                (!IS_ESCAPING_CHECK(mv)))
            {
                // TODO: test this more carefully
                ASSERT(!IS_CHECKING_MOVE(mv));
                UnmakeMove(ctx, mv);
                ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
            }
            else
            {
                // Compute the next search depth for this move/subtree.
                uNextDepth = uDepth - ONE_PLY + iExtend;
                if (uNextDepth >= MAX_DEPTH_PER_SEARCH) uNextDepth = 0;
                ASSERT(pf->fAvoidNullmove == FALSE);
                if (iBestScore == -INFINITY)
                {
                    // First move, full a..b window.
                    ASSERT(uLegalMoves == 1);
                    iScore = -Search(ctx, -iBeta, -iAlpha, uNextDepth);
                }
                else
                {
                    // Moves 2..N, try a minimal window search
                    iScore = -Search(ctx, -iAlpha - 1, -iAlpha, uNextDepth);
                    if ((iAlpha < iScore) && (iScore < iBeta))
                    {
                        iScore = -Search(ctx, -iBeta, -iAlpha, uNextDepth);
                    }
                }

                // Research deeper if history pruning failed
                if ((iExtend < 0) && (iScore >= iBeta))
                {
                    uNextDepth += ONE_PLY;
                    pi->iExtensionAmount = 0;
                    iScore = -Search(ctx, -iBeta, -iAlpha, uNextDepth);
                }
                UnmakeMove(ctx, mv);
                ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
                if (WE_SHOULD_STOP_SEARCHING)
                {
                    iBestScore = iScore;
                    goto end;
                }

                // Check results
                ASSERT(iBestScore <= iAlpha);
                ASSERT(iAlpha < iBeta);
                if (iScore > iBestScore)
                {
                    iBestScore = iScore;
                    mvBest = mv;
                    pi->mvBest = mv;
                    
                    if (iScore > iAlpha)
                    {
                        if (iScore >= iBeta)
                        {
                            // Update history and killers list and store in
                            // the transposition table.
                            UpdateDynamicMoveOrdering(ctx, 
                                                      uDepth, 
                                                      mv, 
                                                      iScore, 
                                                      x);
                            StoreLowerBound(mv, pos, iScore, uDepth, fThreat);
                            KEEP_TRACK_OF_FIRST_MOVE_FHs(uLegalMoves == 1);
                            ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));
                            goto end;
                        }
                        else
                        {
                            // PV move...
                            UpdatePV(ctx, mv);
                            iAlpha = iScore;
                        }
                    }
                }
            }
        }
    }
    while(1); // foreach move

 no_more_moves:
    ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));

    // Detect checkmates and stalemates
    if (0 == uLegalMoves)
    {
        if (pi->fInCheck)
        {
            ASSERT(IS_CHECKING_MOVE(mvLast));
            ASSERT(InCheck(pos, pos->uToMove));
            iBestScore = MATED_SCORE(ctx->uPly);
            ASSERT(iBestScore <= -NMATE);
            goto end;
        }
        else
        {
            iBestScore = 0;
            if ((iAlpha < iBestScore) && (iBestScore < iBeta))
            {
                UpdatePV(ctx, DRAWMOVE);
            }
            goto end;
        }
    }

    // Not checkmate/stalemate; store the result of this search in the
    // hash table.
    if (iAlpha != iInitialAlpha)
    {
        ASSERT(mvBest.uMove != 0);
        if (!IS_CAPTURE_OR_PROMOTION(mvBest))
        {
            UpdateDynamicMoveOrdering(ctx,
                                      uDepth,
                                      mvBest,
                                      iBestScore,
                                      0);
        }
        StoreExactScore(mvBest, pos, iBestScore, uDepth, fThreat, ctx->uPly);
    }
    else
    {
        // IDEA: "I am very well aware of the fact, that the scores
        // you get back outside of the window, are not trustable at
        // all. Still, I have mentioned the case, of all scores being
        // losing mate scores, but one is not. This move will be good
        // to try first. I have seen this, by investigating multi MB
        // large tree dumps, so it is not only there in theory. Often,
        // even with fail soft, I of course will also get multiple
        // moves with the same score (alpha). But then one can see the
        // "best" move as an additional killer move. It was most
        // probably the killer move anyway, when this position was
        // visited the last time. I cannot see a reason, why trying
        // such a move early could hurt. And I do see reductions of
        // tree sizes. I don't try upper-bound moves first. I try them
        // (more or less) after the good captures, and together with
        // the killer moves, but before history moves."
        //                                            --Ed Schroder
        StoreUpperBound(pos, iBestScore, uDepth, fThreat);
    }

 end:
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBestScore) || WE_SHOULD_STOP_SEARCHING);
    ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
    DTLeaveNode(ctx, FALSE, iBestScore, mvBest);
    return(iBestScore);
}


/**

Routine description:

    This routine is called by QSearch, the select part of the
    recursive search code.  Its job is to determine if a move
    generated is worth searching.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : searcher context
    ULONG uMoveNum : the move number we are considering
    SCORE iFutility : the futility line

Return value:

    FLAG : TRUE if the move is worth considering,
           FALSE if it can be skipped

**/
static FLAG INLINE
_ShouldWeConsiderThisMove(IN SEARCHER_THREAD_CONTEXT *ctx,
                          IN ULONG uMoveNum,
                          IN SCORE iFutility,
                          IN FLAG fGeneratedChecks)
{
    MOVE mvLast = ctx->sPlyInfo[ctx->uPly - 1].mv;
    MOVE mv = ctx->sMoveStack.mvf[uMoveNum].mv;
    ULONG uColor;
    SCORE i;

    ASSERT(!IS_CHECKING_MOVE(mvLast));
    ASSERT(!InCheck(&(ctx->sPosition), ctx->sPosition.uToMove));

    if (IS_CAPTURE_OR_PROMOTION(mv))
    {
        // IDEA: if mvLast was a promotion, try everything here?

        // Don't consider promotions to anything but queens unless
        // it's a knight and we are going for a knockout.
        if ((mv.pPromoted) && (!IS_QUEEN(mv.pPromoted)))
        {
            if (!IS_KNIGHT(mv.pPromoted) ||
                !IS_CHECKING_MOVE(mv) ||
                (FALSE == fGeneratedChecks))
            {
                return(FALSE);
            }
        }

        i = ctx->sMoveStack.mvf[uMoveNum].iValue;
        if (i >= SORT_THESE_FIRST)
        {
            i &= STRIP_OFF_FLAGS;
            ASSERT(i >= 0);
            if (mv.pCaptured)
            {
                // If there are very few pieces left on the board,
                // consider all captures because we could be, for
                // example, taking the guy's last pawn and forcing a
                // draw.  Even though the cap looks futile the draw
                // might save the game...
                uColor = GET_COLOR(mv.pCaptured);
                ASSERT(OPPOSITE_COLORS(ctx->sPosition.uToMove, uColor));
                if ((IS_PAWN(mv.pCaptured) &&
                     ctx->sPosition.uPawnCount[uColor] == 1) ||
                    (!IS_PAWN(mv.pCaptured) &&
                     ctx->sPosition.uNonPawnCount[uColor][0] == 2))
                {
                    return(TRUE);
                }

                // Also always consider "dangerous pawn" captures.
                if (IS_PAWN(mv.pMoved) &&
                    (((GET_COLOR(mv.pMoved) == WHITE) && RANK7(mv.cTo)) ||
                     ((GET_COLOR(mv.pMoved) == BLACK) && RANK2(mv.cTo))))
                {
                    return(TRUE);
                }

                // Don't trust the SEE alone for alpha pruning decisions.
                i = MAXU(i, PIECE_VALUE(mv.pCaptured));

                // Also try hard not to prune recaps, the bad trade
                // penalty can make them look "futile" sometimes.
                if ((PIECE_VALUE(mv.pCaptured) ==
                     PIECE_VALUE(mvLast.pCaptured)) &&
                    (i + 200 > iFutility))
                {
                    return(TRUE);
                }
            }

            // Otherwise, even if a move is even/winning, make sure it
            // brings the score up to at least somewhere near alpha.
            if (i > iFutility)
            {
                return(TRUE);
            }
        }

        // If we get here the move was either a losing capture/prom
        // that checked or a "futile" winning capture/prom that may or
        // may not check.  Be more willing to play checking captures
        // even if they look bad.
        if (IS_CHECKING_MOVE(mv) && (TRUE == fGeneratedChecks))
        {
            return(iFutility < +VALUE_ROOK);
        }
    }
    else
    {
        // If we get here we have a checking move that does not
        // capture anything or promote anything.  We are interested in
        // these to some depth.
        ASSERT(IS_CHECKING_MOVE(mv));
        ASSERT(TRUE == fGeneratedChecks);

        // IDEA: don't play obviously losing checks if we are already
        // way below alpha.
        if (iFutility < +VALUE_BISHOP)
        {
            return(TRUE);
        }
        return(SEE(&ctx->sPosition, mv) >= 0);
    }
    return(FALSE);
}


/**

Routine description:

    Side to move is in check and may or may not have had a chance to
    stand pat at a qnode above this point.  Search all legal check
    evasions and return a mate-in-n score if this is checkmate.
    Possibly extend the depth to which we generate checks under this
    node.  If there's a stand pat qnode above us the mate-in-n will be
    weeded out.

Parameters:

    IN SEARCHER_THREAD_CONTEXT *ctx,
    IN SCORE iAlpha,
    IN SCORE iBeta

Return value:

    SCORE

**/
SCORE
QSearchFromCheckNoStandPat(IN SEARCHER_THREAD_CONTEXT *ctx,
                           IN SCORE iAlpha,
                           IN SCORE iBeta)
{
    POSITION *pos = &ctx->sPosition;
    CUMULATIVE_SEARCH_FLAGS *pf = &ctx->sSearchFlags;
    ULONG x, uMoveCount;
    SCORE iBestScore = MATED_SCORE(ctx->uPly);
    SCORE iScore;
    MOVE mv;
    ULONG uLegalMoves = 0;
    ULONG uQsearchCheckExtension = 0;

    ASSERT(InCheck(pos, pos->uToMove));
    GenerateMoves(ctx, NULLMOVE, GENERATE_ESCAPES);
    uMoveCount = MOVE_COUNT(ctx, ctx->uPly);
    if (uMoveCount > 0)
    {
        // Consider extending the number of qsearch plies we consider
        // non-capture checks at during this line.
        if ((pf->fCouldStandPat[pos->uToMove] == FALSE) &&
            (pf->uQsearchDepth < g_uIterateDepth / 2) &&
            (CountKingSafetyDefects(pos, pos->uToMove) > 1))
        {
            if (uMoveCount == 1) {
                uQsearchCheckExtension = 2;
                INC(ctx->sCounters.extension.uQExtend);
            } else if ((uMoveCount == 2) ||
                       (NUM_KING_MOVES(ctx, ctx->uPly) == 0) ||
                       (NUM_CHECKING_PIECES(ctx, ctx->uPly) > 1)) {
                uQsearchCheckExtension = 1;
                INC(ctx->sCounters.extension.uQExtend);
            }
            ctx->sPlyInfo[ctx->uPly].iExtensionAmount = uQsearchCheckExtension;
        }
    }

    for (x = ctx->sMoveStack.uBegin[ctx->uPly];
         x < ctx->sMoveStack.uEnd[ctx->uPly];
         x++)
    {
        SelectBestNoHistory(ctx, x);
        mv = ctx->sMoveStack.mvf[x].mv;
        mv.bvFlags |= WouldGiveCheck(ctx, mv);
#ifdef DEBUG
        ASSERT(0 == (ctx->sMoveStack.mvf[x].bvFlags & MVF_MOVE_SEARCHED));
        ctx->sMoveStack.mvf[x].bvFlags |= MVF_MOVE_SEARCHED;
#endif

        // Note: no selectivity at in-check nodes; search every reply.
        // IDEA: prune if the side in check could have stood pat before.
        if (MakeMove(ctx, mv))
        {
            uLegalMoves++;
            pf->uQsearchNodes++;
            pf->uQsearchDepth++;
            ASSERT(uQsearchCheckExtension < 3);
            pf->uQsearchCheckDepth += uQsearchCheckExtension;
            ASSERT(pf->uQsearchDepth > 0);
            iScore = -QSearch(ctx,
                              -iBeta,
                              -iAlpha);
            pf->uQsearchCheckDepth -= uQsearchCheckExtension;
            pf->uQsearchDepth--;
            UnmakeMove(ctx, mv);
            if (WE_SHOULD_STOP_SEARCHING)
            {
                iBestScore = iScore;
                goto end;
            }

            if (iScore > iBestScore)
            {
                iBestScore = iScore;
                ctx->sPlyInfo[ctx->uPly].mvBest = mv;
                if (iScore > iAlpha)
                {
                    if (iScore >= iBeta)
                    {
                        KEEP_TRACK_OF_FIRST_MOVE_FHs(uLegalMoves == 1);
                        ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));
                        goto end;
                    }
                    else
                    {
                        UpdatePV(ctx, mv);
                        StoreExactScore(mv, pos, iScore, 0, FALSE, ctx->uPly);
                        iAlpha = iScore;
                    }
                }
            }
        }
    }
    ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));

 end:
    ASSERT((uLegalMoves > 0) || (iBestScore <= -NMATE));
    ASSERT(IS_VALID_SCORE(iBestScore) || WE_SHOULD_STOP_SEARCHING);
    return(iBestScore);
}



/**

Routine description:

    The QSearch (Quiescence Search) is a selective search called when
    there is no remaining depth in Search.  Its job is to search only
    moves that stabilize the position -- once it is quiescence (quiet)
    we will run a static evaluation on it and return the score.

    This branch of the QSearch is called when the side to move is in
    danger somehow -- either he has two pieces en prise on the board
    or some piece that seems trapped.  We do not allow him an
    opportunity to stand pat in this position.

Parameters:

    IN SEARCHER_THREAD_CONTEXT *ctx,
    IN SCORE iAlpha,
    IN SCORE iBeta
    IN SCORE iEval

Return value:

    SCORE

**/
SCORE
QSearchInDangerNoStandPat(IN SEARCHER_THREAD_CONTEXT *ctx,
                          IN SCORE iAlpha,
                          IN SCORE iBeta)
{
    POSITION *pos = &ctx->sPosition;
    CUMULATIVE_SEARCH_FLAGS *pf = &ctx->sSearchFlags;
    ULONG x;
    FLAG fIncludeChecks;
    SCORE iBestScore = iAlpha;
    SCORE iScore;
    SCORE iFutility;
    SCORE iEval = GetRoughEvalScore(ctx, iAlpha, iBeta, TRUE);
    MOVE mv;
    ULONG uLegalMoves = 0;
    static ULONG _WhatToGen[] =
    {
        GENERATE_CAPTURES_PROMS,
        GENERATE_CAPTURES_PROMS_CHECKS
    };


    // Set futility:
    // 
    // iEval + move_value + margin < alpha
    //         move_value          < alpha - margin - iEval
    iFutility = 0;
    if (iAlpha < +NMATE)
    {
        iFutility = (iAlpha -
                     (FUTILITY_BASE_MARGIN + ctx->uPositional) -
                     iEval);
        iFutility = MAX0(iFutility);
    }

    // We suspect that the guy on move is in sad shape if we're
    // here...  he has more than one piece en prise or he seems to
    // have a piece trapped.  Allow him to play checks in order to try
    // to save the situation.
    ASSERT(!InCheck(pos, pos->uToMove));
    fIncludeChecks = (pf->uQsearchDepth < g_uIterateDepth / 3);
    GenerateMoves(ctx, NULLMOVE, _WhatToGen[fIncludeChecks]);

    for (x = ctx->sMoveStack.uBegin[ctx->uPly];
         x < ctx->sMoveStack.uEnd[ctx->uPly];
         x++)
    {
        SelectBestNoHistory(ctx, x);
        mv = ctx->sMoveStack.mvf[x].mv;
#ifdef DEBUG
        ASSERT(0 == (ctx->sMoveStack.mvf[x].bvFlags & MVF_MOVE_SEARCHED));
        ctx->sMoveStack.mvf[x].bvFlags |= MVF_MOVE_SEARCHED;
#endif

        // Prune except when pruning all moves could cause us to return
        // -INFINITY (mated) erroneously.
        if (iAlpha > -INFINITY)
        {
            if (ctx->sMoveStack.mvf[x].iValue <= 0)
            {
                ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE | VERIFY_AFTER));
                goto end;
            }
            if (FALSE == _ShouldWeConsiderThisMove(ctx,
                                                   x,
                                                   iFutility,
                                                   TRUE))
            {
                continue;
            }
        }

        if (FALSE == fIncludeChecks)
        {
            mv.bvFlags |= WouldGiveCheck(ctx, mv);
        }

        if (MakeMove(ctx, mv))
        {
            uLegalMoves++;
            pf->uQsearchNodes++;
            pf->uQsearchDepth++;
            iScore = -QSearch(ctx,
                              -iBeta,
                              -iAlpha);
            pf->uQsearchDepth--;
            UnmakeMove(ctx, mv);

            if (iScore > iBestScore)
            {
                iBestScore = iScore;
                ctx->sPlyInfo[ctx->uPly].mvBest = mv;
                if (iScore > iAlpha)
                {
                    if (iScore >= iBeta)
                    {
                        KEEP_TRACK_OF_FIRST_MOVE_FHs(uLegalMoves == 1);
                        ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));
                        goto end;
                    }
                    else
                    {
                        UpdatePV(ctx, mv);
                        StoreExactScore(mv, pos, iScore, 0, FALSE, ctx->uPly);
                        iAlpha = iScore;
                    }
                }
            }
            if (WE_SHOULD_STOP_SEARCHING) goto end;
        }
    }
    ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));

 end:
    // If iAlpha is -INFINITY and side on move has no captures,
    // promotes or checks then we must make up a "stand pat" score
    // here.  We don't want to let him stand pat with iEval because
    // the board looks dangerous.  But we likewise don't want to say
    // "mated" because he's not.
    ASSERT((uLegalMoves > 0) || (iBestScore == iAlpha));
    if (iBestScore == -INFINITY)
    {
        ASSERT(iAlpha == -INFINITY);
        iBestScore = iEval - VALUE_QUEEN;
    }
    ASSERT(IS_VALID_SCORE(iBestScore) || WE_SHOULD_STOP_SEARCHING);
    return(iBestScore);
}


/**

Routine description:

    The QSearch (Quiescence Search) is a selective search called when
    there is no remaining depth in Search.  Its job is to search only
    moves that stabilize the position -- once it is quiescence (quiet)
    we will run a static evaluation on it and return the score.

    TODO: experiment with probing and storing in the hash table here.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : the searcher context
    SCORE iAlpha : lowerbound of search window
    SCORE iBeta : upperbound of search window

Return value:

    SCORE : a score

**/
SCORE FASTCALL
QSearch(IN SEARCHER_THREAD_CONTEXT *ctx,
        IN SCORE iAlpha,
        IN SCORE iBeta)
{
    POSITION *pos = &ctx->sPosition;
    CUMULATIVE_SEARCH_FLAGS *pf = &ctx->sSearchFlags;
    PLY_INFO *pi = &ctx->sPlyInfo[ctx->uPly];
    MOVE mvLast = (pi-1)->mv;
    MOVE mv;
    SCORE iBestScore;
    SCORE iScore;
    SCORE iEval;
    SCORE iFutility;
    ULONG x;
    ULONG uLegalMoves;
    FLAG fIncludeChecks;
    FLAG fOrigStandPat = ctx->sSearchFlags.fCouldStandPat[pos->uToMove];
    static ULONG _WhatToGen[] =
    {
        GENERATE_CAPTURES_PROMS,
        GENERATE_CAPTURES_PROMS_CHECKS
    };

#ifdef DEBUG
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT(ctx->uPly > 0);
    ASSERT(TRUE == pi->fInQsearch);
    memcpy(&pi->sPosition, pos, sizeof(POSITION));
#endif

    INC(ctx->sCounters.tree.u64QNodeCount);
    pi->iExtensionAmount = 0;
    if (TRUE == CommonSearchInit(ctx,
                                 &iAlpha,
                                 &iBeta,
                                 &iBestScore))
    {
        goto end;
    }
    DTEnterNode(ctx, 0, TRUE, iAlpha, iBeta);

    // Probe interior node recognizers; do not allow probes into ondisk
    // EGTB files since we are in qsearch.
    switch(RecognLookup(ctx, &iScore, FALSE))
    {
        case UNRECOGNIZED:
            break;
        case RECOGN_EXACT:
        case RECOGN_EGTB:
            if ((iAlpha < iScore) && (iScore < iBeta))
            {
                UpdatePV(ctx, RECOGNMOVE);
            }
            iBestScore = iScore;
            goto end;
        case RECOGN_LOWER:
            if (iScore >= iBeta)
            {
                iBestScore = iScore;
                goto end;
            }
            break;
        case RECOGN_UPPER:
            if (iScore <= iAlpha)
            {
                iBestScore = iScore;
                goto end;
            }
            break;
#ifdef DEBUG
        default:
            ASSERT(FALSE);
#endif
    }

    // If the side is in check, don't let him stand pat.  Search every
    // reply to check and return a MATE score if applicable.  If the
    // side had a chance to stand pat above then the MATE score will
    // be disregarded there since it's not forced.
    if (IS_CHECKING_MOVE(mvLast))
    {
        ASSERT(InCheck(pos, pos->uToMove));
        iBestScore = QSearchFromCheckNoStandPat(ctx, iAlpha, iBeta);
        goto end;
    }
    ASSERT(!InCheck(pos, pos->uToMove));

    // Even if the side is not in check, do not let him stand pat if
    // his position looks dangerous (i.e. more than one piece en prise
    // or a piece trapped).  Fail low if there's nothing that looks
    // good on this line.
    if (SideCanStandPat(pos, pos->uToMove) == FALSE)
    {
        iBestScore = QSearchInDangerNoStandPat(ctx, iAlpha, iBeta);
        goto end;
    }

    // If we get here then side on move is not in check and this
    // position looks ok enough to allow him the option to stand pat
    // -or- we missed when we probed the dangerhash.  Also remember
    // that this side has had the option to stand pat when searching
    // below this point.
    uLegalMoves = 0;
    iEval = iBestScore = Eval(ctx, iAlpha, iBeta);
    if (iBestScore > iAlpha)
    {
        iAlpha = iBestScore;
        ASSERT(ctx->sPlyInfo[ctx->uPly].PV[ctx->uPly].uMove == 0);
        ASSERT(pi->mvBest.uMove == 0);
        if (iBestScore >= iBeta)
        {
            goto end;
        }
    }
    ctx->sSearchFlags.fCouldStandPat[pos->uToMove] = TRUE;

    // He did not choose to stand pat here; we will be generating
    // moves and searching recursively.  Compute a futility score:
    // any move less than this will not be searched because it will
    // just cause a lazy eval answer; is has no shot to bring the
    // score close enough to alpha to even consider.
    //
    // iEval + move_value + margin < alpha
    //         move_value          < alpha - margin - iEval
    iFutility = 0;
    if (iAlpha < +NMATE)
    {
        iFutility = iAlpha - (FUTILITY_BASE_MARGIN + ctx->uPositional) - iEval;
        iFutility = MAX0(iFutility);
    }

    // We know we are not in check.  Generate moves (including checks
    // if we are below the threshold and the other side has never been
    // allowed to stand pat).  Recurse.
    fIncludeChecks = ((pf->uQsearchDepth < pf->uQsearchCheckDepth) &&
                      (pf->fCouldStandPat[FLIP(pos->uToMove)] == FALSE) &&
                      (pos->uNonPawnMaterial[pos->uToMove] >
                       (VALUE_KING + VALUE_BISHOP)));
    GenerateMoves(ctx, NULLMOVE, _WhatToGen[fIncludeChecks]);
    for (x = ctx->sMoveStack.uBegin[ctx->uPly];
         x < ctx->sMoveStack.uEnd[ctx->uPly];
         x++)
    {
        SelectBestNoHistory(ctx, x);
        if (ctx->sMoveStack.mvf[x].iValue <= 0)
        {
            // We are only intersted in winning/even captures/promotions
            // and (if fIncludeChecks is TRUE) some checking moves too.
            // If we see a move whose value is zero, the rest of the moves
            // in this ply can be tossed.
            ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE | VERIFY_AFTER));
            ASSERT(iBestScore > -NMATE);
            goto end;
        }
        mv = ctx->sMoveStack.mvf[x].mv;
#ifdef DEBUG
        ASSERT(0 == (ctx->sMoveStack.mvf[x].bvFlags & MVF_MOVE_SEARCHED));
        ctx->sMoveStack.mvf[x].bvFlags |= MVF_MOVE_SEARCHED;
#endif

        if (FALSE == _ShouldWeConsiderThisMove(ctx,
                                               x,
                                               iFutility,
                                               fIncludeChecks))
        {
            continue;
        }

        // If fIncludeChecks is FALSE then we still need to see if
        // this move is going to check the opponent; GenerateMoves
        // didn't do it for us to save time in the event of a fail
        // high.
        if (FALSE == fIncludeChecks)
        {
            mv.bvFlags |= WouldGiveCheck(ctx, mv);
        }

        if (MakeMove(ctx, mv))
        {
            uLegalMoves++;
            pf->uQsearchNodes++;
            pf->uQsearchDepth++;
            ASSERT(pf->uQsearchDepth > 0);
            iScore = -QSearch(ctx,
                              -iBeta,
                              -iAlpha);
            pf->uQsearchDepth--;
            UnmakeMove(ctx, mv);

            if (iScore > iBestScore)
            {
                iBestScore = iScore;
                pi->mvBest = mv;

                if (iScore > iAlpha)
                {
                    if (iScore >= iBeta)
                    {
                        KEEP_TRACK_OF_FIRST_MOVE_FHs(uLegalMoves == 1);
                        ASSERT(iBestScore > -NMATE);
                        ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));
                        goto end;
                    }
                    else
                    {
                        UpdatePV(ctx, mv);
                        StoreExactScore(mv, pos, iScore, 0, FALSE, ctx->uPly);
                        iAlpha = iScore;

                        // Readjust futility margin here; it can be wider now.
                        if (iAlpha < +NMATE)
                        {
                            iFutility = (iAlpha -
                                         (FUTILITY_BASE_MARGIN +
                                          ctx->uPositional) -
                                         iEval);
                            iFutility = MAX0(iFutility);
                        }
                    }
                }
            }
            if (WE_SHOULD_STOP_SEARCHING) goto end;
        }
    }
    ASSERT(iBestScore > -NMATE);
    ASSERT(SanityCheckMoves(ctx, x, VERIFY_BEFORE));

 end:
    ctx->sSearchFlags.fCouldStandPat[pos->uToMove] = fOrigStandPat;
    ASSERT(PositionsAreEquivalent(pos, &pi->sPosition));
    ASSERT(IS_VALID_SCORE(iBestScore) || WE_SHOULD_STOP_SEARCHING);
    DTLeaveNode(ctx, TRUE, iBestScore, pi->mvBest);
    return(iBestScore);
}
