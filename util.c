/**

Copyright (c) Scott Gasch

Module Name:

    util.c

Abstract:

    General purpose utility code to support the engine.

Author:

    Scott Gasch (scott.gasch@gmail.com) 8 Apr 2004

Revision History:

    $Id: util.c 355 2008-07-01 15:46:43Z scott $

**/

#include "chess.h"

extern ULONG g_uIterateDepth;
extern ULONG g_uFullWidthDepth;

#define STATE_UNKNOWN  (0)
#define STATE_HEADER   (1)
#define STATE_MOVELIST (2)

CHAR *
ReadNextGameFromPgnFile(FILE *pf)
/**

Routine description:

    Reads the next game in a PGN file and returns a pointer to it.

Parameters:

    FILE *pf : open file pointer

Return value:

    CHAR * : NULL on error, else caller must free via SystemFreeMemory.

**/
{
    CHAR buf[MEDIUM_STRING_LEN_CHAR];
    CHAR *p;
    long lStartOfGame = -1;
    long lEndOfGame = -1;
    long lBefore;
    ULONG uBytes;
    ULONG uState = STATE_UNKNOWN;

    while(1)
    {
        lBefore = ftell(pf);
        if (fgets(buf, ARRAY_LENGTH(buf), pf) <= 0)
        {
            //Trace("fgets failed, errno=%u.\n", errno);
            break;
        }

        //
        // Skip leading whitespace on this line...
        //
        p = buf;
        while(*p && isspace(*p)) p++;

        if (*p == '\0')
        {
            //
            // We just read a blank line
            //
            switch(uState)
            {
                case STATE_UNKNOWN:
                    break;

                case STATE_HEADER:
                    uState = STATE_MOVELIST;
                    break;

                case STATE_MOVELIST:
                    lEndOfGame = lBefore;
                    uBytes = lEndOfGame - lStartOfGame;
                    if (uBytes <= 0)
                    {
                        return(NULL);
                    }

                    p = SystemAllocateMemory(uBytes + 1);
                    ASSERT(p);
                    fseek(pf, lStartOfGame, SEEK_SET);
                    if (1 != fread(p, uBytes, 1, pf))
                    {
                        SystemFreeMemory(p);
                        return(NULL);
                    }
                    p[uBytes] = '\0';
                    return(p);
            }
        }
        else
        {
            //
            // We saw a non-blank line
            //
            switch(uState)
            {
                case STATE_HEADER:
                case STATE_MOVELIST:
                    break;

                case STATE_UNKNOWN:
                    if (*p == '[')
                    {
                        uState = STATE_HEADER;
                        lStartOfGame = lBefore;
                    }
                    break;
            }
        }
    }
    return(NULL);
}


COMMAND(LearnPsqtFromPgn)
/**

Routine description:

Parameters:

    CHAR *szFilename,

Return value:

    FLAG

**/
{
    CHAR *szFilename;
    ULONG uPSQT[12][128];
    CHAR *pPGN;
    FILE *pf = NULL;
    DLIST_ENTRY *p;
    GAME_MOVE *q;
    ULONG x, y, z;
    double d;
    struct stat s;
#ifdef DEBUG
    ULONG uHeap;
#endif

    if (argc < 2)
    {
        Trace("Error (missing filename)\n");
        return;
    }
    szFilename = argv[1];

    if (FALSE == SystemDoesFileExist(szFilename))
    {
        Trace("LearnPsqtFromPgn: file \"%s\" doesn't exist.\n",
              szFilename);
        return;
    }

    if (0 != stat(szFilename, &s))
    {
        Trace("Stat failed.\n");
        return;
    }

    pf = fopen(szFilename, "rb");
    if (NULL == pf)
    {
        Trace("LearnPsqtFromPgn: error reading file \"%s\".\n",
              szFilename);
        return;
    }

    memset(uPSQT, 0, sizeof(uPSQT));
#ifdef DEBUG
    ResetGameList();
    uHeap = GetHeapMemoryUsage();
#endif
    while((pPGN = ReadNextGameFromPgnFile(pf)) != NULL)
    {
        d = (double)ftell(pf);
        d /= (double)s.st_size;
        d *= 100.0;
        printf("(%5.2f%% done)\r", d);

        if (TRUE == LoadPgn(pPGN))
        {
            p = g_GameData.sMoveList.pFlink;
            while(p != &(g_GameData.sMoveList))
            {
                q = CONTAINING_STRUCT(p, GAME_MOVE, links);
                if ((q->uNumber > 10) &&
                    (q->uNumber < 50) &&
                    (!q->mv.pCaptured))
                {
                    uPSQT[q->mv.pMoved - 2][q->mv.cTo] += 1;
                }
                p = p->pFlink;
            }
        }
        SystemFreeMemory(pPGN);
#ifdef DEBUG
        ResetGameList();
        ASSERT(uHeap == GetHeapMemoryUsage());
#endif
    }
    fclose(pf);

    z = 0;
    for (x = 0; x < 12; x++)
    {
        for (y = 0; y < 128; y++)
        {
            if (uPSQT[x][y] > z)
            {
                z = uPSQT[x][y];
            }
        }
    }

    Trace("ULONG g_PSQT[12][128] = {\n");
    for (x = 0; x < 12; x++)
    {
        Trace("// Piece 0x%x (%s)\n", x + 2, PieceAbbrev((PIECE)(x + 2)));
        Trace("{\n");

        for (y = 0; y < 128; y++)
        {
            if (y % 16 == 0) Trace("\n");
            d = (double)uPSQT[x][y];
            d /= (double)z;
            d *= (double)1000.0;
            Trace("%4u,", (ULONG)d);
        }
        Trace("},\n");
    }
    Trace("};\n");
}


COMMAND(GeneratePositionAndBestMoveSuite)
/**

Routine description:

Parameters:

    CHAR *szFilename,

Return value:

    FLAG

**/
{
    CHAR *szFilename;
    CHAR *pPGN;
    FILE *pf = NULL;
    DLIST_ENTRY *p;
    GAME_MOVE *q;
    SEARCHER_THREAD_CONTEXT *ctx = NULL;
    FLAG fPost = g_Options.fShouldPost;
    struct stat s;
    double d;
    POSITION board;

    if (argc < 2)
    {
        Trace("Error (missing filename)\n");
        goto end;
    }
    szFilename = argv[1];

    if (FALSE == SystemDoesFileExist(szFilename))
    {
        Trace("LearnPsqtFromPgn: file \"%s\" doesn't exist.\n",
              szFilename);
        goto end;
    }

    if (0 != stat(szFilename, &s))
    {
        Trace("Stat failed.\n");
        goto end;
    }

    pf = fopen(szFilename, "rb");
    if (NULL == pf)
    {
        Trace("LearnPsqtFromPgn: error reading file \"%s\".\n",
              szFilename);
        goto end;
    }

    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    if (NULL == ctx)
    {
        Trace("Out of memory.\n");
        goto end;
    }

    g_Options.fShouldPost = FALSE;
    while((pPGN = ReadNextGameFromPgnFile(pf)) != NULL)
    {
        d = (double)ftell(pf);
        d /= (double)s.st_size;
        d *= 100.0;
        printf("(%5.2f%% done)\r", d);

        if (TRUE == LoadPgn(pPGN))
        {
            p = g_GameData.sMoveList.pFlink;
            while(p != &(g_GameData.sMoveList))
            {
                q = CONTAINING_STRUCT(p, GAME_MOVE, links);
                if ((q->uNumber > 20) && (q->uNumber < 60))
                {
                    if (FenToPosition(&board, q->szUndoPositionFen))
                    {
                        InitializeSearcherContext(&board, ctx);
                        g_MoveTimer.bvFlags = 0;
                        g_Options.fPondering = FALSE;
                        g_Options.fThinking = TRUE;
                        g_Options.fSuccessfulPonder = FALSE;

                        MaintainDynamicMoveOrdering();
                        DirtyHashTable();

                        g_MoveTimer.uNodeCheckMask = 0x1000 - 1;
                        g_MoveTimer.dStartTime = SystemTimeStamp();
                        g_MoveTimer.bvFlags = 0;
                        g_MoveTimer.dSoftTimeLimit =
                            g_MoveTimer.dStartTime + 3.0;
                        g_MoveTimer.dHardTimeLimit =
                            g_MoveTimer.dStartTime + 3.0;
                        g_Options.uMaxDepth = MAX_DEPTH_PER_SEARCH - 1;

                        //
                        // TODO: Set draw value
                        //
#if (PERF_COUNTERS && MP)
                        ClearHelperThreadIdleness();
#endif
                        GAME_RESULT result = Iterate(ctx);
                        if (result.eResult == RESULT_IN_PROGRESS) {
                            ASSERT(SanityCheckMove(&board,
                                                   ctx->sPlyInfo[0].PV[0]));
                            Trace("setboard %s\n", q->szUndoPositionFen);
                            Trace("gmmove %s\n", q->szMoveInSan);
                            Trace("solution %s\n",
                                  MoveToSan(ctx->sPlyInfo[0].PV[0], &board));
                        }
                    }
                }
                p = p->pFlink;
            }
        }
        SystemFreeMemory(pPGN);
    }

 end:
    if (NULL != pf)
    {
        fclose(pf);
    }

    if (NULL != ctx)
    {
        SystemFreeMemory(ctx);
    }

    g_Options.fShouldPost = fPost;
}


char *
FindChunk(char *sz, ULONG uTargetChunk)
/**

Routine description:

    A support routine used by FenToPosition.  Used to parse apart a
    string and return the Nth chunk of text (delimited by whitespace)

Parameters:

    char *sz,
    ULONG uTargetChunk

Return value:

    char *

**/
{
    char *p = sz;
    ULONG u = 0;

    if (0 == uTargetChunk) return(p);

    do
    {
        //
        // Skip whitespace between chunks
        //
        while(isspace(*p) && *p) p++;
        if (!*p) return(NULL);

        u++;
        if (u == uTargetChunk) return(p);

        //
        // Skip chunks
        //
        while(!isspace(*p) && &p) p++;
        if (!*p) return(NULL);
    }
    while(1);

#ifdef DEBUG
    UtilPanic(SHOULD_NOT_GET_HERE,
              NULL, NULL, NULL, NULL,
              __FILE__, __LINE__);
#endif
    return(NULL);
}



CHAR *
ScoreToString(SCORE iScore)
/**

Routine description:

    Format a chess score as a string that looks like:

        +0.100
        -4.951
        +MATE20
        -MATE4
        +0.000

    etc...

    Note: NOT thread safe.

Parameters:

    SCORE iScore

Return value:

    CHAR *

**/
{
    static char szBuf[10];
    ULONG uDist;

    if (abs(iScore) < +NMATE)
    {
        snprintf(szBuf, ARRAY_LENGTH(szBuf),
                 "%c%u.%02u",
                 (iScore < 0) ? '-' : '+',
                 abs(iScore / 100),
                 abs(iScore % 100));
        return(szBuf);
    }

    if (iScore > 0)
    {
        uDist = +INFINITY - iScore;
        uDist /= 2;
        if (uDist)
        {
            snprintf(szBuf, ARRAY_LENGTH(szBuf), "+MATE%u", uDist);
        }
        else
        {
            snprintf(szBuf, ARRAY_LENGTH(szBuf), "+MATE");
        }
    }
    else
    {
        uDist = +INFINITY + iScore;
        uDist /= 2;
        if (uDist)
        {
            snprintf(szBuf, ARRAY_LENGTH(szBuf), "-MATE%u", uDist);
        }
        else
        {
            snprintf(szBuf, ARRAY_LENGTH(szBuf), "-MATE");
        }
    }
    return(szBuf);
}



CHAR *
CoorToString(COOR c)
/**

Routine description:

    Take a chessboard coordinate and convert it into a string.  Note:
    NOT thread safe.

Parameters:

    COOR c

Return value:

    CHAR *

**/
{
    static char buf[3];

    if (IS_ON_BOARD(c))
    {
        buf[0] = 'a' + FILE(c);
        buf[1] = '0' + RANK(c);
        buf[2] = '\0';
    }
    else
    {
        buf[0] = '-';
        buf[1] = '\0';
    }
    return(buf);
}


CHAR *
ColorToString(ULONG u)
/**

Routine description:

    Convert a color into a string ("WHITE" or "BLACK").  Callers
    should not mess with the contents of the buffer returned!

Parameters:

    ULONG u

Return value:

    char

**/
{
    static CHAR *szColor[2] =
    {
        "BLACK",
        "WHITE"
    };
    ASSERT(IS_VALID_COLOR(u));
    return(szColor[u]);
}


CHAR *
TimeToString(double d)
/**

Routine description:

    Convert a time value into a string.  e.g.

        00:00:01.021                          // seconds with ms
        00:15:21.998                          // minutes+seconds
        01:59:23.232                          // hours+minutes+sec
        27:08:11:103                          // we don't do days

    Note: NOT thread safe.  The caller should not mess with the
    buffer returned.

Parameters:

    double d

Return value:

    CHAR *

**/
{
    ULONG uTotalSecs = (ULONG)d;
    double dFract = d - (double)uTotalSecs;
    ULONG uHours;
    ULONG uMinutes;
    ULONG uFract;
    static char buf[13];

    dFract *= 1000;
    uFract = (int)dFract;
    uHours = uTotalSecs / 60 / 60;
    uTotalSecs -= uHours * 60 * 60;
    uMinutes = uTotalSecs / 60;
    uTotalSecs -= uMinutes * 60;
    ASSERT(uTotalSecs < 60);

    if (uHours != 0)
    {
        snprintf(buf, ARRAY_LENGTH(buf),
                 "%2u:%02u:%02u.%03u",
                 uHours, uMinutes, uTotalSecs, uFract);
    }
    else
    {
        snprintf(buf, ARRAY_LENGTH(buf),
                 "%02u:%02u.%03u",
                 uMinutes, uTotalSecs, uFract);
    }
    return(buf);
}


FLAG
BufferIsZeroed(BYTE *p, ULONG u)
/**

Routine description:

    Verify that a buffer is zeroed out.

Parameters:

    BYTE *p : buffer start
    ULONG u : buffer length

Return value:

    FLAG

**/
{
    while(u)
    {
        if (*p != (BYTE)0)
        {
            return(FALSE);
        }
        p++;
        u--;
    }
    return(TRUE);
}


void CDECL
Trace(CHAR *szMessage, ...)
/**

Routine description:

    Trace a logging message on stdout and to the logfile

Parameters:

    CHAR *szMessage,
    ...

Return value:

    void

**/
{
    va_list ap;                            // Variable argument list
    CHAR buf[MEDIUM_STRING_LEN_CHAR];

    memset(buf, 0, sizeof(buf));

    //
    // Populate buffer
    //
    va_start(ap, szMessage);
    COMPILER_VSNPRINTF(buf, MEDIUM_STRING_LEN_CHAR - 1, szMessage, ap);
    va_end(ap);

    //
    // Send to logfile
    //
    if (NULL != g_pfLogfile) {
        fprintf(g_pfLogfile, buf);
    }

    //
    // Send to stdout
    //
    fprintf(stdout, buf);
    fflush(stdout);
}


void CDECL
Log(CHAR *szMessage, ...)
/**

Routine description:

    Trace a logging message to the logfile only

Parameters:

    CHAR *szMessage,
    ...

Return value:

    void

**/
{
    va_list ap;                            // Variable argument list
    CHAR buf[MEDIUM_STRING_LEN_CHAR];
    memset(buf, 0, sizeof(buf));

    //
    // Populate buffer
    //
    va_start(ap, szMessage);
    COMPILER_VSNPRINTF(buf, MEDIUM_STRING_LEN_CHAR - 1, szMessage, ap);
    va_end(ap);

    //
    // Send to logfile
    //
    if (NULL != g_pfLogfile) {
        fprintf(g_pfLogfile, buf);
        fflush(g_pfLogfile);
    }
}


void CDECL
Bug(CHAR *szMessage, ...)
/**

Routine description:

    Trace an error message to stderr and the logfile

Parameters:

    CHAR *szMessage,
    ...

Return value:

    void

**/
{
    va_list ap;                            // Variable argument list
    CHAR buf[MEDIUM_STRING_LEN_CHAR];

    memset(buf, 0, sizeof(buf));

    //
    // Populate buffer
    //
    va_start(ap, szMessage);
    COMPILER_VSNPRINTF(buf, MEDIUM_STRING_LEN_CHAR - 1, szMessage, ap);
    va_end(ap);

    //
    // Send it to logfile
    //
    if (NULL != g_pfLogfile) {
        fprintf(g_pfLogfile, buf);
        fflush(g_pfLogfile);
    }

    //
    // Send it to stderr
    //
    fprintf(stderr, buf);
    fflush(stderr);
}


#ifdef DEBUG
void
_assert(CHAR *szFile, ULONG uLine)
/**

Routine description:

    An assertion handler

Parameters:

    CHAR *szFile,
    UINT uLine

Return value:

    void

**/
{
    Bug("Assertion failure in %s, line %u.\n", szFile, uLine);
    BREAKPOINT;
}
#endif


ULONG
AcquireSpinLock(volatile ULONG *pSpinlock)
/**

Routine description:

    Atomically acquire a lock or spin trying.

Parameters:

    ULONG *pSpinlock : a pointer to the lock

Return value:

    ULONG : the number of times we had to spin

**/
{
    ULONG uLoops = 0;
    while(0 != LockCompareExchange(pSpinlock, 1, 0))
    {
        uLoops++;
    }
    ASSERT(*pSpinlock == 1);
    return(uLoops);
}

FLAG
TryAcquireSpinLock(volatile ULONG *pSpinlock)
{
    return(0 != LockCompareExchange(pSpinlock, 1, 0));
}

void
ReleaseSpinLock(volatile ULONG *pSpinlock)
/**

Routine description:

    Release a lock so someone else can acquire it.

Parameters:

    ULONG *pSpinlock : a pointer to the lock

Return value:

    void

**/
{
    ULONG u;
    ASSERT(*pSpinlock == 1);
    u = LockCompareExchange(pSpinlock, 0, 1);
    ASSERT(u == 1);
}


FLAG
BackupFile(CHAR *szFile)
/**

Routine description:

    Backup a file to file.000.  If file.000 already exists, back it up
    to file.001 etc...

    Note: this function is recursive and can require quite a lot of
    stack space.  Also it is full of race conditions and should not be
    used when some other code may be accessing the filesystem at the
    same time.

Parameters:

    CHAR *szFile : the file to backup

Return value:

    FLAG

**/
{
    ULONG u;
    CHAR buf[SMALL_STRING_LEN_CHAR];
    CHAR *p;

    if (TRUE == SystemDoesFileExist(szFile))
    {
        if ((strlen(szFile) > 3) &&
            (isdigit(szFile[0])) &&
            (isdigit(szFile[1])) &&
            (isdigit(szFile[2])))
        {
            u = (szFile[0] - '0') * 100;
            u += (szFile[1] - '0') * 10;
            u += szFile[2] - '0' + 1;
            p = &(szFile[3]);
        }
        else
        {
            u = 0;
            p = szFile;
        }
        snprintf(buf, ARRAY_LENGTH(buf), "%03u%s", u, p);
        if (TRUE == SystemDoesFileExist(buf))
        {
            BackupFile(buf);
        }
        return(SystemCopyFile(szFile, buf) && SystemDeleteFile(szFile));
    }
    return(FALSE);
}


BYTE
Checksum(BYTE *p, ULONG uSize)
/**

Routine description:

    Compute the checksum of a buffer.

Parameters:

    BYTE *p,
    ULONG uSize

Return value:

    BYTE

**/
{
    BYTE b = 0;
    while(uSize)
    {
        b ^= *p;
        p++;
        uSize--;
    }
    return(b);
}

void
FinishPVTailFromHash(SEARCHER_THREAD_CONTEXT *ctx,
                     CHAR *buf,
                     ULONG uLenRemain)
{
    MOVE PV[MAX_PLY_PER_SEARCH];
    MOVE mv;
    ULONG uPly = ctx->uPly;
    ULONG uLen;
#ifdef DEBUG
    POSITION board;
    memcpy(&board, &(ctx->sPosition), sizeof(POSITION));
#endif

    if (NULL == g_pHashTable) return;
    do
    {
        mv = GetPonderMove(&ctx->sPosition);
        if (mv.uMove == 0) break;
        PV[ctx->uPly] = mv;

        uLen = (ULONG)strlen(MoveToSan(mv, &ctx->sPosition)) + 1;
        ASSERT(uLen <= 11);
        if (uLenRemain <= uLen) break;
        strcat(buf, MoveToSan(PV[ctx->uPly], &ctx->sPosition));
        strcat(buf, " ");
        uLenRemain -= uLen;

        if (FALSE == MakeMove(ctx, mv)) break;
    }
    while(1);

    //
    // Unmake the moves
    //
    while(ctx->uPly > uPly)
    {
        UnmakeMove(ctx, PV[ctx->uPly - 1]);
    }
    ASSERT(ctx->uPly == uPly);
    ASSERT(PositionsAreEquivalent(&board, &ctx->sPosition));
}



char *
WalkPV(SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    Return a string buffer representing the principle variation.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    CHAR *

**/
{
    static CHAR buf[SMALL_STRING_LEN_CHAR];
    ULONG u = ctx->uPly;
    ULONG v = u;
    MOVE mv;
#ifdef DEBUG
    POSITION board;
    memcpy(&board, &(ctx->sPosition), sizeof(POSITION));
#endif

    buf[0] = '\0';
    while((mv.uMove = ctx->sPlyInfo[v].PV[u].uMove) != 0)
    {
        if (mv.uMove == HASHMOVE.uMove)
        {
            strncat(buf, "<TT> ", ARRAY_LENGTH(buf) - strlen(buf) - 1);
            FinishPVTailFromHash(ctx, buf, ARRAY_LENGTH(buf) - strlen(buf));
            ASSERT(ctx->sPlyInfo[v].PV[u+1].uMove == 0);
            break;
        }
        else if (mv.uMove == RECOGNMOVE.uMove)
        {
            strncat(buf, "<REC>", ARRAY_LENGTH(buf) - strlen(buf) - 1);
            ASSERT(ctx->sPlyInfo[v].PV[u+1].uMove == 0);
            break;
        }
        else if (mv.uMove == DRAWMOVE.uMove)
        {
            strncat(buf, "<=>", ARRAY_LENGTH(buf) - strlen(buf) - 1);
            ASSERT(ctx->sPlyInfo[v].PV[u+1].uMove == 0);
            break;
        }
        else
        {
            strncat(buf, MoveToSan(mv, &ctx->sPosition),
                    ARRAY_LENGTH(buf) - strlen(buf));
            strncat(buf, " ", ARRAY_LENGTH(buf) - strlen(buf) - 1);
        }

        if (FALSE == MakeMove(ctx, mv)) break;
        u++;

        if (strlen(buf) > (SMALL_STRING_LEN_CHAR - 10)) break;
    }

    //
    // Unmake the moves
    //
    u--;
    while(ctx->uPly > v)
    {
        UnmakeMove(ctx, ctx->sPlyInfo[v].PV[u]);
        u--;
    }
    ASSERT(ctx->uPly == v);
    ASSERT(PositionsAreEquivalent(&board, &ctx->sPosition));
    return(buf);
}


void
UtilPanic(ULONG uPanicCode,
          POSITION *pos,
          void *arg1, void *arg2, void *arg3,
          char *szFile, ULONG uLine)
{
    Bug("Typhoon PANIC 0x%x (%p, %p, %p, %p)\n"
        "Location: %s at line %u\n\n",
        uPanicCode, pos, arg1, arg2, arg3, szFile, uLine);
    Banner();
    Bug("----------------------------------------------------------------\n");
    switch(uPanicCode) {
        case CANNOT_INITIALIZE_SPLIT:
        case GOT_ILLEGAL_MOVE_WHILE_PONDERING:
        case CANNOT_OFFICIALLY_MAKE_MOVE:
            DumpPosition(pos);
            DumpMove((ULONG)arg1);
            break;
        case INITIALIZATION_FAILURE:
            Bug("%s\n", (char *)arg1);
            break;
        default:
            break;
    }
    fflush(stdout);
    fflush(g_pfLogfile);
    BREAKPOINT;
    exit(-1);
}

void
UtilPrintPV(SEARCHER_THREAD_CONTEXT *ctx,
            SCORE iAlpha,
            SCORE iBeta,
            SCORE iScore,
            MOVE mv)
/**

Routine description:

    Print a PV out to stdout and the log.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    SCORE iAlpha,
    SCORE iBeta,
    SCORE iScore,
    MOVE mv

Return value:

    void

**/
{
    double dNow = SystemTimeStamp();

    ASSERT(ctx->uThreadNumber == 0);
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT(IS_VALID_SCORE(iScore));

    dNow -= g_MoveTimer.dStartTime;

    //
    // Set the last PV in the context if we have a root PV or a root
    // fail high.
    //
    if ((iAlpha < iScore) && (iScore < iBeta))
    {
        strncpy(ctx->szLastPV, WalkPV(ctx), SMALL_STRING_LEN_CHAR);
    }
    else if (iScore > iBeta)
    {
        strncpy(ctx->szLastPV, MoveToSan(mv, &(ctx->sPosition)),
                SMALL_STRING_LEN_CHAR);
    }

    if ((TRUE == g_Options.fThinking) && (TRUE == g_Options.fShouldPost))
    {
        //
        // Maybe output the PV.  Note: xboard gets confused by PV
        // lines that don't match its requirements exactly; if we are
        // running under xboard, match its expected format.  Otherwise
        // be more helpful / verbose.
        //
        if ((dNow > 0.35) || ((dNow > 0.15) && (g_Options.fVerbosePosting)))
        {
            if (TRUE == g_Options.fRunningUnderXboard)
            {
                if ((iAlpha < iScore) && (iScore < iBeta))
                {
                    Trace("%2u %6d %5u %-12"
                          COMPILER_LONGLONG_UNSIGNED_FORMAT" %s\n",
                          g_uIterateDepth,
                          iScore,
                          (ULONG)(dNow * 100),
                          ctx->sCounters.tree.u64TotalNodeCount,
                          ctx->szLastPV);
                }
                else if (iScore <= iAlpha)
                {
                    Trace("%2u %6d %5u %-12"
                          COMPILER_LONGLONG_UNSIGNED_FORMAT" FL --\n",
                          g_uIterateDepth,
                          iScore,
                          (ULONG)(dNow * 100),
                          ctx->sCounters.tree.u64TotalNodeCount);
                }
                else
                {
                    ASSERT(iScore >= iBeta);
                    Trace("%2u %6d %5u %-12"
                          COMPILER_LONGLONG_UNSIGNED_FORMAT" %s ++\n",
                          g_uIterateDepth,
                          iScore,
                          (ULONG)(dNow * 100),
                          ctx->sCounters.tree.u64TotalNodeCount,
                          ctx->szLastPV);
                }
            }
            else // !running under xboard
            {
                if ((iAlpha < iScore) && (iScore < iBeta))
                {
                    Trace("%2u  %5s  %-9s  %-11"
                          COMPILER_LONGLONG_UNSIGNED_FORMAT"  %s\n",
                          g_uIterateDepth,
                          ScoreToString(iScore),
                          TimeToString(dNow),
                          ctx->sCounters.tree.u64TotalNodeCount,
                          ctx->szLastPV);
                }
                else if (iScore <= iAlpha)
                {
                    Trace("%2u- %5s  %-9s  %-11"
                          COMPILER_LONGLONG_UNSIGNED_FORMAT"  FL --\n",
                          g_uIterateDepth,
                          ScoreToString(iScore),
                          TimeToString(dNow),
                          ctx->sCounters.tree.u64TotalNodeCount);
                }
                else
                {
                    ASSERT(iScore >= iBeta);
                    Trace("%2u+ %5s  %-9s  %-11"
                          COMPILER_LONGLONG_UNSIGNED_FORMAT"  %s ++\n",
                          g_uIterateDepth,
                          ScoreToString(iScore),
                          TimeToString(dNow),
                          ctx->sCounters.tree.u64TotalNodeCount,
                          ctx->szLastPV);
                }
            }
        }
    }
}
