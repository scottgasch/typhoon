/**

Copyright (c) Scott Gasch

Module Name:

    dynamic.c

Abstract:

    Dynamic move ordering functions/structures.  By "dynamic move
    ordering" I mean killer moves and history heuristic stuff.  

    Note 1: there are two history tables here.  One is used for move
    ordering and the other is used for pruning decisions.  Both only
    contain data about non-capture/promote moves.  The former is
    updated with roughly remaining_depth^2 at a fail high while the
    latter is updated so as to maintain an approximate answer to "what
    percent of the time does this move fail high."

    Note 2: the globals in this module may be accessed by more than
    one searcher thread at the same time; spinlocks used to
    synchronize.

    Note 3: All of these tables must be cleared when a new game is
    started or a new position is loaded onto the board.

Author:

    Scott Gasch (scott.gasch@gmail.com) 24 Jun 2004

Revision History:

    $Id: dynamic.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

ULONG g_HistoryCounters[14][128];
#define FH_STATS_TABLE_SIZE (0x20000)

typedef struct _FH_STATS
{
    union
    {
        ULONG uWholeThing;
        struct
        {
            USHORT u16FailHighs;
            USHORT u16Attempts;
        };
    };
} FH_STATS;

FH_STATS g_FailHighs[FH_STATS_TABLE_SIZE];

#ifdef MP
volatile static ULONG g_uDynamicLock;
#define DYN_IS_LOCKED (g_uDynamicLock != 0)
#define LOCK_DYN \
    AcquireSpinLock(&g_uDynamicLock); \
    ASSERT(DYN_IS_LOCKED)
#define UNLOCK_DYN \
    ASSERT(DYN_IS_LOCKED); \
    ReleaseSpinLock(&g_uDynamicLock)
#else // no MP
#define DYN_IS_LOCKED (1)
#define LOCK_DYN
#define UNLOCK_DYN
#endif


FLAG 
InitializeDynamicMoveOrdering(void)
/**

Routine description:

    Initialize dynamic move ordering structures.

Parameters:

    void

Return value:

    FLAG

**/
{
#ifdef MP
    g_uDynamicLock = 0;
#endif
    ClearDynamicMoveOrdering();
    return(TRUE);
}

void 
ClearDynamicMoveOrdering(void)
/**

Routine description:

    Clear the global history table.  Killer moves are per-context
    structures and must be cleared on a per-context basis.

Parameters:

    void

Return value:

    void

**/
{
    ULONG u;

    memset(g_HistoryCounters, 0, sizeof(g_HistoryCounters));
    for (u = 0; u < FH_STATS_TABLE_SIZE; u++) 
    {
        g_FailHighs[u].uWholeThing = 0x00010001;
    }
}


static void 
_RecordMoveFailHigh(MOVE mv)
/**

Routine description:

    Update the "fail high percentage" history table for this move.
    The table is used to make pruning decisions, not to rank moves.

Parameters:

    MOVE mv

Return value:

    static void

**/
{
    ULONG u = MOVE_TO_INDEX(mv);
    ULONG v = g_FailHighs[u].uWholeThing;

    ASSERT(DYN_IS_LOCKED);
    if (((v & 0x0000FFFF) == 0x0000FFFF) || ((v & 0xFFFF0000) == 0xFFFF0000))
    {
        g_FailHighs[u].u16FailHighs >>= 1;
        g_FailHighs[u].u16Attempts >>= 1;
    }
    g_FailHighs[u].u16FailHighs++;
    g_FailHighs[u].u16Attempts++;
    ASSERT(g_FailHighs[u].u16FailHighs != 0);
    ASSERT(g_FailHighs[u].u16Attempts != 0);
    ASSERT(g_FailHighs[u].u16Attempts >= g_FailHighs[u].u16FailHighs);
}


static void 
_RecordMoveFailure(MOVE mv)
/**

Routine description:

    Update the fail high percentage table with the information that a
    move has not produced a fail high cutoff when it was considered.

Parameters:

    MOVE mv

Return value:

    static void

**/
{
    ULONG u = MOVE_TO_INDEX(mv);

    ASSERT(DYN_IS_LOCKED);
    if (g_FailHighs[u].u16Attempts == 0xFFFF)
    {
        g_FailHighs[u].u16FailHighs >>= 1;
        g_FailHighs[u].u16Attempts >>= 1;
    }
    g_FailHighs[u].u16Attempts++;
    ASSERT(g_FailHighs[u].u16Attempts != 0);
    ASSERT(g_FailHighs[u].u16Attempts >= g_FailHighs[u].u16FailHighs);
}


static void 
_NewKillerMove(SEARCHER_THREAD_CONTEXT *ctx, 
               MOVE mv, 
               SCORE iScore)
/**

Routine description:

    Add a new killer move at a ply.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    MOVE mv,
    SCORE iScore

Return value:

    void

**/
{
    ULONG uPly = ctx->uPly;

    ASSERT(uPly >= 0);
    ASSERT(uPly < MAX_PLY_PER_SEARCH);
    ASSERT(!IS_CAPTURE_OR_PROMOTION(mv));
    ASSERT(mv.uMove);

    if (mv.bvFlags & MOVE_FLAG_ESCAPING_CHECK) 
    {
        if (!IS_SAME_MOVE(mv, ctx->mvKillerEscapes[uPly][0]))
        {
            ctx->mvKillerEscapes[uPly][1] = ctx->mvKillerEscapes[uPly][0];
            ctx->mvKillerEscapes[uPly][0] = mv;
        }
        ASSERT(!IS_SAME_MOVE(ctx->mvKillerEscapes[uPly][0], 
                             ctx->mvKillerEscapes[uPly][1]));
    }
    else
    {
        mv.bvFlags |= ((iScore >= +NMATE) * MOVE_FLAG_KILLERMATE);
        if (!IS_SAME_MOVE(mv, ctx->mvKiller[uPly][0]))
        {
            ctx->mvKiller[uPly][1] = ctx->mvKiller[uPly][0];
            ctx->mvKiller[uPly][0] = mv;
            if (ctx->mvKiller[uPly][1].uMove == 0) 
            {
                ctx->mvKiller[uPly][1] = ctx->mvNullmoveRefutations[uPly];
            }
        }
        ASSERT(!IS_SAME_MOVE(ctx->mvKiller[uPly][0], ctx->mvKiller[uPly][1]));
    }
}


static void 
_IncrementMoveHistoryCounter(MOVE mv, 
                             ULONG uDepth)
/**
  
Routine description:

    Increase a move's history counter in the global history table.
    Also affect the history pruning counters in prune.c.

Parameters:

    MOVE mv,
    ULONG uDepth

Return value:

    void

**/
{
    ULONG x, y;
    ULONG uVal;
    ULONG *pu;

    ASSERT(!IS_CAPTURE_OR_PROMOTION(mv));
    uVal = uDepth / ONE_PLY;
    ASSERT(uVal >= 0);
    ASSERT(uVal <= MAX_PLY_PER_SEARCH);
    uVal += 1;
    uVal *= uVal;
    ASSERT(uVal > 0);

    pu = &(g_HistoryCounters[mv.pMoved][mv.cTo]);
    LOCK_DYN;
    *pu += uVal;

    //
    // Make sure that the history weight doesn't get large enough to
    // affect the move flags or else our move ordering algorithm is
    // screwed.
    //
    while (*pu & ~STRIP_OFF_FLAGS)
    {
        for (x = 0; x <= WHITE_KING; x++)
        {
            FOREACH_SQUARE(y)
            {
                g_HistoryCounters[x][y] >>= 4;
            }
        }
    }

#ifdef MP
    //
    // The purpose of this restriction is to ease contention for the
    // memory bandwidth of the machine on dual-core / dual proc
    // systems.  The net result is positive.
    //
    if (uDepth > THREE_PLY)
    {
        _RecordMoveFailHigh(mv);
    }
#else
    _RecordMoveFailHigh(mv);
#endif
    UNLOCK_DYN;
}

static void 
_DecrementMoveHistoryCounter(MOVE mv, 
                             ULONG uDepth)
/**
  
Routine description:

    Decrease a move's history counter in the global history table.

Parameters:

    MOVE mv,
    ULONG uDepth

Return value:

    void

**/
{
    ULONG uVal;
    ULONG *pu;

    ASSERT(!IS_CAPTURE_OR_PROMOTION(mv));
    uVal = uDepth / ONE_PLY;
    ASSERT(uVal >= 0);
    ASSERT(uVal <= MAX_PLY_PER_SEARCH);
    uVal /= 4;
    uVal += 1;
    ASSERT(uVal > 0);

    pu = &(g_HistoryCounters[mv.pMoved][mv.cTo]);
    LOCK_DYN;
    if (*pu >= uVal)
    {
        *pu -= uVal;
    }
    else
    {
        *pu = 0;
    }
    _RecordMoveFailure(mv);
    UNLOCK_DYN;
}


void 
UpdateDynamicMoveOrdering(IN SEARCHER_THREAD_CONTEXT *ctx,
                          IN ULONG uRemainingDepth,
                          IN MOVE mvBest,
                          IN SCORE iScore,
                          IN ULONG uCurrent)
/**

Routine description:

    Update dynamic move ordering structs for a particular move (and,
    possibly, the other moves that were considered prior to this move
    in the move ordering).  This is called when a move beats alpha at
    the root or in Search (but not in QSearch).

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG uRemainingDepth,
    MOVE mvBest,
    SCORE iScore,
    ULONG uCurrent

Return value:

    void

**/
{
    ULONG u;
    MOVE mv;

    //
    // Add this move to the killer list and increment its history count
    //
    if (!IS_CAPTURE_OR_PROMOTION(mvBest))
    {
        _NewKillerMove(ctx, mvBest, iScore);
        _IncrementMoveHistoryCounter(mvBest, uRemainingDepth);
    }

    //
    // If this move was not the first we considered at this node,
    // decrement the history counters of moves we considered before
    // it.
    //
    uCurrent -= (uCurrent != 0);
    for (u = ctx->sMoveStack.uBegin[ctx->uPly];
         u < uCurrent;
         u++)
    {
        ASSERT(IS_SAME_MOVE(ctx->sMoveStack.mvf[uCurrent].mv, mvBest));
        mv = ctx->sMoveStack.mvf[u].mv;
        ASSERT(!IS_SAME_MOVE(mv, mvBest));
        if (!IS_CAPTURE_OR_PROMOTION(mv))
        {
            _DecrementMoveHistoryCounter(mv, uRemainingDepth);
        }
    }
}




ULONG 
GetMoveFailHighPercentage(IN MOVE mv)
/**

Routine description:

    Lookup a move in the fail high percentage history table and return
    its approximate fail high percentage.

Parameters:

    MOVE mv

Return value:

    ULONG

**/
{
    ULONG u = MOVE_TO_INDEX(mv);
    ULONG n, d;

    n = g_FailHighs[u].u16FailHighs;
    d = g_FailHighs[u].u16Attempts;
    if (d == 0)
    {
        return(0);
    }
    n *= 100;
    return(n / d);
}


void 
CleanupDynamicMoveOrdering(void)
/**

Routine description:
   
    Cleanup dynamic move ordering structs -- basically a noop for now.

Parameters:

    void

Return value:

    void

**/
{
    NOTHING;
}


void 
MaintainDynamicMoveOrdering(void)
/**

Routine description:

    Perform routine maintenance on dynamic move ordering data by
    reducing the magnitude of the history counters.

Parameters:

Return value:

    void

**/
{
    ULONG x, y;
    
    LOCK_DYN;
    for (x = 0; x <= WHITE_KING; x++)
    {
        FOREACH_SQUARE(y)
        {
            g_HistoryCounters[x][y] >>= 1;
        }
    }
    UNLOCK_DYN;
}
