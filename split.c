/**

Copyright (c) Scott Gasch

Module Name:

    split.c

Abstract:

    This code is the glue needed to run a parallel search.  Let me
    encourage other chess programmers to read this code carefully.
    Implementing a parallel search is pretty easy and I procrastinated
    doing it for WAY too long because I was afraid to sink multiple
    months into coding it up.  It's not as hard as you think.  If you
    have a good handle on multi-threaded programming then you can do
    something that works pretty well in a week.

    Here's my system (which is not original -- I believe this is the
    same idea that Bruce Moreland used for Ferret and I've been told
    it's simiar to what Bob Hyatt does in Crafty).  You want to split
    the search on all-nodes.  There is some overhead involved in
    splitting the tree and, after you paid the price, you do not want
    one of the moves failing high... because then you have to abort
    the split and return a fail high.

    Once my search has one of these nodes, it calls in here to split
    the tree.  What's involved with splitting the tree is:

        1. populating a "split node" with data about the position
           we are splitting in.
        2. grabbing zero or more idle "worker threads" to come help
           with the split.
        3. all threads searching under the split grab a move from
           the move list and search that move's subtree in parallel.
        4. when a subtree is evaluated... all threads update the
           split node with new information (i.e. raise alpha or, in
           the undesirable case that a thread failed high, toggles
           a flag in the split node to inform the rest of the 
           searchers).
        5. before grabbing the next move, each thread updates its
           local alpha to pick up window-narrowing information 
           encountered by other threads.
        6. goto step 3, stop when there are no more moves or 
           thread finds a fail high.

    ------------------------------------------------------------------

    This is a quote from some win32 blog about the x86 memory model
    which I found helpful to read while I was thinking about
    multithreaded code in this module:
    
    "The most familiar model is X86.  It's a relatively strong model.
     Stores are never reordered with respect to other stores.  But, in
     the absence of data dependence, loads can be reordered with
     respect to other loads and stores.  Many X86 developers don't
     realize that this reordering is possible, though it can lead to
     some nasty failures under stress on big MP machines.

     In terms of the above, the memory model for X86 can be described as:

        1. All stores are actually store.release (upward memory fence).
           This means that normal loads can be moved above the 
           store.release but nothing can be moved below it.
        2. All loads are normal loads.
        3. Any use of the LOCK prefix (e.g. LOCK CMPXCHG or LOCK INC) 
           creates a full memory fence."

Author:

    Scott Gasch (scott.gasch@gmail.com) 25 Jun 2004

Revision History:

    $Id: split.c 345 2007-12-02 22:56:42Z scott $  

**/
#ifdef MP

#include "chess.h"

extern ULONG g_uIterateDepth;
ULONG g_uNumHelperThreads = 0;
#define MAX_SPLITS          (20)
#define IDLE                ((ULONG)-1)

void
HelpSearch(SEARCHER_THREAD_CONTEXT *ctx, ULONG u);

//
// A struct that holds information about helper threads.
//
typedef struct _HELPER_THREAD
{
    ULONG uHandle;
    volatile ULONG uAssignment;
    SEARCHER_THREAD_CONTEXT ctx;
#ifdef PERF_COUNTERS
    UINT64 u64IdleCycles;
    UINT64 u64BusyCycles;
#endif
} HELPER_THREAD;

//
// An array of these structs that is alloced dynamically so that N
// cpus is not a hardcoded thing, the engine can scale N based on
// where it is running.
//
static HELPER_THREAD *g_HelperThreads = NULL;

//
// An array of structs that hold information about where we have split
// the search tree.
//
static SPLIT_INFO g_SplitInfo[MAX_SPLITS];

//
// In addition to this main split database lock each split entry has
// its own private lock.  This way contention between searcher threads
// operating on different split nodes is eliminated.
//
volatile static ULONG g_uSplitLock = 0;
#define SPLITS_LOCKED (g_uSplitLock != 0)
#define LOCK_SPLITS \
    AcquireSpinLock(&g_uSplitLock); \
    ASSERT(SPLITS_LOCKED);
#define UNLOCK_SPLITS \
    ASSERT(SPLITS_LOCKED); \
    ReleaseSpinLock(&g_uSplitLock);

static ULONG g_uNumSplitsAvailable;
volatile ULONG g_uNumHelpersAvailable;


ULONG
HelperThreadIdleLoop(IN ULONG uMyId)
/**

Routine description:

    The entry point of a helper thread.  It will spin in the idle loop
    here until another thread splits the search tree, sees that it is
    idle, and notifies it to come help by changing the assignment
    field in its struct.

Parameters:

    ULONG uMyId : this thread's index in g_HelperThreads

Return value:

    ULONG

**/
{
    SEARCHER_THREAD_CONTEXT *ctx = &(g_HelperThreads[uMyId].ctx);
    ULONG u, v;
    MOVE mv;
    ULONG uIdleLoops = 0;
#ifdef PERF_COUNTERS
    UINT64 u64Then;
    UINT64 u64Now;
#endif
#ifdef DEBUG
    POSITION board;
#endif
    InitializeSearcherContext(NULL, ctx);
    ctx->uThreadNumber = uMyId + 1;
    do
    {
#ifdef PERF_COUNTERS
        if (uIdleLoops == 0) u64Then = SystemReadTimeStampCounter();
#endif
        //
        // Did someone tell us to come help?
        //
        if ((u = g_HelperThreads[uMyId].uAssignment) != IDLE)
        {
            //
            // By now the split info is populated.
            //
            uIdleLoops = 0;
            ReInitializeSearcherContext(&(g_SplitInfo[u].sRootPosition), ctx);
            ctx->pSplitInfo[0] = &(g_SplitInfo[u]);
            ctx->uPositional = g_SplitInfo[u].uSplitPositional;
        
            //
            // Note: the main thread could have already exhausted the
            // split and decremented it from 3->2.  When we leave it
            // will fall to 1 and allow the main thread to continue.
            //
            ASSERT(g_SplitInfo[u].uNumThreadsHelping >= 2);

            //
            // Get from the root of the search to the split position.
            // We do this instead of just initializing the searcher
            // context at the split node so that historic things like
            // draw detection still work in the helper threads.
            //
            v = 0;
            do
            {
                ASSERT(v < MAX_PLY_PER_SEARCH);
                mv.uMove = g_SplitInfo[u].mvPathToHere[v].uMove;
                if (ILLEGALMOVE == mv.uMove) break;
#ifdef DEBUG
                if (mv.uMove)
                {
                    ASSERT(SanityCheckMove(&ctx->sPosition, mv));
                }
#endif
                if (FALSE == MakeMove(ctx, mv))
                {
                    UtilPanic(CANNOT_INITIALIZE_SPLIT,
                              &ctx->sPosition,
                              (void *)mv.uMove,
                              &g_SplitInfo[u],
                              (void *)v,
                              __FILE__, __LINE__);
                }
                v++;
            }
            while(1);
#ifdef DEBUG
            ASSERT(g_SplitInfo[u].uSplitPly == ctx->uPly);
            ASSERT(v > 0);
            ASSERT(IS_SAME_MOVE(g_SplitInfo[u].mvPathToHere[v-1],
                                g_SplitInfo[u].mvLast));
            ASSERT(PositionsAreEquivalent(&(ctx->sPosition),
                                          &(g_SplitInfo[u].sSplitPosition)));
            VerifyPositionConsistency(&(ctx->sPosition), FALSE);
            memcpy(&board, &(ctx->sPosition), sizeof(POSITION));
#endif
            //
            // Populate the move stack of this helper context to make
            // it look like it generated to moves at the split.  The
            // reason I do this is so that ComputeMoveExtension (which
            // looks at the move stack) will work at a split node even
            // if the context it gets is a helper thread's.
            //
            ctx->sMoveStack.uBegin[ctx->uPly] = 0;
            ctx->sMoveStack.uBegin[ctx->uPly + 1] =
                ctx->sMoveStack.uEnd[ctx->uPly] = g_SplitInfo[u].uNumMoves;

            for (v = 0;
                 v < g_SplitInfo[u].uNumMoves;
                 v++)
            {
                ctx->sMoveStack.mvf[v] = g_SplitInfo[u].mvf[v];
                ASSERT(SanityCheckMove(&ctx->sPosition, 
                                       g_SplitInfo[u].mvf[v].mv));
            }

            //
            // Go help with the search
            //
            HelpSearch(ctx, u);
            ASSERT(PositionsAreEquivalent(&board, &(ctx->sPosition)));

            //
            // Done with this assignment, wrap up and go back to idle state
            //
            LOCK_SPLITS;
            g_HelperThreads[uMyId].uAssignment = IDLE;
            g_uNumHelpersAvailable++;
            UNLOCK_SPLITS;
#ifdef PERF_COUNTERS
            u64Now = SystemReadTimeStampCounter();
            g_HelperThreads[uMyId].u64BusyCycles += (u64Now - u64Then);
#endif
        }
#if PERF_COUNTERS
        //
        // There was nothing for us to do, if that happens often
        // enough remember the idle cycles.
        //
        else
        {
            uIdleLoops++;
            if (uIdleLoops > 1000)
            {
                u64Now = SystemReadTimeStampCounter();
                LOCK_SPLITS;
                g_HelperThreads[uMyId].u64IdleCycles += (u64Now - u64Then);
                UNLOCK_SPLITS;
                uIdleLoops = 0;
#ifdef DEBUG
                SystemDeferExecution(1);
#endif
            }
        }
#endif
    }
    while(FALSE == g_fExitProgram);
    Trace("HELPER THREAD: thread terminating.\n");

    return(0);                                // ExitThread
}


FLAG
InitializeParallelSearch(void)
/**

Routine description:

    This routine must be called before any thread can split the search
    tree because it sets up the parallel search system.

Parameters:

    void

Return value:

    FLAG

**/
{
    ULONG u;

    if (g_Options.uNumProcessors < 2) return(FALSE);

    //
    // Initialize split entries
    //
    g_uSplitLock = 0;
    for (u = 0; u < MAX_SPLITS; u++)
    {
        memset(&(g_SplitInfo[u]), 0, sizeof(g_SplitInfo[u]));
    }
    g_uNumSplitsAvailable = MAX_SPLITS;
    ASSERT(NUM_SPLIT_PTRS_IN_CONTEXT <= MAX_SPLITS);

    //
    // Create and initialize helper threads
    //
    g_uNumHelperThreads = g_Options.uNumProcessors - 1;
    ASSERT(g_uNumHelperThreads >= 1);
    g_HelperThreads = SystemAllocateMemory(sizeof(HELPER_THREAD) *
                                           g_uNumHelperThreads);
    ASSERT(g_HelperThreads != NULL);
    for (u = 0; u < g_uNumHelperThreads; u++)
    {
        memset(&(g_HelperThreads[u]), 0, sizeof(g_HelperThreads[u]));
        g_HelperThreads[u].uAssignment = IDLE;
        if (FALSE == SystemCreateThread(HelperThreadIdleLoop,
                                        u,
                                        &(g_HelperThreads[u].uHandle)))
        {
            UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                      NULL,
                      "creating a thread",
                      0,
                      NULL,
                      __FILE__, __LINE__);
        }
    }
    g_uNumHelpersAvailable = g_uNumHelperThreads;
    return(TRUE);
}


#ifdef PERF_COUNTERS
void
ClearHelperThreadIdleness(void)
/**

Routine description:

    Called at the start of a search, if PERF_COUNTERS is
    defined... this function's job is to reset the idleness counter
    for all helper threads.

Parameters:

    void

Return value:

    void

**/
{
    ULONG u;

    LOCK_SPLITS;
    for (u = 0; u < g_uNumHelperThreads; u++)
    {
        g_HelperThreads[u].u64BusyCycles =
            g_HelperThreads[u].u64IdleCycles = 0ULL;
    }
    UNLOCK_SPLITS;
}


void
DumpHelperIdlenessReport(void)
/**

Routine description:

    Called at the end of a search, this function's job is to print
    out a report of how busy/idle each helper threads was.

Parameters:

    void

Return value:

    void

**/
{
    ULONG u;
    double n, d;

    for (u = 0;
         u < g_uNumHelperThreads;
         u++)
    {
        n = (double)g_HelperThreads[u].u64BusyCycles;
        d = (double)g_HelperThreads[u].u64IdleCycles;
        d += n;
        Trace("Helper thread %u: %5.2f percent busy.\n", u, (n / d) * 100.0);
    }
}
#endif


FLAG
CleanupParallelSearch(void)
/**

Routine description:

    Cleanup parallel search system before program shutdown.

Parameters:

    void

Return value:

    FLAG

**/
{
    if (g_HelperThreads != NULL )
    {
        SystemFreeMemory(g_HelperThreads);
    }
    g_uNumHelperThreads = 0;
    return(TRUE);
}


SCORE
StartParallelSearch(IN SEARCHER_THREAD_CONTEXT *ctx,
                    IN OUT SCORE *piAlpha,
                    IN SCORE iBeta,
                    IN OUT SCORE *piBestScore,
                    IN OUT MOVE *pmvBest,
                    IN ULONG uMoveNum,
                    IN INT iPositionExtend,
                    IN ULONG uDepth)
{
/**

Routine description:

    This routine is called from the main Search (not RootSearch or QSearch)
    when:

        1. it thinks the current search tree node looks like it could
           be searched in parallel -and-

        2. it's likely that there are idle helper thread(s) to help.

    It job is to find a free split node, populate it, find idle helper
    threads, assign them to help, and search this node in parallel.
    It _must_ be called after generating moves at this node.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : context of thread requesting split
    SCORE *piAlpha : in/out alpha of split node
    SCORE iBeta : in only (beta doesn't change) beta of split node
    SCORE *piBestScore : in/out best score seen so far at split node
    MOVE *pmvBest : in/out best move seen so far at split node
    ULONG uMoveNum : in next move number in ctx->sMoveStack
    INT iPositionExtend : in position based extensions for split node
    ULONG uDepth : in the depth we need to search to

Return value:

    SCORE : the score of this split subtree, along with out params above

**/
    SCORE iScore;
    ULONG u, v;
    ULONG uSplitNum;
    ULONG uOldStart;
#ifdef DEBUG
    POSITION board;

    ASSERT(IS_VALID_SCORE(*piAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(IS_VALID_SCORE(*piBestScore));
    ASSERT(*piBestScore > -INFINITY);
    ASSERT(pmvBest->uMove);
    ASSERT(*piAlpha < iBeta);

    memcpy(&board, &(ctx->sPosition), sizeof(POSITION));
    VerifyPositionConsistency(&board, FALSE);
#endif

    //
    // Note: This is a lazy lock construction: search.c has peeked at
    // g_uNumHelperThreads before calling us and only calls when it
    // thinks there's a helper available.  (1) We could find that
    // there is no helper available now because of the race condition.
    // (2) On IA64 memory model this type of construct is
    // _not_supported_ (my understanding is that this is supported on
    // X86 and AMD64, though).
    //
    LOCK_SPLITS;
    for (u = 0; u < MAX_SPLITS; u++)
    {
        //
        // Try to find a vacant split
        //
        if (g_SplitInfo[u].uNumThreadsHelping == 0)
        {
            //
            // Found one, populate it.
            //
            ASSERT(g_uNumSplitsAvailable > 0);
            g_uNumSplitsAvailable--;

            //
            // We initialize this to two to double-reference this
            // split node.  This guarantees we are the last ones
            // holding a reference to it (since we want to be the last
            // one out of this split node)
            //
            g_SplitInfo[u].uNumThreadsHelping = 2;

            //
            // Store the path from the root to the split node and the
            // root position to start at.  This is done so that
            // ponders that convert into searches don't crash us and
            // so that helper threads can detect repeated positions
            // before the split point.
            //
            ASSERT(ctx->uPly >= 1);
            ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);
            for (v = 0; v < ctx->uPly; v++)
            {
                g_SplitInfo[u].mvPathToHere[v] = ctx->sPlyInfo[v].mv;
            }
            g_SplitInfo[u].mvPathToHere[v].uMove = ILLEGALMOVE;
            ASSERT(v >= 1);
            g_SplitInfo[u].mvLast = g_SplitInfo[u].mvPathToHere[v-1];
            memcpy(&(g_SplitInfo[u].sRootPosition),
                   GetRootPosition(),
                   sizeof(POSITION));
#if DEBUG
            g_SplitInfo[u].uSplitPly = ctx->uPly;
            memcpy(&(g_SplitInfo[u].sSplitPosition),
                   &(ctx->sPosition),
                   sizeof(POSITION));
#endif
            //
            // What has happened here is that another thread has
            // triggered the "stop searching" bit in the move timer.
            // This also means that the root position may have changed
            // and therefore the split we just populated can be
            // useless.  Before we grab any helper threads, see if we
            // need to bail out of this split.
            //
            if (g_MoveTimer.bvFlags & TIMER_STOPPING) 
            {
                g_uNumSplitsAvailable++;
                g_SplitInfo[u].uNumThreadsHelping = 0;
                UNLOCK_SPLITS;
                return(INVALID_SCORE);
            }

            //
            // More split node initialization
            //
            g_SplitInfo[u].uLock = 0;
            g_SplitInfo[u].fTerminate = FALSE;
            g_SplitInfo[u].uDepth = uDepth;
            g_SplitInfo[u].iPositionExtend = iPositionExtend;
            g_SplitInfo[u].iAlpha = *piAlpha;
            g_SplitInfo[u].iBeta = iBeta;
            g_SplitInfo[u].uSplitPositional = ctx->uPositional;
            g_SplitInfo[u].sSearchFlags = ctx->sSearchFlags;
            ASSERT(FALSE == ctx->sSearchFlags.fAvoidNullmove);
            g_SplitInfo[u].mvBest = *pmvBest;
            g_SplitInfo[u].iBestScore = *piBestScore;
            ASSERT(g_SplitInfo[u].iBestScore <= g_SplitInfo[u].iAlpha);
            g_SplitInfo[u].sCounters.tree.u64TotalNodeCount = 0;
            g_SplitInfo[u].sCounters.tree.u64BetaCutoffs = 0;
            g_SplitInfo[u].sCounters.tree.u64BetaCutoffsOnFirstMove = 0;
            g_SplitInfo[u].PV[0] = NULLMOVE;

            //
            // Copy the remaining moves to be searched from the
            // searcher context that called us into the split node.
            // Note: this thread must have already called
            // GenerateMoves at the split node!
            //
            uOldStart = ctx->sMoveStack.uBegin[ctx->uPly];
            g_SplitInfo[u].uAlreadyDone = uMoveNum - uOldStart + 1;
            ASSERT(g_SplitInfo[u].uAlreadyDone >= 1);
            ctx->sMoveStack.uBegin[ctx->uPly] = uMoveNum;
            for (v = uMoveNum, g_SplitInfo[u].uRemainingMoves = 0;
                 (v < ctx->sMoveStack.uEnd[ctx->uPly]);
                 v++, g_SplitInfo[u].uRemainingMoves++)
            {
                ASSERT(g_SplitInfo[u].uRemainingMoves >= 0);
                ASSERT(g_SplitInfo[u].uRemainingMoves < MAX_MOVES_PER_PLY);

                //
                // If we fail high at this node we have done a lot of
                // work for naught.  We also want to know as soon as
                // possible so that we can vacate this split point,
                // free up a worker thread and get back to the main
                // search.  So forget about the SEARCH_SORT_LIMIT
                // stuff here and sort the whole list of moves from
                // best..worst.
                //
                SelectBestWithHistory(ctx, v);
                ctx->sMoveStack.mvf[v].mv.bvFlags |=
                    WouldGiveCheck(ctx, ctx->sMoveStack.mvf[v].mv);
                ASSERT(!(ctx->sMoveStack.mvf[v].bvFlags & MVF_MOVE_SEARCHED));
                g_SplitInfo[u].mvf[g_SplitInfo[u].uRemainingMoves] =
                    ctx->sMoveStack.mvf[v];
#ifdef DEBUG
                ctx->sMoveStack.mvf[v].bvFlags |= MVF_MOVE_SEARCHED;
#endif
            }
            g_SplitInfo[u].uOnDeckMove = 0;
            g_SplitInfo[u].uNumMoves = g_SplitInfo[u].uRemainingMoves;
#ifdef DEBUG
            for (v = uMoveNum;
                 v < ctx->sMoveStack.uEnd[ctx->uPly];
                 v++)
            {
                ASSERT(ctx->sMoveStack.mvf[v].bvFlags & MVF_MOVE_SEARCHED);
                ASSERT(SanityCheckMove(&ctx->sPosition,
                                       ctx->sMoveStack.mvf[v].mv));
            }
#endif

            //
            // See if we can get some help here or we have to go it
            // alone.  Note: past this point the split we are using
            // may have threads under it -- be careful.
            //
            for (v = 0; v < g_uNumHelperThreads; v++)
            {
                if (g_HelperThreads[v].uAssignment == IDLE)
                {
                    //
                    // Note: there could already be a thread searching
                    // this split; we must obtain its lock now to mess
                    // with the helper count.
                    //
                    AcquireSpinLock(&(g_SplitInfo[u].uLock));
                    g_SplitInfo[u].uNumThreadsHelping += 1;
                    ReleaseSpinLock(&(g_SplitInfo[u].uLock));
                    ASSERT(g_SplitInfo[u].uNumThreadsHelping > 2);
                    ASSERT(g_uNumHelpersAvailable > 0);
                    g_uNumHelpersAvailable -= 1;
                    ASSERT(g_uNumHelpersAvailable >= 0);
                    ASSERT(g_uNumHelpersAvailable < g_uNumHelperThreads);
                    g_HelperThreads[v].uAssignment = u;
                }
            }
            UNLOCK_SPLITS;

            //
            // Update the context of the thread that is initiating the
            // split with a pointer to the split info node we are using.
            //
            for (uSplitNum = 0; 
                 uSplitNum < NUM_SPLIT_PTRS_IN_CONTEXT; 
                 uSplitNum++) 
            {
                if (ctx->pSplitInfo[uSplitNum] == NULL) 
                {
                    ctx->pSplitInfo[uSplitNum] = &(g_SplitInfo[u]);
                    break;
                }
            }
            if (uSplitNum >= NUM_SPLIT_PTRS_IN_CONTEXT)
            {
                ASSERT(FALSE);
                return(INVALID_SCORE);
            }
            ASSERT(ctx->pSplitInfo[uSplitNum] == &(g_SplitInfo[u]));

            //
            // Go search it
            //
            INC(ctx->sCounters.parallel.uNumSplits);
            HelpSearch(ctx, u);

            //
            // We are done searching under this node... make sure all
            // helpers are done too.  When everyone is finished the
            // refcount on this split node will be one because every
            // thread decremented it once and we double referenced
            // it initially.
            //
            while(g_SplitInfo[u].uNumThreadsHelping != 1)
            {
                ASSERT(g_SplitInfo[u].uNumThreadsHelping != 0);
                if (g_fExitProgram) break;
            }

            // 
            // Note: past this point we are the only ones using the
            // split until we return it to the pool by making its
            // refcount zero again.
            //
#ifdef DEBUG
            ASSERT((g_SplitInfo[u].uNumThreadsHelping == 1) ||
                   (g_fExitProgram));
            SystemDeferExecution(rand() % 2);
            ASSERT((g_SplitInfo[u].uNumThreadsHelping == 1) ||
                   (g_fExitProgram));
            if (g_SplitInfo[u].iBestScore < g_SplitInfo[u].iBeta)
            {
                for (v = 0;
                     v < g_SplitInfo[u].uRemainingMoves;
                     v++)
                {
                    ASSERT(g_SplitInfo[u].mvf[v].mv.uMove);
                    ASSERT(g_SplitInfo[u].mvf[v].bvFlags & MVF_MOVE_SEARCHED);
                }
            }
#endif
#ifdef PERF_COUNTERS
            //
            // Grab counters.  Technically we should do with under a
            // lock b/c we want to ensure that any pending memory
            // operations from other cpus are flushed.  But I don't
            // really care too much about these counters and am trying
            // to reduce lock contention.
            //
            if (TRUE == g_SplitInfo[u].fTerminate)
            {
                ASSERT(g_SplitInfo[u].iBestScore >= g_SplitInfo[u].iBeta);
                INC(ctx->sCounters.parallel.uNumSplitsTerminated);
            }
            ctx->sCounters.tree.u64BetaCutoffs =
                g_SplitInfo[u].sCounters.tree.u64BetaCutoffs;
            ctx->sCounters.tree.u64BetaCutoffsOnFirstMove =
                g_SplitInfo[u].sCounters.tree.u64BetaCutoffsOnFirstMove;
#endif
            //
            // Pop off the split info ptr from the stack in the thread's
            // context.
            //
            ASSERT(ctx->pSplitInfo[uSplitNum] == &(g_SplitInfo[u]));
            ctx->pSplitInfo[uSplitNum] = NULL;

            //
            // Grab alpha, bestscore, bestmove, PV etc...  The lock
            // needs to be here to flush any pending memory writes
            // from other processors.
            //
            LOCK_SPLITS;
            ctx->sCounters.tree.u64TotalNodeCount =
                g_SplitInfo[u].sCounters.tree.u64TotalNodeCount;
            iScore = *piBestScore = g_SplitInfo[u].iBestScore;
            *pmvBest = g_SplitInfo[u].mvBest;
            if ((*piAlpha < iScore) && (iScore < iBeta))
            {
                ASSERT(IS_SAME_MOVE(*pmvBest, g_SplitInfo[u].PV[0]));
                ASSERT((*pmvBest).uMove != 0);
                v = 0;
                do
                {
                    ASSERT((ctx->uPly + v) < MAX_PLY_PER_SEARCH);
                    ASSERT(v < MAX_PLY_PER_SEARCH);
                    ctx->sPlyInfo[ctx->uPly].PV[ctx->uPly+v] =
                        g_SplitInfo[u].PV[v];
                    if (0 == g_SplitInfo[u].PV[v].uMove) break;
                    v++;
                }
                while(1);
            }
            *piAlpha = g_SplitInfo[u].iAlpha;
            ASSERT(iBeta == g_SplitInfo[u].iBeta);
            g_uNumSplitsAvailable++;
            g_SplitInfo[u].uNumThreadsHelping = 0;
            UNLOCK_SPLITS;
            ctx->sMoveStack.uBegin[ctx->uPly] = uOldStart;

#ifdef DEBUG
            ASSERT(PositionsAreEquivalent(&board, &ctx->sPosition));
            VerifyPositionConsistency(&ctx->sPosition, FALSE);
            ASSERT(IS_VALID_SCORE(iScore) || WE_SHOULD_STOP_SEARCHING);
#endif
            return(iScore);
        }
    }

    //
    // There was no split available for us; unlock and continue in
    // search.
    //
    UNLOCK_SPLITS;
    return(INVALID_SCORE);
}


static void
_UpdateSplitInfo(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN MOVE mv,
                 IN SCORE iScore,
                 IN ULONG u)
/**

Routine description:

    We think we have some information learned from search to store in
    the split node.  Take the lock and see if we really do.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : context of thread updating split
    MOVE mv : move it just searched
    SCORE iScore : score of the move's subtree
    ULONG u : split node number
    
Return value:

    static void

**/
{
    ULONG v;
    
    AcquireSpinLock(&(g_SplitInfo[u].uLock));

    //
    // See if this split is shutting down
    //
    if (TRUE == g_SplitInfo[u].fTerminate) 
    {
        ReleaseSpinLock(&(g_SplitInfo[u].uLock));
        return;
    }

    if (iScore > g_SplitInfo[u].iBestScore)
    {
        //
        // We found a move better than the best so far, so we want to
        // update the split node and possibly raise alpha.
        //
        g_SplitInfo[u].iBestScore = iScore;
        g_SplitInfo[u].mvBest = mv;
        if (iScore > g_SplitInfo[u].iAlpha)
        {
            if (iScore >= g_SplitInfo[u].iBeta)
            {
                //
                // We failed high so we want to update the split node
                //
                g_SplitInfo[u].fTerminate = TRUE;
                g_SplitInfo[u].PV[0] = NULLMOVE;
                ReleaseSpinLock(&(g_SplitInfo[u].uLock));
                return;
            }

            //
            // Normal PV move, update the split's PV.
            //
            g_SplitInfo[u].iAlpha = iScore;
            UpdatePV(ctx, mv);
            ASSERT(ctx->uPly < MAX_PLY_PER_SEARCH);
            ASSERT(ctx->sPlyInfo[ctx->uPly].PV[ctx->uPly].uMove);
            for (v = 0; v < MAX_PLY_PER_SEARCH; v++)
            {
                ASSERT((ctx->uPly + v) < MAX_PLY_PER_SEARCH);
                mv = ctx->sPlyInfo[ctx->uPly].PV[ctx->uPly + v];
                g_SplitInfo[u].PV[v] = mv;
                if (0 == mv.uMove) break;
            }
        }
    }
    ReleaseSpinLock(&(g_SplitInfo[u].uLock));
}


static void 
_SetFinalStats(IN SEARCHER_THREAD_CONTEXT *ctx, 
               IN ULONG u)
/**

Routine description:

    We are exiting the split node (because it ran out of moves or
    because someone failed high).  Update some stats on the way out.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG u

Return value:

    static void

**/
{
    //
    // Before we stop searching this node, update some stuff.
    //
    AcquireSpinLock(&(g_SplitInfo[u].uLock));
    
    //
    // Counters to persist in the main counter struct via the split.
    //
    g_SplitInfo[u].sCounters.tree.u64TotalNodeCount += 
        ctx->sCounters.tree.u64TotalNodeCount;
    g_SplitInfo[u].sCounters.tree.u64BetaCutoffs += 
        ctx->sCounters.tree.u64BetaCutoffs;
    g_SplitInfo[u].sCounters.tree.u64BetaCutoffsOnFirstMove += 
        ctx->sCounters.tree.u64BetaCutoffsOnFirstMove;

    //
    // TODO: Any other counters we care about?
    //
    
    //
    // IDEA: Save the killers from this context to bring back to main
    //
    
    //
    // Decrement threadcount in this split.  Note: the main thread
    // incremented it by two.
    //
    ASSERT((g_SplitInfo[u].uNumThreadsHelping > 1) || (g_fExitProgram));
    g_SplitInfo[u].uNumThreadsHelping -= 1;
    ASSERT((g_SplitInfo[u].uNumThreadsHelping >= 1) || (g_fExitProgram));
    ReleaseSpinLock(&(g_SplitInfo[u].uLock));
}


static MOVE 
_GetNextParallelMove(OUT SCORE *piAlpha, 
                     OUT SCORE *piBestScore, 
                     OUT ULONG *puMoveNumber,
                     IN ULONG u)
/**
  
Routine description:

    Retrieve the next parallel move to search at the split node.  Also
    update alpha and bestscore.
    
Parameters:

    SCORE *piAlpha : current alpha
    SCORE *piBestScore : current bestscore
    ULONG u : split number

Return value:

    static MOVE

**/
{
    MOVE mv = {0};
    
    AcquireSpinLock(&(g_SplitInfo[u].uLock));
    if (g_SplitInfo[u].fTerminate)
    {
        ReleaseSpinLock(&(g_SplitInfo[u].uLock));
        return(mv);
    }

    if (g_SplitInfo[u].uRemainingMoves != 0)
    {
        //
        // There is another move to search, get it.
        //
        mv = g_SplitInfo[u].mvf[g_SplitInfo[u].uOnDeckMove].mv;
#ifdef DEBUG
        ASSERT(!(g_SplitInfo[u].mvf[g_SplitInfo[u].uOnDeckMove].bvFlags &
                 MVF_MOVE_SEARCHED));
        g_SplitInfo[u].mvf[g_SplitInfo[u].uOnDeckMove].bvFlags |= 
            MVF_MOVE_SEARCHED;
        ASSERT(mv.uMove);
        ASSERT(SanityCheckMove(&g_SplitInfo[u].sSplitPosition, mv));
#endif
        g_SplitInfo[u].uRemainingMoves--;
        *puMoveNumber = g_SplitInfo[u].uOnDeckMove;
        g_SplitInfo[u].uOnDeckMove++;
    }
    *piAlpha = g_SplitInfo[u].iAlpha;
    *piBestScore = g_SplitInfo[u].iBestScore;
    ReleaseSpinLock(&(g_SplitInfo[u].uLock));
    ASSERT(*piBestScore <= *piAlpha);
    return(mv);
}


void 
HelpSearch(IN OUT SEARCHER_THREAD_CONTEXT *ctx, 
           IN ULONG u)
/**

Routine description:

    Help search the split position.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : thread context
    ULONG u : the split index to help search

Return value:

    void

**/
{
    SCORE iScore;
    SCORE iAlpha = 0;
    SCORE iBeta;
    SCORE iBestScore = 0;
    SCORE iRoughEval;
    ULONG uOrigDepth;
    ULONG uDepth;
    ULONG uMoveNum = 0;
    INT iOrigExtend;
    INT iExtend;
    MOVE mv;
#ifdef DEBUG
    POSITION board;
    
    memcpy(&board, &ctx->sPosition, sizeof(POSITION));
    ASSERT(PositionsAreEquivalent(&board, &g_SplitInfo[u].sSplitPosition));
#endif

    iOrigExtend = g_SplitInfo[u].iPositionExtend;
    iBeta = g_SplitInfo[u].iBeta;
    uOrigDepth = g_SplitInfo[u].uDepth;
    ctx->sSearchFlags = g_SplitInfo[u].sSearchFlags;
    do
    {
        iExtend = iOrigExtend;
        uDepth = uOrigDepth;
        ASSERT(ctx->uPly == g_SplitInfo[u].uSplitPly);
        
        mv = _GetNextParallelMove(&iAlpha,
                                  &iBestScore, 
                                  &uMoveNum,
                                  u);
        if (mv.uMove == 0) break;              // Split is terminating

        ASSERT(IS_VALID_SCORE(iBestScore));
        ASSERT(uMoveNum < MAX_MOVES_PER_PLY);
        ASSERT(IS_SAME_MOVE(mv, 
          ctx->sMoveStack.mvf[ctx->sMoveStack.uBegin[ctx->uPly]+uMoveNum].mv));
        ASSERT(uDepth <= MAX_DEPTH_PER_SEARCH);
        ASSERT(IS_VALID_SCORE(iAlpha));
        ASSERT(IS_VALID_SCORE(iBeta));
        ASSERT(iAlpha < iBeta);
        ASSERT(iExtend >= -ONE_PLY);
        ASSERT(iExtend <= +ONE_PLY);
        
        if (MakeMove(ctx, mv))
        {
            ASSERT((IS_CHECKING_MOVE(mv) && 
                    InCheck(&ctx->sPosition, ctx->sPosition.uToMove)) ||
                   (!IS_CHECKING_MOVE(mv) && 
                    !InCheck(&ctx->sPosition, ctx->sPosition.uToMove)));
            iRoughEval = GetRoughEvalScore(ctx, iAlpha, iBeta, TRUE);
            
            // Compute extension
            ComputeMoveExtension(ctx, 
                                 iAlpha,
                                 iBeta,
                                 (ctx->sMoveStack.uBegin[ctx->uPly - 1] + 
                                  uMoveNum),
                                 iRoughEval,
                                 uDepth,
                                 &iExtend);
            
            //
            // Decide whether to history prune
            // 
            if (TRUE == WeShouldDoHistoryPruning(iRoughEval,
                                                 iAlpha,
                                                 iBeta,
                                                 ctx,
                                                 uDepth,
                                                 (g_SplitInfo[u].uAlreadyDone +
                                                  uMoveNum + 1),
                                                 mv,
                                                 (g_SplitInfo[u].uAlreadyDone +
                                                  uMoveNum + 1),
                                                 iExtend))
            {
                ASSERT(iExtend == 0);
                iExtend = -ONE_PLY;
                ctx->sPlyInfo[ctx->uPly].iExtensionAmount = -ONE_PLY;
            }
            
            //
            // Compute next depth
            // 
            uDepth = uDepth - ONE_PLY + iExtend;
            if (uDepth >= MAX_DEPTH_PER_SEARCH) uDepth = 0;
            
            iScore = -Search(ctx, -iAlpha - 1, -iAlpha, uDepth);
            if ((iAlpha < iScore) && (iScore < iBeta))
            {
                iScore = -Search(ctx, -iBeta, -iAlpha, uDepth);
            }

            //
            // Decide whether to research reduced branches to full depth.
            // 
            if ((iExtend < 0) && (iScore >= iBeta))
            {
                uDepth += ONE_PLY;
                ctx->sPlyInfo[ctx->uPly].iExtensionAmount = 0;
                iScore = -Search(ctx, -iBeta, -iAlpha, uDepth);
            }
            UnmakeMove(ctx, mv);
            ASSERT(PositionsAreEquivalent(&ctx->sPosition, &board));
            if (TRUE == g_SplitInfo[u].fTerminate) break;
            if (iScore > iBestScore)
            {
                _UpdateSplitInfo(ctx, mv, iScore, u);
            }
        }
    }
    while(1);
    _SetFinalStats(ctx, u);
}
#endif
