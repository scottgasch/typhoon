/**

Copyright (c) Scott Gasch

Module Name:

    bench.c

Abstract:

    Engine speed benchmarking code.  This is the same test that crafty
    uses to benchmark.

Author:

    Scott Gasch (scott.gasch@gmail.com) 28 Jun 2004

Revision History:

    $Id: bench.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

COMMAND(BenchCommand)
/**

Routine description:

    Run a benchmark

Parameters:

    (hidden) CHAR *szInput
    (hidden) ULONG argc
    (hidden) CHAR *argv[]
    (hidden) POSITION *pos

Return value:

**/
{
    typedef struct _BENCH_NODE
    {
        CHAR *szFen;
        ULONG uDepth;
    }
    BENCH_NODE;
    static BENCH_NODE x[] = 
    {
        {
            "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - 0 0", 11
        },
        {
            "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq - 0 0", 11
        },
        { 
            "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - - 0 0", 13
        },
        {
            "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - - 0 0", 11
        },
        {
            "2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - - 0 0", 11
        },
        {
            "r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq - 0 0", 11
        }
    };
    ULONG u;
    double dStart;
    double dEnd;
    UINT64 u64Nodes = 0;

#ifdef DEBUG
    Trace("You know this is a DEBUG build, right?\n");
#endif
    dStart = SystemTimeStamp();
    for (u = 0;
         u < ARRAY_LENGTH(x) && (FALSE == g_fExitProgram);
         u++)
    {
        VERIFY(PreGameReset(FALSE));
        VERIFY(SetRootPosition(x[u].szFen));
        pos = GetRootPosition();
        g_Options.u64NodesSearched = 0ULL;
        g_Options.uMaxDepth = x[u].uDepth;
        Think(pos);
        u64Nodes += g_Options.u64NodesSearched;
    }
    dEnd = SystemTimeStamp();
    ASSERT(dStart < dEnd);
    ASSERT(u64Nodes > 0);

    Trace("Searched %"COMPILER_LONGLONG_UNSIGNED_FORMAT 
          " nodes in %4.1f sec.\n", u64Nodes, (dEnd - dStart));
    Trace("BENCHMARK>> %8.1f nodes/sec\n", (double)u64Nodes / (dEnd - dStart));
    VERIFY(PreGameReset(TRUE));
}
