/**

Copyright (c) Scott Gasch

Module Name:

    win32.c

Abstract:

    Windows(R) operating system dependent code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 7 Apr 2004

Revision History:

    $Id: win32.c 345 2007-12-02 22:56:42Z scott $

**/

#include "chess.h"
#define _WIN32_WINNT 0x403

#pragma warning(disable:4142) // benign redefinition of type
#include <windows.h>
#pragma warning(default:4142)

#define SYS_MAX_HEAP_ALLOC_SIZE_BYTES 0xfff

//
// Global system lock
//
CRITICAL_SECTION g_SystemLock;
#define LOCK_SYSTEM   EnterCriticalSection(&g_SystemLock)
#define UNLOCK_SYSTEM LeaveCriticalSection(&g_SystemLock)

//
// Thread table
//
typedef struct _SYSTEM_THREAD_TABLE_ENTRY
{
    DLIST_ENTRY links;
    ULONG uWrapperHandle;
    ULONG uTid;
    ULONG uThreadParam;
    THREAD_ENTRYPOINT *pEntry;
    ULONG uExitCode;
    HANDLE hThread;
}
SYSTEM_THREAD_TABLE_ENTRY;
DLIST_ENTRY g_SystemThreadList;

//
// Global buffer to hold a bunch of system dependent numbers / settings
// 
typedef struct _SYS_INFORMATION_BUFFER
{
    FLAG fPopulated;
    SYSTEM_INFO si;
    OSVERSIONINFOEX vi;
    MEMORYSTATUS ms;
}
SYS_INFORMATION_BUFFER;
SYS_INFORMATION_BUFFER g_SystemInfoBuffer;

//
// The frequency and type of our TimeStamp timer
//
double g_dTimeStampFrequency = 0.0;
ULONG g_uTimeStampType = 0;
ULONG g_uMaxHeapAllocationSize = 0;

//
// A history of allocations
// 
#ifdef DEBUG
ULONG g_uTotalAlloced = 0;

#define ALLOC_HASH_SIZE 0x4000

typedef struct _ALLOC_RECORD
{
    void *p;
    ULONG uSize;
} ALLOC_RECORD;
ALLOC_RECORD g_AllocHash[ALLOC_HASH_SIZE];

#define PTR_TO_ALLOC_HASH(x) ((((ULONG_PTR)(x)) >> 3) & (ALLOC_HASH_SIZE - 1))

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

//    Trace("Freed %u bytes at 0x%p.\n", g_AllocHash[u].uSize, p);
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

//    Trace("Alloc %u bytes at 0x%p.\n", g_AllocHash[u].uSize, p);
}
#endif


CHAR *
SystemStrDup(CHAR *p)
/**

Routine description:

    Local implementation of strdup

Parameters:

    CHAR *p

Return value:

    CHAR

**/
{
    CHAR *q = SystemAllocateMemory((ULONG)strlen(p) + 1);

    ASSERT(strlen(p) < (size_t)MAX_ULONG);
    strcpy(q, p);
    return(q);
}


CHAR *
SystemGetDateString(void)
/**

Routine description:

    Get the current date as a string.

Parameters:

    void

Return value:

    CHAR

**/
{
    static CHAR buf[32];
    SYSTEMTIME st;
    
    GetSystemTime(&st);

    snprintf(buf, ARRAY_LENGTH(buf), 
             "%u.%02u.%02u", st.wYear, st.wMonth, st.wDay);
    return(buf);
}


CHAR *
SystemGetTimeString(void)
/**

Routine description:

    Get the current time as a string.

Parameters:

    void

Return value:

    CHAR

**/
{
    static CHAR buf[32];
    SYSTEMTIME st;
    
    GetSystemTime(&st);

    snprintf(buf, ARRAY_LENGTH(buf),
             "%u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
    return(buf);
}


void 
SystemDeferExecution(ULONG uMs)
/**

Routine description:

    Sleep for some number of milliseconds.

Parameters:

    ULONG uMs

Return value:

    void

**/
{
    Sleep(uMs);
}

static SYSTEM_THREAD_TABLE_ENTRY *
_SystemGetThreadTableEntry(ULONG uThreadHandle)
/**

Routine description:

    Support code for the thread functionality wrapper.  Given a thread
    "handle" return its thread table struct.

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

int WINAPI 
SystemThreadEntryPoint(VOID *pParam)
/**

Routine description:

    The real entry point of all new threads.  It calls into the user
    specified entry point.

Parameters:

    VOID *pParam

Return value:

    int WINAPI

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *p = (SYSTEM_THREAD_TABLE_ENTRY *)pParam;
    ULONG uParam;
    int i;
    
    LOCK_SYSTEM;
    UNLOCK_SYSTEM;

    uParam = p->uThreadParam;
    i = (int)(*(p->pEntry))(uParam);
    return(i);
}

FLAG 
SystemCreateThread(THREAD_ENTRYPOINT *pEntry, ULONG uParam, ULONG *puHandle)
/**

Routine description:

    Wrapper function to expose create thread functionality.

Parameters:

    THREAD_ENTRYPOINT *pEntry,
    ULONG uParam,
    ULONG *puHandle

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *p;

    p = HeapAlloc(GetProcessHeap(),
                  HEAP_ZERO_MEMORY,
                  sizeof(SYSTEM_THREAD_TABLE_ENTRY));
    if (NULL == p)
    {
        *puHandle = 0;
        return(FALSE);
    }

    LOCK_SYSTEM;
    p->pEntry = pEntry;
    p->uThreadParam = uParam;
    p->hThread = CreateThread(NULL,
                              0x6000,
                              SystemThreadEntryPoint,
                              (VOID *)p,
                              0,
                              &(p->uTid));
    if (NULL == p->hThread)
    {
        (void)HeapFree(GetProcessHeap(), 0, p);
        return(FALSE);
    }
    *puHandle = p->uWrapperHandle = p->uTid;
    InsertHeadList(&g_SystemThreadList, &(p->links));
    UNLOCK_SYSTEM;
    return(TRUE);
}


FLAG 
SystemWaitForThreadToExit(ULONG uThreadHandle)
/**

Routine description:

    Wait for the specified thread to exit.

Parameters:

    ULONG uThreadHandle

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *q = _SystemGetThreadTableEntry(uThreadHandle);
    
    if (WAIT_OBJECT_0 != WaitForSingleObject(q->hThread, INFINITE))
    {
        ASSERT(FALSE);
        return(FALSE);
    }
    return(TRUE);
}


FLAG 
SystemGetThreadExitCode(ULONG uThreadHandle, ULONG *puCode)
/**

Routine description:

    Get the specified thread's exit value.

Parameters:

    ULONG uThreadHandle,
    ULONG *puCode

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *q = _SystemGetThreadTableEntry(uThreadHandle);
    ULONG uCode;
    
    if (FALSE == GetExitCodeThread(q->hThread, &uCode))
    {
        return(FALSE);
    }
    if (STILL_ACTIVE == uCode)
    {
        return(FALSE);
    }
    *puCode = uCode;
    return(TRUE);
}


FLAG 
SystemDeleteThread(ULONG uThreadHandle)
/**

Routine description:

    Terminate a thread.

Parameters:

    ULONG uThreadHandle

Return value:

    FLAG

**/
{
    SYSTEM_THREAD_TABLE_ENTRY *q = _SystemGetThreadTableEntry(uThreadHandle);
    ULONG u;
    
    if (NULL != q)
    {
        if (FALSE == SystemGetThreadExitCode(uThreadHandle, &u))
        {
            SystemWaitForThreadToExit(uThreadHandle);
        }
        CloseHandle(q->hThread);
        LOCK_SYSTEM;
        RemoveEntryList(&(q->links));
        HeapFree(GetProcessHeap(), 0, q);
        UNLOCK_SYSTEM;
        return(TRUE);
    }
    return(FALSE);
}


static void
_SystemPopulateSystemInformationBuffer(void)
/**

Routine description:
    
    Populates global structure SYS_INFORMATION_BUFFER.  This code should
    be called only once from SystemDependentInitialization.  It should
    definitely never be called by more than one thread at a time.

Parameters:

    void

Return value:

    void

**/
{
#ifdef DEBUG
    static FLAG fInit = FALSE;

    if (TRUE == fInit)
    {
        Bug("_SystemPopulateSystemInformationBuffer: This code should "
            "not be called more than once!\n");
        ASSERT(FALSE);
    }
    fInit = TRUE;
#endif

    //
    // Make some calls to populate the global buffer
    //
    GetSystemInfo(&(g_SystemInfoBuffer.si));
    
    // 
    // Note: This API is not present on Win9x
    //
    g_SystemInfoBuffer.vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (FALSE == GetVersionEx((OSVERSIONINFO *)&(g_SystemInfoBuffer.vi)))
    {
        Bug("_SystemPopulateSystemInformationBuffer: GetVersionEx "
            "call failed, error=%u.\n", GetLastError());
        ASSERT(FALSE);
    }

    GlobalMemoryStatus(&(g_SystemInfoBuffer.ms));
    
    g_SystemInfoBuffer.fPopulated = TRUE;
}


static FLAG 
_SystemEnablePrivilege(TCHAR *szPrivilegeName,
                       FLAG fEnable)
/**

Routine description:

    This code is called by SystemDependentInitialization to add some
    privileges to our process' security token.

Parameters:

    TCHAR *szPrivilegeName,
    FLAG fEnable

Return value:

    FLAG

**/
{
    FLAG fMyRetVal = FALSE;                   // our return value
    HANDLE hToken = INVALID_HANDLE_VALUE;     // handle to process token
    PTOKEN_PRIVILEGES pNewPrivileges;         // privs to enable/disable
    LUID Luid;                                // priv as a LUID
    BYTE buf[sizeof(TOKEN_PRIVILEGES)+
            ((1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))];

    pNewPrivileges = (PTOKEN_PRIVILEGES)buf;
    
    //
    // Try to open the thread's security token first.
    // 
    if (FALSE == OpenThreadToken(GetCurrentThread(),
                                 TOKEN_ADJUST_PRIVILEGES,
                                 FALSE,
                                 &hToken))
    {
        //
        // Well that failed.  If it is because there is no thread token then
        // try to open the process' security token.
        //
        if (ERROR_NO_TOKEN == GetLastError())
        {
            if (FALSE == OpenProcessToken(GetCurrentProcess(),
                                          TOKEN_ADJUST_PRIVILEGES,
                                          &hToken))
            {
                //
                // Can't open the process token either.  Fail.
                // 
                goto end;
            }
        }
        else
        { 
            //
            // Failed to open the thread token for some other reason.  Fail.
            // 
            goto end;
        }
    }
    
    //
    // Convert priv name to an LUID.
    //
    if (FALSE == LookupPrivilegeValue(NULL,
                                      szPrivilegeName,
                                      &Luid))
    {
        goto end;
    }
    
    //
    // Construct new data struct to enable / disable the privi.
    //
    pNewPrivileges->PrivilegeCount = 1;
    pNewPrivileges->Privileges[0].Luid = Luid;
    pNewPrivileges->Privileges[0].Attributes = 
        fEnable ? SE_PRIVILEGE_ENABLED : 0;

    //
    // Adjust the privileges on the token.
    //
    fMyRetVal = AdjustTokenPrivileges(hToken,
                                      FALSE,
                                      pNewPrivileges,
                                      0,
                                      NULL,
                                      NULL);
    if (TRUE == fMyRetVal)
    {
        //
        // This boneheaded API returns TRUE and then expects the
        // caller to test GetLastError to see if it really worked.
        // 
        if (ERROR_SUCCESS != GetLastError())
        {
            fMyRetVal = FALSE;
        }
    }
    
 end:
    if (INVALID_HANDLE_VALUE != hToken)
    {
        CloseHandle(hToken);
    }
    return(fMyRetVal);
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
    DebugBreak();
}

#define SYS_HIGH_PERFORMANCE_TIMER (1)
#define SYS_GET_TICK_COUNT         (2)
#define SYS_MS_PER_SECOND          (1000)

double
SystemTimeStamp(void)
/**

Routine description:

    Return a timestamp to the caller.  The precision of this timestamp
    is based on whether the machine supports a high resolution timer.
    If not, this code rolls back to use GetTickCount().

    This code relies on some initialization code in SystemDependent-
    Initialization.  Once that code has executed, this code is thread
    safe.

Parameters:

    void

Return value:

    double

**/
{
    double dRetVal;
    UINT64 u64;

    switch(g_uTimeStampType)
    {
        case SYS_HIGH_PERFORMANCE_TIMER:
            QueryPerformanceCounter((LARGE_INTEGER *)&u64);
            dRetVal = u64 * g_dTimeStampFrequency;
            break;
        case SYS_GET_TICK_COUNT:
            dRetVal = GetTickCount() / SYS_MS_PER_SECOND;
            break;
        default:
            dRetVal = 0.0;
            Bug("SystemTimeStamp: You have to call SystemDependent"
                "Initialization before you can call this code.\n");
            ASSERT(FALSE);
            break;
    }
    return(dRetVal);
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
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(szFilename))
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            return(FALSE);
        }
    }
    return(TRUE);
}


static FLAG 
_SystemIsAdministrator(void)
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
    BOOL fIsMember = FALSE;                   // our return value
    SID *pAdminSid = NULL;                    // ptr to admin sid
    SID_IDENTIFIER_AUTHORITY NtAuthority =    // used to create admin sid
        SECURITY_NT_AUTHORITY;
    
    //
    // Allocate and initialize a SID for the admin group.
    // 
    if (FALSE == AllocateAndInitializeSid(&NtAuthority,
                                          2,
                                          SECURITY_BUILTIN_DOMAIN_RID,
                                          DOMAIN_ALIAS_RID_ADMINS,
                                          0, 0, 0, 0, 0, 0,
                                          &pAdminSid)) 
    {
        goto end;
    }
    
    //
    // SID allocation succeeded, check this thread or process token
    // for membership in the admin group.
    // 
    if (FALSE == CheckTokenMembership(NULL,
                                      pAdminSid,
                                      &fIsMember))
    {
        fIsMember = FALSE;
    }
    
 end:
    if (NULL != pAdminSid)
    {
        FreeSid(pAdminSid);
    }
    
    return(fIsMember);
}


static void *
_SystemCallVirtualAlloc(ULONG dwSizeBytes)
/**

Routine description:

    Wrapper around VirtualAlloc

Parameters:

    ULONG dwSizeBytes

Return value:

    static void

**/
{
    SIZE_T size = dwSizeBytes;
    void *pMem = NULL;
    
    pMem = VirtualAlloc(NULL,
                        size,
                        MEM_RESERVE | MEM_COMMIT, 
                        PAGE_READWRITE);
    if (NULL == pMem)
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "VirtualAlloc", GetLastError(), size,
                  __FILE__, __LINE__);
    }
    return(pMem);
}

static void 
_SystemCallVirtualFree(void *pMem)
/**

Routine description:

    Wrapper around VirtualFree

Parameters:

    void *pMem

Return value:

    static void

**/
{
    BOOL fRetVal = FALSE;

    if (NULL != pMem)
    {
        fRetVal = VirtualFree(pMem, 0, MEM_RELEASE);
        if (FALSE == fRetVal)
        {
            UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                      NULL, "VirtualFree", GetLastError(), pMem,
                      __FILE__, __LINE__);
        }
    }
}

static void *
_SystemCallHeapAlloc(ULONG dwSizeBytes)
/**

Routine description:

    Wrapper around HeapAlloc.  Returns a zeroed buffer from the heap.
    While on the subject of heaps, let me take a moment to extol the
    value of the Win32 Application Verifier / PageHeap code.  This is
    code that sanity checks the actions of an app.  It is especially
    good at finding heap misuse (double frees, reuse after free,
    buffer overruns, etc...)  It also catches things like
    CRITICAL_SECTION abuse and HANDLE abuse.  To enable it, use
    appverif.exe.

Parameters:

    ULONG dwSizeBytes

Return value:

    static void

**/
{
    void *p = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        dwSizeBytes);
    if (NULL == p)
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "HeapAlloc", GetLastError(), dwSizeBytes,
                  __FILE__, __LINE__);
    }
    return(p);
}

static void 
_SystemCallHeapFree(void *pMem)
/**

Routine description:

    Wrapper around HeapFree

Parameters:

    void *pMem

Return value:

    static void

**/
{
    if (FALSE == HeapFree(GetProcessHeap(),
                          0,
                          pMem))
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "HeapFree", GetLastError(), pMem,
                  __FILE__, __LINE__);
    }
}

void 
SystemFreeMemory(void *pMem)
/**

Routine description:

    Free memory

Parameters:

    void *pMem

Return value:

    static void

**/
{
    _SystemCallHeapFree(pMem);
#ifdef DEBUG
    ReleaseAllocHashEntry(pMem);
#endif
}

void *
SystemAllocateMemory(ULONG dwSizeBytes)
/**

Routine description:

    Allocate some memory, either from the heap or from VirtualAlloc
    depending on the size of the request.  The buffer returned by
    this call is guaranteed to be zeroed out.  Buffers allocated by
    this call should be freed via a call to SystemFreeMemory when
    they are no longer needed.

Parameters:

    ULONG dwSizeBytes : size of the buffer needed in bytes

Return value:

    void

**/
{
    BYTE *p;

    p = _SystemCallHeapAlloc(dwSizeBytes);
#ifdef DEBUG
    MarkAllocHashEntry(p, dwSizeBytes);
#endif
    return(p);
}


void *
SystemAllocateLockedMemory(ULONG dwSizeBytes)
/**

Routine description:

    Allocate a buffer that is locked in memory (i.e. non-pagable).

Parameters:

    ULONG dwSizeBytes

Return value:

    void

**/
{
    void *p = _SystemCallVirtualAlloc(dwSizeBytes);

    if (NULL != p)
    {
        if (FALSE == VirtualLock(p, dwSizeBytes))
        {
            UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                      NULL, "VirtualLock", GetLastError(), p,
                      __FILE__, __LINE__);
        }
    }
#ifdef DEBUG
    MarkAllocHashEntry(p, dwSizeBytes);
#endif
    return(p);
}


FLAG 
SystemMakeMemoryReadOnly(void *pMemory, 
                         ULONG dwSizeBytes)
/**

Routine description:

Parameters:

    void *pMemory,
    ULONG dwSizeBytes

Return value:

    FLAG

**/
{
    BOOL fRetVal = FALSE;
    ULONG dwJunk;
    
    fRetVal = VirtualProtect(pMemory, 
                             dwSizeBytes, 
                             PAGE_READONLY, 
                             &dwJunk);
    if (FALSE == fRetVal)
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "VirtualProtect", GetLastError(), pMemory,
                  __FILE__, __LINE__);
    }
    return((FLAG)fRetVal);
}

FLAG 
SystemMakeMemoryNoAccess(void *pMemory, 
                         ULONG dwSizeBytes)
/**

Routine description:

Parameters:

    void *pMemory,
    ULONG dwSizeBytes

Return value:

    FLAG

**/
{
    BOOL fRetVal = FALSE;
    ULONG dwJunk;
    
    fRetVal = VirtualProtect(pMemory, 
                             dwSizeBytes, 
                             PAGE_NOACCESS,
                             &dwJunk);
    if (FALSE == fRetVal)
    {
        UtilPanic(UNEXPECTED_SYSTEM_CALL_FAILURE,
                  NULL, "VirtualProtect", GetLastError(), pMemory,
                  __FILE__, __LINE__);
    }
    return((FLAG)fRetVal);
}


FLAG 
SystemCopyFile(CHAR *szSource, CHAR *szDest)
/**

Routine description:

Parameters:

    CHAR *szSource,
    CHAR *szDest

Return value:

    FLAG

**/
{
    return(CopyFileA(szSource, szDest, TRUE));
}


FLAG 
SystemDeleteFile(CHAR *szFile)
/**

Routine description:

Parameters:

    CHAR *szFile

Return value:

    FLAG

**/
{
    return(DeleteFileA(szFile));
}


FLAG 
SystemMakeMemoryReadWrite(void *pMemory, ULONG dwSizeBytes)
/**

Routine description:

Parameters:

    void *pMemory,
    ULONG dwSizeBytes

Return value:

    FLAG

**/
{
    BOOL fRetVal = FALSE;
    ULONG dwJunk;
    
    fRetVal = VirtualProtect(pMemory, 
                             dwSizeBytes, 
                             PAGE_READWRITE, 
                             &dwJunk);
    if (FALSE == fRetVal)
    {
        Bug("SystemMakeMemoryReadWrite: VirtualProtect for buffer at %p "
            "(size %u) failed, error=%u.\n", pMemory, dwSizeBytes,
            GetLastError());
    }
    return(fRetVal);
}


UINT64 FASTCALL 
SystemReadTimeStampCounter(void)
/**

Routine description:

Parameters:

    void

Return value:

    UINT64 FASTCALL

**/
{
#if defined(_X86_)
    _asm {
        rdtsc
    }
#elif defined(_AMD64_)
    UINT64 itc;
    itc = __rdtsc();
    return itc;
#elif defined(_IA64_)
    UINT64 itc;
    itc = __getReg(CV_IA64_ApITC);
    return itc;
#else
#error Unknown architecture
#endif
}

#define MAX_LOCKS (8)
typedef struct _WIN32_LOCK_ENTRY
{
    CRITICAL_SECTION lock;
    FLAG fInUse;
} WIN32_LOCK_ENTRY;
WIN32_LOCK_ENTRY g_rgLockTable[MAX_LOCKS];

ULONG
SystemCreateLock(void) 
{
    ULONG u;

    LOCK_SYSTEM;
    for (u = 0; u < MAX_LOCKS; u++)
    {
        if (FALSE == g_rgLockTable[u].fInUse)
        {
            InitializeCriticalSection(&(g_rgLockTable[u].lock));
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
        DeleteCriticalSection(&(g_rgLockTable[u].lock));
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
    CRITICAL_SECTION *p = NULL;
    LOCK_SYSTEM;
    if ((u < MAX_LOCKS) && (g_rgLockTable[u].fInUse == TRUE)) 
    {
        p = &(g_rgLockTable[u].lock);
    }
    UNLOCK_SYSTEM;
    if (p != NULL) 
    {
        EnterCriticalSection(p);
        Trace("lock at %p is obtained.\n", p);
        return(TRUE);
    }
    return(FALSE);
}


FLAG
SystemReleaseLock(ULONG u) 
{
    CRITICAL_SECTION *p = NULL;
    LOCK_SYSTEM;
    if ((u < MAX_LOCKS) && (g_rgLockTable[u].fInUse == TRUE)) 
    {
        p = &(g_rgLockTable[u].lock);
    }
    UNLOCK_SYSTEM;
    if (p != NULL) 
    {
        LeaveCriticalSection(p);
        Trace("lock at %p is released.\n", p);
        return(TRUE);
    }
    return(FALSE);
}


#define MAX_SEMS (8)
HANDLE g_rgSemHandles[MAX_SEMS];

ULONG 
SystemCreateSemaphore(ULONG uValue) 
{
    ULONG u;
    LOCK_SYSTEM;
    for (u = 0; u < MAX_SEMS; u++) 
    {
        if (g_rgSemHandles[u] == (HANDLE)NULL) 
        {
            g_rgSemHandles[u] = CreateSemaphore(NULL, uValue, uValue, NULL);
            if (NULL == g_rgSemHandles[u]) 
            {
                u = (ULONG)-1;
            }
            goto end;
        }
    }
    u = (ULONG)-1;

 end:
    UNLOCK_SYSTEM;
    return(u);
}

FLAG
SystemDeleteSemaphore(ULONG u) 
{
    LOCK_SYSTEM;
    if ((u < MAX_SEMS) && (g_rgSemHandles[u] != (HANDLE)NULL)) 
    {
        (void)CloseHandle(g_rgSemHandles[u]);
        g_rgSemHandles[u] = (HANDLE)NULL;
        UNLOCK_SYSTEM;
        return(TRUE);
    }
    UNLOCK_SYSTEM;
    return(FALSE);
}

void
SystemReleaseSemaphoreResource(ULONG u) 
{
    LOCK_SYSTEM;
    if ((u < MAX_SEMS) && (g_rgSemHandles[u] != (HANDLE)NULL)) 
    {
        ReleaseSemaphore(g_rgSemHandles[u], +1, NULL);
    }
    UNLOCK_SYSTEM;
}

void
SystemObtainSemaphoreResource(ULONG u) 
{
    HANDLE h;
    LOCK_SYSTEM;
    if ((u < MAX_SEMS) && (g_rgSemHandles[u] != (HANDLE)NULL)) 
    {
        h = g_rgSemHandles[u];
        UNLOCK_SYSTEM;
        (void)WaitForSingleObject(h, INFINITE);
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
    UINT64 u64;
#ifdef DEBUG
    static FLAG fInit = FALSE;

    if (TRUE == fInit)
    {
        Bug("SystemDependentInitialization code called more than once!\n");
        ASSERT(FALSE);
        return(FALSE);
    }
    fInit = TRUE;
#endif

    //
    // Initialize the system dependent lock
    //
    __try
    {
        InitializeCriticalSection(&g_SystemLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Bug("SystemDependentInitialization: InitializeCriticalSection "
            "raised an exception, bailing out.\n");
        return(FALSE);
    }

    //
    // Clear a pool of handles
    //
    memset(g_rgLockTable, 0, sizeof(g_rgLockTable));
    memset(g_rgSemHandles, 0, sizeof(g_rgSemHandles));

    //
    // Populate global system info buffer
    //
    _SystemPopulateSystemInformationBuffer();

    //
    // Enable some privileges in this process' security token
    //
    if (FALSE == _SystemEnablePrivilege(SE_INC_BASE_PRIORITY_NAME, TRUE))
    {
        Trace("NOTICE: Can't enable SE_INC_BASE_PRIORITY_NAME privilege, "
              "error=%u\n", GetLastError());
    }
    if (FALSE == _SystemEnablePrivilege(SE_LOCK_MEMORY_NAME, TRUE))
    {
        Trace("NOTICE: Can't enable SE_LOCK_MEMORY_NAME privilege, "
              "error=%u\n", GetLastError());
    }
    
    //
    // Don't let the system hibernate/suspend while the chess program
    // is running.
    // 
    (void)SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

    //
    // Determine the precision of our SystemTimeStamp() code
    //
    ASSERT(g_uTimeStampType == 0);
    if (FALSE != QueryPerformanceFrequency((LARGE_INTEGER *)&u64))
    {
        g_dTimeStampFrequency = 1.0 / u64;
        g_uTimeStampType = SYS_HIGH_PERFORMANCE_TIMER;
    }
    else
    {
        g_dTimeStampFrequency = 1.0 / 1000.0;
        g_uTimeStampType = SYS_GET_TICK_COUNT;
    }
    ASSERT(g_dTimeStampFrequency > 0.0);

    g_uMaxHeapAllocationSize = 0x1000;
    InitializeListHead(&g_SystemThreadList);

    return(TRUE);
}    
