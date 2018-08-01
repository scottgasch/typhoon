/**

Copyright (c) Scott Gasch

Module Name:

    testhash.c

Abstract:

    Test of the main hash table code.

Author:

    Scott Gasch (scott.gasch@gmail.com) 19 Oct 2004

Revision History:

**/

#ifdef TEST
#include "chess.h"

extern UCHAR g_cDirty;

#define CONVERT_HASH_DEPTH_TO_SEARCH_DEPTH(x) \
    ASSERT(((x) & 0xffffff00) == 0); \
    (x) <<= 4

FLAG 
SanityCheckHashEntry(HASH_ENTRY *p)
/**

Routine description:

    Make sure a hash table entry looks valid.

Parameters:

    HASH_ENTRY *p

Return value:

    FLAG

**/
{
    ULONG uDepth;

    if (p->u64Sig == 0)
    {
        if ((p->mv.uMove != 0) ||
            (p->uDepth != 0) ||
            (p->bvFlags != 0) ||
            (p->iValue != 0))
        {
            ASSERT(FALSE);
            return(FALSE);
        }
    }
    else
    {
        uDepth = p->uDepth;
        CONVERT_HASH_DEPTH_TO_SEARCH_DEPTH(uDepth);
        if (!IS_VALID_DEPTH(uDepth))
        {
            ASSERT(FALSE);
            return(FALSE);
        }

        switch(p->bvFlags & HASH_FLAG_VALID_BOUNDS)
        {
            case HASH_FLAG_LOWER:
            case HASH_FLAG_UPPER:
                break;

            case HASH_FLAG_EXACT:
                if (p->mv.uMove == 0)
                {
                    ASSERT(FALSE);
                    return(FALSE);
                }
                break;

            default:
                ASSERT(FALSE);
                return(FALSE);
        }
    }
    return(TRUE);
}


void 
AnalyzeFullHashTable(void)
/**

Routine description:

    Walk over the hash table and validate populated entries.

Parameters:

    void

Return value:

    void

**/
{
    ULONG uEntry;
    ULONG uLine;
    ULONG uNumEntries = g_uHashTableSizeBytes / sizeof(HASH_ENTRY);
    HASH_ENTRY *p;
    ULONG uEmpty = 0;
    ULONG uStale = 0;
    ULONG uUnique = 0;
    ULONG uCount = 0;
    ULONG u;
    double d;
    FLAG fUnique;

    uLine = 0;
    for (uEntry = 0;
         uEntry < uNumEntries;
         uEntry++)
    {
        if ((uEntry % NUM_HASH_ENTRIES_PER_LINE) == 0)
        {
            uLine++;
        }

        p = &(g_pHashTable[uEntry]);

        //
        // See if it's stale
        //
        if ((p->bvFlags & 0xF0) != g_cDirty)
        {
            uStale++;
        }

        //
        // Sanity checks
        //
        if (FALSE == SanityCheckHashEntry(p))
        {
            UtilPanic(TESTCASE_FAILURE,
                      NULL, "TestHash", NULL, NULL,
                      __FILE__, __LINE__);
        }

        //
        // See if it's empty
        //
        if (p->u64Sig == 0)
        {
            uEmpty++;
        }
        else
        {
            uCount++;

            //
            // See if it's unique
            //
            if (uEntry % NUM_HASH_ENTRIES_PER_LINE)
            {
                fUnique = TRUE;
                u = uEntry - 1;
                do
                {
                    if (p->u64Sig == g_pHashTable[u].u64Sig)
                    {
                        fUnique = FALSE;
                        break;
                    }
                    u--;
                }
                while(u % NUM_HASH_ENTRIES_PER_LINE);
            }
            else
            {
                fUnique = TRUE;
            }
            uUnique += fUnique;
        }
    }

    //
    // Show results
    //
    Trace("There are %u entries in the hash table.\n", uNumEntries);

    d = (double)uStale / (double)uNumEntries * 100.0;
    Trace("The hash table is %6.3f percent stale.\n", d);

    d = (double)uEmpty / (double)uNumEntries * 100.0;
    Trace("The hash table is %6.3f percent empty.\n", d);

    d = (double)uUnique / (double)uCount * 100.0;
    Trace("The hash table is %6.3f percent unique.\n", d);
}
#endif
