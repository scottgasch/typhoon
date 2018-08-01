/**

Copyright (c) Scott Gasch

Module Name:

    probe.c

Abstract:

    Routines for probing endgame tablebases; much of this code is
    "borrowed" from crafty.

Author:

    Scott Gasch (scott.gasch@gmail.com) 02 Jul 2004

Revision History:

    $Id: probe.c 355 2008-07-01 15:46:43Z scott $

**/

#include "chess.h"

#define XX (127)                              // sq not on the board
#define C_PIECES (3)                          // max num pieces of one color

ULONG g_uEgtbLock = 0;

//
// define INDEX type
// 
#define T_INDEX64                             // use 64 bit indexes
#if defined (T_INDEX64) && defined (_MSC_VER)
typedef unsigned __int64 INDEX;
#elif defined (T_INDEX64)
typedef unsigned long long INDEX;
#else
typedef unsigned long INDEX;
#endif

//
// define square type and tb type
// 
typedef unsigned int squaret;
typedef signed char tb_t;

//
// define color
// 
typedef int color;
#define x_colorWhite    0
#define x_colorBlack    1
#define x_colorNeutral  2
#define COLOR_DECLARED

//
// define pieces
// 
typedef int piece;
#define x_pieceNone    0
#define x_piecePawn    1
#define x_pieceKnight  2
#define x_pieceBishop  3
#define x_pieceRook    4
#define x_pieceQueen   5
#define x_pieceKing    6
#define PIECES_DECLARED

//
// scores returned from egtb.cpp
// 
#define pageL       65536
#define tbbe_ssL    ((pageL-4)/2)
#define bev_broken  (tbbe_ssL+1)        /* illegal or busted */
#define bev_mi1     tbbe_ssL    /* mate in 1 move */
#define bev_mimin   1   /* mate in max moves */
#define bev_draw    0   /* draw */
#define bev_limax   (-1)        /* mated in max moves */
#define bev_li0     (-tbbe_ssL) /* mated in 0 moves */

//
// define PfnCalcIndex
// 
typedef INDEX(TB_FASTCALL * PfnCalcIndex)(squaret *, 
                                          squaret *, 
                                          squaret, 
                                          int fInverse);

//
// prototypes for functions in egtb.cpp
// 
extern int IDescFindFromCounters(int *);
extern int FRegisteredFun(int, color);
extern PfnCalcIndex PfnIndCalcFun(int, color);
extern int TB_FASTCALL L_TbtProbeTable(int, color, INDEX);
#define PfnIndCalc PfnIndCalcFun
#define FRegistered FRegisteredFun
extern int IInitializeTb(char *);
extern int FTbSetCacheSize(void *buffer, unsigned long size);
extern int TB_CRC_CHECK;

//
// Globals
// 
static int EGTBMenCount = 0;
void *egtb_cache = NULL;
#define EGTB_CACHE_SIZE (8*1024*1024)


void 
InitializeEGTB(void)
/**

Routine description:

    [Re]Initialize the Nalimov EGTB system.  Called during system
    startup and when the user uses "set" to change the EGTB path.

Parameters:

    void (uses g_OPtionz.szEGTBPath)

Return value:

    void

**/
{
    CHAR *szPath = g_Options.szEGTBPath;

    if ((szPath != NULL) && strlen(szPath) > 0) {
        // TB_CRC_CHECK = 1;
        EGTBMenCount = IInitializeTb(szPath);
        if (0 != EGTBMenCount) 
        {
            Trace("Found %d-men endgame tablebases.\n\n", EGTBMenCount);
            if (NULL != egtb_cache) 
            {
                SystemFreeMemory(egtb_cache);
                egtb_cache = NULL;
            }
            egtb_cache = SystemAllocateMemory(EGTB_CACHE_SIZE);
            if (NULL != egtb_cache)
            {
                FTbSetCacheSize(egtb_cache, EGTB_CACHE_SIZE);
            }
        }
    }
}

void 
CleanupEGTB(void)
/**

Routine description:

    Cleanup the Nalimov EGTB system.

Parameters:

    void

Return value:

    void

**/
{
    if (NULL != egtb_cache)
    {
        SystemFreeMemory(egtb_cache);
        egtb_cache = NULL;
    }
}


FLAG 
ProbeEGTB(SEARCHER_THREAD_CONTEXT *ctx,
          SCORE *piScore)
/**

Routine description:

    Search for a board position in the EGTB files on disk.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    SCORE *piScore

Return value:

    FLAG

**/
{
    POSITION *pos = &(ctx->sPosition);
    int pcCount[10];
    int wSquares[C_PIECES*5+1], bSquares[C_PIECES*5+1];
    int iTB;
    ULONG uColor;
    int invert;
    int *wp, *bp;
    int ep;
    INDEX index;
    int value;
    FLAG fResult;
    ULONG x;
    int y;
    COOR c; 
    PIECE p;
    PfnCalcIndex fp;

    //
    // EGTB initialized?
    //
    ULONG wcount = (pos->uNonPawnCount[WHITE][0] +
                    pos->uPawnCount[WHITE]);
    ULONG bcount = (pos->uNonPawnCount[BLACK][0] +
                    pos->uPawnCount[BLACK]);
    if ((wcount + bcount > (ULONG)EGTBMenCount) ||
        (wcount > 3) ||
        (bcount > 3)) 
    {
        return(FALSE);
    }
    INC(ctx->sCounters.egtb.uProbes);
    memset(pcCount, 0, sizeof(pcCount));

    for (x = 0;
         x < pos->uPawnCount[WHITE];
         x++)
    {
        c = pos->cPawns[WHITE][x];
        ASSERT (IS_ON_BOARD(c));
        ASSERT(pos->rgSquare[c].pPiece && IS_PAWN(pos->rgSquare[c].pPiece));
        
        c = TO64(c);
        ASSERT((c >= 0) && (c <= 64));
        
        y = pcCount[0];
        ASSERT(y >= 0);
        ASSERT(y < (C_PIECES * 5 + 1));
        wSquares[y] = c;
        pcCount[0]++;
    }

    for (x = 1;
         x < pos->uNonPawnCount[WHITE][0];
         x++)
    {
        c = pos->cNonPawns[WHITE][x];
        ASSERT (IS_ON_BOARD(c));

        p = pos->rgSquare[c].pPiece;
        ASSERT(p && !IS_PAWN(p) && !IS_KING(p));
        
        //
        // convert:               into:
        // XXX_PAWN               0
        // XXX_KNIGHT             1
        // XXX_BISHOP             2
        // XXX_ROOK               3
        // XXX_QUEEN              4
        //
        p = ((p >> 1) & 0x7) - 1;
        ASSERT(p > 0);
        ASSERT(p < 5);
        
        c = TO64(c);
        ASSERT((c >= 0) && (c <= 64));
        
        y = p * C_PIECES + pcCount[p];
        ASSERT(y >= 0);
        ASSERT(y < (C_PIECES * 5 + 1));
        wSquares[y] = c;
        pcCount[p]++;
    }

    for (x = 0;
         x < pos->uPawnCount[BLACK];
         x++)
    {
        c = pos->cPawns[BLACK][x];
        ASSERT(IS_ON_BOARD(c));
        ASSERT(pos->rgSquare[c].pPiece && IS_PAWN(pos->rgSquare[c].pPiece));
        
        c = TO64(c);
        ASSERT(c >= 0);
        ASSERT(c <= 64);

        y = pcCount[5];
        ASSERT(y >= 0);
        ASSERT(y < (C_PIECES * 5 + 1));
        bSquares[y] = c;
        pcCount[5]++;
    }

    for (x = 1; 
         x < pos->uNonPawnCount[BLACK][0];
         x++)
    {
        c = pos->cNonPawns[BLACK][x];
        ASSERT(IS_ON_BOARD(c));
        
        p = pos->rgSquare[c].pPiece;
        ASSERT(p && !IS_PAWN(p) && !IS_KING(p));
        
        //
        // convert:               into:
        // XXX_PAWN               0
        // XXX_KNIGHT             1
        // XXX_BISHOP             2
        // XXX_ROOK               3
        // XXX_QUEEN              4
        // 
        p = ((p >> 1) & 0x7) - 1;
        ASSERT(p > 0);
        ASSERT(p < 5);
        
        c = TO64(c);
        ASSERT(c >= 0);
        ASSERT(c <= 64);
        
        y = p * C_PIECES + pcCount[5 + p];
        ASSERT(y >= 0);
        ASSERT(y < (C_PIECES * 5 + 1));
        bSquares[y] = c;        
        pcCount[5 + p]++;
    }

    AcquireSpinLock(&g_uEgtbLock);
    iTB = IDescFindFromCounters(pcCount);
    if (iTB == 0) 
    {
        fResult = FALSE;
        goto end;
    }
        
    //
    // Add the kings to the piece lists
    // 
    ASSERT(pos->rgSquare[pos->cNonPawns[WHITE][0]].pPiece == WHITE_KING);
    wSquares[C_PIECES * 5] = TO64(pos->cNonPawns[WHITE][0]);
    ASSERT(wSquares[C_PIECES * 5] >= 0);
    ASSERT(wSquares[C_PIECES * 5] <= 64);

    ASSERT(pos->rgSquare[pos->cNonPawns[BLACK][0]].pPiece == BLACK_KING);
    bSquares[C_PIECES * 5] = TO64(pos->cNonPawns[BLACK][0]);    
    ASSERT(bSquares[C_PIECES * 5] >= 0);
    ASSERT(bSquares[C_PIECES * 5] <= 64);

    if (iTB > 0)
    {
        uColor = (pos->uToMove == WHITE) ? 0 : 1;
        invert = 0;
        wp = wSquares;
        bp = bSquares;
    }
    else 
    {
        uColor = (pos->uToMove == WHITE) ? 1 : 0;
        invert = 1;
        wp = bSquares;
        bp = wSquares;
        iTB = -iTB;
    }
    
    if (!FRegisteredFun(iTB, uColor)) 
    {
        fResult = FALSE;
        goto end;
    }
    ep = XX;
    if (IS_ON_BOARD(pos->cEpSquare)) 
    {
        ASSERT((IS_ON_BOARD(pos->cEpSquare + 1) &&
                IS_PAWN(pos->rgSquare[pos->cEpSquare + 1].pPiece)) ||
               (IS_ON_BOARD(pos->cEpSquare - 1) &&
                IS_PAWN(pos->rgSquare[pos->cEpSquare - 1].pPiece)));
        ep = TO64(pos->cEpSquare);
    }
    
#if 0
    DumpPosition(pos);
    Trace("iTB = %d, uColor = %u, ep = %u, invert = %u\n",
          iTB, uColor, ep, invert);
    Trace("wp =\t\t\tbp =\n");
    for(x = 0; x < 16; x++)
    {
        Trace("%u\t\t\t%u\n", wSquares[x], bSquares[x]);
    }
#endif
    ASSERT(IS_VALID_COLOR(uColor));
    fp = PfnIndCalcFun(iTB, uColor);
    index = fp((squaret *)wp, 
               (squaret *)bp, 
               (squaret)ep, 
               invert);
    value = L_TbtProbeTable(iTB, uColor, index);
    if (value == bev_broken) 
    {
        fResult = FALSE;
        goto end;
    }

    if (value > 0)
    {
        *piScore = INFINITY + (2*(-bev_mi1+value)) - 1;
    }
    else if (value < 0)
    {
        *piScore = -INFINITY + (2*(bev_mi1+value));
    }
    else
    {
        *piScore = 0; // g_iDrawValue[pos->uToMove];
    }
    fResult = TRUE;

 end:
    ReleaseSpinLock(&g_uEgtbLock);
#ifdef PERF_COUNTERS
    if (fResult == TRUE)
    {
        INC(ctx->sCounters.egtb.uHits);
    }
#endif
    return(fResult);
}
