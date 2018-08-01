/**

Copyright (c) Scott Gasch

Module Name:

    testeval.c

Abstract:

    An eval term storage and dumping mechanism.  Also code to sanity
    check the evaluator routines.

Author:

    Scott Gasch (scott.gasch@gmail.com) 18 Jun 2004

Revision History:

    $Id: testeval.c 345 2007-12-02 22:56:42Z scott $

**/
#ifdef EVAL_DUMP

#include "chess.h"

typedef struct _EVALTERM
{
    CHAR szMessage[SMALL_STRING_LEN_CHAR];
    COOR cLoc;
    SCORE iVal;
    DLIST_ENTRY links;
}
EVALTERM;

static EVALTERM g_EvalData[2][8][32];
static ULONG g_EvalCounts[2][8];

void 
EvalTraceClear(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    memset(g_EvalData, 0, sizeof(g_EvalData));
    memset(g_EvalCounts, 0, sizeof(g_EvalCounts));
}

void 
EvalTrace(IN ULONG uColor, 
          IN PIECE p,
          IN COOR c,
          IN SCORE iVal,
          IN CHAR *szMessage)
/**

Routine description:

    Add a term to the current positions evaluation trace database.

Parameters:

    IN ULONG uColor,
    IN PIECE p,
    IN COOR c,
    IN SCORE iVal,
    IN CHAR *szMessage

Return value:

    void

**/
{
    if ((iVal == 0) && (c != ILLEGAL_COOR)) return;

    ASSERT(p <= 7);
    ASSERT(iVal < +2000);
    ASSERT(iVal > -2000);

    //
    // Note: The square control code in eval does this to avoid an if.
    // Just ignore anything that is passed to us that is not BLACK or
    // WHITE.
    //
    if (!IS_VALID_COLOR(uColor))
    {
        return;
    }
    g_EvalData[uColor][p][g_EvalCounts[uColor][p]].cLoc = c;
    g_EvalData[uColor][p][g_EvalCounts[uColor][p]].iVal = iVal;
    strncpy(g_EvalData[uColor][p][g_EvalCounts[uColor][p]].szMessage,
            szMessage, 
            SMALL_STRING_LEN_CHAR);
    g_EvalCounts[uColor][p]++;
}


SCORE 
EvalSigmaForPiece(IN POSITION *pos, IN COOR c)
/**

Routine description:

Parameters:

    IN POSITION *pos,
    IN COOR c

Return value:

    SCORE

**/
{
    SCORE iSum[2] = {0, 0};
    ULONG uColor;
    ULONG u, v;
    
    FOREACH_COLOR(uColor)
    {
        for (u = 0; u < 8; u++)
        {
            for (v = 0; v < g_EvalCounts[uColor][u]; v++)
            {
                if (g_EvalData[uColor][u][v].cLoc == c)
                {
                    Trace("%s %-30s%-3s%4d\n",
                          (uColor == WHITE) ? "white" : "black",
                          g_EvalData[uColor][u][v].szMessage,
                          CoorToString(g_EvalData[uColor][u][v].cLoc),
                          g_EvalData[uColor][u][v].iVal);
                    iSum[uColor] += g_EvalData[uColor][u][v].iVal;
                }
            }
        }
    }
    Trace("Totals are BLACK: %d, WHITE: %d.\n", iSum[BLACK], iSum[WHITE]);
    
    if (IS_ON_BOARD(c))
    {
        if (pos->rgSquare[c].pPiece)
        {
            return(iSum[GET_COLOR(pos->rgSquare[c].pPiece)]);
        }
    }
    return(0);
}


void 
EvalTraceReport(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    PIECE p;
    ULONG uWhitePtr, uBlackPtr;
    CHAR szLine[SMALL_STRING_LEN_CHAR];
    SCORE iPieceTotal[2];
    
    Trace(
"--- WHITE -------------------LOC--VAL---- BLACK --------------------LOC--VAL-\n"
        );

    for (p = 0; 
         p < 8;
         p++)
    {
        if ((g_EvalCounts[WHITE][p] + g_EvalCounts[BLACK][p]) == 0)
        {
            continue;
        }

        Trace("\n");
        iPieceTotal[WHITE] = iPieceTotal[BLACK] = 0;

        Trace("%s:\n---------------\n", g_PieceData[p].szName);

        for (uWhitePtr = 0, uBlackPtr = 0; 
             ((uWhitePtr < g_EvalCounts[WHITE][p]) ||
              (uBlackPtr < g_EvalCounts[BLACK][p]));
             uWhitePtr++, uBlackPtr++)
        {
            memset(szLine, 0, sizeof(szLine));
            
            if (uWhitePtr < g_EvalCounts[WHITE][p])
            {
                sprintf(szLine, "%-30s%-3s%4d",
                        g_EvalData[WHITE][p][uWhitePtr].szMessage,
                        CoorToString(g_EvalData[WHITE][p][uWhitePtr].cLoc),
                        g_EvalData[WHITE][p][uWhitePtr].iVal);
                iPieceTotal[WHITE] += 
                    g_EvalData[WHITE][p][uWhitePtr].iVal;
            }
            else
            {
                sprintf(szLine, "                                     ");
            }
            
            if (uBlackPtr < g_EvalCounts[BLACK][p])
            {
                sprintf(szLine, "%s  %-30s%-3s%4d",
                        szLine,
                        g_EvalData[BLACK][p][uBlackPtr].szMessage,
                        CoorToString(g_EvalData[BLACK][p][uBlackPtr].cLoc),
                        g_EvalData[BLACK][p][uBlackPtr].iVal);
                iPieceTotal[BLACK] += 
                    g_EvalData[BLACK][p][uBlackPtr].iVal;
            }
            strcat(szLine, "\n");
            Trace(szLine);
        }

        if ((iPieceTotal[WHITE] != 0) || (iPieceTotal[BLACK] != 0))
        {
            Trace("       TOTAL WHITE . . . . . . . %4d"
                  "         TOTAL BLACK . . . . . . . %4d\n", 
                  iPieceTotal[WHITE], iPieceTotal[BLACK]);
        }
    }
}         

    
void 
TestEvalWithSymmetry(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    SEARCHER_THREAD_CONTEXT *ctx = malloc(sizeof(SEARCHER_THREAD_CONTEXT));
    POSITION pos;
    SCORE i;
    
    GenerateRandomLegalSymetricPosition(&pos);
    InitializeSearcherContext(&pos, ctx);
        
    i = Eval(ctx, -INFINITY, INFINITY);
    if (i != 0)
    {
        DumpPosition(&ctx->sPosition);
        Trace(">>>> ");
    }
    Trace("Eval: %s\n", ScoreToString(i));
    free(ctx);
}

typedef struct _EVAL_TEST
{
    CHAR *szFen;
    CHAR *szFeature[5];
} EVAL_TEST;

void 
TestEvalWithKnownPositions(void)
/**

Routine description:

Parameters:

    void

Return value:

    void

**/
{
    static const EVAL_TEST x[] =
    {
        {
            // htryc pg 388
            "r1br2k1/pp2pp1p/6p1/3P4/4P3/8/P2KBPPP/1R5R w - - 0 1",
            { "w>b", NULL, NULL, NULL, NULL }
        },
        {
            // trryc pg 389
            "r1r3k1/p2qpp1p/np1p2p1/3P4/PP1QP3/4BP2/6PP/2RR2K1 w - - 0 1",
            { "w>b", "E3>A6", NULL, NULL, NULL }
        },
        {
            // htryc pg 180
            "2r1b1k1/1qr2p2/p1p1p1p1/2R4p/1P5P/P1R1PBP1/5P2/2Q3K1 w - - 0 1",
            { "w>b", "F3>E8", NULL, NULL, NULL }
        },
        {
            // htryc pg 209
            "8/4b3/pp2kppp/2p1p3/P1K1P1PP/1PP1NP2/8/8 w - - 0 1",
            { "w>b", "E3>E7", NULL, NULL, NULL }
        },
        {
            // htryc pg 227
            "1q1rrbk1/1p3p1p/p2p2p1/3Pp3/1P2P3/P3BP2/3Q2PP/R2R2K1 w - - 0 1",
            { "w>b", "E3>F8", NULL, NULL, NULL }
        },
        {
            // htryc 117
            "4r2k/4qp1p/p4p2/2b2P2/2p5/3brRP1/PP1Q2BP/3R2NK w - - 0 1",
            { "b>w", NULL, NULL, NULL, NULL }
        },
        {
            // htryc 161b
            "r2r2k1/pbq3bp/1p4p1/4n3/3BP1Q1/2P3NP/P7/1B1R1RK1 b - - 0 1",
            { "b>w", "B7>B1", "e5>g3", NULL, NULL }
        },
        {
            // htryc 161
            "r2r2k1/pb1q2bp/1p2p1p1/n1p2p2/3PPPP1/2PBB1NP/P7/2R1QRK1 b - - 0 1",
            { "b>w", NULL, NULL, NULL, NULL }
        },
        {
            // htryc pg 136
            "6rr/1pqbkn1p/1Pp1p1pP/2Pp1pP1/3P1P2/Q2BPN2/5K1R/7R b - - 0 1",
            { "w>b", "F3>D7", "d3>d7", NULL, NULL }
        },
        {   
            // htryc pg 133
            "r4rk1/pp1bqn1p/2p1pnp1/3pNp2/2PP1P2/3BP3/PPQN2PP/1R3RK1 b - - 0 1",
            { "w>b", "E5>D7", "d2>d7", "d3>d7", NULL }
        },
        {
            // ppic pg 7
            "6k1/5ppp/8/P7/2p5/8/5PPP/6K1 w - - 0 1",
            { "w>b", NULL, NULL, NULL, NULL }
        },
        {
            // htryc pg 120
            "r4rk1/pp3ppp/5n2/2pPp3/2P1B2n/P3PP2/5P1P/R1B2RK1 b - - 0 1",
            { "b>w", "H4>E4", "h4>c1", "f6>c1", NULL }
        },
        {
            // htryc pg 111
            "8/1p6/npn2k2/3p3p/P2P1B2/2K5/7P/1B6 b - - 0 1",
            { "w>b", NULL, NULL, NULL, NULL }
        },
        {
            // htryc pg 98
            "4r1k1/1p1rq1pp/2p1p3/p1P1Ppb1/P2P4/1PNR1QP1/6KP/3R4 b - - 0 1",
            { "g5>c3", NULL, NULL, NULL, NULL }
        },
        {
            "4rqk1/1pbr2pp/2pNp3/p1P1Pp2/P2P4/1P1R1QP1/6KP/3R4 b - - 0 1",
            { "w>b", "D6>C7", NULL, NULL, NULL }
        },
        {
            // htryc pg 96
            "r2q1rk1/pb3ppp/1p6/3p4/3N4/1Q6/PP2RPPP/R5K1 b - - 0 1",
            { "d4>b7", NULL, NULL, NULL, NULL }
        },
        {
            // htryc pg 96
            "r2q1rk1/pb2nppp/1p2p3/8/3P4/1QN2N2/PP2RPPP/R5K1 b - - 0 1",
            { "b7>c3", "B7>F3", NULL, NULL, NULL }
        },
        {
            // htryc pg 95
            "r3r1k1/pp3ppp/3p1b2/3NpP2/2q1P3/8/PPP3PP/R3QRK1 w - - 0 1",
            { "d5>f6", "w>b", NULL, NULL, NULL }
        },
        {
            // htryc pg 95
            "2rbr1k1/p4ppp/3p4/1pqNpP2/4P3/1PP2R2/P5PP/R3Q2K b - - 0 1",
            { "w>b", NULL, NULL, NULL, NULL }
        },
        {
            //
            "2br2k1/5pp1/1p3q1p/2pBpP2/2P1P3/P6P/2Q3P1/5RK1 w - - 0 1",
            { "d5>c8", NULL, NULL, NULL, NULL }
        },
        {
            // htryc pg 59a
            "k5B1/n7/7P/2p5/1p4P1/p7/7K/8 w - - 0 1",
            { "g8>a7", NULL, NULL, NULL, NULL }
        },
        {
            // htryc pg 59b
            "2br2k1/5pp1/1p3q1p/2p1pP2/2P1P3/P2B3P/2Q3P1/5RK1 w - - 0 1",
            { "b>w", "c8>d3", NULL, NULL, NULL }
        },
    };

    ULONG u, v;
    SEARCHER_THREAD_CONTEXT ctx;
    CHAR *p;
    COOR c1, c2;
    SCORE i, w, b, i1st, i2nd;
    CHAR cRelationship;
    
    Trace("Testing static eval code...\n");
    for (u = 0;
         u < ARRAY_LENGTH(x);
         u++)
    {
        ctx.uPositional = 0;
        if (FALSE == FenToPosition(&(ctx.sPosition), x[u].szFen))
        {
            UtilPanic(INCONSISTENT_STATE,
                      NULL, NULL, NULL, NULL,
                      __FILE__, __LINE__);
        }
        
        i = Eval(&ctx, -INFINITY, +INFINITY);
        Trace("Position %u: SCORE %d for side to move.\n", u, i);
        if (ctx.sPosition.uToMove == WHITE)
        {
            w = i;
            b = -i;
        }
        else
        {
            b = i;
            w = -i;
        }
        
        for (v = 0;
             v < ARRAY_LENGTH(x[u].szFeature);
             v++)
        {
            p = x[u].szFeature[v];
            if (p != NULL)
            {
                Trace("Testing feature \"%s\"\n", p);
                
                cRelationship = '=';
                i1st = INVALID_SCORE;
                i2nd = INVALID_SCORE;
                c1 = ILLEGAL_COOR;
                c2 = ILLEGAL_COOR;
                
                while(*p)
                {
                    if (LooksLikeCoor(p))
                    {
                        if (IS_ON_BOARD(c1))
                        {
                            c2 = FILE_RANK_TO_COOR((tolower(p[0]) - 'a'),
                                                   (tolower(p[1]) - '0'));
                            if (INVALID_SCORE == i1st)
                            {
                                i1st = EvalSigmaForPiece(&(ctx.sPosition), c2);
                            }
                            else
                            {
                                i2nd = EvalSigmaForPiece(&(ctx.sPosition), c2);
                            }
                            p += 2;
                        }
                        else
                        {
                            c1 = FILE_RANK_TO_COOR((tolower(p[0]) - 'a'),
                                                   (tolower(p[1]) - '0'));
                            if (INVALID_SCORE == i1st)
                            {
                                i1st = EvalSigmaForPiece(&(ctx.sPosition), c1);
                            }
                            else
                            {
                                i2nd = EvalSigmaForPiece(&(ctx.sPosition), c1);
                            }
                            p += 2;
                        }
                    }
                    else
                    {
                        switch(*p)
                        {
                            case 'w':
                                if (INVALID_SCORE == i1st)
                                {
                                    i1st = w;
                                }
                                else
                                {
                                    i2nd = w;
                                }
                                break;
                            case 'b':
                                if (INVALID_SCORE == i1st)
                                {
                                    i1st = b;
                                }
                                else
                                {
                                    i2nd = b;
                                }
                                break;
                            case '<':
                                cRelationship = '<';
                                break;
                            case '>':
                                cRelationship = '>';
                                break;
                            default:
                                break;
                        }
                        p++;
                    }
                }
            
                switch(cRelationship)
                {
                    case '<':
                        if (i1st >= i2nd)
                        {
                            Trace("%s NOT TRUE.\n", x[u].szFeature[v]);
                            EvalTraceReport();
                            DumpPosition(&(ctx.sPosition));
                            BREAKPOINT;
                        }
                        break;
                    case '>':
                        if (i2nd >= i1st)
                        {
                            Trace("%s NOT TRUE.\n", x[u].szFeature[v]);
                            EvalTraceReport();
                            DumpPosition(&(ctx.sPosition));
                            BREAKPOINT;
                        }
                        break;
                    default:
                        Trace("Unknown cRelationship.\n");
                        break;
                }
            }
            
        } // v
        
    } // u
}
#endif
