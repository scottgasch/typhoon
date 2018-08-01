/**

Copyright (c) Scott Gasch

Module Name:

    hash.c

Abstract:

    The main transposition table code.  The table is a large range of 
    memory organized into lines (64 bytes).  Each line contains four
    entries that are 16 bytes each (index 0..3).
    
    After a search, data about the position's score and the depth we
    searched to determine this score can be stored in the hash table.
    When information is stored a hash replacement scheme is used to
    overwrite one of the four entries on a line.
    
    Later, before embarking on a deep recursive search, we can check
    the hash table to see if information relevant to this position 
    has been stored already (and can be reused).  If so, we can avoid
    repeating the same search and save time.
    
    A hash entry is 16 bytes long and looks like this:
    
        typedef struct _HASH_ENTRY
        {
            MOVE mv;                 // 0 1 2 3
            UCHAR uDepth;            // 4
            UCHAR bvFlags;           // 5 ==> d  i  r  t  | thr up low exact
            signed short iValue;     // 6 7
            UINT64 u64Sig;           // 8 9 A B C D E F == 16 bytes
        } HASH_ENTRY;
        
    The mv is the best move in a position.  Even if we do not have a
    deep enough uDepth recorded to cut off, mv is searched first.
    uDepth is the depth searched to determine iValue.  iValue is
    either the exact score or a bound score -- depending on the
    value of bvFlag's low order 4 bits.  The high order 4 bits
    in bvFlags holds "dirty" information to aide the hash replacement
    scheme.  Finally the u64Sig value is used to absolutely identify
    the position to which the hash entry's information refers.

    When screwing around with the hash code, turn on PERF_COUNTERS and
    watch the hash hit rate (and usable hash hit rate).  Also here is
    Fine position #70 which is a great sanity check for hash code:
    
    8/k7/3p4/p2P1p2/P2P1P2/8/8/K7/ w - - 0 0 bm 1. Ka1-b1 Ka7-b7 2. Kb1-c1

Author:

    Scott Gasch (scott.gasch@gmail.com) 12 Jun 2004

Revision History:

    $Id: hash.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

//
// Hash entries are 16 bytes each and hash lines are 4 entries = 64
// bytes.  This is done on purpose so that a hash line fits in a cpu
// cache line.  My Athlons have 64 byte cache lines.  To adjust this
// for other processors, change the constant below.
//
#define CONVERT_SEARCH_DEPTH_TO_HASH_DEPTH(x) \
    ASSERT(IS_VALID_DEPTH(x)); \
    (x) >>= 4; \
    ASSERT(((x) & 0xffffff00) == 0);

#define HASH_ENTRY_IS_DIRTY(x) \
    (((x).bvFlags & 0xF0) != g_cDirty)
    
#undef ADJUST_MATE_IN_N

#ifdef MP
//
// On MP builds we lock ranges of the hash table in order to mitigate
// against two searcher threads colliding on the same entry at once.
// I did not have a free bit to use in the hash entry so I lock every
// 256th hash table entry at once.
// 
#define NUM_HASH_LOCKS 256
volatile static ULONG g_uHashLocks[NUM_HASH_LOCKS];
#define HASH_IS_LOCKED(x) ((g_uHashLocks[(x)]) != 0)
#define LOCK_HASH(x) \
    AcquireSpinLock(&(g_uHashLocks[(x)])); \
    ASSERT(HASH_IS_LOCKED(x));
#define UNLOCK_HASH(x) \
    ASSERT(HASH_IS_LOCKED(x)); \
    ReleaseSpinLock(&(g_uHashLocks[(x)]));
#else // no MP
#define HASH_IS_LOCKED(x)
#define LOCK_HASH(x)
#define UNLOCK_HASH(x)
#endif

ULONG g_uHashTableSizeEntries = 0;
ULONG g_uHashTableSizeBytes = 0;
HASH_ENTRY *g_pHashTable = NULL;
UCHAR g_cDirty = 0;

void 
DirtyHashTable(void)
/**

Routine description:

    The dirty pattern is bitwise ored onto the bvFlags part of a 
    hash table entry.  Since the low nibble is used to hold flags
    like upper bound, lower bound, exact score, and threat the 
    dirty indicator is limited to operating in the upper nibble
    of bvFlags (giving it a range of 16 values)

Parameters:

    void

Return value:

    void

**/
{
    ASSERT((g_cDirty & 0x0F) == 0);
    g_cDirty += 0x10;
    ASSERT((g_cDirty & 0x0F) == 0);
#ifdef TEST
    if (g_Options.fShouldPost)
    {
        Trace("Hash table dirty char: 0x%02x\n", g_cDirty);
    }
#endif
}


void 
CleanupHashSystem(void)
/**

Routine description:

    Cleanup the hash table by freeing the memory and zeroing some 
    variables.  This is called before building an opening book in
    order to free some virtual address space up.

Parameters:

    void

Return value:

    void

**/
{
    if (NULL != g_pHashTable)
    {
        ASSERT(g_uHashTableSizeBytes > 0);
        SystemFreeMemory(g_pHashTable);
        g_pHashTable = NULL;
        g_uHashTableSizeBytes = 0;
        g_uHashTableSizeEntries = 0;
    }
}

FLAG 
InitializeHashSystem(void)
/**

Routine description:

    Initialize the hash system... allocate the table and get everything
    ready to go.

Parameters:

    void

Return value:

    FLAG

**/
{
    ULONG uBytesToAlloc;
    
    if (!IS_A_POWER_OF_2(g_Options.uNumHashTableEntries))
    {
        return(FALSE);
    }    
    g_uHashTableSizeEntries = g_Options.uNumHashTableEntries;
    if (g_uHashTableSizeEntries != 0)
    {
        ASSERT(g_pHashTable == NULL);
        uBytesToAlloc = (sizeof(HASH_ENTRY) * g_uHashTableSizeEntries);
        g_pHashTable = SystemAllocateMemory(uBytesToAlloc);
        g_uHashTableSizeBytes = uBytesToAlloc;
        ClearHashTable();
#ifdef MP
        memset((BYTE *)g_uHashLocks, 0, sizeof(g_uHashLocks));
#endif
    }
    return(TRUE);
}


void 
ClearHashTable (void) 
/**

Routine description:

    Zero out the entire hash table.  This function is called when a new
    position is loaded into the root board.

Parameters:

    void

Return value:

    void

**/
{
    if (NULL != g_pHashTable)
    {
        g_cDirty = 0;
        ASSERT(0 != g_uHashTableSizeBytes);
        ASSERT(0 != g_uHashTableSizeEntries);
        memset(g_pHashTable, 0, g_uHashTableSizeBytes);
    }
}


//
// (g_uHashTableSizeEntries) =>  1000 0000 ... 0000
// NUM_HASH_ENTRIES_PER_LINE =>  -             0100
// ------------------------------------------------
//                               1111 1111 ... 1100 => Start of a hash line
//
#define KEY_TO_HASH_LINE_START(x) \
    ((x) & (g_uHashTableSizeEntries - NUM_HASH_ENTRIES_PER_LINE))

static HASH_ENTRY *
_SelectHashLine(UINT64 u64Key)
/**

Routine description:

    On an MP system, this routine locks part of the hash table, the
    caller should do its thing and then unlock the locked portion of
    the hash table by calling _DeselectHashLine(u64Key).

Parameters:

    UINT64 u64Key

Return value:

    static HASH_ENTRY

**/
{
    ULONG uLow = (ULONG)(u64Key >> 32);
    ULONG uLine = KEY_TO_HASH_LINE_START(uLow);
#ifdef MP
    ULONG uLock = (uLine >> 2) & (NUM_HASH_LOCKS - 1);
    LOCK_HASH(uLock);
    ASSERT(IS_A_POWER_OF_2(NUM_HASH_LOCKS));
#endif
    ASSERT(g_uHashTableSizeBytes != 0);
    ASSERT(g_uHashTableSizeEntries != 0);
    ASSERT(g_pHashTable != NULL);
    ASSERT(IS_A_POWER_OF_2(g_uHashTableSizeEntries));
    ASSERT(((g_uHashTableSizeEntries - NUM_HASH_ENTRIES_PER_LINE) &
            (NUM_HASH_ENTRIES_PER_LINE - 1)) == 0);
    return(&(g_pHashTable[uLine]));
}

#ifdef MP
static void 
_DeselectHashLine(UINT64 u64Key)
/**

Routine description:

    This releases the lock taken by _SelectHashLine

Parameters:

    UINT64 u64Key

Return value:

    static void

**/
{
    ULONG uLow = (ULONG)(u64Key >> 32);
    ULONG uLine = KEY_TO_HASH_LINE_START(uLow);
    ULONG uLock = (uLine >> 2) & (NUM_HASH_LOCKS - 1);
    
    ASSERT(IS_A_POWER_OF_2(NUM_HASH_LOCKS));
    ASSERT(MP);
    UNLOCK_HASH(uLock);
}
#endif


static HASH_ENTRY *
_SelectHashEntry(UINT64 u64Key,
                 ULONG uDepth,
                 ULONG uType,
                 SCORE iValue)
/**

Routine description:

    This routine takes a lock.  The caller should do its thing and then 
    release the lock by calling _DeselectHashEntry.

Parameters:

    UINT64 u64Key,
    ULONG uDepth

Return value:

    static HASH_ENTRY

**/
{
    HASH_ENTRY *pEntry;
    HASH_ENTRY *pStore;

    ASSERT(g_uHashTableSizeBytes != 0);
    ASSERT(g_uHashTableSizeEntries != 0);
    ASSERT(g_pHashTable != NULL);
    ASSERT(IS_A_POWER_OF_2(g_uHashTableSizeEntries));

    //
    // _SelectHashLine takes a lock on MP builds.
    //
    pEntry = _SelectHashLine(u64Key);
    ASSERT(pEntry != NULL);
#ifdef DEBUG
    pStore = NULL;
#endif
    
    //
    // Note: Hash lines consist of four entries currently:
    //
    // 1. The first is a depth-priority hash entry (store here if you
    // can beat the depth of what's already in there).
    //
    // 2. The second is an always-store hash entry (store here if you
    // can't store in the depth-priority entry).
    //
    // 3. and 4. The last two entries are where we bump the data from
    // the depth- priority table and always-store table when they are
    // going to be overwritten by newer data.
    // 

    //
    // Replace the 0th entry on the line if:
    // 
    // ...the new entry has greater depth
    // ...the current entry is stale (from an old search)
    //
    if ((pEntry[0].uDepth <= uDepth) ||
        HASH_ENTRY_IS_DIRTY(pEntry[0]))
    {
        //
        // Do we need to back up the entry we will overwrite?
        //
        if ((pEntry[0].u64Sig != u64Key) ||
            ((pEntry[0].bvFlags & HASH_FLAG_VALID_BOUNDS) != uType))
        {
            pEntry[2] = pEntry[0];
        }
        pStore = &(pEntry[0]);
        goto end;
    }

    //
    // We could not replace the depth priority 0th entry; instead,
    // replace the "always replace" 1st entry on the line unless it
    // contains mate information and is not stale.
    //
    if (((abs(pEntry[1].iValue >= +NMATE)) && 
         (abs(iValue) < +NMATE)) &&
        (!HASH_ENTRY_IS_DIRTY(pEntry[1])))
    {
        pStore = &(pEntry[3]);
        goto end;
    }
    if ((pEntry[1].u64Sig != u64Key) ||
        ((pEntry[1].bvFlags & HASH_FLAG_VALID_BOUNDS) != uType))
    {
        pEntry[3] = pEntry[1];
    }
    pStore = &(pEntry[1]);
    
 end:
    ASSERT(pStore != NULL);
    return(pStore);
}


#ifdef MP
static void 
_DeselectHashEntry(UINT64 u64Key)
/**

Routine description:

    Releases the lock taken by _SelectHashEntry.

Parameters:

    UINT64 u64Key

Return value:

    static void

**/
{
    _DeselectHashLine(u64Key);                // this releases a lock
}
#endif


void 
StoreUpperBound(//MOVE mvBest, 
    POSITION *pos, 
    SCORE iValue,
    ULONG uDepth,
    FLAG fThreat)
/**

Routine description:

    Store an upper bound in the hash table.

Parameters:

    MOVE mvBest : singular move from the node, if any
    POSITION *pos,
    SCORE iValue,
    ULONG uDepth,
    FLAG fThreat

Return value:

    void

**/
{
    UINT64 u64Key;
    HASH_ENTRY *pHash;
    
    ASSERT(IS_VALID_SCORE(iValue));
    ASSERT(IS_VALID_DEPTH(uDepth));
    
    //
    // It does not make sense to store hash positions that are worth
    // at best ~INFINITY.  Every position is and this is a waste of
    // hash space.  Ignore these.
    //
    if (iValue >= +NMATE) return;
    if (NULL == g_pHashTable) return;

    CONVERT_SEARCH_DEPTH_TO_HASH_DEPTH(uDepth);
    u64Key = pos->u64NonPawnSig ^ pos->u64PawnSig;

    //
    // _SelectHashEntry takes a lock on MP builds
    //
    pHash = _SelectHashEntry(u64Key, uDepth, HASH_FLAG_UPPER, iValue);
    ASSERT(pHash != NULL);
    pHash->uDepth = (UCHAR)uDepth;
    if (pHash->u64Sig != u64Key)
    {
        //
        // Note: don't overwrite a good hash move with "none" if we
        // are replacing a hash entry for the same position.  Also
        // if we are klobbering data about the same position save
        // a memory write to update the (same) sig.
        //
        pHash->mv.uMove = 0;
        pHash->u64Sig = u64Key;
    }
    ASSERT((g_cDirty & HASH_FLAG_UPPER) == 0);
    pHash->bvFlags = g_cDirty | HASH_FLAG_UPPER;
    ASSERT((pHash->bvFlags & 0xF0) == g_cDirty);
    if (TRUE == fThreat)
    {
        ASSERT((pHash->bvFlags & HASH_FLAG_THREAT) == 0);
        pHash->bvFlags |= HASH_FLAG_THREAT;
    }
    
    if (iValue <= -NMATE)
    {
        //
        // If this is an upper bound near -INFINITY
        // (i.e. ~-INFINITY at best) convert it into -NMATE at
        // best (at best mate in N).
        // 
        pHash->iValue = -NMATE;
    }
    else
    {
        //
        // Otherwise hash the exact value.
        //
        ASSERT(iValue > -NMATE);
        ASSERT(iValue < +NMATE);
        pHash->iValue = (signed short)iValue;
    }
#ifdef MP
    //
    // Release the lock on MP builds
    //
    _DeselectHashEntry(u64Key);
#endif
}


void 
StoreExactScore(MOVE mvBestMove, 
                POSITION *pos, 
                SCORE iValue, 
                ULONG uDepth, 
                FLAG fThreat, 
                ULONG uPly)
/**
  
Routine description:

    Store a best move and its exact score in the hash table.
  
Parameters:

    MOVE mvBestMove,
    POSITION *pos,
    SCORE iValue,
    ULONG uDepth,
    FLAG fThreat,
    ULONG uPly

Return value:

    void

**/
{
    HASH_ENTRY *pHash;
    UINT64 u64Key;
    
    ASSERT(IS_VALID_SCORE(iValue));
    ASSERT(IS_VALID_DEPTH(uDepth));
    if (NULL == g_pHashTable) return;

    CONVERT_SEARCH_DEPTH_TO_HASH_DEPTH(uDepth);
    u64Key = pos->u64NonPawnSig ^ pos->u64PawnSig;

    //
    // _SelectHashEntry takes a lock on MP builds.
    //
    pHash = _SelectHashEntry(u64Key, uDepth, HASH_FLAG_EXACT, iValue);
    ASSERT(pHash != NULL);
        
    //
    // Populate the hash entry
    //
    pHash->uDepth = (UCHAR)uDepth;
    ASSERT(mvBestMove.uMove);
    pHash->mv.uMove = mvBestMove.uMove;
    ASSERT((g_cDirty & HASH_FLAG_EXACT) == 0);
    pHash->bvFlags = g_cDirty | HASH_FLAG_EXACT;
    ASSERT((pHash->bvFlags & 0xF0) == g_cDirty);
    if (TRUE == fThreat)
    {
        ASSERT((pHash->bvFlags & HASH_FLAG_THREAT) == 0);
        pHash->bvFlags |= HASH_FLAG_THREAT;
    }
    pHash->u64Sig = u64Key;
        
    if (iValue >= +NMATE)
    {
#ifdef ADJUST_MATE_IN_N
        ASSERT((iValue + (int)uPly) < +INFINITY);
        ASSERT((iValue + (int)uPly) > -INFINITY);
        pHash->iValue = (signed short)(iValue + (int)uPly);
#else
        pHash->bvFlags = g_cDirty | HASH_FLAG_LOWER;
        ASSERT((pHash->bvFlags & 0xF0) == g_cDirty);
        pHash->iValue = +NMATE;
#endif
    }
    else if (iValue <= -NMATE)
    {
#ifdef ADJUST_MATE_IN_N
        ASSERT((iValue + (int)uPly) < +INFINITY);
        ASSERT((iValue - (int)uPly) > -INFINITY);
        pHash->iValue = (signed short)(iValue - (int)uPly);
#else
        pHash->bvFlags = g_cDirty | HASH_FLAG_UPPER;
        ASSERT((pHash->bvFlags & 0xF0) == g_cDirty);
        pHash->iValue = +NMATE;
#endif
    }
    else
    {
        ASSERT(iValue > -NMATE);
        ASSERT(iValue < +NMATE);
        pHash->iValue = (short signed)iValue;
    }
#ifdef MP
    //
    // Release the lock on MP builds
    //
    _DeselectHashEntry(u64Key);               // Release the lock
#endif
}


void 
StoreLowerBound(MOVE mvBestMove, 
                POSITION *pos, 
                SCORE iValue,
                ULONG uDepth, 
                FLAG fThreat)
/**

Routine description:

    Store a lower bound score and move in the hash table.

Parameters:

    MOVE mvBestMove,
    POSITION *pos,
    SCORE iValue,
    ULONG uDepth,
    FLAG fThreat

Return value:

    void

**/
{
    HASH_ENTRY *pHash;
    UINT64 u64Key;

    ASSERT(IS_VALID_SCORE(iValue));
    ASSERT(IS_VALID_DEPTH(uDepth));

    //
    // Do not bother to hash this node if it is telling us that the
    // value of this position is worth at least ~-INFINITY.  This is
    // useless information and a waste of hash space.
    // 
    if (iValue <= -NMATE) return;
    if (NULL == g_pHashTable) return;
    
    CONVERT_SEARCH_DEPTH_TO_HASH_DEPTH(uDepth);
    u64Key = pos->u64NonPawnSig ^ pos->u64PawnSig;

    //
    // _SelectHashEntry takes a lock on MP builds
    //
    pHash = _SelectHashEntry(u64Key, uDepth, HASH_FLAG_LOWER, iValue);
    ASSERT(pHash != NULL);
        
    //
    // Populate the entry
    //
    pHash->uDepth = (UCHAR)uDepth;
    if ((mvBestMove.uMove) || (pHash->u64Sig != u64Key))
    {
        //
        // Note: only overwrite the existing hash move if either we
        // have a move (i.e. not null move) or the entry we are
        // overwriting is for a different position.
        //
        pHash->mv.uMove = mvBestMove.uMove;
    }
    pHash->u64Sig = u64Key;
    ASSERT((g_cDirty & HASH_FLAG_LOWER) == 0);
    pHash->bvFlags = g_cDirty | HASH_FLAG_LOWER;
    ASSERT((pHash->bvFlags & 0xF0) == g_cDirty);
    pHash->bvFlags |= (HASH_FLAG_THREAT * (fThreat == TRUE));
    if (iValue < +NMATE)
    {
        ASSERT(iValue < +NMATE);
        ASSERT(iValue > -NMATE);
        pHash->iValue = (signed short)iValue;
    }
    else
    {
        //
        // If the value is greater than mate in N don't store how many
        // moves N is because this depends on where in the search we
        // have found this position.  Just say that this position is
        // worth at least mate in N.
        // 
        pHash->iValue = +NMATE;
    }
#ifdef MP
    //
    // Release the lock on MP builds
    //
    _DeselectHashEntry(u64Key);
#endif
}


MOVE 
GetPonderMove(POSITION *pos)
/**

Routine description:

    Get a move to ponder.

Parameters:

    POSITION *pos

Return value:

    MOVE

**/
{
    HASH_ENTRY *pHash;
    ULONG x;
    UINT64 u64Key = pos->u64NonPawnSig ^ pos->u64PawnSig;
    MOVE mv;
    
    mv.uMove = 0;
    if (NULL == g_pHashTable) return(mv);

    //
    // _SelectHashLine takes a lock on MP builds
    //
    pHash = _SelectHashLine(u64Key);
    for (x = 0;
         x < NUM_HASH_ENTRIES_PER_LINE;
         x++)
    {
        if (u64Key == pHash->u64Sig)
        {
            mv = pHash->mv;
            if ((pHash->bvFlags & HASH_FLAG_VALID_BOUNDS) == 
                HASH_FLAG_EXACT)
            {
                goto end;
            }
        }
    }

 end:
#ifdef MP
    //
    // Release the lock.
    //
    _DeselectHashLine(u64Key);
#endif
    return(mv);
}


HASH_ENTRY *
HashLookup(SEARCHER_THREAD_CONTEXT *ctx, 
           ULONG uDepth, 
           ULONG uNextDepth, 
           SCORE iAlpha, 
           SCORE iBeta, 
           FLAG *pfThreat, 
           FLAG *pfAvoidNull, 
           MOVE *pHashMove, 
           SCORE *piScore)
/**

Routine description:

    Lookup any information we have about the current position in the
    hash table.  Cutoff if we can.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    ULONG uDepth,
    ULONG uNextDepth,
    SCORE iAlpha,
    SCORE iBeta,
    FLAG *pfThreat,
    FLAG *pfAvoidNull,
    MOVE *pHashMove,
    SCORE *piScore

Return value:

    HASH_ENTRY

**/
{
    HASH_ENTRY *pHash;
    ULONG x;
    POSITION *pos = &(ctx->sPosition);
    UINT64 u64Key = pos->u64NonPawnSig ^ pos->u64PawnSig;
    ULONG uMoveScore = 0;
    ULONG uThisScore;
    
#define COMPUTE_MOVE_SCORE(mv, depth) \
    ((depth) * 8 + PIECE_VALUE_OVER_100((mv).pCaptured) + 1)

    ASSERT(IS_VALID_DEPTH(uDepth));
    ASSERT(IS_VALID_DEPTH(uNextDepth));
    ASSERT(uNextDepth <= uDepth);
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT(pHashMove->uMove == 0);
    ASSERT(NULL != g_pHashTable);
    INC(ctx->sCounters.hash.u64Probes);
    
    //
    // These are out parameters for the caller, initialize them
    // saying we don't know anything.
    //
    *pfAvoidNull = FALSE;
    *pfThreat = FALSE;
    
    CONVERT_SEARCH_DEPTH_TO_HASH_DEPTH(uDepth);
    CONVERT_SEARCH_DEPTH_TO_HASH_DEPTH(uNextDepth);

    //
    // _SelectHashLine takes a lock on MP builds.
    //
    pHash = _SelectHashLine(u64Key);
    for (x = 0;
         x < NUM_HASH_ENTRIES_PER_LINE;
         x++)
    {
        if (u64Key == pHash->u64Sig)
        {
            //
            // This entry hit, make it "clean" so it is harder to replace
            // later on.
            // 
            pHash->bvFlags &= 0x0F;
            ASSERT((g_cDirty & 0x0F) == 0);
            pHash->bvFlags |= g_cDirty;
            ASSERT((pHash->bvFlags & 0xF0) == g_cDirty);
            
            //
            // Parse the matching entry
            //
            *pfThreat |= ((pHash->bvFlags & HASH_FLAG_THREAT) > 0);
            
            //
            // If we have an exact match and sufficient depth to
            // satisfy the depth to which we will later search after a
            // nullmove, see if the known value of this node is less
            // than beta.  If so, doing nothing in this position will
            // probably not be better than doing something therefore
            // its not wise to perform the nullmove search.  It's just
            // a waste of nodes and time.
            // 
            if ((pHash->uDepth >= uNextDepth) &&
                ((pHash->bvFlags & HASH_FLAG_UPPER) ||
                 (pHash->bvFlags & HASH_FLAG_EXACT)) &&
                (pHash->iValue < iBeta))
            {
                *pfAvoidNull = TRUE;
            }

            // Because we can encounter several moves while looking for
            // a hash cutoff, have a semantic about which one we return
            // to the search.
            INC(ctx->sCounters.hash.u64OverallHits);
            if (pHash->mv.uMove) {
                uThisScore = COMPUTE_MOVE_SCORE(pHash->mv, uDepth);
                ASSERT(uThisScore != 0);
                if (uThisScore > uMoveScore) {
                    uMoveScore = uThisScore;
                    *pHashMove = pHash->mv;
                    ASSERT(SanityCheckMove(pos, pHash->mv));
                }
            }
            
            //
            // We have a hit, but is it useful?
            //
            if (pHash->uDepth >= uDepth)
            {
                switch (pHash->bvFlags & HASH_FLAG_VALID_BOUNDS)
                {
                    case HASH_FLAG_EXACT:
                        *piScore = pHash->iValue;
                        INC(ctx->sCounters.hash.u64UsefulHits);
                        INC(ctx->sCounters.hash.u64ExactScoreHits);
#ifdef ADJUST_MATE_IN_N
                        //
                        // Readjust mate in N scores.
                        //
                        if (pHash->iValue >= +NMATE)
                        {
                            *piScore -= (int)uPly;
                        }
                        else if (pHash->iValue <= -NMATE)
                        {
                            *piScore += (int)uPly;
                        }
#endif
                        ASSERT(IS_VALID_SCORE(*piScore));
                        goto end;
                        break;
                        
                    case HASH_FLAG_LOWER:
                        if (pHash->iValue >= iBeta)
                        {
                            INC(ctx->sCounters.hash.u64UsefulHits);
                            INC(ctx->sCounters.hash.u64LowerBoundHits);
                            *piScore = pHash->iValue;
                            ASSERT(IS_VALID_SCORE(*piScore));
                            goto end;
                        }
                        break;
                    
                    case HASH_FLAG_UPPER:
                        if (pHash->iValue <= iAlpha)
                        {
                            INC(ctx->sCounters.hash.u64UsefulHits);
                            INC(ctx->sCounters.hash.u64UpperBoundHits);
                            *piScore = pHash->iValue;
                            ASSERT(IS_VALID_SCORE(*piScore));
                            goto end;
                        }
                        break;
#ifdef DEBUG
                    default:
                        ASSERT(FALSE);
                        break;
#endif
                }
            }
        }
        pHash++;
    }
    pHash = NULL;
    
 end:
#ifdef MP
    //
    // Release the lock.
    //
    _DeselectHashLine(u64Key);
#endif
    return(pHash);
}
