/**

Copyright (c) Scott Gasch

Module Name:

    gamelist.c

Abstract:

    Maintain a list of game moves.  This list is used to take back
    official moves.  It's also used to produce PGN at the end of a
    game and to detect draws by repetition.  This code also maintains
    the official "current position" of the game in progress.

Author:

    Scott Gasch (scott.gasch@gmail.com) 14 May 2004

Revision History:

    $Id: gamelist.c 355 2008-07-01 15:46:43Z scott $

**/

#include "chess.h"

GAME_DATA g_GameData;
ULONG g_uMoveNumber[2] = { 1, 1 };
ULONG g_uToMove;
ALIGN64 POSITION g_RootPosition;

#ifdef DEBUG
ULONG g_uAllocs = 0;
#endif

#define MOVE_POOL_SIZE (256)
GAME_MOVE g_MovePool[MOVE_POOL_SIZE];

static GAME_MOVE *
_GetMoveFromPool(void)
/**

Routine description:

    Fast GAME_MOVE allocator that uses a preallocated pool.

Parameters:

    void

Return value:

    static GAME_MOVE *

**/
{
    ULONG x = (rand() % (MOVE_POOL_SIZE - 15));
    ULONG y;

    for (y = x; y < x + 15; y++)
    {
        if (g_MovePool[y].fInUse == FALSE)
        {
            LockIncrement(&(g_MovePool[y].fInUse));
            return(&(g_MovePool[y]));
        }
    }
#ifdef DEBUG
    g_uAllocs++;
#endif
    return(SystemAllocateMemory(sizeof(GAME_MOVE)));
}

static void 
_ReturnMoveToPool(GAME_MOVE *p)
/**

Routine description:

    Return a GAME_MOVE to the pool.

Parameters:

    GAME_MOVE *p

Return value:

    static void

**/
{
    ULONG x = (ULONG)((BYTE *)p - (BYTE *)&g_MovePool);
    
    x /= sizeof(GAME_MOVE);
    if (x < MOVE_POOL_SIZE)
    {
        g_MovePool[x].fInUse = FALSE;
    }
    else
    {
#ifdef DEBUG
        g_uAllocs--;
#endif
        SystemFreeMemory(p);
    }
}

static void 
_FreeIfNotNull(void *p)
/**

Routine description:

    Free a pointer if it's not null.

Parameters:

    void *p

Return value:

    static void

**/
{
    if (NULL != p)
    {
        SystemFreeMemory(p);
#ifdef DEBUG
        g_uAllocs--;
#endif
    }
}

static void 
_FreeGameMove(GAME_MOVE *p)
/**

Routine description:

    Free memory allocated to store a game move.

Parameters:

    GAME_MOVE *p

Return value:

    static void

**/
{
    ASSERT(NULL != p);
    
    _FreeIfNotNull(p->szComment);
    _FreeIfNotNull(p->szDecoration);
    _FreeIfNotNull(p->szMoveInSan);
    _FreeIfNotNull(p->szMoveInIcs);
    _FreeIfNotNull(p->szUndoPositionFen);
    _ReturnMoveToPool(p);
}

static void 
_FreeGameList(void)
/**

Routine description:

    Free the whole game list.

Parameters:

    void

Return value:

    static void

**/
{
    DLIST_ENTRY *p;
    GAME_MOVE *q;

    if (NULL != g_GameData.sMoveList.pFlink)
    {
        while(FALSE == IsListEmpty(&(g_GameData.sMoveList)))
        {
            p = RemoveHeadList(&(g_GameData.sMoveList));
            q = CONTAINING_STRUCT(p, GAME_MOVE, links);
            ASSERT(NULL != q);
            _FreeGameMove(q);
        }
    }
    _FreeIfNotNull(g_GameData.sHeader.szGameDescription);
    _FreeIfNotNull(g_GameData.sHeader.szLocation);
    _FreeIfNotNull(g_GameData.sHeader.sPlayer[WHITE].szName);
    _FreeIfNotNull(g_GameData.sHeader.sPlayer[WHITE].szDescription);
    _FreeIfNotNull(g_GameData.sHeader.sPlayer[BLACK].szName);
    _FreeIfNotNull(g_GameData.sHeader.sPlayer[BLACK].szDescription);
    _FreeIfNotNull(g_GameData.sHeader.szInitialFen);
    memset(&g_GameData, 0, sizeof(g_GameData));
}

void
ResetGameList(void)
/**

Routine description:

    Reset the game list (between games etc...)

Parameters:

    void

Return value:

    void

**/
{
    _FreeGameList();
    ASSERT(g_uAllocs == 0);
    memset(g_MovePool, 0, sizeof(g_MovePool));
    g_uMoveNumber[BLACK] = g_uMoveNumber[WHITE] = 1;
    g_uToMove = WHITE;
    InitializeListHead(&(g_GameData.sMoveList));
    g_GameData.sHeader.result.eResult = RESULT_IN_PROGRESS;
}

FLAG 
SetRootPosition(CHAR *szFen)
/**

Routine description:

    Set the root position.

Parameters:

    CHAR *szFen

Return value:

    FLAG

**/
{
    if (TRUE == FenToPosition(&g_RootPosition, szFen))
    {
        ResetGameList();
	g_uToMove = g_RootPosition.uToMove;
        TellGamelistThatIPlayColor(FLIP(g_RootPosition.uToMove));
        if (0 != strcmp(szFen, STARTING_POSITION_IN_FEN))
        {
            g_GameData.sHeader.szInitialFen = STRDUP(szFen);
#ifdef DEBUG
            if (g_GameData.sHeader.szInitialFen)
            {
                g_uAllocs++;
            }
#endif
        }
        if (g_Options.fShouldPost)
        {
            DumpPosition(&g_RootPosition);
        }
        return(TRUE);
    }
    return(FALSE);
}

POSITION *
GetRootPosition(void)
/**

Routine description:

    Get the root position.

Parameters:

    void

Return value:

    POSITION

**/
{
#ifdef DEBUG
    static POSITION p;

    memcpy(&p, &g_RootPosition, sizeof(POSITION));
    
    return(&p);
#else
    return(&g_RootPosition);
#endif
}

void 
SetGameResultAndDescription(GAME_RESULT result)
/**

Routine description:

    Set the game result and a description (both for PGN).

Parameters:

    GAME_RESULT result
    
Return value:

    void

**/
{
    g_GameData.sHeader.result = result;
}

GAME_RESULT GetGameResult(void)
/**

Routine description:

    Get the game result.

Parameters:

    void

Return value:

    INT

**/
{
    return g_GameData.sHeader.result;
}


GAME_MOVE *
GetNthOfficialMoveRecord(ULONG n)
/**

Routine description:

   Get the official move record for move N.

Parameters:

    ULONG n

Return value:

    GAME_MOVE

**/
{
    DLIST_ENTRY *p = g_GameData.sMoveList.pFlink;
    GAME_MOVE *q;
    
    while((p != &(g_GameData.sMoveList)) &&
          (n != 0))
    {
        p = p->pFlink;
        n--;
    }
    
    if (n == 0)
    {
        q = CONTAINING_STRUCT(p, GAME_MOVE, links);
        return(q);
    }
    return(NULL);
}


FLAG 
DoesSigAppearInOfficialGameList(UINT64 u64Sig)
/**

Routine description:

    Does sig X appear in the official move list?

Parameters:

    UINT64 u64Sig

Return value:

    FLAG

**/
{
    DLIST_ENTRY *p = g_GameData.sMoveList.pBlink;
    GAME_MOVE *q;

    while(p != &(g_GameData.sMoveList))
    {
        q = CONTAINING_STRUCT(p, GAME_MOVE, links);
        if (q->u64PositionSigAfterMove == u64Sig)
        {
            return(TRUE);
        }
        if ((q->mv.pCaptured) || (IS_PAWN(q->mv.pMoved)))
        {
            break;
        }
        p = p->pBlink;
    }
    return(FALSE);
}

ULONG CountOccurrancesOfSigInOfficialGameList(UINT64 u64Sig)
/**

Routine description:

    How many times does sig X appear in the official move list?

Parameters:

    UINT64 u64Sig

Return value:

    ULONG

**/
{
    DLIST_ENTRY *p = g_GameData.sMoveList.pBlink;
    GAME_MOVE *q;
    ULONG uCount = 0;

    while(p != &(g_GameData.sMoveList))
    {
        q = CONTAINING_STRUCT(p, GAME_MOVE, links);
        if (q->u64PositionSigAfterMove == u64Sig)
        {
            uCount++;
        }
        if ((q->mv.pCaptured) || (IS_PAWN(q->mv.pMoved)))
        {
            break;
        }
        p = p->pBlink;
    }
    return(uCount);
}


FLAG 
IsLegalDrawByRepetition(void)
/**

Routine description:

    Have we just drawn the game by repetition?

Parameters:

    void

Return value:

    FLAG

**/
{
    POSITION *pos = GetRootPosition();
    UINT64 u64Sig = (pos->u64PawnSig ^ pos->u64NonPawnSig);

    if (3 == CountOccurrancesOfSigInOfficialGameList(u64Sig))
    {
        return(TRUE);
    }
    return(FALSE);
}


void 
DumpGameList(void)
/**

Routine description:

    Dump the move list to stdout.

Parameters:

    void

Return value:

    void

**/
{
    GAME_MOVE *q;
    ULONG u = 0;

    Trace("\twhite\tblack\n"
          "\t-----\t-----\n");
    q = GetNthOfficialMoveRecord(u);
    while((q != NULL) && (q->mv.uMove != 0))
    {
        if (GET_COLOR(q->mv.pMoved) == WHITE)
        {
            Trace("%2u.\t%s\t", q->uNumber, q->szMoveInSan);
        }
        else
        {
            Trace("%s\n", q->szMoveInSan);
        }
        u++;
        q = GetNthOfficialMoveRecord(u);
    }
    Trace("\n");
}

void 
DumpPgn(void)
/**

Routine description:

    Dump the PGN of the current game to stdout.

Parameters:

    void

Return value:

    void

**/
{
    DLIST_ENTRY *p = g_GameData.sMoveList.pFlink;
    GAME_MOVE *q;
    ULONG u = 0;

    Trace("[Event \"Computer Chess Game\"]\n");
    if (g_GameData.sHeader.szLocation != NULL)
    {
        Trace("[Site \"%s\"]\n", g_GameData.sHeader.szLocation);
    }
    Trace("[Date \"%s\"]\n", SystemGetDateString());
    Trace("[Round 0]\n");
    Trace("[White \"%s\"]\n", g_GameData.sHeader.sPlayer[WHITE].szName);
    Trace("[Black \"%s\"]\n", g_GameData.sHeader.sPlayer[BLACK].szName);
    switch(g_GameData.sHeader.result.eResult)
    {
        case RESULT_BLACK_WON:
            Trace("[Result \"0-1\"]\n");
            break;
        case RESULT_WHITE_WON:
            Trace("[Result \"1-0\"]\n");
            break;
        case RESULT_DRAW:
            Trace("[Result \"1/2-1/2\"]\n");
            break;
        default:
            Trace("[Result \"*\"]\n");
            break;
    }
    if (g_GameData.sHeader.sPlayer[WHITE].uRating != 0)
    {
        Trace("[WhiteElo \"%u\"]\n", g_GameData.sHeader.sPlayer[WHITE].uRating);
    }
    if (g_GameData.sHeader.sPlayer[BLACK].uRating != 0)
    {
        Trace("[BlackElo \"%u\"]\n", g_GameData.sHeader.sPlayer[BLACK].uRating);
    }
    if (NULL != g_GameData.sHeader.szInitialFen)
    {
        Trace("[InitialFEN \"%s\"]\n", g_GameData.sHeader.szInitialFen);
    }
    // [ECO: XX]
    Trace("[Time \"%s\"]\n", SystemGetTimeString());
    /*
9.6.1: Tag: TimeControl

This uses a list of one or more time control fields.  Each field contains a
descriptor for each time control period; if more than one descriptor is present
then they are separated by the colon character (":").  The descriptors appear
in the order in which they are used in the game.  The last field appearing is
considered to be implicitly repeated for further control periods as needed.

There are six kinds of TimeControl fields.

The first kind is a single question mark ("?") which means that the time
control mode is unknown.  When used, it is usually the only descriptor present.

The second kind is a single hyphen ("-") which means that there was no time
control mode in use.  When used, it is usually the only descriptor present.

The third Time control field kind is formed as two positive integers separated
by a solidus ("/") character.  The first integer is the number of moves in the
period and the second is the number of seconds in the period.  Thus, a time
control period of 40 moves in 2 1/2 hours would be represented as "40/9000".

The fourth TimeControl field kind is used for a "sudden death" control period.
It should only be used for the last descriptor in a TimeControl tag value.  It
is sometimes the only descriptor present.  The format consists of a single
integer that gives the number of seconds in the period.  Thus, a blitz game
would be represented with a TimeControl tag value of "300".

The fifth TimeControl field kind is used for an "incremental" control period.
It should only be used for the last descriptor in a TimeControl tag value and
is usually the only descriptor in the value.  The format consists of two
positive integers separated by a plus sign ("+") character.  The first integer
gives the minimum number of seconds allocated for the period and the second
integer gives the number of extra seconds added after each move is made.  So,
an incremental time control of 90 minutes plus one extra minute per move would
be given by "4500+60" in the TimeControl tag value.

The sixth TimeControl field kind is used for a "sandclock" or "hourglass"
control period.  It should only be used for the last descriptor in a
TimeControl tag value and is usually the only descriptor in the value.  The
format consists of an asterisk ("*") immediately followed by a positive
integer.  The integer gives the total number of seconds in the sandclock
period.  The time control is implemented as if a sandclock were set at the
start of the period with an equal amount of sand in each of the two chambers
and the players invert the sandclock after each move with a time forfeit
indicated by an empty upper chamber.  Electronic implementation of a physical
sandclock may be used.  An example sandclock specification for a common three
minute egg timer sandclock would have a tag value of "*180".

Additional TimeControl field kinds will be defined as necessary.
    */
    
    Trace("\n");

    while(p != &(g_GameData.sMoveList))
    {
        q = CONTAINING_STRUCT(p, GAME_MOVE, links);
        if (GET_COLOR(q->mv.pMoved) == WHITE)
        {
            Trace("%2u. %s ", q->uNumber, q->szMoveInSan);
        }
        else
        {
            Trace("%s ", q->szMoveInSan);
        }
        u++;
        if (u > 10)
        {
            Trace("\n");
            u = 0;
        }
        p = p->pFlink;
    }
    switch(g_GameData.sHeader.result.eResult)
    {
        case RESULT_BLACK_WON:
            Trace("0-1\n");
            break;
        case RESULT_WHITE_WON:
            Trace("1-0\n");
            break;
        case RESULT_DRAW:
            Trace("1/2-1/2\n");
            break;
        default:
            Trace("*\n");
            break;
    }
    Trace("\n");
    if (strlen(g_GameData.sHeader.result.szDescription)) {
        Trace("%s\n", g_GameData.sHeader.result.szDescription);
    }
}

FLAG 
OfficiallyTakebackMove(void)
/**

Routine description:

    Take back a move (i.e. unmake it and delete it from the move
    list).

Parameters:

    void

Return value:

    FLAG

**/
{
    DLIST_ENTRY *p;
    GAME_MOVE *q;
    POSITION *pos = &g_RootPosition;

    if (FALSE == IsListEmpty(&(g_GameData.sMoveList)))
    {
        p = RemoveTailList(&(g_GameData.sMoveList));
        ASSERT(p);
        q = CONTAINING_STRUCT(p, GAME_MOVE, links);
        if (FALSE == FenToPosition(pos, q->szUndoPositionFen))
        {
            UtilPanic(INCONSISTENT_STATE,
                      pos,
                      q->szUndoPositionFen,
                      NULL,
                      NULL,
                      __FILE__, __LINE__);
        }
        _FreeGameMove(q);
        VerifyPositionConsistency(pos, FALSE);
        
        ASSERT(g_uMoveNumber[g_uToMove] > 0);
        g_uMoveNumber[g_uToMove] -= 1;
        g_uToMove = FLIP(g_uToMove);
        return(TRUE);
    }
    return(FALSE);
}

FLAG 
OfficiallyMakeMove(MOVE mv, 
                   SCORE iMoveScore,
                   FLAG fFast)
/**

Routine description:

    Officially make a move (i.e. add it to the game list)

Parameters:

    MOVE mv,
    SCORE iMoveScore

Return value:

    FLAG

**/
{
    static SEARCHER_THREAD_CONTEXT ctx;
    FLAG fRet = FALSE;
    GAME_MOVE *q;
    POSITION *pos = &g_RootPosition;
    UINT64 u64SigBefore = pos->u64PawnSig ^ pos->u64NonPawnSig;
    
    ReInitializeSearcherContext(pos, &ctx);
    ASSERT(mv.uMove);
    ASSERT(GET_COLOR(mv.pMoved) == g_uToMove);
    if (FALSE == MakeUserMove(&ctx, mv))
    {
        goto end;
    }
    
    q = _GetMoveFromPool();
    if (NULL == q)
    {
        goto end;
    }
    q->uNumber = g_uMoveNumber[g_uToMove];
    ASSERT(q->uNumber > 0);
    g_uMoveNumber[g_uToMove] += 1;
    ASSERT(g_uMoveNumber[g_uToMove] != 0);
    q->mv.uMove = mv.uMove;
    q->szComment = NULL;
    q->szDecoration = NULL;
    q->iMoveScore = iMoveScore;

    q->szMoveInSan = STRDUP(MoveToSan(mv, pos));
#ifdef DEBUG
    if (q->szMoveInSan)
    {
        g_uAllocs++;
    }
#endif
    q->szMoveInIcs = STRDUP(MoveToIcs(mv));
#ifdef DEBUG
    if (q->szMoveInIcs)
    {
        g_uAllocs++;
    }
#endif
    q->szUndoPositionFen = PositionToFen(pos);
#ifdef DEBUG
    g_uAllocs++;
#endif
    InsertTailList(&(g_GameData.sMoveList), &(q->links));
    q->u64PositionSigAfterMove = (ctx.sPosition.u64NonPawnSig ^ 
                                  ctx.sPosition.u64PawnSig);
    q->u64PositionSigBeforeMove = u64SigBefore;
    g_uToMove = FLIP(g_uToMove);

    //
    // Update the root position
    //
    memcpy(pos, &(ctx.sPosition), sizeof(POSITION));
    VerifyPositionConsistency(&(ctx.sPosition), FALSE);
    fRet = TRUE;
    
 end:
    return(fRet);
}

ULONG 
GetMyColor(void)
/**

Routine description:

    What color am I playing?

Parameters:

    void

Return value:

    ULONG

**/
{
    switch(g_Options.ePlayMode)
    {
        case I_PLAY_WHITE:
            return(WHITE);
        case I_PLAY_BLACK:
        default:
            return(BLACK);
    }
}

ULONG 
GetOpponentsColor(void)
/**

Routine description:

    What color is the opponent playing?

Parameters:

    void

Return value:

    ULONG

**/
{
    return(FLIP(GetMyColor()));
}

void 
SetOpponentsName(CHAR *sz)
/**

Routine description:

    Set my opponent's name for the PGN record.  Hello, my name is
    Inigo Montoya.  You killed my father.  Prepare to die.

Parameters:

    CHAR *sz

Return value:

    void

**/
{
    _FreeIfNotNull(g_GameData.sHeader.sPlayer[GetOpponentsColor()].szName);
    g_GameData.sHeader.sPlayer[GetOpponentsColor()].szName = STRDUP(sz);
#ifdef DEBUG
    if (g_GameData.sHeader.sPlayer[GetOpponentsColor()].szName)
    {
        g_uAllocs++;
    }
#endif
}

void SetMyName(void)
/**

Routine description:

    Set my name for the PGN record.  Some people call me... Tim?

Parameters:

    void

Return value:

    void

**/
{
    ULONG u = GetMyColor();
    
    _FreeIfNotNull(g_GameData.sHeader.sPlayer[u].szName);
    g_GameData.sHeader.sPlayer[u].szName = STRDUP("typhoon");
#ifdef DEBUG
    if (g_GameData.sHeader.sPlayer[u].szName)
    {
        g_uAllocs++;
    }
#endif
    _FreeIfNotNull(g_GameData.sHeader.sPlayer[u].szDescription);
    g_GameData.sHeader.sPlayer[u].szDescription =
        STRDUP("Ver: " VERSION " Build Time: " __TIME__ " " __DATE__);
#ifdef DEBUG
    if (g_GameData.sHeader.sPlayer[u].szDescription)
    {
        g_uAllocs++;
    }
#endif
    g_GameData.sHeader.sPlayer[u].fIsComputer = TRUE;
}

void 
SetMyRating(ULONG u)
/**

Routine description:

    Set my rating for the PGN record.

Parameters:

    ULONG u

Return value:

    void

**/
{
    g_GameData.sHeader.sPlayer[GetMyColor()].uRating = u;
}

void 
SetOpponentsRating(ULONG u)
/**

Routine description:

    Set the opponent's rating for the PGN record.

Parameters:

    ULONG u

Return value:

    void

**/
{
    g_GameData.sHeader.sPlayer[GetOpponentsColor()].uRating = u;
}

void 
TellGamelistThatIPlayColor(ULONG u)
/**

Routine description:

    The gamelist needs to know what color the computer is playing.
    This routine sets it.

Parameters:

    ULONG u

Return value:

    void

**/
{
    ULONG uOldColor = GetMyColor();
    GAME_PLAYER x;
    
    ASSERT(IS_VALID_COLOR(u));
    ASSERT(IS_VALID_COLOR(uOldColor));

    if (u != uOldColor)
    {
        x = g_GameData.sHeader.sPlayer[uOldColor];
        g_GameData.sHeader.sPlayer[uOldColor] =
            g_GameData.sHeader.sPlayer[u];
        g_GameData.sHeader.sPlayer[u] = x;
    }
}

ULONG 
GetMoveNumber(ULONG uColor)
/**

Routine description:

    What move number is it?

Parameters:

    void

Return value:

    ULONG

**/
{
    return(g_uMoveNumber[uColor]);
}

void
MakeStatusLine(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    char buf[SMALL_STRING_LEN_CHAR];
    ASSERT(IS_VALID_COLOR(g_uToMove));

    if (g_Options.fStatusLine) 
    {
        strcpy(buf, "_\0");
        if (!g_Options.fRunningUnderXboard) 
        {
            if ((TRUE == g_Options.fPondering) &&
                (g_Options.mvPonder.uMove != 0))
            {
                snprintf(buf, SMALL_STRING_LEN_CHAR - 1,
                         "_[pondering %s] ", 
                         MoveToSan(g_Options.mvPonder, &g_RootPosition));
            }
            snprintf(buf, SMALL_STRING_LEN_CHAR - strlen(buf),
                     "%s%s(move %u)_", 
                     buf, 
                     (g_uToMove == WHITE) ? "white" : "black",
                     g_uMoveNumber[g_uToMove]);
            Trace("%s\n", buf);
        }
    }
}

static void 
_ParsePgnHeaderLine(CHAR *p, CHAR **ppVar, CHAR **ppVal)
/**

Routine description:

Parameters:

    CHAR *p,
    CHAR **ppVar,
    CHAR **ppVal

Return value:

    static void

**/
{
    *ppVar = NULL;
    *ppVal = NULL;
    if (*p != '[') return;
    
    p++;
    *ppVar = p;
    while(*p && !isspace(*p)) p++;

    p++;
    *ppVal = p;
    while(*p && *p != ']') p++;
}

static void 
_ParsePgnHeaderTag(CHAR *p)
/**

Routine description:

    Parse PGN headers and extract people's names, ratings, game
    result, etc...

Parameters:

    CHAR *p

Return value:

    static void

**/
{
    CHAR *pVar, *pVal;
    
    _ParsePgnHeaderLine(p, &pVar, &pVal);
    if ((NULL == pVar) || (NULL == pVal))
    {
        return;
    }
    
    if (!STRNCMPI(pVar, "BLACK ", 6))
    {
        p = pVal;
        while(*p && *p != ']') p++;
        if (*p)
        {
            *p = '\0';
            _FreeIfNotNull(g_GameData.sHeader.sPlayer[BLACK].szName);
            g_GameData.sHeader.sPlayer[BLACK].szName = STRDUP(pVal);
#ifdef DEBUG
            if (g_GameData.sHeader.sPlayer[BLACK].szName)
            {
                g_uAllocs++;
            }
#endif
            *p = ']';
            p++;
        }
    }
    else if (!STRNCMPI(pVar, "WHITE ", 6))
    {
        p = pVal;
        while(*p && *p != ']') p++;
        if (*p)
        {
            *p = '\0';
            _FreeIfNotNull(g_GameData.sHeader.sPlayer[WHITE].szName);
            g_GameData.sHeader.sPlayer[WHITE].szName = STRDUP(pVal);
#ifdef DEBUG
            if (g_GameData.sHeader.sPlayer[WHITE].szName)
            {
                g_uAllocs++;
            }
#endif
            *p = ']';
            p++;
        }
    }
    else if (!STRNCMPI(pVar, "RESULT", 6))
    {
        p = pVal;
        if (*p == '"') p++;
        if (!STRNCMPI(p, "1-0", 3))
        {
            GAME_RESULT res;
            res.eResult = RESULT_WHITE_WON;
            strcpy(res.szDescription, "PGN game");
            SetGameResultAndDescription(res);
        }
        else if (!STRNCMPI(p, "0-1", 3))
        {
            GAME_RESULT res;
            res.eResult = RESULT_BLACK_WON;
            strcpy(res.szDescription, "PGN game");
            SetGameResultAndDescription(res);
        }
        else if (!STRNCMPI(p, "1/2", 3))
        {
            GAME_RESULT res;
            res.eResult = RESULT_DRAW;
            strcpy(res.szDescription, "PGN game");
            SetGameResultAndDescription(res);
        }
        while(*p && *p != ']') p++;
        if (*p) p++;
    }
    //
    // TODO: add others
    //
}    

FLAG 
LoadPgn(CHAR *szPgn)
/**

Routine description:

    Load a PGN blob and create a move list from it.

Parameters:

    CHAR *szPgn

Return value:

    FLAG

**/
{
    POSITION *pos;
    CHAR szMove[16];
    MOVE mv;
    CHAR *p = szPgn;
    ULONG uMoveCount = 0;
    ULONG x;
    FLAG fRet = FALSE;
    FLAG fOldPost = g_Options.fShouldPost;
    
    g_Options.fShouldPost = FALSE;
    ResetGameList();
    SetRootToInitialPosition();
    pos = GetRootPosition();
    
    //
    // Get rid of newlines, non-space spaces and .'s
    // 
    while(*p)
    {
        if (isspace(*p)) *p = ' ';
        if (*p == '.') *p = ' ';
        p++;
    }
    
    p = szPgn;
    while(*p)
    {
        while(*p && isspace(*p)) p++;
        while(*p && (isdigit(*p) || (*p == '.'))) p++;
        if (*p == '[')
        {
            _ParsePgnHeaderTag(p);
            while(*p && *p != ']') p++;
            if (*p) p++;           
        }
        else if (*p == '{')
        {
            while(*p && *p != '}') p++;
            if (*p) p++;
        }
        else if (isalpha(*p))
        {
            //
            // Copy the thing we think is a move.
            //
            memset(szMove, 0, sizeof(szMove));
            x = 0;
            while(*p && (!isspace(*p)))
            {
                if (x < (ARRAY_LENGTH(szMove) - 1)) 
                {
                    szMove[x] = *p;
                }
                x++;
                p++;
            }
            
            switch (LooksLikeMove(szMove))
            {
                case MOVE_SAN:
                    mv = ParseMoveSan(szMove, pos);
                    break;
                case MOVE_ICS:
                    mv = ParseMoveIcs(szMove, pos);
                    break;
                default:
                    mv.uMove = 0;
                    break;
            }
                
            //if (FALSE == SanityCheckMove(pos, mv))
            // {
            //    Trace("Error in PGN(?)  Bad chunk \"%s\"\n", szMove);
            //    goto end;
            // }

            if (FALSE == OfficiallyMakeMove(mv, 0, TRUE))
            {
                Trace("Error in PGN(?)  Can't play move \"%s\"\n", szMove);
                goto end;
            }
            pos = GetRootPosition();
            uMoveCount++;
        }
        else
        {
            while(*p && !isspace(*p)) p++;
        }
    }
    fRet = TRUE;
 end:
    g_Options.fShouldPost = fOldPost;
    return(fRet);
}


char *
CleanupString(char *pIn)
/**

Routine description:

    Ckeans up a PGN tag containing part of the opening line
    name... gets rid of the quotes around the name and the newline.

Parameters:

    char *pIn

Return value:

    char

**/
{
    static char buf[256];
    char *p = pIn;
    char *q = buf;

    //
    // Skip leading whitespace in input
    //
    buf[0] = '\0';
    while(isspace(*p)) p++;
    if (*p == '\0') return(buf);

    do
    {
        //
        // Copy A-Z, 0-9, space, -, ., (, or )
        // 
        // Ignore others
        // 
        if ((*p == ' ') || 
            (isalpha(*p)) || 
            (isdigit(*p)) ||
            (*p == '-') ||
            (*p == '.') ||
            (*p == ',') ||
            (*p == '(') ||
            (*p == ')'))
        {
            *q = *p;
            q++;
            if ((q - buf) > (sizeof(buf) - 1)) break;
        }
        p++;
    }
    while(*p);

    *q = '\0';
    buf[255] = 0;
    return(buf);
}
