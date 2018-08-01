/**

Copyright (c) Scott Gasch

Module Name:

    vars.c

Abstract:

    This is code that implements variables in the engine.  Some of
    these variables are read only but others can be set by the engine
    user to affect certain aspects of engine operation.

Author:

    Scott Gasch (scott.gasch@gmail.com) 18 May 2004

Revision History:

    $Id: vars.c 354 2008-06-30 05:10:08Z scott $

**/

#include "chess.h"

extern ULONG g_uNumInputEvents;

typedef void(PUSERVARFUN)(void);

//
// Here's how we expose engine variables to the user.
// 
typedef struct _USER_VARIABLE
{
    char *szName;                             // variable name
    char *szType;                             // variable type:
                                              //   * - char *, strdup/free
                                              //   H - hash size
                                              //   B - boolean
                                              //   U - unsigned
                                              //   I - int
                                              //   M - move
                                              //   D - double
                                              //   S - string buffer
    void *pValue;                             // pointer to value
    PUSERVARFUN *pFunction;                   // funct to call when it changes
} USER_VARIABLE;

extern ULONG g_uBookProbeFailures;

USER_VARIABLE g_UserVarList[] = 
{
    { "AnnounceOpening",
      "B",
      (void *)&(g_Options.fShouldAnnounceOpening),
      NULL },
    { "BatchMode",
      "b",
      (void *)&(g_Options.fNoInputThread),
      NULL },
    { "BlackPlayer",
      "*",
      (void *)&(g_GameData.sHeader.sPlayer[BLACK].szName),
      NULL },
    { "BlackRating",
      "U",
      (void *)&(g_GameData.sHeader.sPlayer[BLACK].uRating),
      NULL },
    { "BlackDescription",
      "*",
      (void *)&(g_GameData.sHeader.sPlayer[BLACK].szDescription),
      NULL },
    { "BlackIsComputer",
      "B",
      (void *)&(g_GameData.sHeader.sPlayer[BLACK].fIsComputer),
      NULL },
    { "BookFileName",
      "S",
      (void *)&(g_Options.szBookName),
      NULL },
    { "BookProbeFailures",
      "u",
      (void *)&(g_uBookProbeFailures),
      NULL },
    { "ComputerTimeRemainingSec",
      "U",
      (void *)&(g_Options.uOpponentsClock),
      NULL }, //&DoVerifyClocks },
    { "EGTBPath",
      "S",
      (void *)&(g_Options.szEGTBPath),
      InitializeEGTB },
    { "FastScript",
      "B",
      (void *)&(g_Options.fFastScript),
      NULL },
    { "GameDescription",
      "*",
      (void *)&(g_GameData.sHeader.szGameDescription),
      NULL },
    { "GameLocation",
      "*",
      (void *)&(g_GameData.sHeader.szLocation),
      NULL },
    { "GameIsRated",
      "B",
      (void *)&(g_GameData.sHeader.fGameIsRated),
      NULL },
    { "GameResultComment",
      "*",
      (void *)&(g_GameData.sHeader.result.szDescription),
      NULL },
    { "LastEval",
      "I",
      (void *)&(g_Options.iLastEvalScore),
      NULL },
    { "LogfileName",
      "S",
      (void *)&(g_Options.szLogfile),
      NULL },
    { "MoveToPonder",
      "m",
      (void *)&(g_Options.mvPonder),
      NULL },
    { "MovesPerTimePeriod",
      "U",
      (void *)&(g_Options.uMovesPerTimePeriod),
      NULL },
    { "OpponentTimeRemainingSec",
      "U",
      (void *)&(g_Options.uMyClock),
      NULL }, //&DoVerifyClocks },
    { "PendingInputEvents",
      "u",
      (void *)&(g_uNumInputEvents),
      NULL },
    { "PonderingNow",
      "b", 
      (void *)&(g_Options.fPondering),
      NULL },
    { "PostLines",
      "B",
      (void *)&(g_Options.fShouldPost),
      NULL },
    { "ResignThreshold",
      "I",
      (void *)&(g_Options.iResignThreshold),
      NULL },
    { "SearchDepthLimit",
      "U",
      (void *)&(g_Options.uMaxDepth),
      NULL }, //&DoVerifySearchDepth },
    { "SearchTimeLimit",
      "U",
      (void *)&(g_Options.uMyIncrement),
      NULL }, //&DoVerifySearchTime },
    { "SearchStartedTime",
      "d",
      (void *)&(g_MoveTimer.dStartTime),
      NULL },
    { "SearchSoftTimeLimit",
      "d",
      (void *)&(g_MoveTimer.dSoftTimeLimit),
      NULL },
    { "SearchHardTimeLimit",
      "d",
      (void *)&(g_MoveTimer.dHardTimeLimit),
      NULL },
    { "SearchNodesBetweenTimeCheck",
      "u",
      (void *)&(g_MoveTimer.uNodeCheckMask),
      NULL },
    { "StatusLine",
      "B",
      (void *)&(g_Options.fStatusLine),
      NULL },
    { "ThinkOnOpponentsTime",
      "B",
      (void *)&(g_Options.fShouldPonder),
      NULL },
    { "ThinkingNow",
      "b",
      (void *)&(g_Options.fThinking),
      NULL },
    { "TournamentMode",
      "B",
      (void *)&(g_fTournamentMode),
      NULL },
    { "VerbosePosting",
      "B",
      (void *)&(g_Options.fVerbosePosting),
      NULL },
    { "WhitePlayer",
      "*",
      (void *)&(g_GameData.sHeader.sPlayer[WHITE].szName),
      NULL },
    { "WhiteRating",
      "U",
      (void *)&(g_GameData.sHeader.sPlayer[WHITE].uRating),
      NULL },
    { "WhiteDescription",
      "*",
      (void *)&(g_GameData.sHeader.sPlayer[WHITE].szDescription),
      NULL },
    { "WhiteIsComputer",
      "B",
      (void *)&(g_GameData.sHeader.sPlayer[WHITE].fIsComputer),
      NULL },
    { "Xboard",
      "B",
      (void *)&(g_Options.fRunningUnderXboard),
      NULL }
};




COMMAND(SetCommand)
/**

Routine description:

    This function implements the 'set' engine command.  

    Usage:
    
        set [variable name [value]]

        With no arguments, set dumps a list of all variables and their
        current values.

        With one argument, set displays the specified variable's value.

        With two arguments, set changes the value of the specified 
        variable to the given value.

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
    ULONG x, y;
    FLAG fFound;
    
    if (argc == 1)
    {
        for (x = 0;
             x < ARRAY_SIZE(g_UserVarList);
             x++)
        {
            switch(toupper(g_UserVarList[x].szType[0]))
            {
                case 'H':
                case 'U':
                    Trace("%-28s => %u\n", 
                          g_UserVarList[x].szName,
                          *((unsigned int *)g_UserVarList[x].pValue));
                    break;
                case 'I':
                    Trace("%-28s => %d\n",
                          g_UserVarList[x].szName,
                          *((int *)g_UserVarList[x].pValue));
                    break;
                case 'S':
                    Trace("%-28s => \"%s\"\n",
                          g_UserVarList[x].szName,
                          (char *)g_UserVarList[x].pValue);
                    break;
                case 'M':
                    Trace("%-28s => \"%s\"\n",
                          g_UserVarList[x].szName,
                          MoveToIcs(*((MOVE *)g_UserVarList[x].pValue)));
                    break;
                case 'B':
                    Trace("%-28s => \"%s\"\n",
                          g_UserVarList[x].szName,
                          (*((FLAG *)g_UserVarList[x].pValue) == TRUE) ?
                          "TRUE" : "FALSE");
                    break;
                case 'D':
                    Trace("%-28s => %f\n",
                          g_UserVarList[x].szName,
                          *((double *)g_UserVarList[x].pValue));
                    break;
                case '*':
                    Trace("%-28s => \"%s\"\n",
                          g_UserVarList[x].szName,
                          *((char **)g_UserVarList[x].pValue));
                    break;
                default:
                    Trace("%-28s => ???\n",
                          g_UserVarList[x].szName);
                    // ASSERT(FALSE);
                    break;
            }
        }
        return;
    }
    else if (argc == 2)
    {
        fFound = FALSE;
        for (x = 0;
             x < ARRAY_SIZE(g_UserVarList);
             x++)
        {
            if (!STRNCMPI(argv[1], 
                          g_UserVarList[x].szName,
                          strlen(argv[1])))
            {
                fFound = TRUE;
                switch(toupper(g_UserVarList[x].szType[0]))
                {
                    case 'H':
                    case 'U':
                        Trace("%-28s => %u\n", 
                              g_UserVarList[x].szName,
                              *((unsigned int *)g_UserVarList[x].pValue));
                        break;
                    case 'I':
                        Trace("%-28s => %d\n",
                              g_UserVarList[x].szName,
                              *((int *)g_UserVarList[x].pValue));
                        break;
                    case 'S':
                        Trace("%-28s => \"%s\"\n",
                              g_UserVarList[x].szName,
                              (char *)g_UserVarList[x].pValue);
                        break;
                    case 'M':
                        Trace("%-28s => \"%s\"\n",
                              g_UserVarList[x].szName,
                              MoveToIcs(*((MOVE *)g_UserVarList[x].pValue)));
                        break;
                    case 'B':
                        Trace("%-28s => \"%s\"\n",
                              g_UserVarList[x].szName,
                              (*((FLAG *)g_UserVarList[x].pValue) == TRUE) ?
                              "TRUE" : "FALSE");
                        break;
                    case 'D':
                        Trace("%-28s => %f\n",
                              g_UserVarList[x].szName,
                              *((double *)g_UserVarList[x].pValue));
                        break;
                    case '*':
                        Trace("%-28s => \"%s\"\n",
                              g_UserVarList[x].szName,
                              *((char **)g_UserVarList[x].pValue));
                        break;
                    default:
                        Trace("%-28s => ???\n",
                              g_UserVarList[x].szName);
                        // ASSERT(FALSE);
                        break;
                }
            }
        }
        
        if (FALSE == fFound)
        {
            Trace("Error (invalid variable): %s\n", argv[1]);
        }
        return;
    }
    else 
    {
        ASSERT(argc > 2);
        
        fFound = FALSE;
        
        for (x = 0;
             x < ARRAY_SIZE(g_UserVarList);
             x++)
        {
            if (!STRNCMPI(argv[1], 
                         g_UserVarList[x].szName,
                         strlen(argv[1])))
            {
                fFound = TRUE;
                
                if (islower(g_UserVarList[x].szType[0]))
                {
                    Trace("Error (variable is read only): %s\n",
                          argv[1]);
                }
                
                switch(g_UserVarList[x].szType[0])
                {
                    case 'H':
                        y = atoi(argv[2]);
                        if ((y < 0) || (y & (y - 1)))
                        {
                            Trace("Error (invalid size): %d\n", y);
                        }
                        else
                        {
                            *((int *)g_UserVarList[x].pValue) = y;
                        }
                        break;
                        
                    case '*':
                        if (NULL != *((char **)g_UserVarList[x].pValue))
                        {
                            SystemFreeMemory( *((char **)g_UserVarList[x].pValue ) );
                        }
                        *((char **)g_UserVarList[x].pValue) = STRDUP(argv[2]);
                        if (NULL == *((char **)g_UserVarList[x].pValue))
                        {
                            Trace("Error (out of memory)\n");
                            return;
                        }
                        break;
                        
                    case 'S':
                        strncpy(((char *)g_UserVarList[x].pValue),
                                argv[2], 
                                127);
                        break;
                        
                    case 'U':
                        y = atoi(argv[2]);
                        if (y < 0)
                        {
                            Trace("Error (invalid value): %d\n", y);
                            return;
                        }
                        *((unsigned *)g_UserVarList[x].pValue) = y;
                        break;

                    case 'I':
                        y = atoi(argv[2]);
                        *((int *)g_UserVarList[x].pValue) = y;
                        break;
                        
                    case 'B':
                        if (toupper(*(argv[2])) == 'T')
                        {
                            *((FLAG *)g_UserVarList[x].pValue) = TRUE;
                        }
                        else if (toupper(*(argv[2])) == 'F')
                        {
                            *((FLAG *)g_UserVarList[x].pValue) = FALSE;
                        }
                        else
                        {
                            Trace("Error (invalid value, must be TRUE or "
                                  "FALSE): %s\n", argv[2]);
                        }
                        break;
                        
                    default:
                        ASSERT(FALSE);
                        break;
                }
                
                if (NULL != g_UserVarList[x].pFunction)
                {
                    (void)(g_UserVarList[x].pFunction)();
                }
            }
        }
        
        if (FALSE == fFound)
        {
            Trace("Error (invalid variable): %s\n", argv[1]);
        }
    }
}

//
// TODO: unset?
//
