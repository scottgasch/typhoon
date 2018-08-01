/**

Copyright (c) Scott Gasch

Module Name:

    book.c

Abstract:

    Opening book routines: everything from probing the opening book to
    building a new one to editing the current book.  The structure of
    a book entry is in book.h.  The book is kept on disk and sorted by
    position signature.  There may be more than one entry per
    signature -- there will be one entry per book move from that
    positions so for example the initial position will have quite a
    few entries.

    Probing the book (BookMove, called from Think in brain.c) entails
    binary searching the on disk book file for a matching signature,
    reading all entries that match, computing a "weight" for each one,
    and then randomly choosing one.
 
    Building a new book is done with a temporary "membook" buffer in
    memory.  It requires quite a bit of memory so before the operation
    takes place the main hash table and pawn hash table structures are
    freed (see movelist.c).  A PGN file is read one game at a time and
    the moves are made on a POSITION struct.  Once the game is over
    the signature-move pairs are used to create new entries in the
    membook.  Finally the membook is sorted and written to disk.  The
    process can take a while, especially for a large PGN file or on a
    machine with limited memory resources.

Author:

    Scott Gasch (scott.gasch@gmail.com) 27 Jun 2004

Revision History:

    $Id: book.c 355 2008-07-01 15:46:43Z scott $

**/

#include "chess.h"

static int g_fdBook = -1;                     // file descriptor for open book
static ULONG g_uMemBookCount = 0;             // count of entries in membook
static BOOK_ENTRY *g_pMemBook = NULL;         // allocated membook struct
static BOOK_ENTRY *g_pMemBookHead = NULL;     // ptr to 1st entry in membook

//
// Globals exported
// 
FLAG g_fTournamentMode = FALSE;               // are we in tournament mode
ULONG g_uBookProbeFailures = 0;               // how many times have we missed
ULONG g_uMemBookSize = 0x1000000;
OPENING_NAME_MAPPING *g_pOpeningNames = NULL; // opening names file

static FLAG 
_VerifyBook(CHAR *szName)
/**

Routine description:

Parameters:

    CHAR *szName

Return value:

    static FLAG

**/
{
    int fd = -1;
    int iRet;
    ULONG uPos;
    ULONG uNum;
    ULONG u;
    struct stat s;
    UINT64 u64LastSig;
    ULONG uLastMove;
    BOOK_ENTRY be;
    FLAG fMyRetVal = FALSE;
        
    fd = open(szName, O_RDONLY | O_BINARY);
    if (fd < 0)
    {
        Trace("VerifyBook: Can't open %s for read/write.\n", szName);
        goto end;
    }

    if (0 != stat(szName, &s))
    {
        Trace("VerifyBook: Can't stat %s.\n", szName);
        goto end;
    }

    if ((s.st_size % sizeof(BOOK_ENTRY)) != 0)
    {
        Trace("VerifyBook: Size of book is not a multiple of entry size.\n");
        goto end;
    }

    //
    // Foreach book entry...
    //
    uNum = (s.st_size / sizeof(BOOK_ENTRY));
    u64LastSig = 0;
    uLastMove = 0;
    for (u = 0; u < uNum; u++)
    {
        //
        // Seek to and read the entry.
        //
        uPos = u * sizeof(BOOK_ENTRY);
        if (uPos != lseek(fd, uPos, SEEK_SET))
        {
            Trace("VerifyBook: Can't seek to entry %u (offset %u) in book.\n",
                  u, uPos);
            goto end;
        }
        iRet = read(fd, &be, sizeof(BOOK_ENTRY));
        if (sizeof(BOOK_ENTRY) != iRet)
        {
            Trace("VerifyBook: Can't read entry number %u (offset %u) in the "
                  "book. Read returned %u but I expected %u.\n",
                  u, uPos, iRet, sizeof(BOOK_ENTRY));
            goto end;
        }

        // 
        // Make sure the book is sorted by signature
        //
        if (be.u64Sig < u64LastSig)
        {
            Trace("VerifyBook: Book signatures out-of-order at index %u "
                  "(offset %u).\n", u, uPos);
            goto end;
        }
        
        //
        // If our sig is equal to the last entry's then the move here must
        // be greater than or equal to the last entry's move.
        // 
        else if (be.u64Sig == u64LastSig)
        {
            if (be.mvNext.uMove < uLastMove)
            {
                Trace("VerifyBook: Book moves out-of-order at index %u "
                      "(offset %u).\n", u, uPos);
                goto end;
            }
        }
        
        u64LastSig = be.u64Sig;
        uLastMove = be.mvNext.uMove;
    }
    fMyRetVal = TRUE;
    
 end:
    if (-1 != fd)
    {
        close(fd);
    }
    return(fMyRetVal);
}


static CHAR *
_BookLineToString(UINT64 u64Line)
/**

Routine description:

    Converts a position signature into an opening name, if known.

Parameters:

    UINT64 u64Line

Return value:

    static CHAR

**/
{
    ULONG u = 0;
    
    if (NULL == g_pOpeningNames) return(NULL);
    while(g_pOpeningNames[u].szString != NULL)
    {
        if (g_pOpeningNames[u].u64Sig == u64Line)
        {
            return(g_pOpeningNames[u].szString);
        }
        u++;
    }
    return(NULL);
}


static void 
_DumpUserOpeningList(void)
/**

Routine description:

    Called in response to a user "book dump openings" command, this
    routine simply shows every named opening in the opening names
    file.  This file is not the opening book, it's an auxillary disk
    file containing a list of moves and a name for the opening.  It's
    only used to announce the opening line played in a game and
    annotate the PGN for a game stored in monsoon's logfile.

Parameters:

    void

Return value:

    static void

**/
{
    int fd = open(OPENING_LEARNING_FILENAME, 
                  O_RDWR | O_CREAT | O_BINARY | O_RANDOM, 
                  _S_IREAD | _S_IWRITE);
    ULONG uNumRecords;
    ULONG u;
    OPENING_LEARNING_ENTRY ole;
    struct stat s;
    CHAR *p;

    //
    // If we failed to open it, abort.
    // 
    if (fd < 0) goto end;

    //
    // Stat it to see how many entries there are.
    // 
    if (0 != stat(OPENING_LEARNING_FILENAME, &s)) 
    {
        ASSERT(FALSE);
        Trace("UpdateUserOpeningList: Corrupt %s file.\n", 
              OPENING_LEARNING_FILENAME);
        goto end;
    }
    if ((s.st_size % sizeof(OPENING_LEARNING_ENTRY)) != 0) 
    {
        ASSERT(FALSE);
        Trace("UpdateUserOpeningList: Corrupt %s file.\n",
              OPENING_LEARNING_FILENAME);
        goto end;
    }
    uNumRecords = s.st_size / sizeof(OPENING_LEARNING_ENTRY);
    Trace("Learned data on %u openings:\n\n", uNumRecords);
    
    //
    // Read each entry looking for a match.
    // 
    for (u = 0; u < uNumRecords; u++)
    {
        if (sizeof(OPENING_LEARNING_ENTRY) != 
            read(fd, &ole, sizeof(OPENING_LEARNING_ENTRY)))
        {
            Trace("UpdateUserOpeningList: Read error.\n");
            goto end;
        }

        p = _BookLineToString(ole.u64Sig);
        if (NULL != p)
        {
            Trace("%-45s ww=%u, dr=%u, bw=%u\n",
                  p, ole.uWhiteWins, ole.uDraws, ole.uBlackWins);
        }
    }
    
 end:
    if (fd >= 0)
    {
        close(fd);
    }
}

FLAG 
InitializeOpeningBook(void)
/**

Routine description:

Parameters:

    void

Return value:

    FLAG

**/
{
    return(TRUE);
}

void 
ResetOpeningBook(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    g_uBookProbeFailures = 0;
}

void 
CleanupOpeningBook(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    ULONG x = 0;
    
    //
    // If we have read an "opening names" list and created a dynamic memory
    // structure for it, free that now.
    // 
    if (NULL != g_pOpeningNames)
    {
        while (g_pOpeningNames[x].szString != NULL)
        {
            SystemFreeMemory(g_pOpeningNames[x].szString);
            g_pOpeningNames[x].szString = NULL;
            g_pOpeningNames[x].u64Sig = 0;
            x++;
        }
        SystemFreeMemory(g_pOpeningNames);
        g_pOpeningNames = NULL;
    }
    
    //
    // Cleanup membook if they quit in the middle of book building somehow
    // 
    if (NULL != g_pMemBook)
    {
        ASSERT(FALSE);
        SystemFreeMemory(g_pMemBook);
    }
}


static FLAG 
_BookOpen(void)
/**

Routine description:

Parameters:

    void

Return value:

    static FLAG

**/
{
    //
    // Need to have set the book name before calling this.
    // 
    if (!strlen(g_Options.szBookName))
    {
        return(FALSE);
    }
    
    //
    // Open it.
    // 
    ASSERT(g_fdBook == -1);
    g_fdBook = open(g_Options.szBookName, 
                    O_RDWR | O_CREAT |
                    O_RANDOM | O_BINARY | _S_IREAD | _S_IWRITE);
    if (g_fdBook < 0)
    {
        Bug("_BookOpen: Failed to open book \"%s\".\n", g_Options.szBookName);
        return(FALSE);
    }
    
    return((FLAG)(g_fdBook >= 0));
}


static void 
_BookClose(void)
/**

Routine description:

Parameters:

    void

Return value:

    static void

**/
{
    if (-1 != g_fdBook)
    {
        (void)close(g_fdBook);
    }
    g_fdBook = -1;
}


static FLAG 
_BookSeek(ULONG n)
/**

Routine description:

Parameters:

    ULONG n

Return value:

    static FLAG

**/
{
    ULONG uPos;

    //
    // During normal book operations we have an open file handle and we
    // simply calculate and set its file pointer.
    //
    // However when in the process of building the opening book from a
    // file full of PGN games all book commands access a large buffer in
    // memory... so instead of setting a file pointer to a byte offset in
    // the book file on disk, we set a normal pointer to an offset in
    // this large book building buffer.
    //
    if (g_pMemBook == NULL)
    {
        uPos = n * sizeof(BOOK_ENTRY);
        ASSERT(g_fdBook != -1);
    
        if (uPos != lseek(g_fdBook, uPos, SEEK_SET))
        {
            Trace("_BookSeek: Cannot seek to byte offset %u\n", uPos);
            return(FALSE);
        }
    }
    else
    {
        ASSERT(g_uMemBookSize >= 0);
        ASSERT(n < g_uMemBookSize);
        g_pMemBookHead = g_pMemBook + n;
    }
    return(TRUE);
}

#if 0
static FLAG 
_BookWrite(BOOK_ENTRY *x)
/**

Routine description:

Parameters:

    BOOK_ENTRY *x

Return value:

    static FLAG

**/
{
    int iNum;
    
    //
    // If we are building the book, operate on memory
    // buffer... otherwise operate on the book file.
    //
    if (g_pMemBook != NULL)
    {
        memcpy(g_pMemBookHead, x, sizeof(BOOK_ENTRY));
    }
    else
    {
        ASSERT(g_fdBook != -1);
    
        iNum = write(g_fdBook, x, sizeof(BOOK_ENTRY));
        if (sizeof(BOOK_ENTRY) != iNum)
        {
            Trace("_BookWrite: I/O error on fd=%d, got %d and expected %d\n", 
                  g_fdBook, iNum, sizeof(BOOK_ENTRY));
            ASSERT(FALSE);
            return(FALSE);
        }
    }
    return(TRUE);
}
#endif

static FLAG 
_BookRead(BOOK_ENTRY *x)
/**

Routine description:

Parameters:

    BOOK_ENTRY *x

Return value:

    static FLAG

**/
{
    int iNum;

    if (g_pMemBook != NULL)
    {
        memcpy(x, g_pMemBookHead, sizeof(BOOK_ENTRY));
    }
    else
    {
        ASSERT(g_fdBook != -1);

        iNum = read(g_fdBook, x, sizeof(BOOK_ENTRY));
        if (sizeof(BOOK_ENTRY) != iNum)
        {
            Trace("_BookRead: I/O error on fd=%d, got %d and expected %d\n", 
                  g_fdBook, iNum, sizeof(BOOK_ENTRY));
            perror("_BookRead");
            ASSERT(FALSE);
            return(FALSE);
        }
    }
    return(TRUE);
}


static ULONG 
_BookCount(void)
/**

Routine description:

Parameters:

    void

Return value:

    static ULONG

**/
{
    struct stat s;
    
    if (NULL != g_pMemBook)
    {
        return(g_uMemBookCount);
    }
    else
    {
        if (!strlen(g_Options.szBookName))
        {
            return(0);
        }
        
        if (0 != stat(g_Options.szBookName, &s))
        {
            Trace("_BookCount: Stat failed.\n");
            ASSERT(FALSE);
            return(0);
        }
        else
        {
            if ((s.st_size % sizeof(BOOK_ENTRY)) != 0)
            {
                Trace("_BookCount: Size is %u... not a mult of %u!\n", 
                      s.st_size, sizeof(BOOK_ENTRY));
                return(0);
            }
            return(s.st_size / sizeof(BOOK_ENTRY));
        }
    }
}

#if 0
static FLAG 
_BookAppend(BOOK_ENTRY *x)
/**

Routine description:

Parameters:

    BOOK_ENTRY *x

Return value:

    static FLAG

**/
{
    if (g_pMemBook != NULL)
    {
        g_pMemBookHead = g_pMemBook + g_uMemBookCount;
        memcpy(g_pMemBookHead, x, sizeof(BOOK_ENTRY));
        g_uMemBookCount++;
        return(TRUE);
    }
    else
    {
        ASSERT(g_fdBook != -1);
        
        if (-1 == lseek(g_fdBook, 0, SEEK_END))
        {
            ASSERT(FALSE);
            return(FALSE);
        }
        return((FLAG)(sizeof(BOOK_ENTRY) == 
                      write(g_fdBook, x, sizeof(BOOK_ENTRY))));
    }
}
#endif

static ULONG 
_BookFindSig(UINT64 u64Sig, ULONG *puLow, ULONG *puHigh)
/**

Routine description:

Parameters:

    UINT64 u64Sig,
    ULONG *puLow,
    ULONG *puHigh

Return value:

    static ULONG

**/
{
    ULONG uLow = 0;
    ULONG uHigh = 0;
    ULONG uCurrent;
    ULONG uIndex;
    ULONG uTotalEntries = _BookCount();
    BOOK_ENTRY entry;
    ULONG uRetVal = 0;
    FLAG fFound = FALSE;

    //
    // Open the book.
    //
    if (0 == uTotalEntries)
    {
        Trace("BookFindSig: Opening book \"%s\" is empty.\n", 
              g_Options.szBookName);
        goto end;
    }
    if (g_fdBook < 0)
    {
        Trace("BookFindSig: No book is opened!\n");
        goto end;
    }

    //
    // Perform a a binary search looking for an entry with a signature
    // that matches this position's.
    // 
    uLow = 0;
    uHigh = uTotalEntries - 1;

    while ((uLow <= uHigh) && 
           (FALSE == fFound))
    {
        uCurrent = (uLow + uHigh) / 2;
        _BookSeek(uCurrent);
        _BookRead(&entry);

        if (u64Sig == entry.u64Sig)
        {
            fFound = TRUE;
            break;
        }
        else if (u64Sig < entry.u64Sig)
        {
            uHigh = uCurrent - 1;
        }
        else
        { 
            ASSERT(u64Sig > entry.u64Sig);
            uLow = uCurrent + 1;
        }
    }
    if (FALSE == fFound)
    {
        goto end;                             // not found
    }
    ASSERT(uCurrent >= 0);
    ASSERT(uCurrent < uTotalEntries);
    uRetVal = uCurrent;
    
    //
    // Ok, we matched a signature at current.  Since there can be many
    // entries with the same checksum (many moves from the same board
    // position), seek backwards until the first one matching the
    // checksum.
    //
    uIndex = uCurrent - 1;
    while (uIndex != (ULONG)-1)
    {
        _BookSeek(uIndex);
        _BookRead(&entry);

        if (entry.u64Sig != u64Sig) 
        {
            uIndex++;
#ifdef DEBUG
            ASSERT(uIndex <= uCurrent);
            _BookSeek(uIndex);
            _BookRead(&entry);
            ASSERT(entry.u64Sig == u64Sig);
#endif
            break;
        }
        uIndex--;
    }
    uLow = uIndex;

    //
    // Walk forward until the last book entry with a matching signature.
    //
    uIndex = uCurrent + 1;
    while (uIndex < uTotalEntries)
    {
        _BookSeek(uIndex);
        _BookRead(&entry);
        if (entry.u64Sig != u64Sig) 
        {
            uIndex--;
#ifdef DEBUG
            ASSERT(uIndex >= uCurrent);
            _BookSeek(uIndex);
            _BookRead(&entry);
            ASSERT(entry.u64Sig == u64Sig);
#endif
            break;
        }
        uIndex++;
    }
    uHigh = uIndex;

 end:
    *puLow = uLow;
    *puHigh = uHigh;
    return(uRetVal);
}


static ULONG _BookComputeWeight(BOOK_ENTRY *e)
/**

Routine description:

  
    Given a book entry, compute its weight.  The higher the more
    likely it will be chosen and played.  Used in BookMove and
    BookEdit.  The formula is:
                            
        weight(mv) = winningness_coefficient * popularity_factor
   
                                       wins
        winningness_coefficient = ---------------
                                  (wins + losses)
   
        popularity_factor = wins + losses + draws
  
Parameters:

    BOOK_ENTRY *e

Return value:

    static ULONG

**/
{
    ULONG uRet;                               // computed return value
    double dWinning = 0.0;                    // winningness coefficient
    double dPopular;                          // popularity factor

    //
    // dWinning is 0.0 now.  The only way it gets a value is if we've
    // seen this line win at least once.
    // 
    if (e->uWins > 0)
    {
        dWinning = e->uWins;
        dWinning /= (double)(e->uWins + e->uLosses);
        
        //
        // dWinning is now the ratio of wins to deterministic games
        // (games that were not draws).  If this is less than 20% then
        // ignore the line (give the move a zero weight).
        // 
        if (dWinning < 0.20) return(0);
    }

    //
    // If we get here dWinning, the winningness coefficient, has been
    // computed and is non-zero.  Compute the rest of the weight term
    // which is based on the popularity of the move (how many times it
    // has been played).
    // 
    dPopular = (double)(e->uWins + e->uLosses + e->uDraws);
    uRet = (ULONG)(dPopular * dWinning);
    return(uRet);
}


static void 
_QuickSortBook(ULONG uLeft, ULONG uRight)
/**

Routine description:

Parameters:

    ULONG uLeft,
    ULONG uRight

Return value:

    static void

**/
{
    UINT64 u64Pivot;
    ULONG uPivotSec;
    ULONG uL, uR, uMid;
    BOOK_ENTRY temp;

    ASSERT(uLeft >= 0);
    ASSERT(uLeft < g_uMemBookSize);
    ASSERT(uRight >= 0);
    ASSERT(uRight < g_uMemBookSize);
    if (uLeft < uRight)
    {
        //
        // Select a pivot point.
        //
        u64Pivot = g_pMemBook[uLeft].u64Sig;
        uPivotSec = g_pMemBook[uLeft].mvNext.uMove;
        
        //
        // Partition the space based on the pivot.  Everything left of 
        // it is less, everything right is greater.
        // 
        uL = uLeft;
        uR = uRight;
        while (uL < uR)
        {
            while (((g_pMemBook[uL].u64Sig < u64Pivot) ||
                    ((g_pMemBook[uL].u64Sig == u64Pivot) &&
                     (g_pMemBook[uL].mvNext.uMove <= uPivotSec))) &&
                   (uL <= uRight))
            {
                uL++;
            }
            
            while (((g_pMemBook[uR].u64Sig > u64Pivot) ||
                    ((g_pMemBook[uR].u64Sig == u64Pivot) &&
                     (g_pMemBook[uR].mvNext.uMove > uPivotSec))) &&
                   (uR >= uLeft))
            {
                uR--;
            }
            
            if (uL < uR)
            {
                temp = g_pMemBook[uL];
                g_pMemBook[uL] = g_pMemBook[uR];
                g_pMemBook[uR] = temp;
            }
        }
        uMid = uR;
        
        temp = g_pMemBook[uLeft];
        g_pMemBook[uLeft] = g_pMemBook[uMid];
        g_pMemBook[uMid] = temp;

        //
        // Recurse on the two halves
        // 
        if (uLeft != uMid)
        {
            _QuickSortBook(uLeft, uMid - 1);
        }
        if (uMid != uRight)
        {
            _QuickSortBook(uMid + 1, uRight);
        }
    }
}


static void 
_CompactMemBook(void)
/**

Routine description:

Parameters:

    void

Return value:

    static void

**/
{
    ULONG uCount = 0;
    ULONG i;
    ULONG uOccurances;
    
    Trace("Compacting the opening book... one moment.\n");

    //
    // Lose the blank signatures to reduce the working set.
    //
    for (i = 0;
         i < g_uMemBookSize; 
         i++)
    {
        if (0 != g_pMemBook[i].u64Sig) 
        {
            uOccurances = (g_pMemBook[i].uWins +
                           g_pMemBook[i].uDraws +
                           g_pMemBook[i].uLosses);
            
            if (0 != uOccurances)
            {
                memcpy(&(g_pMemBook[uCount]),
                       &(g_pMemBook[i]),
                       sizeof(BOOK_ENTRY));
                uCount++;
            }
        }
    }

    g_uMemBookCount = 0;
    if (uCount > 0)
    {
        g_uMemBookCount = uCount - 1;
    }
}


static void 
_StrainMemBook(ULONG uLimit)
/**

Routine description:

Parameters:

    ULONG uLimit

Return value:

    static void

**/
{
    ULONG i;
    ULONG uOccurances;

    Trace("Straining out unpopular positions to compact buffer...\n");

    for (i = 0; 
         i < g_uMemBookSize; 
         i++)
    {
        if (0 != g_pMemBook[i].u64Sig)
        {
            uOccurances = (g_pMemBook[i].uWins +
                           g_pMemBook[i].uDraws +
                           g_pMemBook[i].uLosses);

            if (uOccurances <= uLimit)
            {
                memset(&(g_pMemBook[i]), 
                       0, 
                       sizeof(BOOK_ENTRY));
                g_uMemBookCount--;
            }
        }
    }
}
              

//+----------------------------------------------------------------------------
//
// Function:  _BookHashFind
//
// Arguments: BOOK_ENTRY *p - pointer to a book entry
//            
// Returns:   BOOK_ENTRY * - pointer to a membook location to store in
//
//+----------------------------------------------------------------------------





BOOK_ENTRY *_BookHashFind(BOOK_ENTRY *p)
/**

Routine description:

    This routine returns a pointer to the membook entry that should be
    used to store data about a position-move.  It's called while
    building a new opening book (BookBuild, see below).  It uses a
    simple hash-lookup algorthm with linear probing in the event of a
    collision.

Parameters:

    BOOK_ENTRY *p

Return value:

    BOOK_ENTRY

**/
{
    ULONG uKey = (ULONG)((p->u64Sig >> 32) & (g_uMemBookSize - 1));
    BOOK_ENTRY *pBe = g_pMemBook + uKey;
#ifdef DEBUG
    ULONG uInitialKey = uKey;

    ASSERT(g_pMemBook != NULL);
    ASSERT(p->u64Sig);
#endif
    
    while(1)
    {
        //
        // If this is an empty book entry, use it.
        //
        if (pBe->u64Sig == 0) break;

        //
        // If this book entry is the same position and move as the one
        // we are adding, return it.
        //
        if ((pBe->u64Sig == p->u64Sig) &&
            (pBe->mvNext.uMove == p->mvNext.uMove))
        {
            break;
        }
        
        //
        // Otherwise check the next book entry... if we go off the end
        // then loop around.
        //
        uKey++;
        if (uKey >= g_uMemBookSize) uKey = 0;
        pBe = g_pMemBook + uKey;

        //
        // This should never happen -- it means a totally full book.
        // Before we let the membook fill up we should have already
        // called the strain routine to drop unpopular position-moves.
        //
        ASSERT(uKey != uInitialKey);
    }
    return(pBe);
}



MOVE 
BookMove(POSITION *pos, BITV bvFlags)
/**

Routine description:

    This is a bit confusing.
 
    The primary purpose of this function is, given a position pointer,
    to find and return a book move to play.
 
    This behavior can be achieved by calling with a good pos pointer
    and bvFlags containing BOOKMOVE_SELECT_MOVE.

    If bvFlags doesn't contain this flag then no move will be
    returned.

    If bvFlags contains BOOKMOVE_DUMP this function will trace a bunch
    of info about move weights.  Hence, something like BookDump can be
    achieved via:
 
        (void)BookMove(pos, BOOKMOVE_DUMP);

    This is done so that the code to find all moves from a position
    and show them is not duplicated more than once.

Parameters:

    POSITION *pos,
    BITV bvFlags

Return value:

    MOVE

*/
{
    LIGHTWEIGHT_SEARCHER_CONTEXT *ctx = NULL;
    UINT64 u64Sig;
    ULONG uTotalWeight = 0;
    static ULONG uMoveWeights[32];
    static CHAR *szNames[32];
    ULONG uMoveNum;
    static BOOK_ENTRY entry;
    ULONG uBestWeight = 0;
    ULONG uBestIndex = (ULONG)-1;
    ULONG uLow, uHigh;
    ULONG uIndex;
    MOVE mvBook = {0};
    ULONG uRandom;
    ULONG uCounter;
    ULONG x;
    double d;
    CHAR *szName = NULL;
    FLAG fOldValue = g_Options.fShouldAnnounceOpening;

    g_Options.fShouldAnnounceOpening = FALSE;
#ifdef DEBUG
    memset(uMoveWeights, 0, sizeof(uMoveWeights));
#endif
    ctx = SystemAllocateMemory(sizeof(LIGHTWEIGHT_SEARCHER_CONTEXT));
    if (NULL == ctx) 
    {
        Trace("Out of memory.\n");
        goto end;
    }

    //
    // We cannot probe the opening book if membook is non-NULL (which
    // indicates the opening book is currently in-memory and still
    // being built.
    //
    if (g_pMemBook != NULL) 
    {
        Trace("Cannot probe the book while its still being built!\n");
        ASSERT(FALSE);
        goto end;
    }
    if (FALSE == _BookOpen())
    {
        Trace("BookMove: Could not open the book file.\n");
        goto end;
    }

    //
    // Binary search for sig in the book, this sets iLow and iHigh.
    //
    u64Sig = pos->u64PawnSig ^ pos->u64NonPawnSig;
    if (0 == _BookFindSig(u64Sig, &uLow, &uHigh))
    {
        Trace("BookMove: Signature not in book.\n", u64Sig);
        goto end;
    }
    ASSERT((uHigh - uLow) < 32);
    ASSERT(uLow <= uHigh);
    
    //
    // Consider each entry from iLow to iHigh and determine the chances
    // of playing it.
    //
    for (uIndex = uLow, uMoveNum = 0; 
         uIndex <= uHigh; 
         uIndex++, uMoveNum++)
    {
        _BookSeek(uIndex);
        _BookRead(&entry);
#ifdef DEBUG
        u64Sig = pos->u64PawnSig ^ pos->u64NonPawnSig;
        ASSERT(entry.u64Sig == u64Sig);
#endif
        uMoveWeights[uMoveNum] = 0;
        szNames[uMoveNum] = NULL;

        //
        // Don't consider the move if its flagged deleted or disabled.
        //
        if ((entry.bvFlags & FLAG_DISABLED) ||
            (entry.bvFlags & FLAG_DELETED))
        {
            continue;
        }

        //
        // Make sure it's legal and doesn't draw... also get the name
        // of this opening.
        //
        InitializeLightweightSearcherContext(pos, ctx);
        if (FALSE == MakeMove((SEARCHER_THREAD_CONTEXT *)ctx, entry.mvNext))
        {
            ASSERT(FALSE);
            continue;
        }
        else
        {
            if (TRUE == IsDraw((SEARCHER_THREAD_CONTEXT *)ctx))
            {
                UnmakeMove((SEARCHER_THREAD_CONTEXT *)ctx, entry.mvNext);
                continue;
            }
            
            u64Sig = (ctx->sPosition.u64PawnSig ^
                      ctx->sPosition.u64NonPawnSig);
            szNames[uMoveNum] = _BookLineToString(u64Sig);
            UnmakeMove((SEARCHER_THREAD_CONTEXT *)ctx, entry.mvNext);
        }
        
        //
        // If one is marked always, just return it.
        //
        if ((entry.bvFlags & FLAG_ALWAYSPLAY) &&
            (bvFlags & BOOKMOVE_SELECT_MOVE))
        {
            mvBook = entry.mvNext;
            szName = szNames[uMoveNum];
            goto end;
        }

        //
        // Set this move's weight.
        //
        uMoveWeights[uMoveNum] = _BookComputeWeight(&entry);
        uTotalWeight += uMoveWeights[uMoveNum];
        
        //
        // If we are in tournament mode we will play the top weighted
        // move; look for it now.
        //
        if (uMoveWeights[uMoveNum] > uBestWeight)
        {
            uBestWeight = uMoveWeights[uMoveNum];
            uBestIndex = uIndex;
        }
        
    } // next book move

    //
    // If we threw every move out (there is no total weight) then we
    // don't have a book move here.
    //
    if (0 == uTotalWeight)
    {
        ASSERT(mvBook.uMove == 0);
        goto end;
    }

    //
    // We are done assigning weights to moves.  If we are in tournament
    // mode we should have picked a best one.  If so, return it.
    //
    if ((TRUE == g_fTournamentMode) && (bvFlags & BOOKMOVE_SELECT_MOVE))
    {
        if (uBestIndex != (ULONG)-1)
        {
            ASSERT(uBestWeight);
            _BookSeek(uBestIndex);
            _BookRead(&entry);
            mvBook = entry.mvNext;
            ASSERT(uBestIndex >= uLow);
            szName = szNames[uBestIndex - uLow];
            goto end;
        }
    }

    //
    // We didn't throw everything out... so pick a move randomly based
    // on the weights assigned.
    //
    uRandom = (randomMT() % uTotalWeight) + 1;
    if (bvFlags & BOOKMOVE_DUMP)
    {
        u64Sig = pos->u64PawnSig ^ pos->u64NonPawnSig;
        Trace(" BookMove: List of moves in position 0x%" 
              COMPILER_LONGLONG_HEX_FORMAT ":\n",
              u64Sig);
        for (x = 0; x < uMoveNum; x++)
        {
            _BookSeek(uLow + x);
            _BookRead(&entry);
            ASSERT(entry.u64Sig == u64Sig);
        
            d = 0.0;
            if (uMoveWeights[x] != 0)
            {
                d = (double)uMoveWeights[x] / (double)uTotalWeight;
                d *= 100.0;
            }
            Trace("%02u. %-4s [+%u =%u -%u] \"%s\" @ %5.3f\n", 
                  x, 
                  MoveToSan(entry.mvNext, pos),
                  entry.uWins, entry.uDraws, entry.uLosses,
                  szNames[x], 
                  d);
        }
        Trace(" Total weight was %u.\n\n", uTotalWeight);
        if (!(bvFlags & BOOKMOVE_SELECT_MOVE))
        {
            mvBook.uMove = 0;
            goto end;
        }
    }

    //
    // Based on the weighted list and the random number we
    // selected pick a move.
    //
    for (uIndex = 0, uCounter = 0; 
         uIndex < uMoveNum; 
         uIndex++)
    {
        uCounter += uMoveWeights[uIndex];

        if (uCounter >= uRandom)
        {
            _BookSeek(uLow + uIndex);
            _BookRead(&entry);
            mvBook = entry.mvNext;
            szName = szNames[uIndex];
            goto end;
        }
    }
#ifdef DEBUG
    UtilPanic(SHOULD_NOT_GET_HERE,
              NULL, NULL, NULL, NULL, 
              __FILE__, __LINE__);
#endif

 end:
    g_Options.fShouldAnnounceOpening = fOldValue;
    _BookClose();
    
    if (mvBook.uMove)
    {
        ASSERT(bvFlags & BOOKMOVE_SELECT_MOVE);
        
        //
        // We could be probing the book after a predicted ponder move (in
        // which case we will have forced fAnnounceOpening to FALSE because
        // we don't want to whisper two book moves in a row).
        //
        if (FALSE != g_Options.fShouldAnnounceOpening) {
            Trace("tellothers book move %s [+%u =%u -%u]\n",
                  MoveToSan(entry.mvNext, pos),
                  entry.uWins, entry.uDraws, entry.uLosses);
        }
    }
    
    if (NULL != ctx) 
    {
        SystemFreeMemory(ctx);
    }
    return(mvBook);
}


#if 0
//+---------------------------------------------------------------------------
//
// Function:  BookEdit
//
// Synopsis:  Giant hack to edit the book.
//
// Arguments: POSITION *pos
//            
// Returns:   void
//
//+---------------------------------------------------------------------------
void 
BookEdit(POSITION *pos)
{
    int iTotalEntries = _BookCount();
    int iNumMoves;
    int iLow, iHigh;
    FILE *pfLearn = NULL;
    FLAG fFound = FALSE;
    static BOOK_ENTRY entry;
    int iIndex = 0;
    int iCurrent = 0;
    int x;
    char *pCh, *pArg;
    int arg;
    int iSum;
    int iWeights[64];
    float fl;
    FLAG fOldValue = g_Options.fAnnounceOpening;

    g_Options.fAnnounceOpening = FALSE;
    
    //
    // We cannot probe the opening book if membook is non-NULL (which
    // indicates the opening book is currently in-memory and still being
    // built.
    //
    if (g_pMemBook != NULL) 
    {
        ASSERT(FALSE);
        goto end;
    }

    //
    // Open the book.
    //
    iTotalEntries = _BookCount();
    if (0 == iTotalEntries)
    {
        Trace("BookDump: Opening book \"%s\" is empty.\n", 
              g_Options.szBookName);
        goto end;
    }

    ASSERT(g_fdBook == -1);
    if (!_BookOpen())
    {
        Trace("BookDump: Could not open the book.\n");
        goto end;
    }
    
    //
    // Open the learning recorder
    // 
    pfLearn = fopen(BOOK_EDITING_RECORD, "a+b");
    if (NULL == pfLearn)
    {
        Trace("BookEdit: Warning: can't open %s to save learning.\n",
              BOOK_EDITING_RECORD);
    }

    if (!_BookFindSig(pos->u64Sig, &iLow, &iHigh))
    {
        Trace("BookEdit: Position %I64x is not in the book.\n");
        goto end;
    }
    
    iNumMoves = iHigh - iLow + 1;
    iCurrent = 0;
    do
    {
        printf("[H[J\n");
        Trace("--EDITING OPENING BOOK--\n\n");
        Trace("There are %u positions in the opening book \"%s\".\n", 
              iTotalEntries, g_Options.szBookName);
        Trace("There are %u book moves in this position, %u to %u:\n",
              iNumMoves, iLow, iHigh);
        
        for (x = 0, iSum = 0;
             x < iNumMoves;
             x++)
        {
            _BookSeek(iLow + x);
            _BookRead(&entry);
            
            //
            // If the disabled flag is asserted, do not allow this
            // move to be played.  If the disabled flag is not
            // asserted, calculate a weight for this move.
            //
            if ((entry.bvFlags & FLAG_DISABLED) ||
                (entry.bvFlags & FLAG_DELETED))
            {
                iWeights[x] = 0;
            }
            else
            {
                iWeights[x] = _BookComputeWeight(&entry);
            }
            iSum += iWeights[x];
        }
        
        for (x = 0;
             x < iNumMoves;
             x++)
        {
            _BookSeek(iLow + x);
            _BookRead(&entry);
            
            fl = (float)iWeights[x];
            if (iSum)
            {
                fl /= (float)iSum;
                fl *= 100.0;
            }

            if (x == iCurrent)
            {
                Trace(">> ");
            }
            else
            {
                Trace("   ");
            }
            
            Trace("%02u. %6u..%6u = %4s [+%6u =%6u -%6u] @ ~%6.3f%% | %s%s", 
                  x + 1,
                  0,
                  iWeights[x],
                  MoveToSAN(entry.mvNext, pos),
                  entry.iWins,
                  entry.iDraws,
                  entry.iLosses,
                  fl,
                  (entry.bvFlags & FLAG_DISABLED) ? "NEV" : "",
                  (entry.bvFlags & FLAG_ALWAYSPLAY) ? "ALW" : "");
            
            if (x == iCurrent)
            {
                Trace("<<\n");
            }
            else
            {
                Trace("\n");
            }
        }
        
        _BookSeek(iLow + iCurrent);
        _BookRead(&entry);
        Trace("Command (q, p, n, +, =, -, h, l, e, a, d, ?): ");
        pCh = BlockingRead();

        switch(tolower(*pCh))
        {
        case 'q':
            Trace("--DONE EDITING OPENING BOOK--\n\n");
            goto end;
        case 'p':
            iCurrent--;
            if (iCurrent < 0) iCurrent = iNumMoves - 1;
            break;
        case 'n':
            iCurrent++;
            if (iCurrent >= iNumMoves) iCurrent = 0;
            break;
        case '+':
            Trace("Enter new wincount: ");
            pArg = BlockingRead();
            arg = atoi(pArg);
            if (arg)
            {
                entry.iWins = arg;
                _BookSeek(iLow + iCurrent);
                _BookWrite(&entry);
                if (NULL != pfLearn)
                {
                    fprintf(pfLearn, "%I64x (%x) + %u // %s\n", 
                            entry.u64Sig,
                            entry.mvNext.move,
                            arg,
                            BoardToFEN(pos));
                }
            }
            SystemFreeMemory(pArg);
            break;
        case '=':
            Trace("Enter new drawcount: ");
            pArg = BlockingRead();
            arg = atoi(pArg);
            if (arg)
            {
                entry.iDraws = arg;
                _BookSeek(iLow + iCurrent);
                _BookWrite(&entry);
                if (NULL != pfLearn)
                {
                    fprintf(pfLearn, "%I64x (%x) = %u // %s\n",
                            entry.u64Sig,
                            entry.mvNext.move,
                            arg,
                            BoardToFEN(pos));
                }     
            }
            SystemFreeMemory(pArg);
            break;
        case '-':
            Trace("Enter new losscount: ");
            pArg = BlockingRead();
            arg = atoi(pArg);
            if (arg)
            {
                entry.iLosses = arg;
                _BookSeek(iLow + iCurrent);
                _BookWrite(&entry);
                if (NULL != pfLearn)
                {
                    fprintf(pfLearn, "%I64x (%x) - %u // %s\n",
                            entry.u64Sig,
                            entry.mvNext.move,
                            arg,
                            BoardToFEN(pos));
                }
            }
            SystemFreeMemory(pArg);
            break;
        case 'e':
            if (entry.bvFlags & FLAG_DISABLED)
            {
                entry.bvFlags &= ~FLAG_DISABLED;
            }
            else
            {
                entry.bvFlags |= FLAG_DISABLED;
                entry.bvFlags &= ~FLAG_ALWAYSPLAY;
            }
            _BookSeek(iLow + iCurrent);
            _BookWrite(&entry);
            if (NULL != pfLearn)
            {
                fprintf(pfLearn, "%I64x (%x) FLAGS %x // %s\n",
                        entry.u64Sig,
                        entry.mvNext.move,
                        entry.bvFlags,
                        BoardToFEN(pos));
            }
            break;
        case 'a':
            if (entry.bvFlags & FLAG_ALWAYSPLAY)
            {
                entry.bvFlags &= ~FLAG_ALWAYSPLAY;
            }
            else
            {
                entry.bvFlags |= FLAG_ALWAYSPLAY;
                entry.bvFlags &= ~FLAG_DISABLED;
            }
            _BookSeek(iLow + iCurrent);
            _BookWrite(&entry);
            if (NULL != pfLearn)
            {
                fprintf(pfLearn, "%I64x (%x) FLAGS %x // %s\n",
                        entry.u64Sig,
                        entry.mvNext.move,
                        entry.bvFlags,
                        BoardToFEN(pos));
            }
            break;
        case 'h':
            PreferPosition(pos, pfLearn, pos->u64Sig, -1);
            Trace("I hate this position.\n");
            break;
        case 'i':
            PreferPosition(pos, pfLearn, pos->u64Sig, 0);
            Trace("I'm indifferent towards this position.\n");
            break;
        case 'l':
            PreferPosition(pos, pfLearn, pos->u64Sig, 1);
            Trace("I love this position!\n");
            break;
        case '?':
            Trace(
"Commands: \n"
"           +) adjust wins     =) adjust draws   -) adjust losses\n"
"           h) hate this pos   i) indifferent    l) love this pos\n\n"

"           e) enable/disable\n"
"           a) always/sometimes play\n\n"

"           p) previous move   n) next move      q) quit editing\n\n");
            break;
        default:
            break;
        }
        SystemFreeMemory(pCh);
    }
    while(1);

 end:
    g_Options.fAnnounceOpening = fOldValue;
    
    ASSERT(g_fdBook != -1);
    _BookClose();

    if (NULL != pfLearn)
    {
        fclose(pfLearn);
    }
}
#endif





static FLAG 
_BookBuild(void)
/**

Routine description:

    Builds a new opening book.  This routine is called by
    LearnPGNOpenings in movelist.c after it has played through one PGN
    game.  g_MoveList is loaded with position signatures and move
    structs representing this game.  Our goal is to parse these and
    create/update the membook entries to reflect the game played and
    its result.

Parameters:

    void

Return value:

    static FLAG

**/
{
    BOOK_ENTRY entry;
    BOOK_ENTRY *pEntry;
    ULONG n;
    GAME_MOVE *q;
    ULONG u;
    double dFull;
    GAME_RESULT result = GetGameResult();

    if (result.eResult != RESULT_WHITE_WON &&
        result.eResult != RESULT_BLACK_WON &&
        result.eResult != RESULT_DRAW) {
        return(FALSE);
    }
    
    //
    // If the membook has not yet been allocated, do it now.
    // 
    if (g_pMemBook == NULL)
    {
        ASSERT((g_uMemBookSize & (g_uMemBookSize - 1)) == 0);
        g_uMemBookCount = 0;
        g_pMemBookHead = NULL;
        g_pMemBook = (BOOK_ENTRY *)
            SystemAllocateMemory(g_uMemBookSize * sizeof(BOOK_ENTRY));
        g_pMemBookHead = g_pMemBook;
        ASSERT(g_pMemBook != NULL);
    }

    //
    // Consider each position in the game we have played up until now.
    //
    u = 0;
    q = GetNthOfficialMoveRecord(u);
    while((q != NULL) && (q->mv.uMove != 0))
    {
        entry.u64Sig = q->u64PositionSigBeforeMove;
        entry.mvNext = q->mv;
        entry.u64NextSig = q->u64PositionSigAfterMove;
        
        //
        // Have we ever seen this position before?
        //
        pEntry = _BookHashFind(&entry);
        if (0 != pEntry->u64Sig)
        {
            //
            // Yes we have seen this signature-move before.  Update the
            // win/draw/loss counter at this entry in the membook.
            // 
            ASSERT(pEntry->u64Sig == entry.u64Sig);
            ASSERT(pEntry->mvNext.uMove == entry.mvNext.uMove);
            if (((result.eResult == RESULT_BLACK_WON) && 
                 (GET_COLOR(entry.mvNext.pMoved) == BLACK)) ||
                ((result.eResult == RESULT_WHITE_WON) &&
                 (GET_COLOR(entry.mvNext.pMoved) == WHITE)))
            {
                pEntry->uWins++;
            }
            else
            {
                if (RESULT_DRAW == result.eResult)
                {
                    pEntry->uDraws++;
                }
                else
                {
                    pEntry->uLosses++;
                }
            }
        }
        
        //
        // Nope never seen this signature-move entry before, create a new
        // entry for it and add it to the membook.
        // 
        else
        {
            memset(pEntry, 0, sizeof(pEntry));
            pEntry->u64Sig = entry.u64Sig;
            pEntry->mvNext = entry.mvNext;
            pEntry->u64NextSig = entry.u64NextSig;
            
            if (((result.eResult == RESULT_BLACK_WON) && 
                 (GET_COLOR(entry.mvNext.pMoved) == BLACK)) ||
                ((result.eResult == RESULT_WHITE_WON) && 
                 (GET_COLOR(entry.mvNext.pMoved) == WHITE)) )
            {
                pEntry->uWins = 1;
            }
            else
            {
                if (RESULT_DRAW == result.eResult)
                {
                    pEntry->uDraws = 1;
                }
                else
                {
                    pEntry->uLosses = 1;
                }
            }
            g_uMemBookCount++;

            //
            // If the in memory buffer is getting too full, throw out
            // all positions that occur only once in it in order to
            // compact it.  We should try to limit this because it can
            // weaken the book.
            //
            n = 1;
            dFull = (double)g_uMemBookCount;
            dFull /= (double)g_uMemBookSize;
            dFull *= 100.0;
            while (dFull > 97.0)
            {
                _StrainMemBook(n);
                n++;
                
                dFull = (double)g_uMemBookCount;
                dFull /= (double)g_uMemBookSize;
                dFull *= 100.0;
            }
        }
        u++;
        q = GetNthOfficialMoveRecord(u);
    }
    return(TRUE);
}


static FLAG 
_BookToDisk(ULONG uStrainLimit)
/**

Routine description:

Parameters:

    ULONG uStrainLimit

Return value:

    static FLAG

**/
{
    ULONG i;
    int ifd;
    CHAR *szTempBook = "tempbook.bin";
    CHAR szCmd[SMALL_STRING_LEN_CHAR];
    BOOK_ENTRY sBe;
    ULONG uOldBook = 0;
    ULONG uNewBook = 0;
    ULONG uOld_BookCount;
    ULONG uNew_BookCount;
    BOOK_ENTRY *pOldBook = NULL;
    BOOK_ENTRY *pNewBook = NULL;
    FLAG fMyRetVal = FALSE;
    struct stat s;

    if (!strlen(g_Options.szBookName))
    {
        return(FALSE);
    }
    i = g_uMemBookCount;

    //
    // Drops all book entries with less than limit occurrances
    //
    _StrainMemBook(uStrainLimit);

    //
    // Drops all unused entries
    //
    Trace("Stage 2: Compacting the book.\n");
    _CompactMemBook();

    //
    // Sorts the by signature.
    //
    Trace("Stage 3: Sorting the book.\n");
    _QuickSortBook(0, g_uMemBookCount);
    uNew_BookCount = g_uMemBookCount;

    //
    // Create the temporary book on the disk.
    // 
    (void)unlink(szTempBook);
    if (SystemDoesFileExist(szTempBook))
    {
        Trace("_BookToDisk: Can't create a temporary book!\n");
        return(FALSE);
    }
    
    ifd = open(szTempBook, 
               O_RDWR | O_CREAT | O_BINARY,
               _S_IREAD | _S_IWRITE);
    if (ifd < 0)
    {
        Trace("_BookToDisk: Can't create a temporary book!\n");
        return(FALSE);
    }
#ifndef _MSC_VER
    fchmod(ifd, 0644);
#endif
    
    //
    // Count the entries in the old book on disk.
    // 
    uOld_BookCount = 0;
    if ((TRUE == _VerifyBook(g_Options.szBookName)) && (TRUE == _BookOpen()))
    {
        if (0 == stat(g_Options.szBookName, &s))
        {
            uOld_BookCount = (s.st_size / sizeof(BOOK_ENTRY));
        }
    }
    
    //
    // Merge the old book (on disk) with the new (in memory) in the
    // temp book.
    // 
    Trace("Merging book lines and writing to disk...\n");
    do
    {
        pOldBook = pNewBook = NULL;
        
        //
        // Read the next old book entry.
        // 
 retry:
        if (uOldBook < uOld_BookCount)
        {
            if ((int)(uOldBook * sizeof(BOOK_ENTRY)) != 
                lseek(g_fdBook, (uOldBook * sizeof(BOOK_ENTRY)), SEEK_SET))
            {
                Trace("_BookToDisk: Cannot seek to byte offset %u in old "
                      "book.\n", uOldBook * sizeof(BOOK_ENTRY));
                ASSERT(FALSE);
                goto end;
            }

            if (sizeof(BOOK_ENTRY) != read(g_fdBook,
                                           &sBe, 
                                           sizeof(BOOK_ENTRY)))
            {
                Trace("_BookToDisk: Can't read error in old book, trying "
                      "to recover...\n");
                uOldBook++;
                goto retry;
            }
            pOldBook = &sBe;
        }
        
        //
        // Get the next new book entry
        // 
        if (uNewBook < uNew_BookCount)
        {
            ASSERT(0 != g_pMemBook[uNewBook].u64Sig);
            pNewBook = &(g_pMemBook[uNewBook]);
        }

        //
        // Decide which to write into the merged (temporary) book we
        // are constructing next...
        // 
        if ((NULL == pNewBook) && (NULL == pOldBook))
        {
            break;
        }
        else if ((NULL == pNewBook) && (NULL != pOldBook))
        {
            goto writeoldbook;
        }
        else if ((NULL != pNewBook) && (NULL == pOldBook))
        {
            goto writenewbook;
        }
        else
        {
            if (pOldBook->u64Sig < pNewBook->u64Sig)
            {
                goto writeoldbook;
            }
            else if (pOldBook->u64Sig > pNewBook->u64Sig)
            {
                goto writenewbook;
            }
            else
            {
                ASSERT(pOldBook->u64Sig == pNewBook->u64Sig);
                if (pOldBook->mvNext.uMove < pNewBook->mvNext.uMove)
                {
                    goto writeoldbook;
                }
                else if (pOldBook->mvNext.uMove > pNewBook->mvNext.uMove)
                {
                    goto writenewbook;
                }
                else
                {
                    //
                    // Same position and move...
                    //
                    pNewBook->uWins += pOldBook->uWins;
                    pNewBook->uDraws += pOldBook->uDraws;
                    pNewBook->uLosses += pOldBook->uLosses;
                    pNewBook->bvFlags |= pOldBook->bvFlags;
                    uOldBook++;
                    goto writenewbook;
                }                
            }
        }
#ifdef DEBUG
        UtilPanic(SHOULD_NOT_GET_HERE,
                  NULL, NULL, NULL, NULL, 
                  __FILE__, __LINE__);
#endif
        
 writeoldbook:
        if (-1 == lseek(ifd, 0, SEEK_END))
        {
            ASSERT(FALSE);
        }
        if (sizeof(BOOK_ENTRY) != write(ifd, pOldBook, sizeof(BOOK_ENTRY)))
        {
            Trace("_BookToDisk: Error writing new book, aborting procedure.\n");
            goto end;
        }
        uOldBook++;
        continue;
        
 writenewbook:
        if (-1 == lseek(ifd, 0, SEEK_END))
        {
            ASSERT(FALSE);
        }
        if (sizeof(BOOK_ENTRY) != write(ifd, pNewBook, sizeof(BOOK_ENTRY)))
        {
            Trace("_BookToDisk: Error writing new book, aborting procedure."
                  "\n");
            goto end;
        }
        uNewBook++;
        continue;
    }
    while(1);
    fMyRetVal = TRUE;
    
 end:
    close(ifd);
    SystemFreeMemory(g_pMemBook);
    g_pMemBook = NULL;
    g_pMemBookHead = NULL;
    g_uMemBookCount = 0;
    _BookClose();

    //
    // Verify the consistency of the new book before klobbering the old one
    //
    if (FALSE == _VerifyBook(szTempBook))
    {
        Trace("_BookToDisk: The book I just wrote is corrupt!!!\n");
        fMyRetVal = FALSE;
        ASSERT(FALSE);
    }
    else
    {
        //
        // HACKHACK: put this in system, use CopyFile in win32
        //
#ifdef _MSC_VER
        _snprintf(szCmd, 255, "move %s %s%c", szTempBook, 
                  g_Options.szBookName, 0);
#else
        snprintf(szCmd, 255, "/bin/mv %s %s%c", szTempBook, 
                 g_Options.szBookName, 0);
#endif
        system(szCmd);
    }
    
    if (TRUE == fMyRetVal)
    {
        Trace("Book successfully built.\n");
    }
    return(fMyRetVal);
}


static FLAG 
_LearnPgnOpenings(CHAR *szFilename, ULONG uStrainLimit)
/**

Routine description:

Parameters:

    CHAR *szFilename,
    ULONG uStrainLimit

Return value:

    static FLAG

**/
{
    CHAR *p;
    FILE *pf = NULL;
    struct stat s;
    double d;

    if (FALSE == SystemDoesFileExist(szFilename))
    {
        Trace("LearnPgnOpenings: file \"%s\" doesn't exist.\n",
              szFilename);
        return(FALSE);
    }
    
    if (0 != stat(szFilename, &s))
    {
        Trace("LearnPgnOpenings: Can't read file \"%s\".\n",
              szFilename);
        return(FALSE);
    }
    
    pf = fopen(szFilename, "rb");
    if (NULL == pf) 
    {    
        Trace("LearnPgnOpenings: error reading file %s.\n", 
              szFilename);
        return(FALSE);
    }
    
    CleanupHashSystem();

    Trace("Stage 1: reading and parsing PGN\n");
    while((p = ReadNextGameFromPgnFile(pf)) != NULL)
    {
        if (TRUE == LoadPgn(p))
        {
            _BookBuild();
            d = (double)ftell(pf);
            d /= (double)s.st_size;
            d *= 100.0;
            printf("%u/%u (%5.2f%% done) ",
                   (unsigned)ftell(pf), (unsigned)s.st_size, d);
            d = (double)g_uMemBookCount;
            d /= (double)g_uMemBookSize;
            d *= 100.0;
            printf("[membook %5.2f%% full]\r", d);
        }
        SystemFreeMemory(p);
    }
    fclose(pf);
    
    _BookToDisk(uStrainLimit);
    
    InitializeHashSystem();
    
    return(TRUE);
}


#if 0
static void
_UpdateUserOpeningList(INT iResult)
{
    int fd = open(OPENING_LEARNING_FILENAME, 
                  O_RDWR | O_CREAT | O_BINARY | O_RANDOM, 
                  _S_IREAD | _S_IWRITE);
    ULONG uNumRecords;
    ULONG u;
    OPENING_LEARNING_ENTRY ole;
    ULONG uPos;
    struct stat s;

    //
    // If we failed to open it, abort.
    // 
    if (fd < 0) goto end;

    //
    // Stat it to see how many entries there are.
    // 
    if (0 != stat(OPENING_LEARNING_FILENAME, &s)) 
    {
        ASSERT(FALSE);
        Trace("UpdateUserOpeningList: Corrupt %s file.\n", 
              OPENING_LEARNING_FILENAME);
        goto end;
    }
    if ((s.st_size % sizeof(OPENING_LEARNING_ENTRY)) != 0) 
    {
        ASSERT(FALSE);
        Trace("UpdateUserOpeningList: Corrupt %s file.\n",
              OPENING_LEARNING_FILENAME);
        goto end;
    }
    uNumRecords = s.st_size / sizeof(OPENING_LEARNING_ENTRY);
    
    //
    // Read each entry looking for a match.
    // 
    for (u = 0; u < uNumRecords; u++)
    {
        if (sizeof(OPENING_LEARNING_ENTRY) != 
            read(fd, &ole, sizeof(OPENING_LEARNING_ENTRY)))
        {
            Trace("UpdateUserOpeningList: Read error.\n");
            goto end;
        }
        
        if (ole.u64Sig == g_Options.u64OpeningSig)
        {
            //
            // Found it, so update and write back in place.
            // 
            switch (iResult)
            {
                case -1:
                    ole.uBlackWins++;
                    break;
                case 0:
                    ole.uDraws++;
                    break;
                case 1:
                    ole.uWhiteWins++;
                    break;
#ifdef DEBUG
                default:
                    ASSERT(FALSE);
                    break;
#endif
            }
            
            uPos = u * sizeof(OPENING_LEARNING_ENTRY);
            if (uPos != lseek(fd, uPos, SEEK_SET))
            {
                Trace("UpdateUserOpeningList: Seek error.\n");
                goto end;
            }
            if (sizeof(OPENING_LEARNING_ENTRY) != 
                write(fd, &ole, sizeof(OPENING_LEARNING_ENTRY)))
            {
                Trace("UpdateUserOpeningList: Write error.\n");
            }
            goto end;
        }
    }
    
    //
    // We never found it, so add one at the end.
    // 
    memset(&ole, 0, sizeof(ole));
    ole.u64Sig = g_Options.u64OpeningSig;
    switch(iResult)
    {
        case -1:
            ole.uBlackWins++;
            break;
        case 0:
            ole.uDraws++;
            break;
        case 1:
            ole.uWhiteWins++;
            break;
#ifdef DEBUG
        default:
            ASSERT(FALSE);
            break;
#endif
    }
    
    if (-1 == lseek(fd, 0, SEEK_END))
    {
        Trace("UpdateUserOpeningList: Seek error.\n");
        goto end;
    }
    
    if (sizeof(OPENING_LEARNING_ENTRY) != 
        write(fd, &ole, sizeof(OPENING_LEARNING_ENTRY)))
    {
        Trace("UpdateUserOpeningList: Append error.\n");
    }
    
 end:
    if (fd >= 0)
    {
        close(fd);
    }
}
#endif


#if 0
//+----------------------------------------------------------------------------
//
// Function:  BookLearn
//
// Synopsis:  Handles book learning at the end of a game based on the
//            result of the game and the calibre of the opponent.
//
// Arguments: int iResult
//            
// Returns:   void
//
//+----------------------------------------------------------------------------
void 
BookLearn(INT iResult)
{
    ULONG iIndex, x;
    unsigned int iChanged = 0;
    int iTotalEntries = _BookCount();
    int iLow, iHigh, iCurrent = 0;
    BOOK_ENTRY entry;
    FLAG fFound = FALSE;

#ifdef DEBUG
    if (g_Options.ePlayMode == I_PLAY_BLACK)
    {
        Trace("BookLearn: I was playing black\n");
    }
    else if (g_Options.ePlayMode == I_PLAY_WHITE)
    {
        Trace("BookLearn: I was playing white\n");   
    }

    if (iResult == -1)
    {
        Trace("BookLearn: Black won.\n");
    }
    else if (iResult == 1)
    {
        Trace("BookLearn: White won.\n");
    }
    else
    {
        Trace("BookLearn: The game was a draw.\n");
    }
#endif
            
    //
    // Should we learn anything?
    //

    //
    // Not if the game was bullet
    //
    if ((g_Options.eGameType == GAME_BULLET) ||
        (g_Options.eGameType == GAME_UNKNOWN))
    {
        Trace("BookLearn: Game was too fast or unknown speed, don't learn.\n");
        goto end;
    }

    //
    // Not if the game wasn't rated.
    //
    if ((g_Options.fGameIsRated == FALSE))
    {
        Trace("BookLearn: Game was unrated, don't learn.\n");
        goto end;
    }

    //
    // Not if someone lost on time or because of forfeit.
    //
    if ((NULL != strstr(g_Options.szDescription, "forfeit")) ||
        (NULL != strstr(g_Options.szDescription, "time")) ||
        (NULL != strstr(g_Options.szDescription, "abort")))
    {
        Trace("BookLearn: Loss by forfeit/abort, don't learn.\n");
        goto end;
    }
    
    //
    // Not if we won but the opponent was weak.
    //
    if (((g_Options.ePlayMode == I_PLAY_WHITE) && (iResult == 1)) ||
        ((g_Options.ePlayMode == I_PLAY_BLACK) && (iResult == -1)))
    {
#ifdef DEBUG
        Trace("BookLearn: I won, my opponent's rating was %u, "
              "my rating is %u.\n",
              g_Options.iOpponentsRating,
              g_Options.iMyRating);
#endif

        if (g_Options.iOpponentsRating < g_Options.iMyRating - 300)
        {
            Trace("BookLearn: Opponent sucks, not learning.\n");
            goto end;
        }
    }

    //
    // Not if we lost but the opponent was strong.
    //
    if (((g_Options.ePlayMode == I_PLAY_BLACK) && (iResult == 1)) ||
        ((g_Options.ePlayMode == I_PLAY_WHITE) && (iResult == -1)))
    {
        Trace("BookLearn: I lost, my opponent's rating was %u, "
              "my rating is %u.\n",
              g_Options.iOpponentsRating,
              g_Options.iMyRating);

        if (g_Options.iOpponentsRating > g_Options.iMyRating + 300)
        {
            Trace("BookLearn: Opponent kicks ass, not learning.\n");
            goto end;
        }
        ToXboard("tellics say Nice game!\n");
    }

    //
    // Yes, we have decided to learn.
    //
    Trace("BookLearn: Revising opening book line...\n");
    
    //
    // 1. Save the learning on a user-friendly list...
    // 
    UpdateUserOpeningList(iResult);
    
    //
    // 2. Save the learning in the real opening book...
    // 
    if ((0 == iTotalEntries) || 
        (FALSE == _BookOpen()))
    {
        Trace("BookLearn: Could not open the book.\n");
        goto end;
    }

    //
    // Look at the moves in the move list.
    //
    for (iIndex = 0; 
         iIndex < g_iMoveNum - 1; 
         iIndex++)
    {
        //
        // Only learn on the first 30 moves.
        //
        if (iIndex > 60) break;

        Trace("BookLearn: Seeking %I64x with move %s (%x) in the book.\n", 
            g_MoveList[iIndex].u64Sig,
            MoveToICS(g_MoveList[iIndex + 1].mv),
            g_MoveList[iIndex + 1].mv.move);

        //
        // Binary search for sig in the book, this sets iLow and iHigh.
        //
        if (0 != _BookFindSig(g_MoveList[iIndex].u64Sig, &iLow, &iHigh))
        {
            Trace("BookLearn: Found a (P) : %I64x at %u.\n", 
                g_MoveList[iIndex].u64Sig, iCurrent);

            //
            // Current now points to the first book entry matching
            // the sig... look for the entry matching the move too.
            //
            for (x = iLow; x <= iHigh; x++)
            {
                _BookSeek(x);
                _BookRead(&entry);

                ASSERT(entry.u64Sig ==
                       g_MoveList[iIndex].u64Sig);

                //
                // If we match the move and signature, update the entry
                // based on this game's results.
                //
                if (entry.mvNext.move == g_MoveList[iIndex + 1].mv.move)
                {
                    Trace("BookLearn: Matched mv : %x at %u.\n",
                        g_MoveList[iIndex + 1].mv.move, x);
                    iChanged++;

                    //
                    // Did the side making the move win in the end?
                    //
                    if (((iResult == -1) && 
                         (IS_BLACK(g_MoveList[iIndex + 1].mv.data.pMoved))) ||
                        ((iResult == 1) &&
                         (IS_WHITE(g_MoveList[iIndex + 1].mv.data.pMoved))))
                    {
                        Trace("BookLearn: Side moving won, enforce this "
                              "book entry.\n");
                        entry.iWins++;
                    }
                    else
                    {
                        //
                        // No so it drew or lost.
                        //
                        if (0 == iResult)
                        {
                            Trace("BookLearn: Game was a draw, increase "
                                  "drawishness of this book entry.\n");
                            entry.iDraws++;
                        }
                        else
                        {
                            Trace("BookLearn: Side moving lost, discourage "
                                  "this book entry.\n");
                            entry.iLosses++;
                        }
                    }

                    //
                    // Write it back into the book.
                    //
                    _BookSeek(x);
                    _BookWrite(&entry);

                    //
                    // Done looking for this move.
                    //
                    break;

                } // entry.move matches

            } // loop to look at all moves on entries w/ matching sig

        } // was sig found in book?

    } // main game move loop

 end:
    Trace("BookLearn: Changed %u book positions.\n", iChanged);
    _BookClose();
}


//+----------------------------------------------------------------------------
//
// Function:  PreferPosition
//
// Synopsis:  Given a position, love, hate or be neutral towards it.
// 
//            If the position is to be loved, all other book entries
//            that lead to it will be flagged "always play".
// 
//            If the position is to be hated, all other book entries
//            that lead to it will be flagged "never play".
// 
//            A neutral setting will cause all other book entries that
//            lead to have their flags cleared.
//
// Arguments: POSITION pos  - the pos, needed to make a FEN
//            FILE *fd      - file to record permanently in
//            INT64 i64Targ - the signature to love/hate/neuter
//            int i         : -1 means hate
//                          : 0 means neuter
//                          : 1 means love
//            
// Returns:   void
//
//+----------------------------------------------------------------------------
void 
PreferPosition(POSITION *pos, FILE *fd, INT64 i64Targ, int i)
{
    int iTotalEntries = _BookCount();
    int x;
    int iChanged = 0;
    static BOOK_ENTRY entry;

    ASSERT(pos->u64Sig == i64Targ);
    
    if (g_pMemBook != NULL) 
    {
        ASSERT(FALSE);
        return;
    }

    iTotalEntries = _BookCount();
    
    for (x = 0;
         x < iTotalEntries - 1;
         x++)
    {
        if ((x % 10000) == 0)
        {
            printf("%u / %u\r", x, iTotalEntries);
        }
        _BookSeek(x);
        _BookRead(&entry);

        if (entry.i64NextSig == i64Targ)
        {
            iChanged++;

            switch(i)
            {
            case -1:
                _BookSeek(x);
                entry.bvFlags = FLAG_DISABLED;
                _BookWrite(&entry);
                if (NULL != fd)
                {
                    fprintf(fd, "%I64x (%x) FLAGS %x // %s\n",
                            entry.u64Sig,
                            entry.mvNext.move,
                            entry.bvFlags,
                            BoardToFEN(pos));
                }
                break;
            case 0:
                _BookSeek(x);
                entry.bvFlags = 0;
                _BookWrite(&entry);
                if (NULL != fd)
                {
                    fprintf(fd, "%I64x (%x) FLAGS %x // %s\n",
                            entry.u64Sig,
                            entry.mvNext.move,
                            entry.bvFlags,
                            BoardToFEN(pos));
                }
                break;
            case 1:
                _BookSeek(x);
                entry.bvFlags = FLAG_ALWAYSPLAY;
                _BookWrite(&entry);
                if (NULL != fd)
                {
                    fprintf(fd, "%I64x (%x) FLAGS %x // %s\n",
                            entry.u64Sig,
                            entry.mvNext.move,
                            entry.bvFlags,
                            BoardToFEN(pos));
                }
                break;
            }
        }
    }
    
    printf("\n\n");
    Trace("PreferPosition: Changed %u lines\n", iChanged);
}

//+----------------------------------------------------------------------------
//
// Function:  ImportEditing
//
// Synopsis:  Affect the opening book file based on an old editing record
//            (.EDT file).  Apply editing / learning record to the new book
//            positions.
//
// Arguments: char *szFile
//            
// Returns:   FLAG
//
//+----------------------------------------------------------------------------
FLAG 
ImportEditing(char *szFile)
{
    FILE *p = fopen(szFile, "r");
    FLAG fMyRetVal = FALSE;
    static char szLine[512];
    char szOpcode[512];
    int iArg;
    int iTotalEntries;
    INT64 u64Sig;
    MOVE mv;
    int iLow, iHigh, iCurrent = 0;
    int iIndex;
    BOOK_ENTRY entry;
    BOOK_ENTRY klobber;
    
    //
    // We cannot probe the opening book if membook is non-NULL (which
    // indicates the opening book is currently in-memory and still being
    // built.
    //
    if (g_pMemBook != NULL) 
    {
        goto end;
    }
    
    //
    // Open the book.
    //
    iTotalEntries = _BookCount();
    if (0 == iTotalEntries)
    {
        Trace("BookMove: Opening book \"%s\" is empty.\n", 
              g_Options.szBookName);
        goto end;
    }
    
    if (FALSE == _BookOpen())
    {
        Trace("BookMove: Could not open the book.\n");
        goto end;
    }
    
    if (NULL != p)
    {
        while(fgets(szLine, 511, p))
        {
            if (_snscanf(szLine, 511, "%I64x (%x) %s", 
                         &u64Sig,
                         &mv.move,
                         &szOpcode) != 3)
            {
                continue;
            }

#ifdef DEBUG
            Trace("Searching for %I64x move %x\n",
                  u64Sig, mv.move);
#endif
            
            if (_BookFindSig(u64Sig, &iLow, &iHigh))
            {
                //
                // Walk forward looking for the right move.
                //
                memset(&entry, 0, sizeof(entry));
                for (iIndex = iLow; 
                     iIndex <= iHigh;
                     iIndex++)
                {
                    _BookSeek(iIndex);
                    _BookRead(&entry);
                    ASSERT(entry.u64Sig == u64Sig);
                    
                    if (entry.mvNext.move == mv.move) break;
                }

                //
                // Did we find the move?
                // 
                if (entry.mvNext.move != mv.move)
                {
                    Trace("Move %x not present in book at position %I64x\n",
                          mv.move, u64Sig);
                    continue;
                }

                //
                // Yes.  What should we do with it?
                // 
                if (!STRNCMP(szOpcode, "FLAGS", 5))
                {
                    if (_snscanf(szLine, 511, "%I64x (%x) %s %x", 
                                 &u64Sig,
                                 &mv.move,
                                 &szOpcode, 
                                 &iArg) == 4)
                    {
                        if (iArg >= 0) entry.bvFlags = iArg;
                    }
                    else
                    {
                        Trace("Error\n");
                    }
                }
                else if (!STRNCMP(szOpcode, "+", 1))
                {
                    if (_snscanf(szLine, 511, "%I64x (%x) %s %u", 
                                 &u64Sig,
                                 &mv.move,
                                 &szOpcode, 
                                 &iArg) == 4)
                    {
                        if (iArg >= 0) entry.iWins = iArg;
                    }
                    else
                    {
                        Trace("Error\n");
                    }
                }
                else if (!STRNCMP(szOpcode, "=", 1))
                {
                    if (_snscanf(szLine, 511, "%I64x (%x) %s %u", 
                                 &u64Sig,
                                 &mv.move,
                                 &szOpcode, 
                                 &iArg) == 4)
                    {
                        if (iArg >= 0) entry.iDraws = iArg;
                    }
                    else
                    {
                        Trace("Error\n");
                    }
                }
                else if (!STRNCMP(szOpcode, "-", 1))
                {
                    if (_snscanf(szLine, 511, "%I64x (%x) %s %u", 
                                 &u64Sig,
                                 &mv.move,
                                 &szOpcode, 
                                 &iArg) == 4)
                    {
                        if (iArg >= 0) entry.iLosses = iArg;
                    }
                    else
                    {
                        Trace("Error\n");
                    }
                }
                else 
                {
                    Trace("Error\n");
                }
                
                //
                // Write it back to the book
                //
                _BookSeek(iIndex);
                _BookRead(&klobber);

                if ((entry.u64Sig == 
                     klobber.u64Sig) &&
                    (entry.mvNext.move ==
                     klobber.mvNext.move))
                {
                    _BookWrite(&entry);
                }
            }
        }
    }

    fMyRetVal = TRUE;
 end:
    _BookClose();
    return(fMyRetVal);
}
#endif


static FLAG 
_FindTerminalPositions(CHAR *szFilename, 
                       ULONG uPlyBeforeMate)
/**

Routine description:

Parameters:

    CHAR *szFilename,
    ULONG uMovesBeforeMate

Return value:

    static FLAG

**/
{
    CHAR *p;
    FILE *pf = NULL;
    struct stat s;
    POSITION *pos;
    SEARCHER_THREAD_CONTEXT *ctx;
    FLAG fInCheck;
    MOVE mv;
    ULONG u;
    FLAG fRet = FALSE;

    ctx = SystemAllocateMemory(sizeof(SEARCHER_THREAD_CONTEXT));
    if (NULL == ctx) 
    {
        Trace("Out of memory.\n");
        goto end;
    }
    
    if (FALSE == SystemDoesFileExist(szFilename))
    {
        Trace("FindTerminalPositions: file \"%s\" doesn't exist.\n",
              szFilename);
        goto end;
    }
    
    if (0 != stat(szFilename, &s))
    {
        Trace("FindTerminalPositions: Can't read file \"%s\".\n",
              szFilename);
        goto end;
    }
    
    pf = fopen(szFilename, "rb");
    if (NULL == pf) 
    {    
        Trace("FindTerminalPositions: error reading file %s.\n", 
              szFilename);
        goto end;
    }

    mv.uMove = 0;
    while((p = ReadNextGameFromPgnFile(pf)) != NULL)
    {
        if (TRUE == LoadPgn(p))
        {
            pos = GetRootPosition();

            fInCheck = InCheck(pos, pos->uToMove);
            if (TRUE == fInCheck)
            {
                InitializeSearcherContext(pos, ctx);
                GenerateMoves(ctx, mv, GENERATE_ESCAPES);
                if (0 == MOVE_COUNT(ctx, 0))
                {
                    u = uPlyBeforeMate;
                    while(u)
                    {
                        if (FALSE == OfficiallyTakebackMove())
                        {
                            break;
                        }
                        u--;
                    }
                    pos = GetRootPosition();
                    InitializeSearcherContext(pos, ctx);
                    DumpPosition(pos);
                    (void)Eval(ctx, -INFINITY, +INFINITY);
                    Trace("King safeties: W=%d, B=%d\n",
                          ctx->sPlyInfo[ctx->uPly].iKingScore[WHITE],
                          ctx->sPlyInfo[ctx->uPly].iKingScore[BLACK]);
                }
            }
        }
        SystemFreeMemory(p);
    }
    fclose(pf);
    fRet = TRUE;
    
 end:
    if (NULL != ctx) 
    {
        SystemFreeMemory(ctx);
    }
    return(fRet);
}



COMMAND(BookCommand)
{
    CHAR *szOpcode;
    CHAR *pDot;

    ASSERT(!STRNCMPI(argv[0], "BOOK", 4));
    if (argc < 2)
    {
        Trace("Available opening book commands:\n\n"
              "    book name          : set opening book file\n"
              "    book import        : import PGN or EDT data to book file\n"
              "    book dump learning : show book learning\n"
              "    book dump moves    : show book moves in current position\n"
              "    book edit          : edit book moves in current position\n"
              "    book tourn         : view / toggle tournament mode\n"
              "    book openings      : read opening line names PGN file\n"
              "    book hackhack      : read PGN files looking for mate\n"
              "\n");
        return;
    }
    
    szOpcode = argv[1];
    if (!STRNCMPI(szOpcode, "NAME", 4))
    {
        //
        // book name <filename> : set opening book to use
        //
        if (argc == 2)
        {
            Trace("Usage: book name <required-filename>\n");
            return;
        }        
        strncpy(g_Options.szBookName, argv[2], SMALL_STRING_LEN_CHAR);
        Trace("Opening book name set to \"%s\"\n", argv[2]);
    }
    else if (!STRNCMPI(argv[1], "DUMP", 4))
    {
        //
        // book dump : show book dump help
        // 
        if (argc == 2)
        {
            Trace("Available book dump commands:\n"
                  "    book dump learning : show book learning\n"
                  "    book dump moves    : show book moves in current"
                  "position\n"
                  "\n");
            return;
        }
        ASSERT(argc > 2);
        
        if (!STRNCMPI(argv[2], "LEARNING", 5))
        {
            //
            // book dump learning : show book learning on stdout
            // 
            _DumpUserOpeningList();
        }
        else
        {
            //
            // book dump moves : show book moves in this position
            // 
            BookMove(pos, BOOKMOVE_DUMP);
        }
    }
    else if (!STRNCMPI(argv[1], "TOURNAMENTMODE", 5))
    {
        if (argc == 2)
        {
            //
            // book tourn : show current tournament mode setting
            //
            if (g_fTournamentMode == TRUE)
            {
                Trace("Tournament mode is currently TRUE.\n");
            }
            else
            {
                Trace("Tournament mode is currently FALSE.\n");
            }
        }
        else
        {
            //
            // book tourn <t|f> : set tournament mode on or off
            // 
            if (tolower(*argv[2]) == 't')
            {
                g_fTournamentMode = TRUE;
                Trace("Tournament mode set to TRUE.\n");
            }
            else if (tolower(*argv[2]) == 'f')
            {
                g_fTournamentMode = FALSE;
                Trace("Tournament mode set to FALSE.\n");
            }
            else
            {
                Trace("Usage: book tournamentmode [<true | false>]\n");
            }   
        }
    }
    
#if 0
    else if (!STRNCMPI(argv[1], "EDIT", 4))
    {
        //
        // book edit : edit book moves in this position
        // 
        BookEdit(pos);
        return(TRUE);
    }
#endif

    else if (!STRNCMPI(argv[1], "HACKHACK", 8))
    {
        if (argc == 2)
        {
            Trace("Usage: book hackhack <required-filename>\n");
            return;
        }
        _FindTerminalPositions(argv[2], 4);
    }
    
    else if (!STRNCMPI(argv[1], "OPENINGS", 8))
    {
        //
        // book openings : read book opening names file
        // 
        if (argc == 2)
        {
            Trace("Usage: book openings <required-filename>\n");
            return;
        }
        
        if (FALSE == SystemDoesFileExist(argv[2]))
        {
            Trace("Error (file doesn't exist): %s\n", argv[2]);
            return;
        }
        
//        Trace("%u opening lines named.\n", LearnOpeningNames(argv[2]));
    }
    
    else if (!STRNCMPI(argv[1], "IMPORT", 6))
    {
        //
        // book import : create new book file from PGN -or-
        //               apply editing / learning to a book file
        // 
        if (argc == 2)
        {
            Trace("Usage: book import <required-filename>\n");
            return;
        }

        if (FALSE == SystemDoesFileExist(argv[2]))
        {
            Trace("Error (file doesn't exist): %s\n", argv[2]);
            return;
        }

        pDot = strrchr(argv[2], '.');
        if (NULL == pDot)
        {
            Trace("Error (unknown filetype): %s\n", argv[2]);
            return;
        }

        if (!STRNCMPI(pDot, ".pgn", 4))
        {
            if (TRUE == SystemDoesFileExist(g_Options.szBookName))
            {
                Trace("The opening book %s already exists, "
                      "importing new lines\n", g_Options.szBookName);
                _LearnPgnOpenings(argv[2], 0);
            }
            else
            {
                Trace("The opening book %s does not exists, "
                      "creating new book\n", g_Options.szBookName);
                _LearnPgnOpenings(argv[2], 1);
            }
        }
#if 0
        else if (!STRNCMPI(pDot, ".edt", 4))
        {
            if (FALSE == ImportEditing(argv[2]))
            {
                Trace("An error occurred while importing .edt file.\n");
            }
        }
#endif
        else
        {
            Trace("Error (unknown filetype): %s\n", argv[2]);
        }
    }
}
