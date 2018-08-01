/**

Copyright (c) Scott Gasch

Module Name:

    main.c

Abstract:

    Program entry point and setup code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 8 Apr 2004

Revision History:

    $Id: main.c 355 2008-07-01 15:46:43Z scott $

**/

#include "chess.h"

FILE *g_pfLogfile = NULL;
GAME_OPTIONS g_Options;
CHAR g_szInitialCommand[SMALL_STRING_LEN_CHAR];
ULONG g_uInputThreadHandle = (ULONG)-1;

void
Banner(void)
/**

Routine description:

    Describe the compile/build options and program version.

Parameters:

    void

Return value:

    void

**/
{
    char *p;

    Trace("Typhoon %s (built on %s %s):\n", VERSION, __DATE__, __TIME__);
    Trace("    Copyright (C) 2000-2007, Scott Gasch (scott.gasch@gmail.com)\n");
    Trace("    " REVISION);
    Trace("    " COMPILER_STRING);
    Trace("    Make profile used: %s\n", PROFILE);
#ifdef MP
    Trace("    Multiprocessor enabled; %u searcher thread%s\n",
          g_Options.uNumProcessors, (g_Options.uNumProcessors > 1) ? "s" : "");
#else
    Trace("    Single processor version\n");
#endif
#ifdef DEBUG
    Trace("    DEBUG build\n");
#endif
#ifdef TEST
    Trace("    Testcode compiled in\n");
#endif
#ifdef EVAL_DUMP
    Trace("    Evaluation terms\n");
#endif
#ifdef EVAL_TIME
    Trace("    Evaluation timing\n");
#endif
#ifdef PERF_COUNTERS
    Trace("    Performance counters\n");
#ifdef TEST_NULL
    Trace("    Testing nullmove prediction algorithms\n");
#endif
#endif
#ifdef DUMP_TREE
    Trace("    Search tree dumpfile generation enabled\n");
#endif
    Trace("    Hash sizes: %u Mb (main), %u Mb / thread (pawn), %u Mb / thread (eval)\n", 
          (g_uHashTableSizeEntries * sizeof(HASH_ENTRY)) / MB,
          PAWN_HASH_TABLE_SIZE * sizeof(PAWN_HASH_ENTRY) / MB,
          EVAL_HASH_TABLE_SIZE * sizeof(EVAL_HASH_ENTRY) / MB);
    Trace("    QCheckPlies: %u\n", QPLIES_OF_NON_CAPTURE_CHECKS);
    Trace("    FutilityBase: %u\n", FUTILITY_BASE_MARGIN);
    p = ExportEvalDNA();
    Trace("    Logging Eval DNA.\n");
    Log(p);
    free(p);
#ifdef DO_IID
    Trace("    IID R-factor: %u ply\n", IID_R_FACTOR / ONE_PLY);
#endif
}


void
EndLogging(void)
/**

Routine description:

    Close the main logfile.

Parameters:

    void

Return value:

    void

**/
{
    if (NULL != g_pfLogfile) {
        fflush(g_pfLogfile);
        fflush(stdout);
        fclose(g_pfLogfile);
        g_pfLogfile = NULL;
    }
}


FLAG
BeginLogging(void)
/**

Routine description:

    Open the main logfile.

Parameters:

    void

Return value:

    FLAG

**/
{
    if (NULL != g_pfLogfile) {
        EndLogging();
    }
    if (!strlen(g_Options.szLogfile) ||
        !strcmp(g_Options.szLogfile, "-")) {
        return(FALSE);
    }

    // If the logfile we want to write already exists, back it up.
    if (TRUE == SystemDoesFileExist(g_Options.szLogfile)) {
        VERIFY(BackupFile(g_Options.szLogfile));
    }
    ASSERT(FALSE == SystemDoesFileExist(g_Options.szLogfile));

    g_pfLogfile = fopen(g_Options.szLogfile, "wb+");
    if (NULL == g_pfLogfile) {
        UtilPanic(INITIALIZATION_FAILURE,
                  (void *)"Can't open logfile", 
                  (void *)&(g_Options.szLogfile), 
                  (void *)"write", 
                  NULL,
                  __FILE__, __LINE__);
    }
    return(TRUE);
}


FLAG
PreGameReset(FLAG fResetBoard)
/**

Routine description:

    Reset internal state between games.

Parameters:

    void

Return value:

    FLAG

**/
{
    if (TRUE == fResetBoard)
    {
        SetRootToInitialPosition();
    }
    SetMyName();
    ClearDynamicMoveOrdering();
    ClearHashTable();
    ResetOpeningBook();
    return(TRUE);
}


static ULONG
_ParseHashOption(char *p,
                 ULONG uEntrySize)
{
    ULONG u, v;
    CHAR c;

    if ((!STRCMPI(p, "none")) || (!STRCMPI(p, "no"))) return(0);
    if (2 == sscanf(p, "%u%1c", &u, &c))
    {
        v = u;
        switch(c)
        {
            case 'K':
            case 'k':
                u *= 1024;
                break;
            case 'M':
            case 'm':
                u *= 1024 * 1024;
                break;
            case 'G':
            case 'g':
                u *= 1024 * 1024 * 1024;
                break;
            default:
                Trace("Error (unrecognized size): \"%s\"\n", p);
                break;
        }
        if (u < v)
        {
            Trace("Error (size too large): \"%s\"\n", p);
            u = v;
        }
        u /= uEntrySize;
    }
    else
    {
        u = (ULONG)atoi(p);
    }

    while(!IS_A_POWER_OF_2(u))
    {
        u &= (u - 1);
    }
    return(u);
}



void
InitializeOptions(int argc, char *argv[])
/**

Routine description:

    Set global options to their initial state.  This code also is
    responsible for parsing the commandline in order to override the
    default state of global options.

Parameters:

    int argc,
    char *argv[]

Return value:

    void

**/
{
    int i;

    //
    // Defaults
    //
    memset(&g_Options, 0, sizeof(g_Options));
    g_szInitialCommand[0] = '\0';
    g_Options.uMyClock = g_Options.uOpponentsClock = 600;
    g_Options.fGameIsRated = FALSE;
    g_Options.fOpponentIsComputer = FALSE;
    g_Options.uSecPerMove = 0;
    g_Options.uMaxDepth = MAX_PLY_PER_SEARCH - 1;
    g_Options.fShouldPonder = TRUE;
    g_Options.fPondering = g_Options.fThinking = FALSE;
    g_Options.mvPonder.uMove = 0;
    g_Options.fShouldPost = TRUE;
    g_Options.fForceDrawWorthZero = FALSE;
    g_Options.uMyIncrement = 0;
    g_Options.uMovesPerTimePeriod = 0;
    g_Options.szAnalyzeProgressReport[0] = '\0';
    g_Options.fShouldAnnounceOpening = TRUE;
    g_Options.eClock = CLOCK_NONE;
    g_Options.eGameType = GAME_UNKNOWN;
    g_Options.ePlayMode = FORCE_MODE;
    g_Options.fNoInputThread = FALSE;
    g_Options.fVerbosePosting = TRUE;
    strcpy(g_Options.szEGTBPath, "/egtb/three;/etc/four;/egtb/five");
    strcpy(g_Options.szLogfile, "typhoon.log");
    strcpy(g_Options.szBookName, "book.bin");
    g_Options.uNumHashTableEntries = 0x10000;
    g_Options.uNumProcessors = 1;
    g_Options.fStatusLine = TRUE;
    g_Options.iResignThreshold = 0;
    g_Options.u64MaxNodeCount = 0ULL;

    i = 1;
    while(i < argc)
    {
#ifdef MP
        if ((!STRCMPI(argv[i], "--cpus")) && (argc > i))
        {
            g_Options.uNumProcessors = (ULONG)atoi(argv[i+1]);
            if ((g_Options.uNumProcessors == 0) ||
                (g_Options.uNumProcessors > 64))
            {
                g_Options.uNumProcessors = 2;
            }
            i++;
        }
        else
#endif
        if ((!STRCMPI(argv[i], "--command")) && (argc > i))
        {
            strncpy(g_szInitialCommand,
                    argv[i+1],
                    SMALL_STRING_LEN_CHAR - 2);
            strcat(g_szInitialCommand, "\r\n");
            i++;
        }
        else if ((!STRCMPI(argv[i], "--hash")) && (argc > i))
        {
            g_Options.uNumHashTableEntries =
                _ParseHashOption(argv[i+1],
                                 sizeof(HASH_ENTRY));
            i++;
        }
        else if ((!STRCMPI(argv[i], "--egtbpath")) && (argc > i))
        {
            if (!strcmp(argv[i+1], "-")) {
                g_Options.szEGTBPath[0] = '\0';
            } else {
                strncpy(g_Options.szEGTBPath, argv[i+1], SMALL_STRING_LEN_CHAR);
            }
            i++;
        }
        else if (!STRCMPI(argv[i], "--book") && argc > i)
        {
            if (!strcmp(argv[i+1], "-")) {
                g_Options.szBookName[0] = '\0';
            } else {
                strncpy(g_Options.szBookName, argv[i+1], SMALL_STRING_LEN_CHAR);
            }
            i++;
        }
        else if (!STRCMPI(argv[i], "--batch"))
        {
            g_Options.fNoInputThread = TRUE;
        }
        else if ((!STRCMPI(argv[i], "--dnafile")) && (argc > i)) 
        {
            if (!ReadEvalDNA(argv[i+1])) 
            {
                UtilPanic(INITIALIZATION_FAILURE,
                          (void *)"Bad DNA file",
                          (void *)&(argv[i+1]),
                          (void *)"read",
                          NULL,
                          __FILE__, __LINE__);
            }
            i++;
        }
        else if ((!STRCMPI(argv[i], "--logfile")) && (argc > i)) 
        {
            strncpy(g_Options.szLogfile, argv[i+1],
                    SMALL_STRING_LEN_CHAR);
            i++;
        }
        else if (!STRCMPI(argv[i], "--help")) {
            Trace("Usage: %s [--batch] [--command arg] [--logfile arg] [--egtbpath arg]\n"
                  "                [--dnafile arg] [--cpus arg] [--hash arg]\n\n"
                  "    --batch    : operate the engine without an input thread\n"
                  "    --book     : specify the opening book to use or '-' for none\n"
                  "    --command  : specify initial command(s) (requires arg)\n"
                  "    --cpus     : indicate the number of cpus to use (1..64)\n"
                  "    --hash     : indicate desired hash size (e.g. 16m, 1g)\n"
                  "    --egtbpath : supplies the egtb path or '-' for none\n"
                  "    --logfile  : indicate desired output logfile name or '-' for none\n"
                  "    --dnafile  : indicate desired eval profile input (requres arg)\n\n"
                  "Example: %s --hash 256m --cpus 2\n"
                  "         %s --logfile test.out --batch --command \"test\" --dnafile test.in\n", argv[0], argv[0], argv[0]);
            exit(-1);
        }
        else
        {
            Trace("Error (unknown argument): \"%s\"; try --help\n", argv[i]);
        }

        //
        // TODO: parse other commandline options and override above
        //

        i++;
    }
}


void
CleanupOptions(void)
/**

Routine description:

    Cleanup options...

Parameters:

    void

Return value:

    void

**/
{
    NOTHING;
}


FLAG
MainProgramInitialization(int argc, char *argv[])
/**

Routine description:

    Perform main program initialization.  This includes stuff like:
    precomputing some data structures, resetting the global options,
    parsing the commandline to override said options, and calling out
    to other modules to tell them to initialize themselves.

Parameters:

    int argc,
    char *argv[]

Return value:

    FLAG

**/
{
    srand((unsigned int)time(0));
    InitializeOptions(argc, argv);
    InitializeTreeDump();
    InitializeEGTB();
    InitializeSigSystem();
    InitializeInteriorNodeRecognizers();
    InitializeSearchDepthArray();
    InitializeWhiteSquaresTable();
    InitializeVectorDeltaTable();
    InitializeSwapTable();
    InitializeDistanceTable();
    InitializeOpeningBook();
    InitializeDynamicMoveOrdering();
    InitializeHashSystem();
    InitializePositionHashSystem();
#ifdef MP
    InitializeParallelSearch();
#endif
    VERIFY(PreGameReset(TRUE));
    (void)BeginLogging();
    return TRUE;
}


FLAG
MainProgramCleanup(void)
/**

Routine description:

    Perform main program cleanup.  Tear down the stuff we set up in
    MainProgramInitialization.

Parameters:

    void

Return value:

    FLAG

**/
{
    CleanupOpeningBook();
    CleanupEGTB();
    CleanupDynamicMoveOrdering();
#ifdef MP
    CleanupParallelSearch();
#endif
    CleanupPositionHashSystem();
    CleanupHashSystem();
    CleanupTreeDump();
    CleanupOptions();
    EndLogging();
    return(TRUE);
}


#ifdef TEST
FLAG
RunStartupProgramTest(void)
/**

Routine description:

    If we built this version with -DTEST then this code is the start
    of the self-test that runs at program startup time.

Parameters:

    void

Return value:

    FLAG

**/
{
    ULONG u, x, y;

#ifdef _X86_
    Trace("Testing CPP macros...\n");
    for (u = 0; u < 1000000; u++)
    {
        x = rand() * rand();
        y = rand() * rand();
        x = x & ~(0x80000000);
        y = y & ~(0x80000000);

        if (MAX(x, y) != MAXU(x, y))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "max maxu", NULL, NULL, __FILE__, __LINE__);
        }

        if (MAXU(x, y) != MAXU(y, x))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "maxu assoc", NULL, NULL, __FILE__, __LINE__);
        }

        if (MIN(x, y) != MINU(x, y))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "min minu", NULL, NULL, __FILE__, __LINE__);
        }

        if (MINU(x, y) != MINU(y, x))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "minu assoc", NULL, NULL, __FILE__, __LINE__);
        }

        if (abs(x - y) != ABS_DIFF(x, y))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "abs_diff", NULL, NULL, __FILE__, __LINE__);
        }

        if (ABS_DIFF(x, y) != ABS_DIFF(y, x))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "abs_diff assoc", NULL, NULL, __FILE__, __LINE__);
        }
    }
#endif

    TestDraw();
#ifdef EVAL_DUMP
    TestEval();
#endif
    TestBitboards();
    TestSan();
    TestIcs();
    TestGetAttacks();
    TestMoveGenerator();
    TestLegalMoveGenerator();
    TestFenCode();
    TestLiftPlaceSlidePiece();
    TestExposesCheck();
    TestIsAttacked();
    TestMakeUnmakeMove();
    TestSearch();

    return(TRUE);
}
#endif // TEST



void DeclareTerminalStates(GAME_RESULT result) {
    switch(result.eResult) {
    case RESULT_IN_PROGRESS:
        return;
    case RESULT_BLACK_WON:
        Trace("0-1 {%s}\n", result.szDescription);
        SetGameResultAndDescription(result);
        DumpPgn();
        g_Options.ePlayMode = FORCE_MODE;
        break;
    case RESULT_WHITE_WON:
        Trace("1-0 {%s}\n", result.szDescription);
        SetGameResultAndDescription(result);
        DumpPgn();
        g_Options.ePlayMode = FORCE_MODE;
        break;
    case RESULT_DRAW:
        Trace("1/2-1/2 {%s}\n", result.szDescription);
        Trace("tellics draw\n");
        SetGameResultAndDescription(result);
        DumpPgn();
        g_Options.ePlayMode = FORCE_MODE;
        break;
    case RESULT_ABANDONED:
    case RESULT_UNKNOWN:
        ASSERT(FALSE);
        SetGameResultAndDescription(result);
        DumpPgn();
        g_Options.ePlayMode = FORCE_MODE;
        break;
    }
}


// Main work loop
void MainProgramLoop() {
    POSITION pos;
    GAME_RESULT result;
    FLAG fThink;
    FLAG fPonder;

    while (!g_fExitProgram) {
        
        // Prepare to search or ponder
        memcpy(&pos, GetRootPosition(), sizeof(POSITION));
        fThink = fPonder = FALSE;
        result.eResult = RESULT_IN_PROGRESS;
        result.szDescription[0] = '\0';
        
        // What should we do?
        switch(g_Options.ePlayMode) {
        case I_PLAY_WHITE:
            fThink = (pos.uToMove == WHITE);
            fPonder = (pos.uToMove == BLACK);
            break;
        case I_PLAY_BLACK:
            fThink = (pos.uToMove == BLACK);
            fPonder = (pos.uToMove == WHITE);
            break;
        case FORCE_MODE:
        default:
            fThink = fPonder = FALSE;
            break;
        }

        if (fThink) {
            result = Think(&pos);
            if ((NumberOfPendingInputEvents() != 0) &&
                (FALSE == g_fExitProgram))
            {
                MakeStatusLine();
                ParseUserInput(FALSE);
            }
            DeclareTerminalStates(result);
        } else if (fPonder) {
            if (g_Options.fShouldPonder) {
                result = Ponder(&pos);
            }
            if (FALSE == g_fExitProgram)
            {
                MakeStatusLine();
                ParseUserInput(FALSE);
            }
            DeclareTerminalStates(result);
        } else {
            MakeStatusLine();
            ParseUserInput(FALSE);
        }
    }
}


int CDECL
main(int argc, char *argv[])
/**

Routine description:

    Chess engine entry point and main loop.

Parameters:

    int argc,
    char *argv[]

Return value:

    int CDECL

**/
{
    // Initial setup work...
    Trace("Setting up, please wait...\n");
    if (FALSE == SystemDependentInitialization())
    {
        Trace("main: Operating system dependent init code failed, "
              "aborting.\n");
        exit(-1);
    }
    if (FALSE == MainProgramInitialization(argc, argv))
    {
        Trace("main: Main program init code failed, aborting.\n");
        exit(-1);
    }
    Banner();
#ifdef TEST
    (void)RunStartupProgramTest();
#endif
    if (TRUE == g_Options.fNoInputThread)
    {
        InitInputSystemInBatchMode();
    }
    else
    {
        g_uInputThreadHandle = InitInputSystemWithDedicatedThread();
    }
    
    // Enter the main work loop code...
    MainProgramLoop();  
    
    // If we get here g_fExitProgram is set -- clean up.
    Trace("MAIN THREAD: thread terminating.\n");
    if (FALSE == MainProgramCleanup())
    {
        Trace("main: Main program cleanup code failed, aborting.\n");
        exit(-1);
    }
    exit(0); 
}
