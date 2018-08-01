/**

Copyright (c) Scott Gasch

Module Name:

    command.c

Abstract:

    Process commands received by the input thread.  Note: the COMMAND
    macro is defined in chess.h and used heavily in this module to
    build function declarations.  It hides four parameters:

        CHAR *szInput : the full line of input
        ULONG argc    : number of argument chunks
        CHAR *argv[]  : array of ptrs to each argument chunk
        POSITION *pos : a POSITION pointer to operate on

Author:

    Scott Gasch (scott.gasch@gmail.com) 13 May 2004

Revision History:

    $Id: command.c 354 2008-06-30 05:10:08Z scott $

**/

#include "chess.h"

#define MAX_ARGS 32
int g_eModeBeforeAnalyze = -1;

typedef void (COMMAND_PARSER_FUNCTION)(CHAR *szInput,
                                       ULONG argc, 
                                       CHAR *argv[], 
                                       POSITION *pos);
typedef struct _COMMAND_PARSER_ENTRY
{
    CHAR *szOpcode;
    COMMAND_PARSER_FUNCTION *pFunction;
    FLAG fHandleOnTheFly;
    FLAG fInvisible;
    FLAG fWinBoard;
    CHAR *szSimpleHelp;
}
COMMAND_PARSER_ENTRY;


COMMAND(AnalyzeCommand)
/**

Routine description:

    This function implements the 'analyze' engine command.  

    Usage:

        analyze

        This command takes no arguments and causes the engine to enter
        analyze mode.  See also: exit
    
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
    if (g_Options.ePlayMode != ANALYZE_MODE)
    {
        g_eModeBeforeAnalyze = g_Options.ePlayMode;
        g_Options.ePlayMode = ANALYZE_MODE;
    }
}    

COMMAND(AnalyzeProgressCommand)
/**

Routine description:

    This function implements the 'analyzeprogressreport' engine
    command.

    Usage:

        analyzeprogressreport

        This command takes no arguments and causes the engine to dump
        the current analysis.
    
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
    if (g_Options.ePlayMode == ANALYZE_MODE)
    {
        Trace("%s\n", g_Options.szAnalyzeProgressReport);
    }
}

COMMAND(BlackCommand)
/**

Routine description:

    This function implements the 'black' engine command.

    Usage:

        black

        This command takes no arguments and causes the engine to play
        white and set black to move.
    
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
    TellGamelistThatIPlayColor(WHITE);
    g_Options.ePlayMode = I_PLAY_WHITE;
    pos->uToMove = BLACK;
    pos->u64NonPawnSig = ComputeSig(pos);
}


COMMAND(BoardCommand)
/**

Routine description:

    This function implements the 'board' engine command.  

    Usage:

        board

        This command takes no arguments and displays the current board
        position on stdout and in the log.
    
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
    DumpPosition(pos);
}

COMMAND(ComputerCommand)
/**

Routine description:

    This function implements the 'computer' engine command.  

    Usage:

        computer

        This command takes no argument and informs the engine that it
        is playing against another computer.
    
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
    g_Options.fOpponentIsComputer = TRUE;
    Trace("tellics +alias whisper kib\n");
}

COMMAND(DrawCommand)
{
    //if (g_iNumDrawScoresInARow > 4)
    //{
    //    ToXboard("tellopponent I accept your draw offer.\n");
    //    ToXboard("tellics draw\n");
    //}
    //else
    //{
    //Trace("tellopponent Sorry I do not accept your draw offer at "
    //      "this time.\n");
    //Trace("tellics decline draw\n");
    //}
}


//
// TODO: timestamp command
//

void 
DumpSizes(void)
/**

Routine description:

    Dump the sizes of some internal data structures on stdout.

Parameters:

    void

Return value:

    void

**/
{
    Trace("sizeof(PAWN_HASH_ENTRY). . . . . . . . . %u bytes\n"
          "sizeof(HASH_ENTRY) . . . . . . . . . . . %u bytes\n"
          "sizeof(MOVE) . . . . . . . . . . . . . . %u bytes\n"
          "sizeof(ATTACK_BITV). . . . . . . . . . . %u bytes\n"
          "sizeof(SQUARE) . . . . . . . . . . . . . %u bytes\n"
          "sizeof(POSITION) . . . . . . . . . . . . %u bytes\n"
          "sizeof(MOVE_STACK) . . . . . . . . . . . %u bytes\n"
          "sizeof(PLY_INFO) . . . . . . . . . . . . %u bytes\n"
          "sizeof(COUNTERS) . . . . . . . . . . . . %u bytes\n"
          "sizeof(SEARCHER_THREAD_CONTEXT). . . . . %u bytes\n"
          "sizeof(GAME_OPTIONS) . . . . . . . . . . %u bytes\n"
          "sizeof(MOVE_TIMER) . . . . . . . . . . . %u bytes\n"
          "sizeof(PIECE_DATA) . . . . . . . . . . . %u bytes\n"
          "sizeof(VECTOR_DELTA) . . . . . . . . . . %u bytes\n"
          "sizeof(GAME_PLAYER). . . . . . . . . . . %u bytes\n"
          "sizeof(GAME_HEADER). . . . . . . . . . . %u bytes\n"
          "sizeof(GAME_MOVE). . . . . . . . . . . . %u bytes\n"
          "sizeof(GAME_DATA). . . . . . . . . . . . %u bytes\n"
          "sizeof(SEE_LIST) . . . . . . . . . . . . %u bytes\n"
          "sizeof(BOOK_ENTRY) . . . . . . . . . . . %u bytes\n"
          "-------------------------------------------------\n"
          "Current main hash table size . . . . . . %u bytes (~%u Mb)\n",
          sizeof(PAWN_HASH_ENTRY), sizeof(HASH_ENTRY), sizeof(MOVE),
          sizeof(ATTACK_BITV), sizeof(SQUARE), sizeof(POSITION),
          sizeof(MOVE_STACK), sizeof(PLY_INFO), sizeof(COUNTERS),
          sizeof(SEARCHER_THREAD_CONTEXT), sizeof(GAME_OPTIONS),
          sizeof(MOVE_TIMER), sizeof(PIECE_DATA), sizeof(VECTOR_DELTA),
          sizeof(GAME_PLAYER), sizeof(GAME_HEADER), sizeof(GAME_MOVE),
          sizeof(GAME_DATA), sizeof(SEE_LIST), sizeof(BOOK_ENTRY),
          (g_uHashTableSizeEntries * sizeof(HASH_ENTRY)),
          ((g_uHashTableSizeEntries * sizeof(HASH_ENTRY)) / MB));
    ASSERT_ASM_ASSUMPTIONS;
}


COMMAND(DumpCommand)
/**

Routine description:

    This function implements the 'dump' engine command.  

    Usage:

        dump board
        dump moves
        dump pgn
        dump sizes
        dump eval [optional coor]
        dump evaldna

        This command is used to dump internal information to the
        stdout.  

        When used with the 'board' argument, a dump of the internal
        representation of the current board position is produced.

        When used with the 'moves' argument, a list of the moves played
        thus far in the current game is produced in list format.

        When used with the 'pgn' argument, a list of the moves played 
        thus far in the current game is produced in PGN format.

        When used with the 'sizes' argument, a dump of internal data
        structure sizes is produced.

        When used with the 'eval' argument and no optional coordinate
        a dump of all eval terms from the last time the eval function
        was invoked is produced.  If the optional coordinate is
        provided then the eval terms DIRECTLY affected by the piece at
        the supplied coordinate is produced.  Note: to use dump eval
        the engine must have been built with -DEVAL_DUMP.
    
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
#ifdef EVAL_DUMP
    COOR c;
#endif

    if (argc < 2)
    {
        Trace("Error (missing argument)\n");
        return;
    }
    if (!STRCMPI(argv[1], "board"))
    {
        DumpPosition(pos);
    }
    else if (!STRCMPI(argv[1], "moves"))
    {
        DumpGameList();
    }
    else if (!STRCMPI(argv[1], "pgn"))
    {
        DumpPgn();
    }
    else if (!STRCMPI(argv[1], "sizes"))
    {
        DumpSizes();
    }
    else if (!STRCMPI(argv[1], "evaldna")) 
    {
        char *p = ExportEvalDNA();
        Trace("EvalDNA: %s\n", p);
        free(p);
    }
#ifdef EVAL_DUMP
    else if (!STRCMPI(argv[1], "eval"))
    {
        if (argc < 3)
        {
            EvalTraceReport();
        }
        else
        {
            if (!STRCMPI(argv[2], "terms"))
            {
                EvalTraceReport();
            } else {
                if (LooksLikeCoor(argv[2]))
                {
                    c = FILE_RANK_TO_COOR((tolower(argv[2][0]) - 'a'),
                                          (tolower(argv[2][1]) - '0'));
                    (void)EvalSigmaForPiece(pos, c);
                } else {
                    (void)EvalSigmaForPiece(pos, ILLEGAL_COOR);
                }
            }
        }
    }
#endif
}

COMMAND(EasyCommand)
/**

Routine description:

    This function implements the 'easy' engine command.  

    Usage:

        easy

        This command takes no arguments and causes the engine not to
        think on the opponent's time (i.e. not to ponder)
    
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
    g_Options.fShouldPonder = FALSE;
}

COMMAND(EditCommand)
/**

Routine description:

    This function implements the 'edit' engine command.  

    Usage:

        This command is deprecated and should not be used.
    
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
    Trace("Error (obsolete command): %s\n", argv[0]);
    Trace("\nPlease upgrade your version of xboard at "
          "http://www.timmann.org/\n");
}

COMMAND(EvalCommand)
/**

Routine description:

    This function implements the 'eval' engine command.  

    Usage:

        eval

        This command takes no arguments and causes the engine to print
        out a static evaluation of the current board position.
    
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
    SEARCHER_THREAD_CONTEXT *ctx;
    SCORE i;

    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    if (NULL != ctx) 
    {
        InitializeSearcherContext(pos, ctx);
        i = Eval(ctx, -INFINITY, +INFINITY);
        Trace("Static eval: %s\n", ScoreToString(i));
        SystemFreeMemory(ctx);
    } else {
        Trace("Out of memory.\n");
    }
}


COMMAND(EvalDnaCommand)
/**

Routine description:

    This function implements the 'evaldna' engine command.  

    Usage:

        evaldna write <filename>
        evaldna read <filename>
        evaldna dump

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
    char *p;
    if (argc < 2)
    {
        Trace("Usage: evaldna <read | write> <filename>\n"
              "       evaldna dump\n");
        return;
    }
    if (!STRCMPI(argv[1], "write"))
    {
        if (argc < 3) {
            Trace("Error (missing argument)\n");
            return;
        }
        if (!WriteEvalDNA(argv[2])) 
        {
            Trace("Error writing dna file.\n");
        } else {
            Trace("DNA file written.\n");
        }
    }
    else if (!STRCMPI(argv[1], "read") ||
             !STRCMPI(argv[1], "load"))
    {
        if (argc < 3) {
            Trace("Error (missing argument)\n");
            return;
        }
        if (!ReadEvalDNA(argv[2])) 
        {
            Trace("Error reading dna file.\n");
        } else {
            Trace("Loaded dna file \"%s\"\n", argv[2]);
            p = ExportEvalDNA();
            Log("(New) dna: %s\n", p);
            free(p);
        }
    }
    else if (!STRCMPI(argv[1], "dump"))
    {
        p = ExportEvalDNA();
        Trace("EvalDNA: %s\n", p);
        free(p);
    }
}


COMMAND(ExitCommand)
/**

Routine description:

    This function implements the 'exit' engine command.  

    Usage:

        exit

        This command takes no arguments and causes the engine to leave
        analyze mode.  The engine reverts to the mode it was in prior
        to entering analyze mode.  See also: analyze
    
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
    if (g_Options.ePlayMode == ANALYZE_MODE)
    {
        ASSERT(g_eModeBeforeAnalyze != ANALYZE_MODE);
        g_Options.ePlayMode = g_eModeBeforeAnalyze;
    }
}

COMMAND(ForceCommand)
/**

Routine description:

    This function implements the 'force' engine command.  

    Usage:

        force

        This command takes no arguments and causes the engine to enter
        'force mode'.  In this mode it will play neither side.
    
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
    TellGamelistThatIPlayColor(BLACK);
    g_Options.ePlayMode = FORCE_MODE;
}

COMMAND(GenerateCommand)
/**

Routine description:

    This function is not directly callable.  See 'test generate'
    instead.
    
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
    SEARCHER_THREAD_CONTEXT *ctx;
    MOVE mv;
    ULONG u, uLegal = 0;

    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    if (NULL != ctx)
    {
        memcpy(&(ctx->sPosition), pos, sizeof(POSITION));
        mv.uMove = 0;
        GenerateMoves(ctx, mv, (InCheck(pos, pos->uToMove) ? GENERATE_ESCAPES :
                                                             GENERATE_ALL_MOVES));
        ASSERT(ctx->uPly == 0);
        for (u = ctx->sMoveStack.uBegin[0];
             u < ctx->sMoveStack.uEnd[0];
             u++)
        {
            mv = ctx->sMoveStack.mvf[u].mv;
            if (MakeMove(ctx, mv))
            {
                ASSERT(!InCheck(&(ctx->sPosition), GET_COLOR(mv.pMoved)));
                uLegal++;
                UnmakeMove(ctx, mv);
                Trace("%2u. %s (%d)\t", 
                      uLegal, 
                      MoveToSan(mv, &(ctx->sPosition)),
                      ctx->sMoveStack.mvf[u].iValue);
                if (uLegal % 3 == 0)
                {
                    Trace("\n");
                }
            }
        }
        Trace("\n%u legal move(s).\n", uLegal);
        SystemFreeMemory(ctx);
    }
}

COMMAND(PlayOtherCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    if (pos->uToMove == WHITE)
    {
        TellGamelistThatIPlayColor(BLACK);
        g_Options.ePlayMode = I_PLAY_BLACK;
    } else {
        ASSERT(pos->uToMove == BLACK);
        TellGamelistThatIPlayColor(WHITE);
        g_Options.ePlayMode = I_PLAY_WHITE;
    }
}


COMMAND(GoCommand)
/**

Routine description:

    This function implements the 'go' engine command.  

    Usage:
    
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
    if (pos->uToMove == WHITE)
    {
        TellGamelistThatIPlayColor(WHITE);
        g_Options.ePlayMode = I_PLAY_WHITE;
    } else {
        ASSERT(pos->uToMove == BLACK);
        TellGamelistThatIPlayColor(BLACK);
        g_Options.ePlayMode = I_PLAY_BLACK;
    }
}

COMMAND(HardCommand)
{
    g_Options.fShouldPonder = TRUE;
}

COMMAND(HelpCommand);

COMMAND(HintCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    Trace("Error (not implemented): %s\n", argv[0]);
}

CHAR *OnlyDigits(CHAR *sz)
{
    static CHAR out[SMALL_STRING_LEN_CHAR];
    CHAR *p = out;
    ULONG uCount = 0;
    
    while((*sz) && (uCount < SMALL_STRING_LEN_CHAR))
    {
        if (isdigit(*sz))
        {
            uCount++;
            *p++ = *sz++;
        }
    }
    *p++ = '\0';
    return(out);
}

COMMAND(LevelCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    ULONG x;

    if (argc < 4)
    {
        Trace("Error (syntax): %s\n", argv[0]);
    } else {
        g_Options.uMovesPerTimePeriod = atoi(OnlyDigits(argv[1]));
        g_Options.uMyClock = atoi(OnlyDigits(argv[2]));
        g_Options.uMyIncrement = atoi(OnlyDigits(argv[3]));

        g_Options.eClock = CLOCK_NORMAL;
        if (g_Options.uMyIncrement)
        {
            g_Options.eClock = CLOCK_INCREMENT;
        }
        
        x = (g_Options.uMyClock + ((g_Options.uMyIncrement * 2) / 3));
        g_Options.uMyClock *= 60;
        
        Trace("etime = %u (clock %u and inc %u)\n", x,
              g_Options.uMyClock, g_Options.uMyIncrement);
        if (x <= 6)
        {
            Trace("bullet game\n");
            g_Options.eGameType = GAME_BULLET;
        }
        else if (x < 15)
        {
            Trace("blitz game\n");
            g_Options.eGameType = GAME_BLITZ;
        } else {
            Trace("standard game\n");
            g_Options.eGameType = GAME_STANDARD;
        }
    }
}

COMMAND(MoveNowCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    Trace("MOVE NOW COMMAND --> stop searching now\n");
    g_MoveTimer.bvFlags |= TIMER_STOPPING;
}

COMMAND(MovesCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    DumpGameList();
}

COMMAND(XboardCommand);
COMMAND(NameCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    if (argc < 2)
    {
        XboardCommand(szInput, argc, argv, pos);
    } else {
        SetOpponentsName(argv[1]);
    }
}

COMMAND(NewCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    Trace("tellics -alias whisp*\n");
    PreGameReset(TRUE);
    TellGamelistThatIPlayColor(BLACK);
    g_Options.ePlayMode = I_PLAY_BLACK;
    g_Options.uMaxDepth = MAX_PLY_PER_SEARCH - 1;
}

COMMAND(NoPostCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    g_Options.fShouldPost = FALSE;
}

COMMAND(OtimCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    ULONG x;
    
    if (argc < 2)
    {
        Trace("My opponent's clock stands at %u sec remaining.\n",
              g_Options.uOpponentsClock);
        
        switch (g_Options.eClock)
        {
            case CLOCK_FIXED:
                Trace("The move timer is fixed, %u sec / move.\n",
                      g_Options.uMyIncrement);
                break;
            case CLOCK_NORMAL:
                if (g_Options.uMovesPerTimePeriod != 0)
                {
                    Trace("The move timer is normal, %u moves / time "
                          "period.\n",
                          g_Options.uMovesPerTimePeriod);
                } else {
                    Trace("The move timer is normal, finish entire "
                          "game in allotted time.\n");
                }
                break;
            case CLOCK_INCREMENT:
                Trace("The move timer is increment, additional %u sec "
                      "/ move.\n",
                      g_Options.uMyIncrement);
                break;
            case CLOCK_NONE:
            default:
                Trace("The clock and move timer are disabled.\n");
                break;
        }
    } else {
        x = atoi(argv[1]);
        if (x >= 0)
        {
            g_Options.uOpponentsClock = x / 100; // convert to seconds
        } else {
            Trace("Error (invalid time): %d\n", x);
        }
    }
}

COMMAND(PingCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    if (argc > 1)
    {
        Trace("pong %s\n", argv[1]);
    }
}

COMMAND(PostCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    g_Options.fShouldPost = TRUE;
}

COMMAND(ProtoverCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    Trace("feature sigint=0 sigterm=0 myname=\"typhoon %s\" ping=1 "
          "setboard=1 playother=1 colors=0 time=1 analyze=1 pause=1 "
          "reuse=1 done=1\n", VERSION);
}

COMMAND(QuitCommand)
/**

Routine description:

    This function implements the 'quit' engine command.  

    Usage: quit
    
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
#ifdef MP
    ULONG u = 0;
#endif

    //
    // Stop searching; if we are on an MP box wait until all helper
    // threads return to the idle state and exit.
    //
    g_MoveTimer.bvFlags |= TIMER_STOPPING;
#ifdef MP
    u = 0;
    do
    {
        SystemDeferExecution(100);
        if (++u == 5) break;
    }
    while(g_uNumHelpersAvailable != g_uNumHelperThreads);
#endif

    // 
    // We should be the last thread in the process now.
    //
}

COMMAND(RatedCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    g_Options.fGameIsRated = TRUE;
}

COMMAND(RatingCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    ULONG x;
    
    if (argc < 3)
    {
        Trace("Error (bad usage): %s\n", argv[0]);
    } else {
        x = atoi(argv[1]);
        if (x >= 0) 
        {
            SetMyRating(x);
        }
        x = atoi(argv[2]);
        if (x >= 0) 
        {
            SetOpponentsRating(x);
        }
    }
}

COMMAND(RandomCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    Trace("%u\n", rand());
}

COMMAND(RemoveCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    if (TRUE == OfficiallyTakebackMove())
    {
        if (TRUE == OfficiallyTakebackMove())
        {
            return;
        }
    }
    Trace("Error (unable to takeback move)\n");    
}

COMMAND(ResultCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    GAME_RESULT result;
    result.eResult = RESULT_UNKNOWN;
    result.szDescription[0] = '\0';
    
    if (argc < 2)
    {
        Trace("Argc was %u\n", argc);
        Trace("Usage: result <result> [optional comment]\n");
    } else {
        if (!STRNCMPI("1-0", argv[1], 3))
        {
            result.eResult = RESULT_WHITE_WON;
        }
        else if (!STRNCMPI("0-1", argv[1], 3))
        {
            result.eResult = RESULT_BLACK_WON;
        }
        else if ((!STRNCMPI("1/2", argv[1], 3)) ||
                 (!STRNCMPI("=", argv[1], 1)))
        {
            result.eResult = RESULT_DRAW;
        }
        if (argc < 2) {
          strncpy(result.szDescription, argv[2], 256);
        }
        SetGameResultAndDescription(result);
        if (g_Options.ePlayMode != FORCE_MODE)
        {
            DumpPgn();
            TellGamelistThatIPlayColor(BLACK);
            g_Options.ePlayMode = FORCE_MODE;
        }
    }
}

COMMAND(SearchDepthCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    ULONG x;
    
    if (argc < 2)
    {
        Trace("Current maximum depth limit is %u ply.\n"
              "The largest depth the engine can search to is %u ply.\n",
              g_Options.uMaxDepth,
              MAX_PLY_PER_SEARCH);
    } else {
        x = atoi(argv[1]);
        if (x <= 0)
        {
            Trace("Error (illegal depth): %d\n", x);
        }
        else if (x >= MAX_PLY_PER_SEARCH)
        {
            Trace("Error (my max depth is %u): %u\n", 
                  MAX_PLY_PER_SEARCH - 1, x);
        } else {
            g_Options.uMaxDepth = x;
        }
    }
}

COMMAND(SearchNodesCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    UINT64 x;
    
    if (argc < 2)
    {
        g_Options.u64MaxNodeCount = 0;
        Trace("Search nodecount limit disabled.\n");
    } else {
        x = strtoull(argv[1], NULL, 10);
        if (x >= 0)
        {
            g_Options.u64MaxNodeCount = x;
            Trace("Search node limit set to %llu.\n", x);
        } else {
            g_Options.u64MaxNodeCount = 0;
            Trace("Search nodecount limit disabled.\n");
        }
    }
}


COMMAND(SearchTimeCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    ULONG x;
    
    if (argc < 2)
    {
        Trace("My clock stands at %u sec remaining.\n",
              g_Options.uMyClock);
        switch (g_Options.eClock)
        {
            case CLOCK_FIXED:
                Trace("The move timer is fixed, %u sec / move.\n",
                      g_Options.uMyIncrement);
                break;
            case CLOCK_NORMAL:
                if (g_Options.uMovesPerTimePeriod != 0)
                {
                    Trace("The move timer is normal, %u moves / "
                          "time period.\n",
                          g_Options.uMovesPerTimePeriod);
                } else {
                    Trace("The move timer is normal, finish entire game in "
                          "allotted time.\n");
                }
                break;
            case CLOCK_INCREMENT:
                Trace("The move timer is increment, additional %u sec "
                      "/ move.\n",
                      g_Options.uMyIncrement);
                break;
            case CLOCK_NONE:
            default:
                Trace("The clock and move timer are disabled.\n");
                break;
        }
    } else {
        x = atoi(argv[1]);
        if (x >= 0)
        {
            g_Options.uMyIncrement = x;
            g_Options.eClock = CLOCK_FIXED;
        } else {
            Trace("Error (illegal time per move): %u\n", x);
        }
    }
}

COMMAND(SetBoardCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    char *p;

    if (argc < 2)
    {
        if (NULL != (p = PositionToFen(pos)))
        {
            Trace("Current board FEN is: %s\n", p);
            SystemFreeMemory(p);
        }
    } else {
        p = szInput + (argv[1] - argv[0]);
        if (FALSE == SetRootPosition(p))
        {
            Trace("Error (malformed FEN): %s\n", p);
            return;
        }
        VERIFY(PreGameReset(FALSE));
        g_Options.ePlayMode = FORCE_MODE;
    }
}

COMMAND(SEECommand)
{
    MOVE mv;
    SCORE s;

    if (argc < 3)
    {
        Trace("Usage: test see <required-move>\n");
        return;
    }
    switch(LooksLikeMove(argv[2]))
    {
        case MOVE_SAN:
            mv = ParseMoveSan(argv[2], pos);
            break;
        case MOVE_ICS:
            mv = ParseMoveIcs(argv[2], pos);
            break;
        default:
            mv.uMove = 0;
    }
    if (!mv.uMove)
    {
        Trace("Error (invalid move): '%s'\n", argv[2]);
        return;
    }
    s = SEE(pos, mv);
    Trace("SEE says that the value of move '%s' is %d\n", argv[2], s);
}


COMMAND(TestCommand)
/**

Routine description:

    This function implements the 'test' engine command.

    Usage:

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
    if (argc < 2)
    {
        Trace("Error (missing argument)\n");
        return;
    }

    if (!STRCMPI(argv[1], "generate"))
    {
        GenerateCommand(szInput, argc, argv, pos);
    }
    else if (!STRCMPI(argv[1], "draw"))
    {
        Trace("drawcount = %u\n",
              CountOccurrancesOfSigInOfficialGameList(pos->u64NonPawnSig ^
                                                      pos->u64PawnSig));
    }
    else if (!STRCMPI(argv[1], "see"))
    {
        SEECommand(szInput, argc, argv, pos);
    }
#ifdef EVAL_DUMP
    else if (!STRCMPI(argv[1], "eval"))
    {
        TestEvalWithSymmetry();
    }
#endif

    //
    // TODO: #ifdef TEST allow individual tests to be run here.
    //
}

COMMAND(TimeCommand)
/**

Routine description:

    This function implements the '' engine command.

    Usage:

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
    ULONG x;
    
    if (argc < 2)
    {
        Trace("My clock stands at %u sec remaining.\n",
              g_Options.uMyClock);
        
        switch (g_Options.eClock)
        {
            case CLOCK_FIXED:
                Trace("The move timer is fixed, %u sec / move.\n",
                      g_Options.uMyIncrement);
                break;
            case CLOCK_NORMAL:
                if (g_Options.uMovesPerTimePeriod != 0)
                {
                    Trace("The move timer is normal, %u moves / time "
                          "period.\n",
                          g_Options.uMovesPerTimePeriod);
                } else {
                    Trace("The move timer is normal, finish entire "
                          "game in allotted time.\n");
                }
                break;
            case CLOCK_INCREMENT:
                Trace("The move timer is increment, additional %u sec "
                      "/ move.\n",
                      g_Options.uMyIncrement);
                break;
            case CLOCK_NONE:
            default:
                Trace("The clock and move timer are disabled.\n");
                break;
        }
    } else {
        x = atoi(argv[1]);
        if (x >= 0)
        {
            g_Options.uMyClock = x / 100; // convert to seconds
        } else {
            Trace("Error (illegal time): %d\n", x);
        }
    }
}

COMMAND(UndoCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    if (FALSE == OfficiallyTakebackMove())
    {
        Trace("Error (can't undo): %s\n", argv[0]);
    }
}

COMMAND(UnratedCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    g_Options.fGameIsRated = FALSE;
}


COMMAND(VariantCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    Trace("Error (not implemented): %s\n", argv[0]);
    Trace("tellics abort\n");
    ASSERT(FALSE);
}

COMMAND(VersionCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    Banner();
}

COMMAND(WhiteCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    TellGamelistThatIPlayColor(BLACK);
    g_Options.ePlayMode = I_PLAY_BLACK;
    pos->uToMove = WHITE;
    pos->u64NonPawnSig = ComputeSig(pos);
}


COMMAND(XboardCommand)
/**

Routine description:

    This function implements the '' engine command.  

    Usage:
    
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
    g_Options.fRunningUnderXboard = TRUE;
    Trace("\n\n");
}

COMMAND(AcceptedCommand) 
{
    ;;
}


// HEREHERE
COMMAND_PARSER_ENTRY g_ParserTable[] = 
{
    { "?",
      MoveNowCommand,
      TRUE,
      FALSE,
      TRUE,
      "Stop thinking and move immediately" },
    { ".",
      AnalyzeProgressCommand,
      TRUE,
      FALSE,
      TRUE,
      "Show engine's current best line when in analyze mode" },
    { "accepted",
      AcceptedCommand,
      TRUE,
      TRUE,
      TRUE,
      "Internal, part of the WinBoard protocol" },
    { "analyze",
      AnalyzeCommand,
      FALSE,
      FALSE,
      TRUE,
      "Enter analyze mode" },
    { "avoid",
      AvoidCommand,
      FALSE,
      FALSE,
      FALSE,
      "Set a move to avoid (up to three)" },
    { "bench",
      BenchCommand,
      FALSE,
      FALSE,
      FALSE,
      "Run a crafty benchmark and show results" },
    { "bestmove",
      GeneratePositionAndBestMoveSuite,
      FALSE,
      TRUE,
      FALSE,
      "Generate a best move suite from a PGN file" },
    { "black",
      BlackCommand,
      FALSE,
      FALSE,
      TRUE,
      "Sets black to move and computer to play white" },
    { "book",
      BookCommand,
      FALSE,
      FALSE,
      FALSE,
      "Manage the opening book" },
    { "computer",
      ComputerCommand,
      TRUE,
      FALSE,
      FALSE,
      "Informs the engine that it is playing another computer" },
    { "dump",
      DumpCommand,
      TRUE,
      FALSE,
      FALSE,
      "Dumps a structure, position, moves, or pgn" },
    { "easy",
      EasyCommand,
      FALSE,
      FALSE,
      TRUE,
      "Disable pondering" },
    { "draw",
      DrawCommand,
      TRUE,
      FALSE,
      FALSE,
      "Offer the engine a draw" },
    { "edit",
      EditCommand,
      FALSE,
      TRUE,
      TRUE,
      "Depreciated command, do not use" },
    { "eval",
      EvalCommand,
      FALSE,
      FALSE,
      FALSE,
      "Run a static eval" },
    { "evaldna",
      EvalDnaCommand,
      FALSE,
      FALSE,
      FALSE,
      "Read or write the DNA from the eval code." },
    { "exit",
      ExitCommand,
      FALSE,
      FALSE,
      TRUE,
      "Leave analyze mode" },
    { "force",
      ForceCommand,
      FALSE,
      FALSE,
      TRUE,
      "Enter force mode, engine plays neither side" },
    { "go",
      GoCommand,
      FALSE,
      FALSE,
      TRUE,
      "Sets engine to play side on move, begin thinking" },
    { "hard",
      HardCommand,
      FALSE,
      FALSE,
      TRUE,
      "Enable pondering" },
    { "help",
      HelpCommand,
      TRUE,
      FALSE,
      FALSE,
      "Show a list of commands available" },
    { "hint",
      HintCommand,
      FALSE,
      FALSE,
      TRUE,
      "Ask the engine for a hint" },
    { "id",
      IdCommand,
      TRUE,
      FALSE,
      FALSE,
      "Set a problem id name" },
    { "level",
      LevelCommand,
      FALSE,
      FALSE,
      TRUE,
      "Sets game time controls" },
    { "name",
      NameCommand,
      TRUE,
      FALSE,
      FALSE,
      "Tell the engine your name" },
    { "new",
      NewCommand,
      FALSE,
      FALSE,
      TRUE,
      "Start a new game" },
    { "nopost",
      NoPostCommand,
      TRUE,
      FALSE,
      TRUE,
      "Do not show thinking" },
    { "otim",
      OtimCommand,
      TRUE,
      FALSE,
      TRUE,
      "Inform the engine about opponent's clock" },
    { "perft",
      PerftCommand,
      FALSE,
      FALSE,
      FALSE,
      "Run a perft test on the move generator code." },
    { "ping",
      PingCommand,
      TRUE,
      TRUE,
      TRUE,
      "Internal part of the WinBoard protocol" },
    { "playother",
      PlayOtherCommand,
      FALSE,
      FALSE,
      TRUE,
      "Play the other side" },
    { "post",
      PostCommand,
      TRUE,
      FALSE,
      TRUE,
      "Show thinking" },    
    { "protover",
      ProtoverCommand,
      TRUE,
      TRUE,
      TRUE,
      "Internal part of the WinBoard protocol" },
    { "psqt",
      LearnPsqtFromPgn,
      FALSE,
      TRUE,
      FALSE,
      "Learn a set of PSQT settings from a PGN file" },
    { "quit",
      QuitCommand,
      TRUE,
      FALSE,
      FALSE,
      "Exit the engine" },
    { "random",
      RandomCommand,
      TRUE,
      TRUE,
      TRUE,
      "no-op" },
    { "rated",
      RatedCommand,
      TRUE,
      FALSE,
      FALSE,
      "Inform the engine that this is a rated game" },
    { "rating",
      RatingCommand,
      TRUE,
      FALSE,
      TRUE,
      "Inform the engine ratings" },
    { "remove",
      RemoveCommand,
      FALSE,
      FALSE,
      TRUE,
      "Take back a full move" },
    { "result",
      ResultCommand,
      FALSE,
      FALSE,
      TRUE,
      "Inform the engine about a game result" },
    { "script",
      ScriptCommand,
      FALSE,
      FALSE,
      FALSE,
      "Run a script" },
    { "sd",
      SearchDepthCommand, 
      TRUE,
      FALSE,
      TRUE,
      "Set engine maximum search depth" },
    { "set",
      SetCommand,
      TRUE,
      FALSE,
      FALSE,
      "View or change an engine variable" },
    { "setboard",
      SetBoardCommand,
      FALSE,
      FALSE,
      TRUE,
      "Load a board position" },
    { "solution",
      SolutionCommand,
      FALSE,
      FALSE,
      FALSE,
      "Set a solution move (up to three)" },
    { "sn",
      SearchNodesCommand,
      FALSE,
      FALSE,
      TRUE,
      "Set the engine max nodes per search limit" },
    { "st",
      SearchTimeCommand,
      FALSE,
      FALSE,
      TRUE,
      "Set the engine search time" },
    { "test",
      TestCommand,
      FALSE,
      FALSE,
      FALSE,
      "Run an internal debugging command" },
    { "time",
      TimeCommand,
      TRUE,
      FALSE,
      TRUE,
      "Tell the engine how its clock stands" },
    { "undo",
      UndoCommand,
      FALSE,
      FALSE,
      TRUE,
      "Take back a halfmove" },
    { "unrated",
      UnratedCommand,
      TRUE,
      FALSE,
      FALSE,
      "Tell the engine that the game is unrated" },
    { "variant",
      VariantCommand,
      FALSE,
      FALSE,
      TRUE,
      "Tell the engine what variant the game is" },    
    { "version",
      VersionCommand,
      TRUE,
      FALSE,
      FALSE,
      "Display current version number and build options" },
    { "white",
      WhiteCommand,
      FALSE,
      FALSE,
      TRUE,
      "Sets white to move, engine plays black" },
    { "winboard",
      XboardCommand,
      TRUE,
      TRUE,
      TRUE,
      "Internal part of the WinBoard protocol" },
    { "xboard",
      XboardCommand,
      TRUE,
      TRUE,
      TRUE,
      "Internal part of the WinBoard protocol" },
};


COMMAND(HelpCommand)
{
    char *command = NULL;
    ULONG u;

    //
    // If passed an argument, give help only about that command.
    // TODO: divert to a more specific help page here if available.
    //
    if (argc > 1) 
    {
        command = argv[1];
        for (u = 0; u < ARRAY_LENGTH(g_ParserTable); u++) 
        {
            if ((!g_ParserTable[u].fInvisible) &&
                (!STRCMPI(command, g_ParserTable[u].szOpcode))) 
            {
                Trace("%-13s: %s\n",
                      g_ParserTable[u].szOpcode,
                      g_ParserTable[u].szSimpleHelp);
            }
        }
        return;
    }

    //
    // Otherwise general help about all commands.
    //
    Trace("xboard/WinBoard protocol support commands:\n"
          "------------------------------------------\n");
    for (u = 0; u < ARRAY_LENGTH(g_ParserTable); u++) 
    {
        if ((g_ParserTable[u].fWinBoard) && (!g_ParserTable[u].fInvisible)) {
            Trace("%-13s: %s\n",
                  g_ParserTable[u].szOpcode,
                  g_ParserTable[u].szSimpleHelp);
        }
    }
    Trace("\nInternal engine commands:\n"
          "-------------------------\n");
    for (u = 0; u < ARRAY_LENGTH(g_ParserTable); u++) 
    {
        if ((!g_ParserTable[u].fWinBoard) && (!g_ParserTable[u].fInvisible)) {
            Trace("%-13s: %s\n", 
                  g_ParserTable[u].szOpcode,
                  g_ParserTable[u].szSimpleHelp);
        }
    }
}


void 
ParseUserInput(FLAG fSearching)
/**

Routine description:

Parameters:

    FLAG fSearching

Return value:

    void

**/
{
    CHAR *pCapturedInput = NULL;
    CHAR *pFreeThis = NULL;
    CHAR *pDontFree;
    CHAR *p, *q;
    ULONG u;
    CHAR *argv[MAX_ARGS];
    ULONG argc = 0;
    FLAG fInQuote = FALSE;
    MOVE mv;
    POSITION *pos = GetRootPosition();
#if 0
    static ULONG uHeapAllocated = 0;

    if (0 == uHeapAllocated)
    {
        uHeapAllocated = GetHeapMemoryUsage();
    } else {
        ASSERT(uHeapAllocated == GetHeapMemoryUsage());
        uHeapAllocated = GetHeapMemoryUsage();
    }
#endif
    //
    // If we are in batch mode and there is no waiting input, exit.
    //
    if (g_Options.fNoInputThread && (0 == NumberOfPendingInputEvents()))
    {
        Trace("Exhausted input in batch mode, exiting...\n");
        exit(-1);
    }
    
	//
	// By the time we see this the input thread is likely already
	// exited so don't try to read from the queue...
	//
    if (g_fExitProgram == TRUE) 
    {
		QuitCommand("quit", 1, NULL, pos);
		return;
	}
    
    //
    // If we are searching, don't actually read the input event yet. 
    // Just peek and leave it there until we know whether we can do 
    // it or not.
    //
    if (TRUE == fSearching)
    {
        ASSERT(g_Options.fPondering || g_Options.fThinking);
        p = pDontFree = PeekNextInput();
    } else {
        p = pFreeThis = BlockingReadInput();
    }
    if (NULL == p) 
    {
        goto end;
    }
    
    //
    // Skip over leading whitespace and do not parse comment lines
    //
    while((*p) && (isspace(*p))) p++;
    if (((*p) && (*p == '#')) ||
        ((*p) && (*(p+1)) && (*p == '/') && (*(p+1) == '/')))
    {
        goto consume_if_searching;
    }

    //
    // Chop the last \n out of the string
    //
    q = p;
    while(*q)
    {
        if (*q == '\n') 
        {
            *q = '\0';
            break;
        }
        q++;
    }

    //
    // Break it up into chunks for the COMMAND functions.
    //
    pCapturedInput = STRDUP(p);
    memset(argv, 0, sizeof(argv));
    do
    {
        while(*p && isspace(*p)) p++;
        if (!*p) break;
        argv[argc++] = p;
        fInQuote = FALSE;
        while (*p)
        {
            if (*p == '"')
            {
                fInQuote = !fInQuote;
            }
            else if (*p == '{')
            {
                fInQuote = TRUE;
            }
            else if (*p == '}')
            {
                fInQuote = FALSE;
            }
            else if (isspace(*p) && (FALSE == fInQuote))
            {
                break;
            }
            p++;
        }
        if (!*p) break;
        *p = '\0';
        p++;
    }
    while(argc < MAX_ARGS);
    if (argc == 0) 
    {
        goto consume_if_searching;
    }
    
    //
    // Does it look like a move?
    //
    mv.uMove = 0;
    switch(LooksLikeMove(argv[0]))
    {
        case MOVE_SAN:
            mv = ParseMoveSan(argv[0], pos);
            break;
        case MOVE_ICS:
            mv = ParseMoveIcs(argv[0], pos);
            break;
        case NOT_MOVE:
            for (u = 0; u < ARRAY_LENGTH(g_ParserTable); u++)
            {
                if (!STRCMPI(g_ParserTable[u].szOpcode, argv[0]))
                {
                    if (TRUE == fSearching) 
                    {
                        if (TRUE == g_ParserTable[u].fHandleOnTheFly)
                        {
                            //
                            // We can do that command on the fly in
                            // the middle of the search.  Note: these
                            // commands MUST NOT affect anything that
                            // the search is doing or you'll get
                            // corruption.
                            //
                            (*g_ParserTable[u].pFunction)
                                (pCapturedInput, argc, argv, pos);
                            goto consume_if_searching;
                        } else {
                            //
                            // We can't handle that input without
                            // unrolling the search first.
                            //
                            Trace("COMPLEX INPUT --> stop searching now\n");
                            ASSERT(pDontFree);
                            strcpy(pDontFree, pCapturedInput);
                            g_MoveTimer.bvFlags |= TIMER_STOPPING;
                            goto end;
                        }
                    } else {
                        //
                        // We were called from the main loop or from
                        // the script parser.  No one is searching.
                        // Just do the command.
                        //
                        (*g_ParserTable[u].pFunction)
                            (pCapturedInput, argc, argv, pos);
                        goto end;
                    }
                }
            }
            if ((TRUE == LooksLikeFen(pCapturedInput)) &&
                (TRUE == SetRootPosition(pCapturedInput)))
            {
                goto end;
            }
            break;
#ifdef DEBUG
        default:
            UtilPanic(SHOULD_NOT_GET_HERE,
                      NULL, NULL, NULL, NULL, 
                      __FILE__, __LINE__);
            break;
#endif
    }
    
    //
    // Was it a move?
    //
    if (mv.uMove)
    {
        if (fSearching == TRUE)
        {
            //
            // No one should move (my pieces) while I'm thinking!
            //
            ASSERT(g_Options.fPondering == TRUE);
            ASSERT(g_Options.fThinking == FALSE);
            if (IS_SAME_MOVE(mv, g_Options.mvPonder))
            {
                Trace("PREDICTED MOVE PLAYED --> converting to search\n");
                g_Options.fSuccessfulPonder = TRUE;
                g_Options.fPondering = FALSE;
                g_Options.fThinking = TRUE;
                SetMoveTimerForSearch(TRUE, FLIP(GET_COLOR(mv.pMoved)));
                goto consume_if_searching;
            } else {
                g_MoveTimer.bvFlags |= TIMER_STOPPING;
                if (TRUE == OfficiallyMakeMove(mv, 0, FALSE))
                {
                    Trace("NONPREDICTED MOVE PLAYED --> stop pondering now\n");
                    goto end;
                } else {
                    UtilPanic(GOT_ILLEGAL_MOVE_WHILE_PONDERING,
                              GetRootPosition(),
                              (void *)mv.uMove,
                              NULL,
                              NULL,
                              __FILE__, __LINE__);
                }
            }
        } else {
            if (FALSE == OfficiallyMakeMove(mv, 0, FALSE))
            {
                UtilPanic(CANNOT_OFFICIALLY_MAKE_MOVE,
                          GetRootPosition(),
                          (void *)mv.uMove,
                          NULL,
                          NULL,
                          __FILE__, __LINE__);
            }
        }
    } else {
        Trace("Unknown/illegal move/command: \"%s\"\n", pCapturedInput);
    }

 consume_if_searching:
    if (TRUE == fSearching)
    {
        ASSERT(NULL == pFreeThis);
        ASSERT(NumberOfPendingInputEvents() > 0);
        pFreeThis = BlockingReadInput();
    }
    // fall through
    
 end:
    if (NULL != pFreeThis)
    {
        SystemFreeMemory(pFreeThis);
        pFreeThis = NULL;
    }
    if (NULL != pCapturedInput)
    {
        SystemFreeMemory(pCapturedInput);
        pCapturedInput = NULL;
    }
//    ASSERT(uHeapAllocated == GetHeapMemoryUsage());
}
