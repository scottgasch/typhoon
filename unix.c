/**

Copyright (c) Scott Gasch

Module Name:

    unix.c

Abstract:

    FreeBSD/Linux/OSX system dependent code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 8 Apr 2004

Revision History:

    $Id: unix.c 200 2006-07-05 20:05:07Z scott $

**/

#include "chess.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define SYS_MAX_HEAP_ALLOC_SIZE_BYTES 0xfff
#define HEAP 0x48656170
#define MMAP 0x4d6d6170

pthread_mutex_t g_SystemLock;
#define LOCK_SYSTEM     pthread_mutex_lock(&g_SystemLock)
#define UNLOCK_SYSTEM   pthread_mutex_unlock(&g_SystemLock)

#ifdef DEBUG
ULONG g_uTotalAlloced = 0;

#define ALLOC_HASH_SIZE 0x4000

typedef struct _ALLOC_RECORD
{
    void *p;
    ULONG uSize;
} ALLOC_RECORD;
ALLOC_RECORD g_AllocHash[ALLOC_HASH_SIZE];

#define PTR_TO_ALLOC_HASH(x) ((((ULONG)(x)) >> 3) & (ALLOC_HASH_SIZE - 1))

ULONG
GetHeapMemoryUsage(void)
{
    Trace("There are now %u bytes outstanding.\n", g_uTotalAlloced);
    return(g_uTotalAlloced);
}

static void 
ReleaseAllocHashEntry(void *p)
{
    ULONG u = PTR_TO_ALLOC_HASH(p);
    ULONG v = u;

    LOCK_SYSTEM;
    while(g_AllocHash[u].p != p)
    {
        u++;
        u &= (ALLOC_HASH_SIZE - 1);
        if (v == u) return;
    }
    g_AllocHash[u].p = NULL;
    g_uTotalAlloced -= g_AllocHash[u].uSize;
    UNLOCK_SYSTEM;

//    Trace("Freed %u bytes at %p.\n", g_AllocHash[u].uSize, p);
}

static void
MarkAllocHashEntry(void *p, ULONG uSize)
{
    ULONG u = PTR_TO_ALLOC_HASH(p);

    if (uSize > (1024 * 1024 * 30)) return;

    LOCK_SYSTEM;
    while(g_AllocHash[u].p != NULL)
    {
        u++;
        u &= (ALLOC_HASH_SIZE - 1);
    }
    g_AllocHash[u].p = p;
    g_AllocHash[u].uSize = uSize;
    g_uTotalAlloced += uSize;
    UNLOCK_SYSTEM;

//    Trace("Alloc %u bytes at %p.\n", g_AllocHash[u].uSize, p);
}
#endif // DEBUG

//
// Thread table
//
typedef struct _SYSTEM_THREAD_TABLE_ENTRY
{
    DLIST_ENTRY links;
    ULONG uWrapperHandle;
    ULONG uTid;
    ULONG uThreadParam;
    pthread_t thread;
    THREAD_ENTRYPOINT *pEntry;
    ULONG uExitCode;
}
SYSTEM_THREAD_TABLE_ENTRY;
DLIST_ENTRY g_SystemThreadList;
ULONG g_uNextThreadHandle = 6000;

CHAR *
SystemGetTimeString(void)
/**

Routine description:

    Get the current time string from the OS.  Not thread safe.

Parameters:

    void

Return value:

    CHAR

**/
{
    static CHAR buf[32];
    time_t t = time(NULL);
    struct tm *p;

    p = localtime(&t);
    snprintf(buf, ARRAY_LENGTH(buf), 
             "%u:%02u:%02u", p->tm_hour, p->tm_min, p->tm_sec);
    return(buf);
}

CHAR *
SystemGetDateString(void)
/**

Routine description:

    Get the current date string from the OS.  Not thread safe.

Parameters:

    void

Return value:

    CHAR

**/
{
    static CHAR buf[32];
    time_t t = time(NULL);
    struct tm *p;

    p = localtime(&t);
    snprintf(buf, ARRAY_LENGTH(buf), 
             "%u.%02u.%02u", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday);
    return(buf);
}

//
// Thread code
//

static SYSTEM_THREAD_TABLE_ENTRY *
_SystemGetThreadTableEntry(ULONG uThreadHandle)
/**

Routine description:

    Support routine for thread functionality wrapper.  Given a
    uThreadHandle look up its entry in the global table.

Parameters:

    ULONG uThreadHandle

Return value:

    SYSTEM_THREAD_TABLE_ENTRY *

**/
{
    DLIST_ENTRY *p;
    SYSTEM_THREAD_TABLE_ENTRY *q;
    
    LOCK_SYSTEM;
    p = g_SystemThreadList.pFlink;
    while(p != &g_SystemThreadList)
    {
        q = CONTAINING_STRUCT(p, SYSTEM_THREAD_TABLE_ENTRY, links);
        if (q->uWrapperHandle == uThreadHandle)
        {
            goto end;
        }
        p = p->pFlink;
    }
    q = NULL;
 end:
    UNLOCK_SYSTEM;
    return(q);
}

void *
SystemThreadEntryPoint(void *pParam)
/**

Routine description:

    The first code that any newly created thread executes.

Parameters:

    void *pParam

Return value:

    void

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *p = (SYSTEM_THREAD_TABLE_ENTRY *)pParam;
    ULONG uParam;
    int i;
    
    LOCK_SYSTEM;
    UNLOCK_SYSTEM;

    uParam = p->uThreadParam;
    i = (int)(*(p->pEntry))(uParam);          // call thread's user-supplied entry
    return((void *)i);
}

FLAG 
SystemCreateThread(THREAD_ENTRYPOINT *pEntry, ULONG uParam, ULONG *puHandle)
/**

Routine description:

    Wraps the OS/Library dependent thread creation call.

Parameters:

    THREAD_ENTRYPOINT *pEntry : where should the new thread start?
    ULONG uParam : parameter to pass to the new thread
    ULONG *puHandle : "handle" to the new thread

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *p;

    p = malloc(sizeof(SYSTEM_THREAD_TABLE_ENTRY));
    if (NULL == p)
    {
        *puHandle = 0;
        return(FALSE);
    }
    
    LOCK_SYSTEM;
    p->pEntry = pEntry;
    p->uThreadParam = uParam;
    if (0 != pthread_create(&(p->thread),
                            NULL, 
                            SystemThreadEntryPoint, 
                            p))
    {
        free(p);
        return(FALSE);
    }
    *puHandle = p->uWrapperHandle = g_uNextThreadHandle;
    g_uNextThreadHandle++;
    InsertHeadList(&g_SystemThreadList, &(p->links));
    UNLOCK_SYSTEM;
    return(TRUE);
}


FLAG 
SystemWaitForThreadToExit(ULONG uThreadHandle)
/**

Routine description:

    Blocks until the thread whose handle is provieded exits.

Parameters:

    ULONG uThreadHandle

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *q = _SystemGetThreadTableEntry(uThreadHandle);
    
    pthread_join(q->thread, NULL);
    return(TRUE);
}


FLAG 
SystemGetThreadExitCode(ULONG uThreadHandle, ULONG *puCode)
/**

Routine description:

    Get the exit value of an already-exited thread.

Parameters:

    ULONG uThreadHandle,
    ULONG *puCode

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *q = _SystemGetThreadTableEntry(uThreadHandle);
    void *p;

    pthread_join(q->thread, &p);
    *puCode = ((ULONG)p);
    return(TRUE);
}


FLAG 
SystemDeleteThread(ULONG uThreadHandle)
/**

Routine description:

    Kill a thread.

Parameters:

    ULONG uThreadHandle

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *q = _SystemGetThreadTableEntry(uThreadHandle);
    
    if (NULL != q)
    {
        pthread_kill(q->thread, SIGKILL);
        LOCK_SYSTEM;
        RemoveEntryList(&(q->links));
        free(q);
        UNLOCK_SYSTEM;
        return(TRUE);
    }
    return(FALSE);
}



void FORCEINLINE 
SystemDebugBreakpoint(void)
/**

Routine description:

    A hardcoded breakpoint.

Parameters:

    void

Return value:

    void

**/
{
    __asm__("int3\n");
}

UINT64 FASTCALL 
SystemReadTimeStampCounter(void)
/**

Routine description:

    Read the processor's timestamp counter.

Parameters:

    void

Return value:

    UINT64 FASTCALL

**/
{
    __asm__("rdtsc;"
        :
        :
        : "%eax", "%edx");
}


double INLINE
SystemTimeStamp(void)
/**

Routine description:

    Return a timestamp to the caller.

Parameters:

    void

Return value:

    double

**/
{
    struct timeval tv;

    //
    // Number of seconds and microseconds since the Epoch.
    //
    if (0 != gettimeofday(&tv, NULL))
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "gettimeofday", (void *)errno, NULL,
                  __FILE__, __LINE__);
    }
    return((double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6);
}


FLAG
SystemDoesFileExist(CHAR *szFilename)
/**

Routine description:

    Determine if a file exists (non-atomically).

Parameters:

    CHAR *szFilename

Return value:

    FLAG

**/
{
    struct stat sb;
    int i = stat(szFilename, &sb);

    if ((i == -1) && (errno == ENOENT)) return(FALSE);
    return(TRUE);
}


static FLAG 
_SystemIsRoot(void)
/**

Routine description:

    This code is called by SystemDependentInitialization.  Its job is to
    determine whether this process is running with administrative powers.

Parameters:

    void

Return value:

    static FLAG

**/
{
    if (0 == geteuid())
    {
        return(TRUE);
    }
    return(FALSE);
}


void FORCEINLINE 
SystemDeferExecution(ULONG dwMs)
/**

Routine description:

    Sleep for a few ms.

Parameters:

    ULONG dwMs

Return value:

    void FORCEINLINE

**/
{
    struct timeval t = { 0, dwMs * 1000 };
    if (select(0, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &t) < 0) 
    {
        if (usleep(dwMs * 1000) < 0) 
        {
            Bug("SystemDeferExecution: I have insomnia...\n");
        }
    }
}


static void *
_SystemCallMmap(ULONG dwSizeBytes)
/**

Routine description:

    Wrapper around mmap.

Parameters:

    ULONG dwSizeBytes

Return value:

    static void

**/
{
    void *pMem = mmap(0,
                      (size_t)dwSizeBytes,
                      PROT_READ | PROT_WRITE,
                      MAP_ANON,
                      -1,
                      0);
    if (MAP_FAILED == pMem)
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "mmap", (void *)errno, (void *)dwSizeBytes,
                  __FILE__, __LINE__);
    }
    (void)madvise(pMem, dwSizeBytes, MADV_RANDOM | MADV_WILLNEED);
    return(pMem);
}


static void 
_SystemCallMunmap(void *pMem)
/**

Routine description:

    Wrapper around munmap

Parameters:

    void *pMem

Return value:

    static void

**/
{
    if (0 != munmap(pMem, (size_t)-1))
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "munmap", (void *)errno, pMem,
                  __FILE__, __LINE__);
    }
}


static void *_SystemCallMalloc(ULONG dwSizeBytes)
/**

Routine description:

    Wrapper around malloc.

Parameters:

    ULONG dwSizeBytes

Return value:

    static void

**/
{
    void *p = malloc(dwSizeBytes);
    if (NULL == p)
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "malloc", (void *)errno, (void *)dwSizeBytes,
                  __FILE__, __LINE__);
    }
    memset(p, 0, dwSizeBytes);
    return(p);
}


static void 
_SystemCallFree(void *p)
/**

Routine description:

    Wrapper around free.

Parameters:

    void *p

Return value:

    static void

**/
{
    free(p);
}

char *
SystemStrDup(char *p)
/**

Routine description:

    Implements strdup functionality.

Parameters:

    char *p

Return value:

    char

**/
{
    char *q = SystemAllocateMemory(strlen(p) + 1);
    strcpy(q, p);
    return(q);
}

void *
SystemAllocateMemory(ULONG dwSizeBytes)
/**

Routine description:
 
    Allocate some memory.

Parameters:

    ULONG dwSizeBytes

Return value:

    void

**/
{
    DWORD *p;

    if (0) // (dwSizeBytes > SYS_MAX_HEAP_ALLOC_SIZE_BYTES)
    {
        p = _SystemCallMmap(dwSizeBytes + sizeof(DWORD));
        if (NULL == p)
        {
            UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                      NULL, "mmap", (void *)errno, NULL,
                      __FILE__, __LINE__);
        }
        *p = MMAP;
        p += 1;
    }
    else
    {
        p = _SystemCallMalloc(dwSizeBytes + sizeof(DWORD));
        if (NULL == p)
        {
            UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                      NULL, "malloc", (void *)errno, NULL,
                      __FILE__, __LINE__);
        }
        *p = HEAP;
        p += 1;
    }
#ifdef DEBUG
    MarkAllocHashEntry(p, dwSizeBytes);
#endif
    ASSERT(NULL != p);
    return(p);
}

void 
SystemFreeMemory(void *pMemory)
/**

Routine description:

    Free an allocation previously allocated by SystemAllocMemory.

Parameters:

    void *pMemory

Return value:

    void

**/
{
    DWORD *p = pMemory;
    
    p -= 1;
#ifdef DEBUG
    ReleaseAllocHashEntry(pMemory);
#endif
    if (*p == HEAP)
    {
        _SystemCallFree(p);
    }
    else if (*p == MMAP)
    {
        _SystemCallMunmap(p);
    }
#ifdef DEBUG
    else
    {
        UtilPanic(SHOULD_NOT_GET_HERE,
                  NULL, NULL, NULL, NULL,
                  __FILE__, __LINE__);
    }
#endif
}


void *
SystemAllocateLockedMemory(ULONG dwSizeBytes)
/**

Routine description:

    Allocate non-pagable memory (if possible).

Parameters:

    ULONG dwSizeBytes

Return value:

    void

**/
{
    void *p = SystemAllocateMemory(dwSizeBytes);
    if (NULL != p)
    {
        if (0 != mlock(p, dwSizeBytes))
        {
            //
            // Note: don't panic the program over this... do dump a 
            // warning though as it should not happen.
            //
            Bug("WARNING: mlock for buffer at 0x%p (%u bytes) failed, "
                "errno=%d\n\n", p, dwSizeBytes, errno);
        }
    }
    return(p);
}


FLAG 
SystemMakeMemoryReadOnly(void *pMemory, 
                         ULONG dwSizeBytes)
/**

Routine description:

    Make some memory region read-only (if possible).

Parameters:

    void *pMemory,
    ULONG dwSizeBytes

Return value:

    FLAG

**/
{
    if (0 != mprotect(pMemory, dwSizeBytes, PROT_READ))
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "mprotect", (void *)errno, (void *)PROT_READ,
                  __FILE__, __LINE__);
    }
    return(TRUE);
}



FLAG 
SystemMakeMemoryReadWrite(void *pMemory, ULONG dwSizeBytes)
/**

Routine description:

    Make a region of memory read/write.

Parameters:

    void *pMemory,
    ULONG dwSizeBytes

Return value:

    FLAG

**/
{
    if (0 != mprotect(pMemory, dwSizeBytes, PROT_READ | PROT_WRITE))
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, 
                  "mprotect", 
                  (void *)errno,
                  (void *)(PROT_READ | PROT_WRITE),
                  __FILE__, __LINE__);
    }
    return(TRUE);
}



FLAG 
SystemCopyFile(CHAR *szSource, CHAR *szDest)
/**

Routine description:

    Make a copy of a file.

Parameters:

    CHAR *szSource,
    CHAR *szDest

Return value:

    FLAG

**/
{
    CHAR *buf = NULL;
    CHAR *x = "/bin/cp -f ";
    ULONG uLen = strlen(szSource) + strlen(szDest) + strlen(x) + 2;

    buf = SystemAllocateMemory(uLen);
    snprintf(buf, uLen, 
             "%s%s %s", x, szSource, szDest);
    if (0 == system(buf))
    {
        SystemFreeMemory(buf);
        return(TRUE);
    }
    SystemFreeMemory(buf);
    return(FALSE);
}


FLAG 
SystemDeleteFile(CHAR *szFile)
/**

Routine description:

    Delete a file.

Parameters:

    CHAR *szFile

Return value:

    FLAG

**/
{
    if (0 == unlink(szFile))
    {
        return(TRUE);
    }
    return(FALSE);
}


#define MAX_LOCKS (8)
typedef struct _UNIX_LOCK_ENTRY
{
    pthread_mutex_t lock;
    FLAG fInUse;
} UNIX_LOCK_ENTRY;
UNIX_LOCK_ENTRY g_rgLockTable[MAX_LOCKS];

ULONG
SystemCreateLock(void) 
{
    ULONG u;

    LOCK_SYSTEM;
    for (u = 0; u < MAX_LOCKS; u++)
    {
        if (FALSE == g_rgLockTable[u].fInUse)
        {
            pthread_mutex_init(&(g_rgLockTable[u].lock), NULL);
            g_rgLockTable[u].fInUse = TRUE;
            goto end;
        }
    }
    u = (ULONG)-1;                            // no free slot

 end:
    UNLOCK_SYSTEM;
    return(u);
}

FLAG
SystemDeleteLock(ULONG u)
{
    LOCK_SYSTEM;
    if ((u < MAX_LOCKS) && (g_rgLockTable[u].fInUse == TRUE))
    {
        pthread_mutex_destroy(&(g_rgLockTable[u].lock));
        g_rgLockTable[u].fInUse = FALSE;
        UNLOCK_SYSTEM;
        return(TRUE);
    }
    UNLOCK_SYSTEM;
    return(FALSE);
}

FLAG
SystemBlockingWaitForLock(ULONG u) 
{
    pthread_mutex_t *p = NULL;
    LOCK_SYSTEM;
    if ((u < MAX_LOCKS) && (g_rgLockTable[u].fInUse == TRUE)) 
    {
        p = &(g_rgLockTable[u].lock);
    }
    UNLOCK_SYSTEM;
    if (p != NULL)
    {
        pthread_mutex_lock(p);
        return(TRUE);
    }
    return(FALSE);
}


FLAG
SystemReleaseLock(ULONG u) 
{
    pthread_mutex_t *p = NULL;
    LOCK_SYSTEM;
    if ((u < MAX_LOCKS) && (g_rgLockTable[u].fInUse == TRUE)) 
    {
        p = &(g_rgLockTable[u].lock);
    }
    UNLOCK_SYSTEM;
    if (p != NULL) 
    {
        pthread_mutex_unlock(p);
        return(TRUE);
    }
    return(FALSE);
}


#define MAX_SEM (8)
int g_rgSemaphores[MAX_SEM];
#ifdef DEBUG
ULONG g_rgSemValues[MAX_SEM];
#endif

ULONG
SystemCreateSemaphore(ULONG uValue) 
{
    ULONG u;
    union semun {
            int val;
            struct semid_ds *buf;
            unsigned short  *array;
    } value;

    if (uValue > MAX_USHORT) return((ULONG)-1);
    LOCK_SYSTEM;
    for (u = 0; u < MAX_SEM; u++) 
    {
        if (g_rgSemaphores[u] < 0) 
        {
            g_rgSemaphores[u] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
            if (g_rgSemaphores[u] < 0) 
            {
                u = (ULONG)-1;
                goto end;
            }
            value.val = uValue;
            if (semctl(g_rgSemaphores[u], 0, SETVAL, value) < 0) 
            {
                (void)SystemDeleteSemaphore(u);
                u = (ULONG)-1;
                goto end;
            }
#ifdef DEBUG
            g_rgSemValues[u] = uValue;
            Trace("Semaphore #%u created, initial value is %u\n", 
                  u, uValue);
#endif
            goto end;
        }
    }
    u = (ULONG)-1;
 end:
    UNLOCK_SYSTEM;
    return(u);
}

FLAG 
SystemDeleteSemaphore(ULONG u) {
    union semun {
            int val;
            struct semid_ds *buf;
            unsigned short  *array;
    } value;

    LOCK_SYSTEM;
    if ((u < MAX_SEM) && (g_rgSemaphores[u] >= 0)) 
    {
        if (semctl(g_rgSemaphores[u], 0, IPC_RMID, value) < 0) 
        {
            return(FALSE);
        }
        g_rgSemaphores[u] = -1;
    }
    UNLOCK_SYSTEM;
    return(TRUE);
}

void
SystemReleaseSemaphoreResource(ULONG u) 
{
    struct sembuf operation;

    LOCK_SYSTEM;
    if ((u < MAX_SEM) && (g_rgSemaphores[u] >= 0)) 
    {
        operation.sem_num = u;
        operation.sem_op = +1;
        operation.sem_flg = 0;
        if (semop(g_rgSemaphores[u], &operation, 1) < 0) 
        {
            UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                      NULL, "semop", (void *)errno, (void *)0,
                      __FILE__, __LINE__);
        }
#ifdef DEBUG
        g_rgSemValues[u] += 1;
        Trace("Semaphore #%u value incremented to %u\n", u, 
              g_rgSemValues[u]);
#endif
    }
    UNLOCK_SYSTEM;
}

void
SystemObtainSemaphoreResource(ULONG u) 
{
    struct sembuf operation;

    LOCK_SYSTEM;
    if ((u < MAX_SEM) && (g_rgSemaphores[u] >= 0))
    {
        operation.sem_num = u;
        operation.sem_op = -1;
        operation.sem_flg = 0;
        UNLOCK_SYSTEM;
        if (semop(g_rgSemaphores[u], &operation, 1) < 0) 
        {
            UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                      NULL, "semop", (void *)errno, (void *)1,
                      __FILE__, __LINE__);
        }
#ifdef DEBUG
        if (g_rgSemValues[u] > 0) 
        {
            g_rgSemValues[u] -= 1;
            Trace("Semaphore #%u value decremented to %u\n", u, 
                  g_rgSemValues[u]);
        }
#endif
        return;
    }
    UNLOCK_SYSTEM;
}


FLAG
SystemDependentInitialization(void)
/**

Routine description:

    This code is called to initialize operating system dependent code.
    It is unsynchronized.  It should only ever be called once.  It
    should definitely never be called by two threads at once!

Parameters:

    void

Return value:

    FLAG : TRUE on success, FALSE otherwise

**/
{
    ULONG u;
#ifdef DEBUG
    static FLAG fInit = FALSE;

    if (TRUE == fInit)
    {
        Bug("SystemDependentInitialization code called more than once!\n");
        ASSERT(FALSE);
        return(FALSE);
    }
    fInit = TRUE;

    memset(g_AllocHash, 0, sizeof(g_AllocHash));
#endif
    for (u = 0; u < MAX_SEM; u++) 
    {
        g_rgSemaphores[u] = -1;
    }
    InitializeListHead(&g_SystemThreadList);
    pthread_mutex_init(&g_SystemLock, NULL);
    return(TRUE);
}    
