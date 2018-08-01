/**

Copyright (c) Scott Gasch

Module Name:

    script.c

Abstract:

    Some glue code to support running test scripts and keeping track
    of test suite results.

Author:

    Scott Gasch (scott.gasch@gmail.com) 18 May 2004

Revision History:

    $Id: script.c 355 2008-07-01 15:46:43Z scott $

**/

#include "chess.h"

// ----------------------------------------------------------------------
//
// Global testsuite position counters
// 
#define SUITE_MAX_SOLUTIONS                   (4)
#define SUITE_MAX_AVOIDS                      (4)
#define SUITE_NUM_HISTOGRAM                   (8)

//
// A big struct of counters we want to maintain when running test scripts.
//
typedef struct _SUITE_COUNTERS
{
    CHAR szId[SMALL_STRING_LEN_CHAR];
    ULONG uNumSolutions;
    MOVE mvSolution[SUITE_MAX_SOLUTIONS];
    ULONG uNumAvoids;
    MOVE mvAvoid[SUITE_MAX_AVOIDS];
    ULONG uCorrect;
    ULONG uIncorrect;
    ULONG uTotal;
    UINT64 u64TotalNodeCount;
    double dCurrentSolutionTime;
    ULONG uCurrentSolvedPlies;
    ULONG uSigmaDepth;
    double dSigmaSolutionTime;
    double dAverageNps;
    double dAverageFirstMoveBeta;
    double dAverageTimeToSolution;
    ULONG uHistogram[SUITE_NUM_HISTOGRAM];
}
SUITE_COUNTERS;
static SUITE_COUNTERS g_SuiteCounters;


static void 
_ClearSuiteData(void)
/**

Routine description:

    Clear data/counters we have for the current test script.

Parameters:

    void

Return value:

    static void

**/
{
    g_SuiteCounters.uNumSolutions = 0;
    memset(g_SuiteCounters.mvSolution,
           0,
           sizeof(g_SuiteCounters.mvSolution));
    g_SuiteCounters.uNumAvoids = 0;
    memset(g_SuiteCounters.mvAvoid,
           0,
           sizeof(g_SuiteCounters.mvAvoid));
    memset(g_SuiteCounters.szId,
           0,
           sizeof(g_SuiteCounters.szId));
    g_SuiteCounters.uCurrentSolvedPlies = 0;
    g_SuiteCounters.dCurrentSolutionTime = 0;
}


COMMAND(IdCommand)
/**

Routine description:

    This function implements the 'id' engine command.  

    Usage:
    
        id [optional string]

        With no agruments, id returns the current position's id tag.
        With a string argument, id sets the current position's id tag.

        id tags are useful[?] for naming positions in test suites...

Parameters:

    The COMMAND macro hides four arguments from the input parser:

        CHAR *szInput : the full line of input
        ULONG argc    : number of argument chunks
        CHAR *argv[]  : array of ptrs to each argument chunk
        POSITION *pos : a POSITION pointer to operate on

Return value:

    void

**/
{
    CHAR *p;
    ULONG u;

    if (argc < 2)
    {
        Trace("The current position's id is: \"%s\"\n",
              g_SuiteCounters.szId);
    }
    else
    {
        p = szInput;
        while(*p && !isspace(*p))
        {
            p++;
        }
        while(*p && isspace(*p))
        {
            p++;
        }
        u = 0;
        while(*p)
        {
            if (!isspace(*p))
            {
                g_SuiteCounters.szId[u] = *p;
                u++;
            }
            if (u > ARRAY_LENGTH(g_SuiteCounters.szId))
            {
                break;
            }
            p++;
        }
    }
}

COMMAND(SolutionCommand)
/**

Routine description:

    This function implements the 'solution' engine command.  

    Usage:
    
        solution [optional move string]

        With no agruments, solution prints out a list of the "correct
        solution" move(s) set for the current position.

        With a move string argument solution sets another move as the
        correct answer to the current position.  The move string can
        be in ICS or SAN format.

        When the engine is allowed to search a position it will
        compare its best move from the search against the solution
        move(s) for the position and keep track of how many it gets
        right/wrong and how long it took to solve.

        A position can have up to three solutions.  A position cannot
        have both solution moves and avoid moves.

Parameters:

    The COMMAND macro hides four arguments from the input parser:

        CHAR *szInput : the full line of input
        ULONG argc    : number of argument chunks
        CHAR *argv[]  : array of ptrs to each argument chunk
        POSITION *pos : a POSITION pointer to operate on

Return value:

    void

**/
{
    ULONG u;
    MOVE mv;

    if (argc < 2)
    {
        Trace("There %s %u solution move%s set:\n",
              (g_SuiteCounters.uNumSolutions > 1) ? "are" : "is",
              g_SuiteCounters.uNumSolutions,
              (g_SuiteCounters.uNumSolutions > 1) ? "s" : "");
        for(u = 0;
            u < g_SuiteCounters.uNumSolutions;
            u++)
        {
            Trace("    %s\n", MoveToSan(g_SuiteCounters.mvSolution[u], pos));
        }
    }
    else
    {
        mv.uMove = 0;
        switch(LooksLikeMove(argv[1]))
        {
            case MOVE_SAN:
                mv = ParseMoveSan(argv[1], pos);
                break;
            case MOVE_ICS:
                mv = ParseMoveIcs(argv[1], pos);
                break;
            case NOT_MOVE:
                break;
        }

        if (mv.uMove == 0)
        {
            Trace("Error (bad move): %s\n", argv[1]);
            return;
        }
        
        if (g_SuiteCounters.uNumSolutions < SUITE_MAX_SOLUTIONS)
        {
            g_SuiteCounters.mvSolution[g_SuiteCounters.uNumSolutions] = mv;
            g_SuiteCounters.uNumSolutions++;
        }
    }
}


COMMAND(AvoidCommand)
/**

Routine description:

    This function implements the 'avoid' engine command.  

    Usage:
    
        avoid [optional move string]

        With no agruments, avoid prints out a list of the "don't play
        this" move(s) set for the current position.

        With a move string argument, avoid sets another move as the
        incorrect answer to the current position.  The move string can
        be in ICS or SAN format.

        When the engine is allowed to search a position it will
        compare its best move from the search against the avoid
        move(s) for the position and keep track of how many it gets
        right/wrong and how long it took to solve.

        A position can have up to three avoid moves.  A position
        cannot have both solution moves and avoid moves.

Parameters:

    The COMMAND macro hides four arguments from the input parser:

        CHAR *szInput : the full line of input
        ULONG argc    : number of argument chunks
        CHAR *argv[]  : array of ptrs to each argument chunk
        POSITION *pos : a POSITION pointer to operate on

Return value:

    void

**/
{
    ULONG u;
    MOVE mv;

    if (argc < 2)
    {
        Trace("There %s %u avoid move%s set:\n",
              (g_SuiteCounters.uNumAvoids > 1) ? "are" : "is",
              g_SuiteCounters.uNumAvoids,
              (g_SuiteCounters.uNumAvoids > 1) ? "s" : "");
        for(u = 0;
            u < g_SuiteCounters.uNumAvoids;
            u++)
        {
            Trace("    %s\n", MoveToSan(g_SuiteCounters.mvAvoid[u], pos));
        }
    }
    else
    {
        mv.uMove = 0;
        switch(LooksLikeMove(argv[1]))
        {
            case MOVE_SAN:
                mv = ParseMoveSan(argv[1], pos);
                break;
            case MOVE_ICS:
                mv = ParseMoveIcs(argv[1], pos);
                break;
            case NOT_MOVE:
                break;
        }

        if (mv.uMove == 0)
        {
            Trace("Error (bad move): %s\n", argv[1]);
            return;
        }
        
        if (g_SuiteCounters.uNumAvoids < SUITE_MAX_AVOIDS)
        {
            g_SuiteCounters.mvAvoid[g_SuiteCounters.uNumAvoids] = mv;
            g_SuiteCounters.uNumAvoids++;
        }
    }
}



COMMAND(ScriptCommand)
/**

Routine description:

    This function implements the 'script' engine command.  

    Usage:
    
        script <required filename>

        The script command opens the reads the required filename and
        reads it line by line.  Each line of the script file is
        treated as a command and executed in sequence.  Each command
        is executed fully before the next script line is read and
        executed.  When all lines in the script file have been read
        and executed the engine will print out a post-script report of
        test suite statistics and then begin to pay attention to
        commands from the keyboard again.

        Scripts are meant to facilitate execution of test suites.

Parameters:

    The COMMAND macro hides four arguments from the input parser:

        CHAR *szInput : the full line of input
        ULONG argc    : number of argument chunks
        CHAR *argv[]  : array of ptrs to each argument chunk
        POSITION *pos : a POSITION pointer to operate on

Return value:

    void

**/
{
    FILE *p;
    static CHAR szLine[SMALL_STRING_LEN_CHAR];
    double dSuiteStart;
    ULONG u, v, uMax;
    double dMult;

    if (argc < 2)
    {
        Trace("Error (Usage: script <filename>): %s\n", argv[0]);
        return;
    }
    if (FALSE == SystemDoesFileExist(argv[1]))
    {
        Trace("Error (file doesn't exist): %s\n", argv[1]);
        return;
    }
    
    p = fopen(argv[1], "rb");
    if (NULL != p)
    {
        // Doing the script...
        dSuiteStart = SystemTimeStamp();
        memset(&(g_SuiteCounters), 0, sizeof(g_SuiteCounters));
        while(fgets(szLine, ARRAY_LENGTH(szLine), p))
        {
            Trace("SCRIPT> %s\n", szLine);
            if (!STRNCMPI(szLine, "go", 2))
            {
                (void)Think(pos);
                g_SuiteCounters.u64TotalNodeCount += 
                    g_Options.u64NodesSearched;
            }
            else
            {
                PushNewInput(szLine);
                if (TRUE == g_fExitProgram) break;
                ParseUserInput(FALSE);
            }
        }
        fclose(p);

        // Banner and end results
        dMult = (double)g_Options.uMyIncrement / (double)SUITE_NUM_HISTOGRAM;
        Banner();
        Trace("\n"
              "TEST SCRIPT execution complete.  Final statistics:\n"
              "--------------------------------------------------\n"
              "    correct solutions   : %u\n"
              "    incorrect solutions : %u\n"
              "    total problems      : %u\n"
              "    total nodecount     : %" 
                   COMPILER_LONGLONG_UNSIGNED_FORMAT "\n"
              "    avg. search speed   : %6.1f nps\n"
              "    avg. solution time  : %3.1f sec\n"
              "    avg. search depth   : %4.1f ply\n"
              "    script time         : %6.1f sec\n\n",
              g_SuiteCounters.uCorrect,
              g_SuiteCounters.uIncorrect,
              g_SuiteCounters.uTotal,
              g_SuiteCounters.u64TotalNodeCount,
              ((double)g_SuiteCounters.u64TotalNodeCount / 
               (SystemTimeStamp() - dSuiteStart)),
              (g_SuiteCounters.dSigmaSolutionTime / 
               (double)g_SuiteCounters.uCorrect),
              ((double)g_SuiteCounters.uSigmaDepth / 
               (double)g_SuiteCounters.uTotal),
              (SystemTimeStamp() - dSuiteStart));
        
        // Histogram stuff
        if (g_SuiteCounters.uTotal > 0) {
            uMax = g_SuiteCounters.uHistogram[0];
            for (u = 1; u < SUITE_NUM_HISTOGRAM; u++) 
            {
                if (g_SuiteCounters.uHistogram[u] > uMax) 
                {
                    uMax = g_SuiteCounters.uHistogram[u];
                }
            }
            ASSERT(uMax > 0);
            for (u = 0; u < SUITE_NUM_HISTOGRAM; u++) 
            {
                Trace("%4.1f .. %4.1f: ", 
                      (double)u * dMult, (double)(u + 1) * dMult);
                for (v = 0; 
                     v < 50 * g_SuiteCounters.uHistogram[u] / uMax; 
                     v++) 
                {
                    Trace("*");
                }
                Trace("\n");
            }
        }
    }
}

FLAG 
WeAreRunningASuite(void)
/**

Routine description:

    A function that returns TRUE if the engine is running a test suite
    and FALSE otherwise.

Parameters:

    void

Return value:

    FLAG

**/
{
    if (g_SuiteCounters.uNumSolutions +
        g_SuiteCounters.uNumAvoids)
    {
        return(TRUE);
    }
    return(FALSE);
}


FLAG 
CheckTestSuiteMove(MOVE mv, SCORE iScore, ULONG uDepth)
/**

Routine description:

    This function is called by RootSearch when it has finished a
    search.  The MOVE parameter is the best move that the search
    found.  This code checks that move against the solution or avoid
    moves for the current position, determines if we "got it right"
    and updates the test suite counters accordingly.

Parameters:

    MOVE mv

Return value:

    FLAG

**/
{
    FLAG fSolved = FALSE;
    ULONG u;

    if (!WeAreRunningASuite()) return FALSE;

    //
    // There is(are) solution move(s).
    //
    if (g_SuiteCounters.uNumSolutions)
    {
        ASSERT(g_SuiteCounters.uNumAvoids == 0);
        fSolved = FALSE;
        for (u = 0; u < g_SuiteCounters.uNumSolutions; u++)
        {
            if ((mv.cFrom == g_SuiteCounters.mvSolution[u].cFrom) &&
                (mv.cTo == g_SuiteCounters.mvSolution[u].cTo))
            {
                fSolved = TRUE;
                break;
            }
        }
        if (!fSolved) {
            g_SuiteCounters.uCurrentSolvedPlies = 0;
            g_SuiteCounters.dCurrentSolutionTime = 0.0;
        } else {
            if (g_SuiteCounters.uCurrentSolvedPlies == 0) {
                g_SuiteCounters.dCurrentSolutionTime = 
                    (SystemTimeStamp() - g_MoveTimer.dStartTime);
            }
            g_SuiteCounters.uCurrentSolvedPlies++;
        }
    }
    else if (g_SuiteCounters.uNumAvoids) 
    {
        //
        // Handle avoid-move problems.
        //
        fSolved = TRUE;
        for (u = 0; u < g_SuiteCounters.uNumAvoids; u++)
        {
            if ((mv.cFrom == g_SuiteCounters.mvAvoid[u].cFrom) &&
                (mv.cTo == g_SuiteCounters.mvAvoid[u].cTo))
            {
                fSolved = FALSE;
            }
        }
        if (!fSolved) 
        {
            g_SuiteCounters.uCurrentSolvedPlies = 0;
            g_SuiteCounters.dCurrentSolutionTime = 0.0;
        } else {
            if (g_SuiteCounters.uCurrentSolvedPlies == 0) {
                g_SuiteCounters.dCurrentSolutionTime = 
                    (SystemTimeStamp() - g_MoveTimer.dStartTime);
            }
            g_SuiteCounters.uCurrentSolvedPlies++;
        }
    }
    else
    {
        UtilPanic(SHOULD_NOT_GET_HERE,
                  NULL, NULL, NULL, NULL,
                  __FILE__, __LINE__);
    }

    //
    // Handle "FastScript" variable for solution moves.
    //
    if ((g_Options.fFastScript) &&
        (g_SuiteCounters.uCurrentSolvedPlies > 4) &&
        (iScore >= 0))
    {
        if (((g_Options.uMyIncrement != 0) &&
             (SystemTimeStamp() - g_MoveTimer.dStartTime >=
              (double)g_Options.uMyIncrement / 3)) ||
            ((g_Options.uMaxDepth != 0) &&
             (uDepth >= g_Options.uMaxDepth / 3)))
        {
            Trace("FAST SCRIPT --> stop searching now\n");
            g_MoveTimer.bvFlags |= TIMER_STOPPING;
        }
    }
    return(fSolved);
}


void 
PostMoveTestSuiteReport(SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    If we're running a suite, print a little blurb after each move to
    act as a running total of how we're doing.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    void

**/
{
    MOVE mv = ctx->mvRootMove;
    ULONG uDepth = ctx->uRootDepth / ONE_PLY, y;
    double dMult;

    //
    // This is the real move the searched picked when all was said and
    // done (i.e. it ran out of time or reached a fixed depth).  See
    // if it's right.
    //
    if (WeAreRunningASuite())
    {
        g_SuiteCounters.uTotal++;
        g_SuiteCounters.uSigmaDepth += uDepth;
        
        if (CheckTestSuiteMove(mv, -1, uDepth))
        {
            Trace("Problem %s solved in %3.1f sec.\n", 
                  g_SuiteCounters.szId,
                  g_SuiteCounters.dCurrentSolutionTime);
            g_SuiteCounters.uCorrect++;
            g_SuiteCounters.dSigmaSolutionTime += 
                g_SuiteCounters.dCurrentSolutionTime;

            dMult = (double)g_Options.uMyIncrement / 
                (double)SUITE_NUM_HISTOGRAM;
            ASSERT(SUITE_NUM_HISTOGRAM > 0);
            for (y = 1; y <= SUITE_NUM_HISTOGRAM; y++)
            {
                if (g_SuiteCounters.dCurrentSolutionTime < ((double)y * dMult))
                {
                    g_SuiteCounters.uHistogram[y-1]++;
                    break;
                }
                if (y == SUITE_NUM_HISTOGRAM)
                {
                    //
                    // This one is actually > time-to-think.  This
                    // really happens... like we had 20 sec to think
                    // and solved it in 20.1 or something before the
                    // stop-searching timer was checked.
                    //
                    g_SuiteCounters.uHistogram[y-1]++;
                }
            }
        }
        else
        {
            Trace(">>>>> Problem %s was not solved. <<<<<\n\n",
                  g_SuiteCounters.szId);
            g_SuiteCounters.uIncorrect++;
        }
        Trace("Current suite score: %u correct of %u problems.\n", 
              g_SuiteCounters.uCorrect,
              g_SuiteCounters.uTotal);
        _ClearSuiteData();
    }
}
