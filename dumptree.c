/**

Copyright (c) Scott Gasch

Module Name:

    dumptree.c

Abstract:

    Support routines for dumping a search tree into an XML file for
    offline analysis by typhoonui.exe.

Author:

    Scott Gasch (scott.gasch@gmail.com) 17 Jul 2004

Revision History:

**/

#ifdef DUMP_TREE
#include "chess.h"

#define NHUMAN_READABLE
#ifdef HUMAN_READABLE
#define SPACES_TO_INDENT_PER_PLY (2)
#else
#define SPACES_TO_INDENT_PER_PLY (0)
#endif
#define DISK_SECTOR_SIZE_BYTES   (512)
#define NUM_SECTORS_TO_BUFFER    (2)

FILE *pfTreeDump = NULL;
ULONG g_uBufferSize = 0;
ULONG g_uBufferUsed = 0;
CHAR *g_Buffer = NULL;
#ifdef MP
volatile static ULONG g_uBufferLock = 0;
#define BUF_IS_LOCKED (g_uBufferLock != 0)
#define LOCK_BUF \
    AcquireSpinLock(&g_uBufferLock); \
    ASSERT(BUF_IS_LOCKED);
#define UNLOCK_BUF \
    ASSERT(BUF_IS_LOCKED); \
    ReleaseSpinLock(&g_uBufferLock);
#else // no MP
#define BUF_IS_LOCKED
#define LOCK_BUF
#define UNLOCK_BUF
#endif

void 
InitializeTreeDump(void)
/**

Routine description:

    Initialize tree dumping system.

Parameters:

    void

Return value:

    void

**/
{
    //
    // Allocate an internal buffer so that our writes to disk
    // approximate a sector size.
    //
    g_uBufferSize = DISK_SECTOR_SIZE_BYTES * NUM_SECTORS_TO_BUFFER;
    g_Buffer = SystemAllocateMemory(g_uBufferSize);

    //
    // Here's the file we'll want to write to...
    //
    pfTreeDump = fopen("typhoon.xml", "wb+");
    if (NULL == pfTreeDump)
    {
        UtilPanic(FATAL_ACCESS_DENIED,
                  NULL, "typhoon.xml", "write", "NULL", __FILE__, __LINE__);
    }
    fprintf(pfTreeDump, "<?xml version=\"1.0\" encoding=\"utf-8\""
            " standalone=\"no\"?>\n");
}

static 
void _FlushTheBuffer(void)
/**

Routine description:

    Flush the buffer to the disk file.

Parameters:

    void

Return value:

    void

**/
{
    fprintf(pfTreeDump, g_Buffer);
    g_Buffer[0] = '\0';
    g_uBufferUsed = 0;
}


static void CDECL 
_AppendToBuffer(CHAR *buf)
/**

Routine description:

    Append a string to the buffer; possibly flush the buffer to disk
    first to make enough room for the new string.

Parameters:

    CHAR *buf : the string to append

Return value:

    void

**/
{
    ULONG uSpaceNeeded;
    ASSERT(BUF_IS_LOCKED);
    
    //
    // How much space do we need?
    //
    uSpaceNeeded = strlen(buf);
    ASSERT(uSpaceNeeded < MEDIUM_STRING_LEN_CHAR);
    ASSERT(uSpaceNeeded < g_uBufferSize);
    
    //
    // See if we need to flush the real buffer to the disk before we can
    // append this new message.
    // 
    LOCK_BUF;
    if (g_uBufferUsed + uSpaceNeeded >= g_uBufferSize)
    {
        _FlushTheBuffer();
        ASSERT(g_uBufferUsed + uSpaceNeeded < g_uBufferSize);
    }
    
    //
    // Append the new data to the big buffer
    //
    strcat(g_Buffer, buf);
    g_uBufferUsed += uSpaceNeeded;
    ASSERT(g_uBufferUsed < g_uBufferSize);
    UNLOCK_BUF;
}


static void _IndentTreeDumpMessage(ULONG uPly)
/**

Routine description:

    Append a number of spaces to the buffer to indent the next output
    appropriately based upon the current search ply.

Parameters:

    ULONG uPly : the current search ply

Return value:

    void

**/
{
    ULONG uSpaces = uPly * SPACES_TO_INDENT_PER_PLY;
    ASSERT(uSpaces < g_uBufferSizeBytes);
    ASSERT(HUMAN_READABLE);
    while(uSpaces)
    {
        _AppendToBuffer(" ");
        uSpaces--;
    }
}


void CDECL 
DTTrace(ULONG uPly, CHAR *szMessage, ...)
/**

Routine description:

    A printf-like routine for adding a message to the buffer / dumpfile.
    Note: messages that exceed MEDIUM_STRING_LEN_CHAR are truncated.

Parameters:

    ULONG uPly : the current search ply
    CHAR *szMessage : the message format string
    ... : variable number of interpolated arguments

Return value:

    void 

**/
{
    va_list ap;
    CHAR buf[MEDIUM_STRING_LEN_CHAR];
    buf[0] = '\0';

    //
    // Populate the local buffer
    //
    va_start(ap, szMessage);
    COMPILER_VSNPRINTF(buf, MEDIUM_STRING_LEN_CHAR - 2, szMessage, ap);
    va_end(ap);
#ifdef HUMAN_READABLE
    strcat(buf, "\n");
    _IndentTreeDumpMessage(uPly);
#endif
    _AppendToBuffer(buf);
}


void 
CleanupTreeDump(void)
/**

Routine description:

    Tear down the tree dumping system

Parameters:

    void

Return value:

    void

**/
{
    //
    // Cleanup file and buffer
    //
    if (NULL != pfTreeDump)
    {
        _FlushTheBuffer();
        fflush(pfTreeDump);
        fclose(pfTreeDump);
    }

    if (NULL != g_Buffer)
    {
        SystemFreeMemory(g_Buffer);
        g_Buffer = NULL;
        g_uBufferSize = 0;
    }
}


void 
DTEnterNode(SEARCHER_THREAD_CONTEXT *ctx,
            ULONG uDepth, 
            FLAG fIsQNode,
            SCORE iAlpha, 
            SCORE iBeta)
/**

Routine description:

    Called by search or qsearch upon entering a new node.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : searcher context
    ULONG uDepth : depth
    FLAG fIsQNode : is it a q node?
    SCORE iAlpha : current alpha
    SCORE iBeta : current beta

Return value:

    void

**/
{
    ULONG uPly = ctx->uPly;
    UINT64 u64NodeNum = ctx->sCounters.tree.u64TotalNodeCount;
    CHAR *p;
    
    if (FALSE == fIsQNode)
    {
        DTTrace(uPly, "<n n=\"%"
                COMPILER_LONGLONG_UNSIGNED_FORMAT 
                "\">", u64NodeNum);
    }
    else
    {
        DTTrace(uPly, "<qn n=\"%" 
                COMPILER_LONGLONG_UNSIGNED_FORMAT 
                "\">", u64NodeNum);
        p = PositionToFen(&ctx->sPosition);
        ASSERT(p);
        DTTrace(uPly + 1, "<fen p=\"%s\"/>", p);
        SystemFreeMemory(p);
        DTTrace(uPly + 1, "<i ab=\"%d, %d\" ply=\"%u\" depth=\"%u\"/>",
                iAlpha, iBeta, uPly, uDepth);
    }
}


void 
DTLeaveNode(SEARCHER_THREAD_CONTEXT *ctx, 
            FLAG fQNode,
            SCORE iBestScore,
            MOVE mvBestMove)
/**

Routine description:

    Called by the search upon leaving a normal (non q) node.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : searcher context
    SCORE iBestScore : best score found
    MOVE mvBestMove : best move found

Return value:

    void

**/
{
    ULONG uPly = ctx->uPly;
    
    DTTrace(uPly + 1, "<d score=\"%d\" mv=\"%s\"/>",
            iBestScore, MoveToSan(mvBestMove, &ctx->sPosition));
    if (!fQNode) 
    {
        DTTrace(uPly, "</n>");
    }
    else
    {
        DTTrace(uPly, "</qn>");
    }
}
#endif
