/**

Copyright (c) Scott Gasch

Module Name:

    input.c

Abstract:

    This module implements an input thread dedicated to watching for
    user input on stdin and maintaining a queue of input events for
    consumption by the main engine thread.  Currently only the main
    engine thread (and not helper threads) may consume user input
    because: 1. I see no performance reason to make all threads able
    to consume input and 2. it makes the locking assumptions easier.

Author:

    Scott Gasch (scott.gasch@gmail.com) 14 May 2004

Revision History:

    $Id: input.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"

DLIST_ENTRY g_InputEventList;
volatile static ULONG g_uInputLock;
extern CHAR g_szInitialCommand[SMALL_STRING_LEN_CHAR];

#define INPUT_IS_LOCKED (g_uInputLock != 0)
#define LOCK_INPUT \
    AcquireSpinLock(&g_uInputLock); \
    ASSERT(INPUT_IS_LOCKED);
#define UNLOCK_INPUT \
    ASSERT(INPUT_IS_LOCKED); \
    ReleaseSpinLock(&g_uInputLock); 
volatile FLAG g_fExitProgram = FALSE;
volatile ULONG g_uNumInputEvents;

typedef struct _INPUT_EVENT
{
    DLIST_ENTRY links;
    CHAR *szInput;
}
INPUT_EVENT;

ULONG g_uBlockingInputLock = 0;

static void 
_WaitUntilTheresInputToRead(void) 
{
    if (g_uBlockingInputLock != (ULONG)-1) 
    {
        SystemObtainSemaphoreResource(g_uBlockingInputLock);
        SystemReleaseSemaphoreResource(g_uBlockingInputLock);
    }
    else 
    {
        SystemDeferExecution(100);
    }
}


static void 
_UnblockBlockedReaders(void) 
{
    if (g_uBlockingInputLock != (ULONG)-1)
    {
        SystemReleaseSemaphoreResource(g_uBlockingInputLock);
    }
}


static void 
_BlockBlockingReaders(void) 
{
    if (g_uBlockingInputLock != (ULONG)-1)
    {
        SystemObtainSemaphoreResource(g_uBlockingInputLock);
    }
}


void 
PushNewInput(CHAR *buf)
/**

Routine description:

    Allocate a new input event for some user input and push it onto
    the queue.

Parameters:

    CHAR *buf

Return value:

    void

**/
{
    INPUT_EVENT *pEvent;
    FLAG fDone = FALSE;
    FLAG fInQuote, fSemi;
    CHAR *p;
    CHAR *q;
    
    do
    {
        pEvent = (INPUT_EVENT *)SystemAllocateMemory(sizeof(INPUT_EVENT));
        
        // 
        // Note: you can specify more than one command per line using
        // a semi-colon to separate them.  If the semi-colon is quoted,
        // though, ignore it.
        //
        fInQuote = FALSE;
        fSemi = FALSE;
        p = buf;
        do
        {
            switch(*p)
            {
                case ';':
                    if (FALSE == fInQuote)
                    {
                        *p = '\0';
                        fSemi = TRUE;
                    }
                    else 
                    {
                        p++;
                    }
                    break;
                case '"':
                    fInQuote = FLIP(fInQuote);
                    p++;
                    break;
                case '\0':
                    fDone = TRUE;
                    break;
                default:
                    p++;
                    break;
            }
        }
        while((FALSE == fDone) && (FALSE == fSemi));
        pEvent->szInput = STRDUP(buf);
        
        //
        // Push it onto the queue
        //
        LOCK_INPUT;
        InsertTailList(&g_InputEventList, &(pEvent->links));
        g_uNumInputEvents++;
        if (g_uNumInputEvents == 1) 
        {
            _UnblockBlockedReaders();
        }
        q = pEvent->szInput;
        ASSERT(q);
        while(*q && isspace(*q)) q++;
#ifdef DEBUG
        Trace("INPUT THREAD SAW (event %u): %s", 
              g_uNumInputEvents, 
              pEvent->szInput);
#endif
        if (!STRNCMPI("quit", q, 4)) 
        {
            g_fExitProgram = TRUE;
            UNLOCK_INPUT;
            return;
        }
        UNLOCK_INPUT;

        buf = (p + 1);
    }
    while(FALSE == fDone);
}


static void 
_InitInputSystemCommonCode(void)
/**

Routine description:

    Common code for initializing the input system (called whether we
    are spawing an input thread or running in batch mode with no input
    thread).

Parameters:

    void

Return value:

    void

**/
{
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    setbuf(stderr, NULL);
    
    g_uBlockingInputLock = SystemCreateSemaphore(1);
    if (g_uBlockingInputLock == (ULONG)-1) 
    {
        Bug("_InitInputSystemCommonCode: Failed to create input semaphore.\n");
    }
    g_uNumInputEvents = 0;
    _BlockBlockingReaders();
    InitializeListHead(&g_InputEventList);
    g_uInputLock = 0;
    if (g_szInitialCommand[0] != '\0')
    {
        Trace("INPUT SYSTEM INIT: Pushing \"%s\"\n", g_szInitialCommand);
        PushNewInput(g_szInitialCommand);
        g_szInitialCommand[0] = '\0';
    }
}



void 
InitInputSystemInBatchMode(void)
/**

Routine description:

    User specified --batch on the cmdline... so don't spawn an input
    thread.  Just process the initial command.

Parameters:

    void

Return value:

    void

**/
{
    if (g_szInitialCommand[0] == '\0')
    {
        UtilPanic(INCONSISTENT_STATE, 
                  NULL, "Batch mode specified with no initial command",
                  NULL, NULL, 
                  __FILE__, __LINE__);
    }
    _InitInputSystemCommonCode();
}
    

#ifdef USE_READLINE
char *readline(const char *prompt);
void add_history(const char *line);
#endif

ULONG 
InputThreadEntry(ULONG uUnused)
/**

Routine description:

    The entry point of the input thread.

Parameters:

    ULONG uUnused

Return value:

    ULONG

**/
{
    static CHAR buf[SMALL_STRING_LEN_CHAR];
    INPUT_EVENT *pEvent;
    DLIST_ENTRY *p;
    FLAG fFailure;
#ifdef USE_READLINE
    CHAR *pReadline = NULL;
#endif

    while(TRUE != g_fExitProgram)
    {
        //
        // Get another input
        //
        fFailure = FALSE;
#ifdef USE_READLINE
        memset(buf, 0, sizeof(buf));
        pReadline = readline(NULL);
        if (pReadline != NULL) 
        {
            strncpy(buf, pReadline, sizeof(buf) - 1);
            add_history(pReadline);
            free(pReadline);
        }
        else
        {
            fFailure = TRUE;
        }
#else
        if (NULL == fgets(buf, sizeof(buf), stdin))
        {
            fFailure = TRUE;
        }
#endif
        if (fFailure == TRUE) 
        {
            Trace("INPUT THREAD: got end-of-file on stdin, exiting.\n");
            break;
        }

        //
        // And push it onto the queue
        //
        PushNewInput(buf);
    }
    Trace("INPUT THREAD: thread terminating.\n");
    
    //
    // Free the queue
    //
    while(!IsListEmpty(&g_InputEventList))
    {
        p = RemoveHeadList(&g_InputEventList);
        pEvent = CONTAINING_STRUCT(p, INPUT_EVENT, links);
        SystemFreeMemory(pEvent->szInput);
        SystemFreeMemory(pEvent);
    }
    g_uNumInputEvents = (ULONG)-1;
    return(0);
}


ULONG 
InitInputSystemWithDedicatedThread(void)
/**

Routine description:

    Initialize the input system with a dedicated input thread.

Parameters:

    void

Return value:

    ULONG

**/
{
    ULONG uHandle = -1;

    _InitInputSystemCommonCode();
    if (FALSE == SystemCreateThread(InputThreadEntry, 
                                    0, 
                                    &uHandle))
    {
        Bug("Failed to start input thread!\n");
        uHandle = -1;
    }
    return(uHandle);
}


CHAR *
PeekNextInput(void)
/**

Routine description:

    Peek at what the next input event will be without consuming it.
    Because only one thread can be consuming events from the queue at
    a time (presumably the same thread that called this Peek function)
    it should be safe to do this without a lock.  But the overhead is
    low so I lock here anyway.

Parameters:

    void

Return value:

    CHAR *

**/
{
    INPUT_EVENT *p;
    char *q = NULL;

    LOCK_INPUT;
    if (!IsListEmpty(&g_InputEventList))
    {
        p = CONTAINING_STRUCT(g_InputEventList.pFlink, INPUT_EVENT, links);
        q = p->szInput;
    }
    UNLOCK_INPUT;
    return(q);
}


CHAR *
ReadNextInput(void)
/**

Routine description:

    Read the next input (and dequeue it).

Parameters:

    void

Return value:

    CHAR *

**/
{
    CHAR *pRet = NULL;
    INPUT_EVENT *p;
    DLIST_ENTRY *q;

    LOCK_INPUT;
    if (!IsListEmpty(&g_InputEventList))
    {
        q = RemoveHeadList(&g_InputEventList);
        p = CONTAINING_STRUCT(q, INPUT_EVENT, links);
        g_uNumInputEvents--;
        if (g_uNumInputEvents == 0)
        {
            _BlockBlockingReaders();
        }
        pRet = p->szInput;
        SystemFreeMemory(p);
    }
    UNLOCK_INPUT;
    return(pRet);
}


CHAR *
BlockingReadInput(void)
/**

Routine description:

    Block waiting on the next input read.

Parameters:

    void

Return value:

    CHAR

**/
{
    CHAR *pCh = NULL;

    do
    {
        _WaitUntilTheresInputToRead();
        if (TRUE == g_fExitProgram) 
        {
            if (-1 != g_uBlockingInputLock) 
            {
                SystemDeleteSemaphore(g_uBlockingInputLock);
            }
            return(NULL);
        }
        pCh = ReadNextInput();
        if (NULL != pCh)
        {
            if (strlen(pCh) > 0) break;
            SystemFreeMemory(pCh);
        }
    }
    while(1);
    return(pCh);
}

ULONG 
NumberOfPendingInputEvents(void)
/**

Routine description:

    How many input events are pending on the queue.  Note: the value
    returned is obviously potentially out of date the instant it gets
    sent back to the caller...

Parameters:

    void

Return value:

    ULONG

**/
{
    return(g_uNumInputEvents);
}
