/*++

Copyright (c) Scott Gasch

Module Name:

    poshash.c

Abstract:

    A hash table of information about positions.

Author:

    Scott Gasch (SGasch) 11 Nov 2006

Revision History:

--*/

#include "chess.h"

#define NUM_POSITION_HASH_ENTRIES (1048576) // 16Mb
POSITION_HASH_ENTRY g_PositionHash[NUM_POSITION_HASH_ENTRIES];

#ifdef MP
#define NUM_POSITION_HASH_LOCKS (512)
volatile static ULONG g_uPositionHashLocks[NUM_POSITION_HASH_LOCKS];
#define POSITION_HASH_IS_LOCKED(x) ((g_uPositionHashLocks[(x)]) != 0)
#define LOCK_POSITION_HASH(x) \
    AcquireSpinLock(&(g_uPositionHashLocks[(x)])); \
    ASSERT(POSITION_HASH_IS_LOCKED(x))
#define UNLOCK_POSITION_HASH(x) \
    ASSERT(POSITION_HASH_IS_LOCKED(x)); \
    ReleaseSpinLock(&(g_uPositionHashLocks[(x)]))
#else
#define POSITION_HASH_IS_LOCKED(x)
#define LOCK_HASH(x)
#define UNLOCK_HASH(x)
#endif

void
InitializePositionHashSystem(void) 
{
    memset(&g_PositionHash, 0, sizeof(g_PositionHash));
#ifdef MP
    memset(&g_uPositionHashLocks, 0, sizeof(g_uPositionHashLocks));
#endif
}

void
CleanupPositionHashSystem(void) 
{
    ; // do nothing
}

static INLINE UINT64 PositionToSignatureIgnoringMove(POSITION *pos)
{
    return((pos->u64NonPawnSig ^ pos->u64PawnSig) >> 1);
}

// Note: sig must be pre-shifted to ignore the side-to-move bit.
static INLINE ULONG PositionSigToHashPosition(UINT64 u64Sig) 
{
    ULONG u = (ULONG)u64Sig;
    u &= (NUM_POSITION_HASH_ENTRIES - 1);
    ASSERT(u < NUM_POSITION_HASH_ENTRIES);
    return u;
}

#ifdef MP
static INLINE ULONG HashPositionToLockNumber(ULONG u) 
{
    u &= (NUM_POSITION_HASH_LOCKS - 1);
    ASSERT(u < NUM_POSITION_HASH_LOCKS);
    return u;
}
#endif

void
StoreEnprisePiece(POSITION *pos, COOR cSquare) 
{
    UINT64 u64Sig = PositionToSignatureIgnoringMove(pos);
    ULONG uEntry = PositionSigToHashPosition(u64Sig);
    POSITION_HASH_ENTRY *pHash = &(g_PositionHash[uEntry]);
    PIECE p = pos->rgSquare[cSquare].pPiece;
    ULONG uColor = GET_COLOR(p);
#ifdef MP
    ULONG uLock = HashPositionToLockNumber(uEntry);
    LOCK_POSITION_HASH(uLock);
#endif
    ASSERT(p && IS_VALID_PIECE(p));
    ASSERT(!IS_PAWN(p));
    ASSERT(CAN_FIT_IN_UCHAR(cSquare));
    ASSERT(IS_ON_BOARD(cSquare));
    pHash->cEnprise[uColor] = (UCHAR)cSquare;
    ASSERT(pHash->uEnpriseCount[uColor] < 16);
    pHash->uEnpriseCount[uColor] += 1;
    if (pHash->u64Sig != u64Sig) 
    {
        pHash->u64Sig = u64Sig;
        pHash->uEnpriseCount[uColor] = 1;
        pHash->cTrapped[uColor] = ILLEGAL_COOR;
        uColor = FLIP(uColor);
        pHash->cEnprise[uColor] = ILLEGAL_COOR;
        pHash->cTrapped[uColor] = ILLEGAL_COOR;
        pHash->uEnpriseCount[uColor] = 0;
    }
#ifdef MP
    UNLOCK_POSITION_HASH(uLock);
#endif
}

void
StoreTrappedPiece(POSITION *pos, COOR cSquare) 
{
    UINT64 u64Sig = PositionToSignatureIgnoringMove(pos);
    ULONG uEntry = PositionSigToHashPosition(u64Sig);
    POSITION_HASH_ENTRY *pHash = &(g_PositionHash[uEntry]);
    PIECE p = pos->rgSquare[cSquare].pPiece;
    ULONG uColor = GET_COLOR(p);
#ifdef MP
    ULONG uLock = HashPositionToLockNumber(uEntry);
    LOCK_POSITION_HASH(uLock);
#endif
    ASSERT(p && IS_VALID_PIECE(p));
    ASSERT(!IS_PAWN(p));
    ASSERT(CAN_FIT_IN_UCHAR(cSquare));
    ASSERT(IS_ON_BOARD(cSquare));
    pHash->cTrapped[uColor] = cSquare;
    if (pHash->u64Sig != u64Sig) 
    {
        pHash->u64Sig = u64Sig;
        pHash->cEnprise[uColor] = ILLEGAL_COOR;
        pHash->uEnpriseCount[uColor] = 0;
        uColor = FLIP(uColor);
        pHash->cEnprise[uColor] = ILLEGAL_COOR;
        pHash->cTrapped[uColor] = ILLEGAL_COOR;
        pHash->uEnpriseCount[uColor] = 0;
    }
#ifdef MP
    UNLOCK_POSITION_HASH(uLock);
#endif
}

COOR
GetEnprisePiece(POSITION *pos, ULONG uSide)
{
    UINT64 u64Sig = PositionToSignatureIgnoringMove(pos);
    ULONG uEntry = PositionSigToHashPosition(u64Sig);
    POSITION_HASH_ENTRY *pHash = &(g_PositionHash[uEntry]);
    COOR c = ILLEGAL_COOR;
#ifdef MP
    ULONG uLock = HashPositionToLockNumber(uEntry);
    LOCK_POSITION_HASH(uLock);
#endif
    if (pHash->u64Sig == u64Sig)
    {
        c = pHash->cEnprise[uSide];
    }
#ifdef MP
    UNLOCK_POSITION_HASH(uLock);
#endif
    return c;
}

COOR
GetTrappedPiece(POSITION *pos, ULONG uSide)
{
    UINT64 u64Sig = PositionToSignatureIgnoringMove(pos);
    ULONG uEntry = PositionSigToHashPosition(u64Sig);
    POSITION_HASH_ENTRY *pHash = &(g_PositionHash[uEntry]);
    COOR c = ILLEGAL_COOR;
#ifdef MP
    ULONG uLock = HashPositionToLockNumber(uEntry);
    LOCK_POSITION_HASH(uLock);
#endif
    if (pHash->u64Sig == u64Sig)
    {
        c = pHash->cTrapped[uSide];
    }
#ifdef MP
    UNLOCK_POSITION_HASH(uLock);
#endif
    return c;
}

FLAG
SideCanStandPat(POSITION *pos, ULONG uSide)
{
    UINT64 u64Sig = PositionToSignatureIgnoringMove(pos);
    ULONG uEntry = PositionSigToHashPosition(u64Sig);
    POSITION_HASH_ENTRY *pHash = &(g_PositionHash[uEntry]);
    FLAG fStand = TRUE;
#ifdef MP
    ULONG uLock = HashPositionToLockNumber(uEntry);
    LOCK_POSITION_HASH(uLock);
#endif
    if (pHash->u64Sig == u64Sig) 
    {
        fStand = ((pHash->cTrapped[uSide] == ILLEGAL_COOR) &&
                  (pHash->uEnpriseCount[uSide] < 2));
    }
#ifdef MP
    UNLOCK_POSITION_HASH(uLock);
#endif
    return fStand;
}

ULONG
ValueOfMaterialInTroubleDespiteMove(POSITION *pos, ULONG uSide) 
{
    UINT64 u64Sig = PositionToSignatureIgnoringMove(pos);
    ULONG uEntry = PositionSigToHashPosition(u64Sig);
    POSITION_HASH_ENTRY *pHash = &(g_PositionHash[uEntry]);
    ULONG u = 0;
    COOR c;
#ifdef MP
    ULONG uLock = HashPositionToLockNumber(uEntry);
    LOCK_POSITION_HASH(uLock);
#endif
    if (pHash->u64Sig == u64Sig) 
    {
        if (pHash->uEnpriseCount[uSide] > 1) 
        {
            c = pHash->cEnprise[uSide];
            ASSERT(IS_ON_BOARD(c));
            u = PIECE_VALUE(pos->rgSquare[c].pPiece);
            ASSERT(u);
        }
        c = pHash->cTrapped[uSide];
        if (IS_ON_BOARD(c)) 
        {
            u = MAXU(u, PIECE_VALUE(pos->rgSquare[c].pPiece));
            ASSERT(u);
        }
    }
#ifdef MP
    UNLOCK_POSITION_HASH(uLock);
#endif
    return u;
}

ULONG
ValueOfMaterialInTroubleAfterNull(POSITION *pos, ULONG uSide)
{
    UINT64 u64Sig = PositionToSignatureIgnoringMove(pos);
    ULONG uEntry = PositionSigToHashPosition(u64Sig);
    POSITION_HASH_ENTRY *pHash = &(g_PositionHash[uEntry]);
    ULONG u = 0;
    COOR c;
#ifdef MP
    ULONG uLock = HashPositionToLockNumber(uEntry);
    LOCK_POSITION_HASH(uLock);
#endif
    if (pHash->u64Sig == u64Sig) 
    {
        if (pHash->uEnpriseCount[uSide])
        {
            c = pHash->cEnprise[uSide];
            ASSERT(IS_ON_BOARD(c));
            u += PIECE_VALUE(pos->rgSquare[c].pPiece);
            ASSERT(u);
        }
        c = pHash->cTrapped[uSide];
        if (IS_ON_BOARD(c))
        {
            u = MAXU(PIECE_VALUE(pos->rgSquare[c].pPiece), u);
            ASSERT(u);
        }
    }
#ifdef MP
    UNLOCK_POSITION_HASH(uLock);
#endif
    return u;
}
