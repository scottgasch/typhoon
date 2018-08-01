/**
Copyright (c) Scott Gasch

Module Name:

    eval.c

Abstract:

    Position evaluation routines.

Author:

    Scott Gasch (scott.gasch@gmail.com) 14 Jun 2004

Revision History:

    $Id: eval.c 357 2008-07-03 16:18:11Z scott $

**/              

#include "chess.h"

typedef void (*PEVAL_HELPER)(POSITION *, COOR, PAWN_HASH_ENTRY *);
typedef FLAG (FASTCALL *PMOBILITY_HELPER)(POSITION *, COOR, ULONG *, ULONG *);

//
// To simplify code / maintenance I use the same loop for both colors
// in some places.  These globals coorespond to "ahead of the piece"
// or "behind the piece" for each color.
//
const int g_iAhead[2] = { +1, -1 };
const int g_iBehind[2] = { -1, +1 };

//
// General eval terms
// ---------------------------------------------------------------------------
//
static SCORE TRADE_PIECES[3][17] =
{//     0    1    2    3    4    5    6    7    8 | 9..15 -- down piece count
    {  -1,  90,  81,  73,  67,  62,  57,  52,  50, 50,50,50,50,50,50,50,-1},
    {  -1, 125, 117, 109, 100,  91,  84,  78,  71, 66,66,66,66,66,66,66,-1},
    {  -1, 150, 132, 124, 116, 109, 102,  94,  87, 77,77,77,77,77,77,77,-1},
};

static SCORE DONT_TRADE_PAWNS[3][9] = 
{//     0    1    2    3    4    5    6    7    8 -- up pawn count
    { -43, -15,   0,  +3,  +6, +10, +15, +21, +28 },                  // 0
    { -63, -25,   0,  +4,  +9, +14, +19, +25, +32 },                  // 1
    { -10,   0,   0,  +7, +15, +20, +24, +29, +36 },                  // 2
};

static ULONG REDUCED_MATERIAL_DOWN_SCALER[32] = 
{ 
//  none    na      na      m       na      R       2m      na
    0,      0,      0,      0,      0,      0,      0,      0,

//  Rm      3m/Q    2R      R2m     4m/Qm   2Rm     R3m/QR  Q2m
    0,      1,      1,      1,      1,      1,      2,      3,

//  2R2m    R4m/QRm Q3m     2R3m/   QR2m    Q4m     2R4m/   QR3m
//                          Q2R                     Q2Rm    
    4,      5,      6,      6,      7,      7,      7,      7,

//  na      Q2R2m   QR4m    na      Q2R3m   na      na      full
    8,      8,      8,      8,      8,      8,      8,      8,
};

static ULONG REDUCED_MATERIAL_UP_SCALER[32] = 
{ 
//  none    na      na      m       na      R       2m      na
    8,      8,      8,      8,      8,      8,      8,      8,

//  Rm      3m/Q    2R      R2m     4m/Qm   2Rm     R3m/QR  Q2m
    8,      7,      7,      7,      7,      6,      6,      5,

//  2R2m    R4m/QRm Q3m     2R3m/   QR2m    Q4m     2R4m/   QR3m
//                          Q2R                     Q2Rm    
    4,      3,      3,      3,      2,      2,      2,      1,

//  na      Q2R2m   QR4m    na      Q2R3m   na      na      full
    1,      1,      0,      0,      0,      0,      0,      0,
};

static ULONG PASSER_MATERIAL_UP_SCALER[32] = 
{ 
//  none    na      na      m       na      R       2m      na
    8,      8,      8,      8,      8,      8,      7,      7,

//  Rm      3m/Q    2R      R2m     4m/Qm   2Rm     R3m/QR  Q2m
    7,      6,      7,      5,      4,      5,      4,      3,

//  2R2m    R4m/QRm Q3m     2R3m/   QR2m    Q4m     2R4m/   QR3m
//                          Q2R                     Q2Rm    
    2,      2,      1,      1,      0,      0,      0,      0,

//  na      Q2R2m   QR4m    na      Q2R3m   na      na      full
    0,      0,      0,      0,      0,      0,      0,      0,
};

COOR QUEENING_RANK[2] = { 1, 8 };
COOR JUMPING_RANK[2] = { 7, 2 };

//
// Pawn eval terms
// ---------------------------------------------------------------------------
//
// "There is one case which can be treated as positional or material,
// namely the rook's pawn, which differs from other pawns in that it
// can only capture one way instead of two. Since this handicap cannot
// be corrected without the opponent's help, I teach my students to
// regard the rook's pawn as a different piece type, a crippled
// pawn. Database statistics indicate that it is on average worth
// about 15% less than a normal pawn. The difference is enough so that
// it is usually advantageous to make a capture with a rook's pawn,
// promoting it to a knights pawn, even if that produces doubled pawns
// and even if there is no longer a rook on the newly opened rook's
// file."
//                                                --Larry Kaufman, IM
//                                  "Evaluation of Material Imbalance"
//
static SCORE PAWN_CENTRALITY_BONUS[128] = 
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,0,0,0,0,0,0,0,
      -8,    0,    0,    0,    0,    0,    0,   -8,    0,0,0,0,0,0,0,0,
      -8,    0,    5,    5,    5,    5,    0,   -8,    0,0,0,0,0,0,0,0,
      -8,    0,    5,    9,    9,    5,    0,   -8,    0,0,0,0,0,0,0,0,
//  -------------------------------------------------------------------
      -8,    0,    5,    9,    9,    5,    0,   -8,    0,0,0,0,0,0,0,0,
      -8,    0,    5,    5,    5,    5,    0,   -8,    0,0,0,0,0,0,0,0,
      -8,    0,    0,    0,    0,    0,    0,   -8,    0,0,0,0,0,0,0,0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,0,0,0,0,0,0,0
};

static SCORE BACKWARD_SHIELDED_BY_LOCATION[128] = 
{
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0,
      -7,   -5,   -5,   -7,   -7,   -5,   -5,   -7,    0,0,0,0,0,0,0,0,
      -5,   -4,   -4,   -5,   -5,   -4,   -4,   -5,    0,0,0,0,0,0,0,0,
      -4,   -3,   -3,   -4,   -4,   -3,   -3,   -4,    0,0,0,0,0,0,0,0,
//  -------------------------------------------------------------------
      -4,   -3,   -3,   -4,   -4,   -3,   -3,   -4,    0,0,0,0,0,0,0,0,
      -5,   -4,   -4,   -5,   -5,   -4,   -4,   -5,    0,0,0,0,0,0,0,0,
      -7,   -5,   -5,   -7,   -7,   -5,   -5,   -7,    0,0,0,0,0,0,0,0,
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0
};

static SCORE BACKWARD_EXPOSED_BY_LOCATION[128] = 
{
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0,
     -12,   -9,   -9,  -11,  -11,   -9,   -9,  -12,    0,0,0,0,0,0,0,0,
      -9,   -7,   -7,   -9,   -9,   -7,   -7,   -9,    0,0,0,0,0,0,0,0,
      -7,   -6,   -6,   -7,   -7,   -6,   -6,   -7,    0,0,0,0,0,0,0,0,
//  -------------------------------------------------------------------
      -7,   -6,   -6,   -7,   -7,   -6,   -6,   -7,    0,0,0,0,0,0,0,0,
      -9,   -7,   -7,   -9,   -9,   -7,   -7,   -9,    0,0,0,0,0,0,0,0,
     -12,   -9,   -9,  -11,  -11,   -9,   -9,  -12,    0,0,0,0,0,0,0,0,
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0
};

//
// "The first statement I can confirm is that doubled pawns are indeed
// on average undesirable.  However this statistic needs to be broken
// down to be useful.  Doubled pawns themselves are really more
// serious than this generally, but when your pawns are doubled you
// automatically get an extra half-open file for your rooks (or queen,
// but the queen can also use diagonals).  So it follows logically
// that the net cost of doubled pawns is much greater in the absence
// of major pieces.  The database shows that with all rooks present,
// doubled pawns "cost" only about 1/16th of a pawn on average.  With
// one rook each the cost rises to 1/4th of a pawn and with no rooks
// present to 3/8ths of a pawn.  With the queens present the cost is
// again only 1/16th of a pawn; without them it's a quarter pawn.  So
// the lessons are clear; beware of doubled pawns when major pieces
// have been exchanged, and beware of exchanging major pieces when you
// are the one with the doubled pawns."
//                                                --Larry Kaufman, IM
//                                          "All About Doubled Pawns"
// 
// Note: indexed by number of files with 1+ pawn on them.
static SCORE DOUBLED_PAWN_PENALTY_BY_COUNT[4][9] = 
{    // ------------ count of doubled+ pawns ------------
    {// 0    1     2     3      4     5     6     7     8 : no majors alive
      +0,  -32,  -65,  -99,  -134, -170, -207, -222, -250
    },
    {//                                                   : 1 rook
      +0,  -23,  -47,  -76,  -108, -144, -184, -200, -216
    },
    {//                                                   : 2 rooks or 1 queen
      +0,  -13,  -23,  -34,  -46,   -64,  -86, -111, -138
    },
    {//                                                   : all majors alive
      +0,   -7,  -13,  -25,  -39,   -55,  -73,  -95, -121
    },
};

static SCORE ISOLATED_PAWN_PENALTY_BY_COUNT[9] = 
{//  0   1    2    3    4    5    6     7     8
    +0, -3,  -7,  -9, -16, -35, -50, -85, -100
};

static SCORE ISOLATED_PAWN_BY_PAWNFILE[9] =
{
    0, -7, -8, -9, -10, -10, -9, -8, -7
};

static SCORE ISOLATED_EXPOSED_PAWN = -5;

static SCORE ISOLATED_DOUBLED_PAWN = -11;

//
// Note: -25% to -33% if the enemy occupies or controls the next sq.
//
static SCORE PASSER_BY_RANK[2][9] = 
{//    0    1    2    3    4    5    6    7    8
    { +0,  +0,+162,+111, +62, +36, +18, +13,  +0 }, // black
    { +0,  +0, +13, +18, +36, +62,+111,+162,  +0 }  // white
};

static SCORE CANDIDATE_PASSER_BY_RANK[2][9] = 
{//    0    1    2    3    4    5    6    7   8
    { +0,  +0,  +0, +48, +34, +22, +13,  +9,  +0 }, // black
    { +0,  +0,  +9, +13, +22, +34, +48,  +0,  +0 }  // white
};

// Note: x2
static SCORE CONNECTED_PASSERS_BY_RANK[2][9] = 
{//    0    1    2    3    4    5    6    7   8
    { +0,  +0, +96, +74, +48, +21, +10,  +5,  +0 }, // black
    { +0,  +0,  +5, +10, +21, +48, +74, +96,  +0 }  // white
};

static SCORE SUPPORTED_PASSER_BY_RANK[2][9] = 
{//    0    1    2    3    4    5    6    7   8
    { +0,  +0, +60, +40, +13,  +6,  +3,  +1,  +0 }, // black
    { +0,  +0,  +1,  +3,  +6, +13, +40, +60,  +0 }  // white
};

static SCORE OUTSIDE_PASSER_BY_DISTANCE[9] =
{//    0    1    2    3    4    5    6    7    8
      +0,  +0,  +7, +14, +21, +28, +34, +41, +55 
};

static SCORE PASSER_BONUS_AS_MATERIAL_COMES_OFF[32] = 
{ 
//  none    na      na      m       na      R       2m      na
    +60,    -1,     -1,    +37,    -1,     +43,    +19,    -1,

//  Rm      3m/Q    2R      R2m     4m/Qm   2Rm     R3m/QR  Q2m
    +14,    +9,    +20,    +4,     +3,     +3,      0,      0,

//  2R2m    R4m/QRm Q3m     2R3m/   QR2m    Q4m     2R4m/   QR3m
//                          Q2R                     Q2Rm    
    0,      0,      0,      0,      0,      0,      0,      0,

//  na      Q2R2m   QR4m    na      Q2R3m   na      na      full
    0,      0,      0,      0,      0,      0,      0,      0,
};

SCORE RACER_WINS_RACE = +800;

static SCORE UNDEVELOPED_MINORS_IN_OPENING[5] = 
{//  0    1    2    3    4
     0,  -6, -10, -16, -22
};

//
// Bishop eval terms
// ---------------------------------------------------------------------------
//
static SCORE BISHOP_OVER_KNIGHT_IN_ENDGAME = +33;

//
// "The bishop pair has an average value of half a pawn (more when the
// opponent has no minor pieces to exchange for one of the bishops),
// enough to regard it as part of the material evaluation of the
// position, and enough to overwhelm most positional considerations.
// Moreover, this substantial bishop pair value holds up in all
// situations tested, regardless of what else is on the board...
// 
// ...One rule which I often teach to students is that if you have the
// bishop pair, and your opponent's single bishop is a bad bishop
// (hemmed in by his own pawns), you already have full compensation
// for a pawn... Kasparov has said something similar...
// 
// As noted before, the bishop pair is worth more with fewer pawns on
// the board. Aside from this factor, the half pawn value of the
// bishop pair is remarkably constant, applying even when there are no
// pieces on the board except two minors each."
//                                                 --Larry Kaufman, IM
//                                   "Evaluation of Material Imbalance"
static SCORE BISHOP_PAIR[2][17] =
{
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { 53, 52, 51, 50, 49, 48, 47, 45, 43, 41, 40, 40, 40, 40, 40, 40, 40 }
};

static SCORE STATIONARY_PAWN_ON_BISHOP_COLOR[128] = 
{
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0,
      -4,   -6,   -7,   -7,   -7,   -7,   -6,   -4,    0,0,0,0,0,0,0,0,
      -6,   -7,   -9,   -8,   -8,   -9,   -7,   -6,    0,0,0,0,0,0,0,0,
      -7,   -8,  -10,  -11,  -11,  -10,   -8,   -7,    0,0,0,0,0,0,0,0,
//  -------------------------------------------------------------------
      -7,   -8,  -10,  -11,  -11,  -10,   -8,   -7,    0,0,0,0,0,0,0,0,
      -6,   -7,   -9,   -8,   -8,   -9,   -7,   -6,    0,0,0,0,0,0,0,0,
      -4,   -6,   -7,   -7,   -7,   -7,   -6,   -4,    0,0,0,0,0,0,0,0,
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0
};

static SCORE TRANSIENT_PAWN_ON_BISHOP_COLOR[128] = 
{
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0,
      -1,   -1,   -2,   -2,   -2,   -2,   -1,   -1,    0,0,0,0,0,0,0,0,
      -3,   -4,   -4,   -4,   -4,   -4,   -4,   -3,    0,0,0,0,0,0,0,0,
      -4,   -5,   -7,   -8,   -8,   -7,   -5,   -4,    0,0,0,0,0,0,0,0,
//  -------------------------------------------------------------------
      -4,   -5,   -7,   -8,   -8,   -7,   -5,   -4,    0,0,0,0,0,0,0,0,
      -3,   -4,   -4,   -4,   -4,   -4,   -4,   -3,    0,0,0,0,0,0,0,0,
      -1,   -1,   -2,   -2,   -2,   -2,   -1,   -1,    0,0,0,0,0,0,0,0,
      +0,   +0,   +0,   +0,   +0,   +0,   +0,   +0,    0,0,0,0,0,0,0,0
};

// 
// A bishop can move to between 0..13 squares.
// 
static SCORE BISHOP_MOBILITY_BY_SQUARES[14] =
{//   0    1    2   3   4   5   6   7   8   9  10  11  12  13
    -22, -14, -10, -5, -1,  0, +1, +3, +4, +5, +6, +7, +8, +9
};//                    ^   |  

static SCORE BISHOP_MAX_MOBILITY_IN_A_ROW_BONUS[8] =
{//  0   1   2   3   4    5    6    7
   -10, -4, +1, +3, +4,  +5,  +5,  +5
};

static SCORE BISHOP_UNASSAILABLE_BY_DIST_FROM_EKING[9] =
{//  0    1    2    3    4    5    6    7    8
    +0, +33, +28, +18,  +8,  +4,  +0, -10, -16
};

static SCORE BISHOP_IN_CLOSED_POSITION[33] =
{//  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    10, 10, 10,  9,  9,  8,  8,  7,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
//  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32
};

//
// Knight eval terms
// ---------------------------------------------------------------------------
//
static SCORE KNIGHT_CENTRALITY_BONUS[128] = 
{
     -12,   -8,   -8,   -8,   -8,   -8,   -8,  -12,    0,0,0,0,0,0,0,0,
      -8,   -2,   -2,    0,    0,   -2,   -2,   -8,    0,0,0,0,0,0,0,0,
      -8,   -2,   +4,   +5,   +5,   +4,   -2,   -8,    0,0,0,0,0,0,0,0,
      -8,   -2,   +5,   +8,   +8,   +5,   -2,   -8,    0,0,0,0,0,0,0,0,
//  -------------------------------------------------------------------
      -8,   -2,   +5,   +8,   +8,   +5,   -2,   -8,    0,0,0,0,0,0,0,0,
      -8,   -2,   +4,   +5,   +5,   +4,   -2,   -8,    0,0,0,0,0,0,0,0,
      -8,   -2,   -2,    0,    0,   -2,   -2,   -8,    0,0,0,0,0,0,0,0,
     -12,   -8,   -8,   -8,   -8,   -8,   -8,  -12,    0,0,0,0,0,0,0,0
};

static SCORE KNIGHT_KING_TROPISM_BONUS[9] =
{//    0    1    2    3    4    5    6    7    8
       0, +15, +12,  +9,  +4,  +0,  +0,  +0,  +0 
};


static SCORE KNIGHT_UNASSAILABLE_BY_DIST_FROM_EKING[9] =
{//    0    1    2    3    4    5    6    7    8
      +0, +25, +19, +14,  +6,  +0,  +0,  +0,  +0
};

static SCORE KNIGHT_ON_INTERESTING_SQUARE_BY_RANK[2][9] =
{//    0    1    2    3    4    5    6    7    8
    { +0, +12, +11,  +9,  +6,  +3,  +0,  +0,  +0 }, // black
    { +0,  +0,  +0,  +0,  +3,  +6,  +9, +11, +12 }  // white
};

//
// A knight can move to between 0..8 squares
// -----------------------------------------
// 0 = supported enemy pawn
// 1 = unsupported enemy pawn, friend pawn, enemy B/N
//     enemy controlled empty sq.
// 2 = enemy >B, friend controlled empty sq, no one controlled empty sq
// 
// Total: 16 mobility max
// 
static SCORE KNIGHT_MOBILITY_BY_COUNT[9] =
{//   0    1   2   3   4   5   6   7   8
    -17, -10, -6,  0, +3, +5, +7, +8, +8
};//               ^   |

static SCORE KNIGHT_WITH_N_PAWNS_SUPPORTING[3] = 
{
    +0, +4, +8
};

static SCORE KNIGHT_IN_CLOSED_POSITION[33] =
{//  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  4,  6,  9, 11,
    12, 13, 15, 17, 19, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 27
//  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32
};

//
// Rook eval terms
// ---------------------------------------------------------------------------
//
static SCORE ROOK_ON_FULL_OPEN_BY_DIST_FROM_EKING[8] =
{//  0    1    2    3    4    5    6    7
   +24, +22, +17, +14, +13, +13, +12, +12
};

static SCORE ROOK_ON_HALF_OPEN_WITH_ENEMY_BY_DIST_FROM_EKING[8] = 
{//  0    1    2    3    4    5    6    7
   +12, +11,  +9,  +9,  +8,  +8,  +8,  +7
};

static SCORE ROOK_ON_HALF_OPEN_WITH_FRIEND_BY_DIST_FROM_EKING[8] =
{//  0    1    2    3    4    5    6    7
   +13, +12, +11, +11,  +9,  +9,  +9,  +8
};

static SCORE ROOK_BEHIND_PASSER_BY_PASSER_RANK[2][9] =
{//    0    1    2    3    4    5    6    7   8
    { +0,  +0, +25, +17, +12,  +6,  +1,  +0,  +0 }, // black
    { +0,  +0,  +0,  +1,  +6, +12, +17, +25,  +0 }  // white
};

static SCORE ROOK_LEADS_PASSER_BY_PASSER_RANK[2][9] =
{//    0    1    2    3    4    5    6    7   8
    { +0,  +0, -22, -16, -13,  -9,  -5,  -3,  +0 }, // black
    { +0,  +0,  -3,  -5,  -9, -13, -16, -22,  +0 }  // white
};

static SCORE KING_TRAPPING_ROOK = -40;

static SCORE ROOK_TRAPPING_EKING = +22;

static SCORE ROOK_VALUE_AS_PAWNS_COME_OFF[17] = 
{//   0    1    2    3    4    5    6    7    8 
    +55, +51, +44, +38, +33, +27, +22, +16,  +7,
 //        9   10   11   12   13   14   15   16
          +1,  -3,  -6,  -9, -12, -18, -22, -25
};

//
// Note: these are multiplied by two (one per rook). 
//
static SCORE ROOK_CONNECTED_VERT = +7;        // x2
static SCORE ROOK_CONNECTED_HORIZ = +4;       // x2

//
// A rook can move to between 0..14 squares
// ----------------------------------------
// 0 = supported enemy P/N/B, friend piece
// 1 = unsupported enemy piece, enemy controlled empty sq, enemy R
// 2 = enemy >R, friend controlled empty sq, no one controlled empty sq
// 
// Total: 28 mobility max
// 
static SCORE ROOK_MOBILITY_BY_SQUARES[15] =
{//   0    1    2    3    4   5   6   7   8    9   10   11   12   13   14
    -28, -24, -20, -14,  -7, -2, +0, +4, +8, +12, +15, +17, +19, +21, +22
};//                          ^   |

static SCORE ROOK_MAX_MOBILITY_IN_A_ROW_BONUS[8] =
{//  0    1   2   3   4    5    6    7
   -15,  -6, +0, +4, +8,  +8,  +8,  +8
};

//
// Queen eval terms
// ---------------------------------------------------------------------------
//


//
// A queen can move to between 0..27 squares
// -----------------------------------------
// 0 = supported enemy P/N/B/R, friend piece
// 1 = unsupported enemy piece, enemy controlled empty sq, enemy Q
// 2 = enemy K, no one controlled empty sq, friend controlled empty sq
// 
// Total: 54 mobility max
// 
static SCORE QUEEN_MOBILITY_BY_SQUARES[28] =
{// 0    1    2    3    4     5   6    7    8   9  10  11   12   13   14
  -30, -26, -22, -17, -11,   -8, -4,  -2,  -1, +2, +4, +7, +10, +12, +14,
//                                            |

 //15   16   17   18   19   20   21   22   23   24   25   26   27
  +15, +16, +17, +18, +19, +20, +20, +21, +21, +22, +22, +23, +23
};

static SCORE QUEEN_OUT_EARLY[5] = 
{// 0    1    2    3    4 : num unmoved minors
    0, -14, -22, -26, -33
};

//
// In "Evaluation of Material Imbalance", IM Larry Kaufman makes the
// point that queens are worth more then the standard nine "points".
// Thus, this scale is biased upwards by about 0.15 pawn.
//
static SCORE QUEEN_KING_TROPISM[8] = 
{// 0    1    2    3    4    5    6    7
    0, +34, +28, +22, +19, +17, +16, +15
};

static SCORE QUEEN_ATTACKS_SQ_NEXT_TO_KING = 8; // x #sq_attacked

//
// King eval terms
// ---------------------------------------------------------------------------
//
static ULONG KING_INITIAL_COUNTER_BY_LOCATION[2][128] = 
{
    {
           1,    0,    0,    0,    1,    0,    0,    1,    0,0,0,0,0,0,0,0,
           1,    1,    1,    1,    1,    1,    1,    1,    0,0,0,0,0,0,0,0,
           3,    3,    3,    3,    3,    3,    3,    3,    0,0,0,0,0,0,0,0,
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
    //  -------------------------------------------------------------------
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0
    },
    {
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
    //  -------------------------------------------------------------------
           4,    4,    4,    4,    4,    4,    4,    4,    0,0,0,0,0,0,0,0,
           3,    3,    3,    3,    3,    3,    3,    3,    0,0,0,0,0,0,0,0,
           1,    1,    1,    1,    1,    1,    1,    1,    0,0,0,0,0,0,0,0,
           1,    0,    0,    0,    1,    0,    0,    1,    0,0,0,0,0,0,0,0
    }
};

static SCORE KING_TO_CENTER[128] = 
{
      +0,  +0,  +0,  +0,  +0,  +0,  +0,  +0,    0,0,0,0,0,0,0,0,
      +1,  +3,  +5,  +7,  +7,  +5,  +3,  +1,    0,0,0,0,0,0,0,0,
      +3,  +5, +13, +15, +15, +13,  +5,  +3,    0,0,0,0,0,0,0,0,
      +5,  +7, +17, +23, +23, +17,  +7,  +5,    0,0,0,0,0,0,0,0,
//  ------------------------------------------------------------------
      +5,  +7, +17, +23, +23, +17,  +7,  +5,    0,0,0,0,0,0,0,0,
      +3,  +5, +13, +15, +15, +13,  +5,  +3,    0,0,0,0,0,0,0,0,
      +1,  +3,  +5,  +7,  +7,  +5,  +3,  +1,    0,0,0,0,0,0,0,0,
      +0,  +0,  +0,  +0,  +0,  +0,  +0,  +0,    0,0,0,0,0,0,0,0
};

//
// If a side has less than this much material we don't bother scoring
// the other side's king safety.  Note: this doesn't incl pawn
// material.
//
#define DO_KING_SAFETY_THRESHOLD \
    (VALUE_ROOK + VALUE_BISHOP + VALUE_KING + 1)

//
// If a side has less than this much material the other side's king
// can come out.  Note: this doesn't incl pawn material.
//
#define KEEP_KING_AT_HOME_THRESHOLD \
    (VALUE_ROOK + VALUE_ROOK + VALUE_BISHOP + VALUE_KING + 1)

static ULONG KING_COUNTER_BY_ATTACK_PATTERN[32] = 
{
//p 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
//m 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1
//r 0 0 0 0 1 1 1 1 0 0 0 0 1 1 1 1 0 0 0 0 1 1 1 1 0 0 0 0 1 1 1 1
//q 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1
//k 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
    0,0,2,2,1,1,3,3,1,1,3,3,2,2,4,4,2,2,3,3,2,2,4,4,2,2,4,4,3,4,5,5
};

static SCORE KING_SAFETY_BY_COUNTER[42] =
{
      -0,  -4,  -8, -12, -16, -20, -25, -31, -38, -50, -63, -76, -90,
    -105,-120,-135,-150,-165,-180,-195,-210,-225,-240,-255,-270,-290,
    -310,-330,-350,-370,-390,-410,-440,-470,-500,-500,-500,-500,-500,
    -500,-500,-500
};

static SCORE KING_MISSING_ONE_CASTLE_OPTION = -23;
typedef struct _DNA_BASE_SIZE {
    SCORE *pBase;
    ULONG uCount;
} DNA_BASE_SIZE;

#define DNA_VAR(x)    {(SCORE *)&(x), 1}
#define DNA_ARRAY(x)  {(SCORE *)(x), ARRAY_LENGTH(x)}
#define DNA_MATRIX(x) {(SCORE *)(x), ARRAY_LENGTH(x) * ARRAY_LENGTH((x)[0])}
static DNA_BASE_SIZE g_EvalDNA[] = {
    DNA_MATRIX(TRADE_PIECES),
    DNA_MATRIX(DONT_TRADE_PAWNS),
    DNA_ARRAY(REDUCED_MATERIAL_DOWN_SCALER),
    DNA_ARRAY(REDUCED_MATERIAL_UP_SCALER),
    DNA_ARRAY(PASSER_MATERIAL_UP_SCALER),
    DNA_ARRAY(PAWN_CENTRALITY_BONUS),
    DNA_ARRAY(BACKWARD_SHIELDED_BY_LOCATION),
    DNA_ARRAY(BACKWARD_EXPOSED_BY_LOCATION),
    DNA_MATRIX(DOUBLED_PAWN_PENALTY_BY_COUNT),
    DNA_ARRAY(ISOLATED_PAWN_PENALTY_BY_COUNT),
    DNA_ARRAY(ISOLATED_PAWN_BY_PAWNFILE),
    DNA_VAR(ISOLATED_EXPOSED_PAWN),
    DNA_VAR(ISOLATED_DOUBLED_PAWN),
    DNA_MATRIX(PASSER_BY_RANK),
    DNA_MATRIX(CANDIDATE_PASSER_BY_RANK),
    DNA_MATRIX(CONNECTED_PASSERS_BY_RANK),
    DNA_MATRIX(SUPPORTED_PASSER_BY_RANK),
    DNA_ARRAY(OUTSIDE_PASSER_BY_DISTANCE),
    DNA_ARRAY(PASSER_BONUS_AS_MATERIAL_COMES_OFF),
    DNA_VAR(RACER_WINS_RACE),
    DNA_ARRAY(UNDEVELOPED_MINORS_IN_OPENING),
    DNA_VAR(BISHOP_OVER_KNIGHT_IN_ENDGAME),
    DNA_MATRIX(BISHOP_PAIR),
    DNA_ARRAY(STATIONARY_PAWN_ON_BISHOP_COLOR),
    DNA_ARRAY(TRANSIENT_PAWN_ON_BISHOP_COLOR),
    DNA_ARRAY(BISHOP_MOBILITY_BY_SQUARES),
    DNA_ARRAY(BISHOP_MAX_MOBILITY_IN_A_ROW_BONUS),
    DNA_ARRAY(BISHOP_UNASSAILABLE_BY_DIST_FROM_EKING),
    DNA_ARRAY(BISHOP_IN_CLOSED_POSITION),
    DNA_ARRAY(KNIGHT_CENTRALITY_BONUS),
    DNA_ARRAY(KNIGHT_KING_TROPISM_BONUS),
    DNA_ARRAY(KNIGHT_UNASSAILABLE_BY_DIST_FROM_EKING),
    DNA_MATRIX(KNIGHT_ON_INTERESTING_SQUARE_BY_RANK),
    DNA_ARRAY(KNIGHT_MOBILITY_BY_COUNT),
    DNA_ARRAY(KNIGHT_WITH_N_PAWNS_SUPPORTING),
    DNA_ARRAY(KNIGHT_IN_CLOSED_POSITION),
    DNA_ARRAY(ROOK_ON_FULL_OPEN_BY_DIST_FROM_EKING),
    DNA_ARRAY(ROOK_ON_HALF_OPEN_WITH_ENEMY_BY_DIST_FROM_EKING),
    DNA_ARRAY(ROOK_ON_HALF_OPEN_WITH_FRIEND_BY_DIST_FROM_EKING),
    DNA_MATRIX(ROOK_BEHIND_PASSER_BY_PASSER_RANK),
    DNA_MATRIX(ROOK_LEADS_PASSER_BY_PASSER_RANK),
    DNA_VAR(KING_TRAPPING_ROOK),
    DNA_VAR(ROOK_TRAPPING_EKING),
    DNA_ARRAY(ROOK_VALUE_AS_PAWNS_COME_OFF),
    DNA_VAR(ROOK_CONNECTED_VERT),
    DNA_VAR(ROOK_CONNECTED_HORIZ),
    DNA_ARRAY(ROOK_MOBILITY_BY_SQUARES),
    DNA_ARRAY(ROOK_MAX_MOBILITY_IN_A_ROW_BONUS),
    DNA_ARRAY(QUEEN_MOBILITY_BY_SQUARES),
    DNA_ARRAY(QUEEN_OUT_EARLY),
    DNA_ARRAY(QUEEN_KING_TROPISM),
    DNA_VAR(QUEEN_ATTACKS_SQ_NEXT_TO_KING),
    DNA_MATRIX(KING_INITIAL_COUNTER_BY_LOCATION),
    DNA_ARRAY(KING_TO_CENTER),
    DNA_ARRAY(KING_SAFETY_BY_COUNTER),
    DNA_VAR(KING_MISSING_ONE_CASTLE_OPTION)
};

ULONG 
DNABufferSizeBytes() 
{
    ULONG u;
    ULONG uSize = 0;
    for (u = 0; u < ARRAY_LENGTH(g_EvalDNA); u++) {
        uSize += 10 * g_EvalDNA[u].uCount;
    }
    return uSize + 1;
}


char *
ExportEvalDNA() 
{
    ULONG uSize = DNABufferSizeBytes();
    char *p = malloc(uSize);
    ULONG u, v;
    SCORE *q;
    FLAG fFirst = TRUE;
    
    memset(p, 0, uSize);
    for (u = 0; u < ARRAY_LENGTH(g_EvalDNA); u++) {
        q = g_EvalDNA[u].pBase;
        for (v = 0; v < g_EvalDNA[u].uCount; v++) {
            if (!fFirst) {
                sprintf(p, "%s,%d", p, *q);
            } else {
                sprintf(p, "%s%d", p, *q);
                fFirst = FALSE;
            }
            q++;
        }
        strcat(p, "\n");
        fFirst = TRUE;
    }
    return p; // caller must free
}

FLAG
WriteEvalDNA(char *szFilename) 
{
    FILE *p = NULL;
    char *q = NULL;
    FLAG fRet = FALSE;

    if (SystemDoesFileExist(szFilename)) goto end;
    p = fopen(szFilename, "a+b");
    if (!p) goto end;
    q = ExportEvalDNA();
    if (!q) goto end;
    fprintf(p, q);
    fRet = TRUE;
 end:
    if (p) fclose(p);
    if (q) free(q);
    return fRet;
}

FLAG
ImportEvalDNA(char *p) 
{
    ULONG u, v;

    for (u = 0; u < ARRAY_LENGTH(g_EvalDNA); u++) {
        for (v = 0; v < g_EvalDNA[u].uCount; v++) {
            while(*p && (!isdigit(*p) && (*p != '-'))) p++;
            if (*p == '\0') return FALSE;
            *(g_EvalDNA[u].pBase + v) = atoi(p);
            while(*p && (isdigit(*p) || (*p == '-'))) p++;
        }
    }
    return TRUE;
}

FLAG
ReadEvalDNA(char *szFilename) 
{
    FILE *p = NULL;
    ULONG uSize = DNABufferSizeBytes();
    char *q = malloc(uSize);
    FLAG fRet = FALSE;
    static char line[1024];
    char *c;

    if (!q) goto end;
    memset(q, 0, uSize);
    p = fopen(szFilename, "rb");
    if (!p) {
        Trace("Failed to open file \"%s\"\n", szFilename);
        goto end;
    }
    while(fgets(line, 1024, p) != NULL) {
        c = strchr(line, '#');
        if (c) *c = '\0';
        c = line;
        strcat(q, c);
    }
    fRet = ImportEvalDNA(q);
 end:
    if (q) free(q);
    if (p) fclose(p);
    return fRet;
}



//
// Misc stuff
// ---------------------------------------------------------------------------
//

//
// The three ranks "around" a rank (indexed by rank, used in eval.c)
//
BITBOARD BBADJACENT_RANKS[9] =
{
    0,
    BBRANK11 | BBRANK22,
    BBRANK11 | BBRANK22 | BBRANK33,
    BBRANK22 | BBRANK33 | BBRANK44,
    BBRANK33 | BBRANK44 | BBRANK55,
    BBRANK44 | BBRANK55 | BBRANK66,
    BBRANK55 | BBRANK66 | BBRANK77,
    BBRANK66 | BBRANK77 | BBRANK88,
    BBRANK77 | BBRANK88
};


//
// The files "around" one, indexed by file (used in eval.c)
//
BITBOARD BBADJACENT_FILES[8] =
{
    // A = 0
    BBFILEB,
    
    // B = 1
    BBFILEA | BBFILEC,
    
    // C = 2
    BBFILEB | BBFILED,
    
    // D = 3
    BBFILEC | BBFILEE,
    
    // E = 4
    BBFILED | BBFILEF,
    
    // F = 5
    BBFILEE | BBFILEG,
    
    // G = 6
    BBFILEF | BBFILEH,
    
    // H = 7
    BBFILEG
};

//
// The ranks preceeding one, indexed by rank/color of pawn (used in 
// eval.c)
//
BITBOARD BBPRECEEDING_RANKS[8][2] =
{
    //     BLACK                          WHITE
    //     -----                          -----
    { // 0 == RANK8
        BBRANK72,                         0
    },
    { // 1 == RANK7
        BBRANK62,                         0
    },
    { // 2 == RANK6
        BBRANK52,                         BBRANK77
    },
    { // 3 == RANK5
        BBRANK42,                         BBRANK67
    },
    { // 4 == RANK4
        BBRANK32,                         BBRANK57
    },
    { // 5 == RANK3
        BBRANK22,                         BBRANK47
    },
    { // 6 == RANK2
        0,                                BBRANK37
    },
    { // 7 == RANK1
        0,                                BBRANK27
    }
};


static FLAG 
_IsSquareSafeFromEnemyPawn(IN POSITION *pos, 
                           IN COOR c, 
                           IN BITBOARD bb)
/**

Routine description:

    Determine if a piece in square c is "outposted".  A piece is 
    outposted if no enemy pawns can advance to attack it / drive
    it away.

Parameters:

    POSITION *pos : the board
    COOR c : the square (must not be empty!)
    BITBOARD bb : the bitboard of pawns you want to consider

Return value:

    FLAG : TRUE if the piece is safe/outposted, FALSE otherwise

**/
{
    ULONG uColor = GET_COLOR(pos->rgSquare[c].pPiece);
    ULONG uFile, uRank;
#ifdef DEBUG
    COOR cSquare;
    PIECE p = pos->rgSquare[c].pPiece;
    BITBOARD dbb = bb;

    ASSERT(IS_ON_BOARD(c));
    ASSERT(p && (IS_BISHOP(p) || IS_KNIGHT(p) || IS_PAWN(p)));
    ASSERT(IS_VALID_COLOR(uColor));
#endif
    
    uFile = FILE(c);
    ASSERT(uFile < 8);
    bb &= BBADJACENT_FILES[uFile];
    
    ASSERT(((c & 0x70) >> 4) == (c >> 4));
    uRank = c >> 4;
    ASSERT(uRank < 8);
    bb &= BBPRECEEDING_RANKS[uRank][uColor];
    
#ifdef DEBUG
    if (bb != 0)
    {
        while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&dbb)))
        {
            ASSERT(pos->rgSquare[cSquare].pPiece);
            ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
            if ((FILE(cSquare) == (FILE(c) - 1)) ||
                (FILE(cSquare) == (FILE(c) + 1)))
            {
                switch(uColor)
                {
                    case WHITE:
                        if (RANK(cSquare) > RANK(c))
                        {
                            return(FALSE);
                        }
                        break;
                    case BLACK:
                        if (RANK(cSquare) < RANK(c))
                        {
                            return(FALSE);
                        }
                        break;
                }
            }
        }
        ASSERT(FALSE);
    }
    else
    {
        ASSERT(bb == 0);
        while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&dbb)))
        {
            ASSERT(pos->rgSquare[cSquare].pPiece);
            ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
            if ((FILE(cSquare) == (FILE(c) - 1)) ||
                (FILE(cSquare) == (FILE(c) + 1)))
            {
                switch(uColor)
                {
                    case WHITE:
                        if (RANK(cSquare) > RANK(c))
                        {
                            ASSERT(FALSE);
                        }
                        break;
                    case BLACK:
                        if (RANK(cSquare) < RANK(c))
                        {
                            ASSERT(FALSE);
                        }
                        break;
                }
            }
        }
    }
#endif
    return(bb == 0);
}


static ULONG 
_WhoControlsSquareFast(IN POSITION *pos, 
                       IN COOR c)
/**

Routine description:

    Determine which side controls a board square.

Parameters:

    POSITION *pos : the board
    COOR c : the square in question

Return value:

    static ULONG : (ULONG)-1 if neither side controls it or the sides
    control it evenly, WHITE if white controls the square, or BLACK if
    black controls the square.
    
    TODO: fix this to use counts / xrays

**/
{
    ULONG uWhite = pos->rgSquare[c|8].bvAttacks[WHITE].uSmall |
                   pos->rgSquare[c|8].bvAttacks[WHITE].uXray;
    ULONG uBlack = pos->rgSquare[c|8].bvAttacks[BLACK].uSmall |
                   pos->rgSquare[c|8].bvAttacks[BLACK].uXray;
    ULONG u;
    PIECE p;
    CHAR ch;
    
    ASSERT((c + 8) == (c | 8));
    ASSERT((uWhite & 0xFFFFFF00) == 0);
    ASSERT((uBlack & 0xFFFFFF00) == 0);

    // TODO: keep these and update the table to use them
    uWhite >>= 3;
    uBlack >>= 3;
    
    p = pos->rgSquare[c].pPiece;
    // p -= 2;
    ch = g_SwapTable[p][uWhite][uBlack];
    if (ch != 0)
    {
        u = ch;
        u >>= 7;
        u = u & 1;
        ASSERT(((u == 1) && (ch < 0)) ||
               ((u == 0) && (ch > 0)));
        return(FLIP(u));
    }
    return((ULONG)-1);
}

/**

Routine description:

    Zero out the attack table before building it.

Parameters:

    POSITION *pos

Return value:

    void

**/
#define CLEAR_A_SQ \
        pos->rgSquare[c].bvAttacks[0].uWholeThing = 0; \
        pos->rgSquare[c].bvAttacks[1].uWholeThing = 0;

#define CLEAR_A_RANK \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c += 9;

#define CLEAR_SHORT_RANK \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c++; \
        CLEAR_A_SQ; c += 10;

static void 
_ClearAttackTables(IN OUT POSITION *pos)
{
    register COOR c = 8;
#if 1
    CLEAR_A_SQ; c += 16;
    CLEAR_A_SQ; c += 16;
    CLEAR_A_SQ; c += 16;
    CLEAR_A_SQ; c += 16;
    CLEAR_A_SQ; c += 16;
    CLEAR_A_SQ; c += 16;
    CLEAR_A_SQ; c += 16;
    CLEAR_A_SQ; c = 9;
    CLEAR_SHORT_RANK;
    CLEAR_SHORT_RANK;
    CLEAR_SHORT_RANK;
    CLEAR_SHORT_RANK;
    CLEAR_SHORT_RANK;
    CLEAR_SHORT_RANK;
    CLEAR_SHORT_RANK;
    CLEAR_SHORT_RANK;
#else
    CLEAR_A_RANK;
    CLEAR_A_RANK;
    CLEAR_A_RANK;
    CLEAR_A_RANK;
    CLEAR_A_RANK;
    CLEAR_A_RANK;
    CLEAR_A_RANK;
    CLEAR_A_RANK;
#endif
}


static INLINE void 
_InitializePawnHashEntry(IN OUT PAWN_HASH_ENTRY *pHash, 
                         IN POSITION *pos)
/**

Routine description:

Parameters:

    PAWN_HASH_ENTRY *pHash,
    POSITION *pos

Return value:

    static INLINE void

**/
{
    memset(pHash, 0, sizeof(PAWN_HASH_ENTRY));
    pHash->u64Key = pos->u64PawnSig;
}


static INLINE void
_EvaluateCandidatePasser(IN POSITION *pos,
                         IN OUT PAWN_HASH_ENTRY *pHash,
                         IN COOR c)
/**

Routine description:

Parameters:

    POSITION *pos,
    PAWN_HASH_ENTRY *pHash,
    COOR c

Return value:

    static INLINE

**/
{
    COOR c1, cSquare;
    PIECE pSentry, pHelper;
    ULONG uPawnFile = FILE(c) + 1;
    BITBOARD bb;
    ULONG uSentries, uHelpers;
    ULONG uColor;
    int d1;
#ifdef DEBUG
    COOR cVerifySquare;
    ULONG uVerifySentries;
#endif

    ASSERT(IS_ON_BOARD(c));
    ASSERT(RANK(c) >= 2);
    ASSERT(RANK(c) <= 7);
    pHelper = pos->rgSquare[c].pPiece;
    ASSERT(IS_PAWN(pHelper));
    uColor = GET_COLOR(pHelper);
    ASSERT(IS_VALID_COLOR(uColor));
    
    if (pHash->uCountPerFile[FLIP(uColor)][uPawnFile] != 0)
    {
        ASSERT((pHash->bbPawnLocations[FLIP(uColor)] & BBFILE[FILE(c)]) != 0);
        
        //
        // The only way a pawn can be a passer/candidate if the other
        // side has a pawn on its same file is if that enemy pawn is
        // behind it.  This is a corner case but it is important to
        // detect all passers.
        //
        switch(uColor)
        {
            case WHITE:
                bb = pHash->bbPawnLocations[BLACK] & BBFILE[FILE(c)];
                ASSERT(bb);
                while(IS_ON_BOARD(c1 = CoorFromBitBoardRank8ToRank1(&bb)))
                {
                    ASSERT(FILE(c1) == FILE(c));
                    if (c1 < c) return;
                }
                break;
            case BLACK:
                bb = pHash->bbPawnLocations[WHITE] & BBFILE[FILE(c)];
                ASSERT(bb);
                while(IS_ON_BOARD(c1 = CoorFromBitBoardRank1ToRank8(&bb)))
                {
                    ASSERT(FILE(c1) == FILE(c));
                    if (c1 > c) return;
                }
                break;
        }
    }
        
    //
    // Count FLIP(uColor)'s sentries and determine the location of the
    // critical square.  Note if there are no sentries then this 
    // pawn (on square c) is a passer already, not a candidate.
    // 
    bb = pHash->bbPawnLocations[FLIP(uColor)] & BBADJACENT_FILES[FILE(c)];
    bb &= BBPRECEEDING_RANKS[(c & 0x70) >> 4][uColor];
    if (!bb)
    {
        ASSERT(CountBits(bb) == 0);

        //
        // There are no sentries so this pawn is a passer.
        //
        pHash->bbPasserLocations[uColor] |= COOR_TO_BB(c);
        ASSERT(CountBits(pHash->bbPasserLocations[uColor]) > 0);
        ASSERT(CountBits(pHash->bbPasserLocations[uColor]) <= 8);
        EVAL_TERM(uColor,
                  PAWN,
                  c,
                  pHash->iScore[uColor],
                  PASSER_BY_RANK[uColor][RANK(c)],
                  "passed pawn");
        
        //
        // However, don't give doubled passers such a big bonus.
        //
        if (IS_PAWN(pos->rgSquare[c - 16 * g_iAhead[uColor]].pPiece)) 
        {
            EVAL_TERM(uColor,
                      PAWN,
                      c,
                      pHash->iScore[uColor],
                      -(PASSER_BY_RANK[uColor][RANK(c)] / 2),
                      "doubled passer");
        }
        return;
    }
    uSentries = CountBits(bb);

    //
    // There's one or more sentry pawns so we'll look for helpers to
    // decide if this pawn is a candidate passer.
    //
    if (uColor == WHITE)
    {
        pSentry = BLACK_PAWN;
        cSquare = CoorFromBitBoardRank1ToRank8(&bb);
        cSquare += 0x10;
    }
    else
    {
        pSentry = WHITE_PAWN;
        cSquare = CoorFromBitBoardRank8ToRank1(&bb);
        cSquare -= 0x10;
    }
    cSquare &= 0xF0;
    cSquare |= FILE(c);
    ASSERT(IS_ON_BOARD(cSquare));
    ASSERT(FILE(cSquare) == FILE(c));

#ifdef DEBUG
    uVerifySentries = 0;
    cVerifySquare = 0;
    d1 = 16 * g_iAhead[uColor];
    c1 = c + d1;
    do
    {
        if (IS_ON_BOARD(c1 - 1))
        {
            if (pos->rgSquare[c1 - 1].pPiece == pSentry)
            {
                uVerifySentries++;
                if (0 == cVerifySquare) 
                {
                    cVerifySquare = c1 - d1;
                    ASSERT(cVerifySquare == cSquare);
                }
            }
        }
        if (IS_ON_BOARD(c1 + 1))
        {
            if (pos->rgSquare[c1 + 1].pPiece == pSentry)
            {
                uVerifySentries++;
                if (0 == cVerifySquare)
                {
                    cVerifySquare = c1 - d1;
                    ASSERT(cVerifySquare == cSquare);
                }
            }
        }
        c1 = c1 + d1;
    }
    while(!RANK8(c1) && !RANK1(c1));
    ASSERT(uVerifySentries == uSentries);
#endif

    //
    // Note: we don't do this with bitboards because we want to not
    // consider a pawn to be a potential passer if it can't safely
    // advance to get into helper position.
    //
    // IDEA: scale the candidate passer bonus based on rank AND on
    // the distance the helper(s) have to go to get into position.
    //
    uHelpers = 0;
    d1 = 16 * g_iAhead[uColor];
    ASSERT(-d1 == 16 * g_iBehind[uColor]);
    c1 = cSquare + 1 - d1;
    if ((IS_ON_BOARD(c1)) && (pHash->uCountPerFile[uColor][FILE(c1) + 1]))
    {
        ASSERT(pHash->bbPawnLocations[uColor] & BBFILE[FILE(c1)]);
        if (!(pos->rgSquare[c1 + 8].bvAttacks[FLIP(uColor)].uWholeThing) ||
            (pos->rgSquare[c1 + 8].bvAttacks[uColor].uWholeThing))
        {
            //
            // The square c1 the place a helper pawn must get to in
            // order to aide the candidate past a sentry.
            //
            if (pos->rgSquare[c1].pPiece == pHelper)
            {
                uHelpers = 1;
                goto do_left;
            }
            
            //
            // There is no helper pawn in the support position yet.
            // See if one can get there.
            //
            c1 = c1 - d1;
            while (IS_ON_BOARD(c1) &&
               ((!(pos->rgSquare[c1+8].bvAttacks[FLIP(uColor)].uWholeThing)) ||
                (pos->rgSquare[c1+8].bvAttacks[uColor].uWholeThing)))
            {
                if (pos->rgSquare[c1].pPiece == pHelper)
                {
                    uHelpers = 1;
                    break;
                }
                else if (pos->rgSquare[c1].pPiece == pSentry)
                {
                    break;
                }
                c1 = c1 - d1;
            }
        }
    }
    
 do_left:
    c1 = cSquare - 1 - d1;
    if ((IS_ON_BOARD(c1)) && (pHash->uCountPerFile[uColor][FILE(c1) + 1]))
    {
        ASSERT(pHash->bbPawnLocations[uColor] & BBFILE[FILE(c1)]);

        if (!(pos->rgSquare[c1 + 8].bvAttacks[FLIP(uColor)].uWholeThing) ||
            (pos->rgSquare[c1 + 8].bvAttacks[uColor].uWholeThing))
        {
            //
            // The square c1 is the place a helper pawn must get to in
            // order to aide the candidate.
            //
            if (pos->rgSquare[c1].pPiece == pHelper)
            {
                uHelpers++;
                goto done_helpers;
            }
            
            //
            // There is no pawn in the left support position yet.  See
            // if one can get there.
            //
            c1 -= d1;
            while (IS_ON_BOARD(c1) &&
               ((!(pos->rgSquare[c1+8].bvAttacks[FLIP(uColor)].uWholeThing)) ||
                (pos->rgSquare[c1 + 8].bvAttacks[uColor].uWholeThing)))
            {
                if (pos->rgSquare[c1].pPiece == pHelper)
                {
                    uHelpers++;
                    break;
                }
                else if (pos->rgSquare[c1].pPiece == pSentry)
                {
                    break;
                }
                c1 -= d1;
            }
        }
    }
    
 done_helpers:
    if ((uHelpers >= uSentries) ||
        ((pHash->uCountPerFile[uColor][uPawnFile + 1] +
          pHash->uCountPerFile[uColor][uPawnFile - 1]) &&
         (((WHITE == uColor) && (RANK(c) > 5)) ||
          ((BLACK == uColor) && (RANK(c) < 4)))))
    {
        ASSERT(CANDIDATE_PASSER_BY_RANK[uColor][RANK(c)] > 0);
        EVAL_TERM(uColor,
                  PAWN,
                  c,
                  pHash->iScore[uColor],
                  CANDIDATE_PASSER_BY_RANK[uColor][RANK(c)],
                  "candidate passer");

        //
        // If the other side has no pieces then give this candidate an
        // extra bonus.
        //
        if (pos->uNonPawnCount[FLIP(uColor)][0] == 1)
        {
            EVAL_TERM(uColor,
                      PAWN,
                      c,
                      pHash->iScore[uColor],
                      CANDIDATE_PASSER_BY_RANK[uColor][RANK(c)],
                      "candidate passer in endgame");
        }
    }
}

static void 
_EvaluateConnectedSupportedOutsidePassers(IN POSITION *pos,
                                          IN OUT PAWN_HASH_ENTRY *pHash)
/**

Routine description:

Parameters:

    POSITION *pos,
    PAWN_HASH_ENTRY *pHash

Return value:

    void

**/
{
    ULONG uColor;
    ULONG u, v;
    COOR c;
    COOR cLeftmostPasser, cRightmostPasser;
    PIECE pFriend;
    COOR cSupport;
    BITBOARD bb;
    static const INT iDelta[5] = 
    {
        -1, +1, +17, +15, 0
    };
    
    FOREACH_COLOR(uColor)
    {
        ASSERT(IS_VALID_COLOR(uColor));
        
        if (pHash->bbPasserLocations[uColor])
        {
            ASSERT(pos->uPawnCount[uColor] > 0);
            pFriend = BLACK_PAWN | uColor;
            cLeftmostPasser = cRightmostPasser = ILLEGAL_COOR;
            for (u = A; u <= H; u++)
            {
                bb = pHash->bbPasserLocations[uColor] & BBFILE[u];
                if (bb != 0)
                {
                    ASSERT(pHash->uCountPerFile[uColor][u+1] != 0);
                    while(IS_ON_BOARD(c = CoorFromBitBoardRank8ToRank1(&bb)))
                    {
                        //
                        // Keep track of leftmost/rightmost passer for
                        // outside passer code later on.
                        //
                        ASSERT(FILE(c) == u);
                        ASSERT(RANK(c) > 1);
                        ASSERT(RANK(c) < 8);
                        if (cLeftmostPasser == ILLEGAL_COOR)
                        {
                            cLeftmostPasser = c;
                        }
                        cRightmostPasser = c;
                        
                        v = 0;
                        while(iDelta[v] != 0)
                        {
                            cSupport = c + iDelta[v] * g_iBehind[uColor];
                            if (IS_ON_BOARD(cSupport))
                            {
                                if (pHash->bbPasserLocations[uColor] & 
                                    COOR_TO_BB(cSupport))
                                {
                                    ASSERT(CONNECTED_PASSERS_BY_RANK[uColor]
                                           [RANK(c)] > 0);
                                    EVAL_TERM(uColor,
                                              PAWN,
                                              c,
                                              pHash->iScore[uColor],
                                              CONNECTED_PASSERS_BY_RANK[uColor]
                                              [RANK(c)],
                                              "connected passers");

                                    //
                                    // TODO: connected passers vs a R is
                                    // strong.
                                    //
                                }
                                else if (pos->rgSquare[cSupport].pPiece == 
                                         pFriend)
                                {
                                    ASSERT(SUPPORTED_PASSER_BY_RANK[uColor]
                                           [RANK(c)] > 0);
                                    EVAL_TERM(uColor,
                                              PAWN,
                                              c,
                                              pHash->iScore[uColor],
                                              SUPPORTED_PASSER_BY_RANK[uColor]
                                              [RANK(c)],
                                              "supported passer");
                                }
                            }
                            v++;
                        }
                    }
                }
            }
            
#ifdef DEBUG
            ASSERT(IS_ON_BOARD(cLeftmostPasser));
            ASSERT(IS_ON_BOARD(cRightmostPasser));
            ASSERT(RANK(cLeftmostPasser) > 1);
            ASSERT(RANK(cRightmostPasser) > 1);
            ASSERT(RANK(cLeftmostPasser) < 8);
            ASSERT(RANK(cRightmostPasser) < 8);
            if (CountBits(pHash->bbPasserLocations[uColor]) == 1)
            {
                ASSERT(cLeftmostPasser == cRightmostPasser);
            }
#endif
            for (u = A; u <= D; u++)
            {
                if (pHash->uCountPerFile[FLIP(uColor)][u + 1] != 0)
                {
                    ASSERT(pHash->bbPawnLocations[FLIP(uColor)] & BBFILE[u]);
                    if (!(pHash->bbPasserLocations[FLIP(uColor)] & BBFILE[u]))
                    {
                        break;
                    }
                }
            }
            if (u > FILE(cLeftmostPasser))
            {
                EVAL_TERM(uColor,
                          PAWN,
                          ILLEGAL_COOR,
                          pHash->iScore[uColor],
                          OUTSIDE_PASSER_BY_DISTANCE[u-FILE(cLeftmostPasser)],
                          "left outside passer");
            }

            for (u = H; u >= E; u--)
            {
                if (pHash->uCountPerFile[FLIP(uColor)][u + 1] != 0)
                {
                    ASSERT(pHash->bbPawnLocations[FLIP(uColor)] & BBFILE[u]);
                    if (!(pHash->bbPasserLocations[FLIP(uColor)] & BBFILE[u]))
                    {
                        break;
                    }
                }
            }
            if (u < FILE(cRightmostPasser))
            {
                EVAL_TERM(uColor,
                          PAWN,
                          ILLEGAL_COOR,
                          pHash->iScore[uColor],
                          OUTSIDE_PASSER_BY_DISTANCE[FILE(cRightmostPasser)-u],
                          "right outside passer");
            }
        }
    }
}
                    

#define PAWN_ATTACK_BLACK_DELTA (+15)
#define PAWN_ATTACK_WHITE_DELTA (-17)

static void 
_PopulatePawnAttackBits(IN OUT POSITION *pos)
/**

Routine description:

    Populate the attack table with pawn bits.

Parameters:

    POSITION *pos : the board

Return value:

    void

**/
{
    ULONG u;
    COOR c;
    COOR cAttack;

    //
    // IDEA: combine clearing and initial population somehow?
    //
    // r - - - - - - l : only check L- and R-
    // r - - - - - - l
    // R A A A A A A L
    // R A A A A A A L
    // R A A A A A A L
    // R A A A A A A L
    // r - - - - - - l
    // r - - - - - - l : only check L+ and R+
    _ClearAttackTables(pos);

    ASSERT(pos->uPawnCount[BLACK] <= 8);
    for (u = 0;
         u < pos->uPawnCount[BLACK]; 
         u++)
    {
        c = pos->cPawns[BLACK][u];
        ASSERT(IS_ON_BOARD(c));
        ASSERT(pos->rgSquare[c].pPiece);
        ASSERT(IS_PAWN(pos->rgSquare[c].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[c].pPiece) == BLACK);
            
        //
        // IDEA: These IS_ON_BOARD checks can be removed by a lookup table.
        // 
        cAttack = c + PAWN_ATTACK_BLACK_DELTA;
        if (IS_ON_BOARD(cAttack))
        {
            ASSERT((cAttack + 8) == (cAttack|8));
            ASSERT(!IS_ON_BOARD(cAttack|8));
            pos->rgSquare[cAttack|8].bvAttacks[BLACK].small.uPawn = 1;
        }
        cAttack += 2;
        if (IS_ON_BOARD(cAttack))
        {
            ASSERT((cAttack + 8) == (cAttack|8));
            cAttack |= 8;
            ASSERT(!IS_ON_BOARD(cAttack));
            pos->rgSquare[cAttack].bvAttacks[BLACK].small.uPawn = 1;
        }
    }

    ASSERT(pos->uPawnCount[WHITE] <= 8);
    for (u = 0;
         u < pos->uPawnCount[WHITE]; 
         u++)
    {
        c = pos->cPawns[WHITE][u];
        ASSERT(IS_ON_BOARD(c));
        ASSERT(pos->rgSquare[c].pPiece);
        ASSERT(IS_PAWN(pos->rgSquare[c].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[c].pPiece) == WHITE);
            
        cAttack = c + PAWN_ATTACK_WHITE_DELTA;
        if (IS_ON_BOARD(cAttack))
        {
            ASSERT((cAttack + 8) == (cAttack|8));
            ASSERT(!IS_ON_BOARD(cAttack|8));
            pos->rgSquare[cAttack|8].bvAttacks[WHITE].small.uPawn = 1;
        }
        cAttack += 2;
        if (IS_ON_BOARD(cAttack))
        {
            ASSERT((cAttack + 8) == (cAttack|8));
            cAttack |= 8;
            ASSERT(!IS_ON_BOARD(cAttack));
            pos->rgSquare[cAttack].bvAttacks[WHITE].small.uPawn = 1;
        }
    }
}
    

static PAWN_HASH_ENTRY *
_EvalPawns(IN OUT SEARCHER_THREAD_CONTEXT *ctx, 
           OUT FLAG *pfDeferred)
/**

Routine description:

    Evaluate pawn structures; return a ptr to a pawn hash entry.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx : the searcher thread context

Return value:

    PAWN_HASH_ENTRY *

**/
{
    static ULONG uUnmovedRank[2] = { 0x10, 0x60 };
    POSITION *pos = &ctx->sPosition;
    PAWN_HASH_ENTRY *pHash;
    COOR c, cSquare;
    PIECE p;
    ULONG uIsolated[2];
    ULONG uDoubled[2];
    SCORE iDuos[2];
    ULONG u, uPawnFile;
    ULONG uColor;
    ULONG uUnsupportable;
    BITBOARD bb;
    int d1;
#ifdef DEBUG 
    SCORE t;
#endif

    //
    // Now, look up the hash entry for this position.
    //
    INC(ctx->sCounters.pawnhash.u64Probes);
    pHash = PawnHashLookup(ctx);
    ASSERT(NULL != pHash);
    if (pHash->u64Key == pos->u64PawnSig)
    {
        *pfDeferred = TRUE;
        INC(ctx->sCounters.pawnhash.u64Hits);
        return(pHash);
    }

    //
    // We need the attack table to evaluate the pawns... and we missed
    // the hash table so we have to populate it now.  Can't defer this
    // one. :(
    //
    *pfDeferred = FALSE;
    _InitializePawnHashEntry(pHash, pos);
    _PopulatePawnAttackBits(pos);
    uIsolated[BLACK] = uIsolated[WHITE] = 0;
    uDoubled[BLACK] = uDoubled[WHITE] = 0;
    iDuos[BLACK] = iDuos[WHITE] = 0;

    //
    // First pass
    //
    FOREACH_COLOR(uColor)
    {
        ASSERT(IS_VALID_COLOR(uColor));
        ASSERT(pos->uPawnCount[uColor] <= 8);
        
        d1 = 16 * g_iAhead[uColor];
        for (u = 0; 
             u < pos->uPawnCount[uColor]; 
             u++)
        {
            c = pos->cPawns[uColor][u];
            ASSERT(IS_ON_BOARD(c));
            ASSERT(pos->rgSquare[c].pPiece);
            ASSERT(IS_PAWN(pos->rgSquare[c].pPiece));
            uPawnFile = FILE(c) + 1;
            ASSERT((1 <= uPawnFile) && (uPawnFile <= 8));
            
            //
            // Update files counter
            //
            ASSERT(pHash->uCountPerFile[uColor][uPawnFile] < 6);
            pHash->uCountPerFile[uColor][uPawnFile]++;
            
            //
            // Update bitboard bit
            //
            ASSERT(CountBits(pHash->bbPawnLocations[uColor] & 
                             BBFILE[uPawnFile-1]) < 6);
            ASSERT((pHash->bbPawnLocations[uColor] & COOR_TO_BB(c)) == 0);
            pHash->bbPawnLocations[uColor] |= COOR_TO_BB(c);
            ASSERT(CountBits(pHash->bbPawnLocations[uColor] &
                             BBFILE[uPawnFile-1]) ==
                   pHash->uCountPerFile[uColor][uPawnFile]);

            //
            // Count unmoved pawns
            //
            pHash->uNumUnmovedPawns[uColor] += 
                ((c & 0xF0) == uUnmovedRank[uColor]);
            ASSERT(pHash->uNumUnmovedPawns[uColor] <= 8);
            
            //
            // Detect rammed and other stationary pawns.
            //
            cSquare = c + d1;
            ASSERT(IS_ON_BOARD(cSquare));
            p = pos->rgSquare[cSquare].pPiece; 
            if (IS_PAWN(p))
            {
                pHash->bbStationaryPawns[uColor] |= COOR_TO_BB(c);
                ASSERT(CountBits(pHash->bbStationaryPawns[uColor]) <= 8);
                pHash->uNumRammedPawns += OPPOSITE_COLORS(p, uColor);
                ASSERT(pHash->uNumRammedPawns <= 16);
            }

            //
            // Give central pawns a small bonus; penalize rook pawns:
            //
            EVAL_TERM(uColor,
                      PAWN,
                      c,
                      pHash->iScore[uColor],
                      PAWN_CENTRALITY_BONUS[c],
                      "centrality");
        }
    }

    //
    // We counted rammed pawns twice (once for the black pawn and once
    // for the white one).  Fix this now.
    //
    ASSERT(!(pHash->uNumRammedPawns & 1));
    pHash->uNumRammedPawns /= 2;

    //
    // Second pass
    // 
    FOREACH_COLOR(uColor)
    {
        ASSERT(IS_VALID_COLOR(uColor));
        ASSERT(pos->uPawnCount[uColor] <= 8);
        ASSERT(CountBits(pHash->bbPawnLocations[uColor]) <= 8);
        
        d1 = 16 * g_iAhead[uColor];
        for (u = 0; 
             u < pos->uPawnCount[uColor];
             u++)
        {
            c = pos->cPawns[uColor][u];
            ASSERT(IS_ON_BOARD(c));
            ASSERT(IS_PAWN(pos->rgSquare[c].pPiece));
            ASSERT((1 < RANK(c)) && (RANK(c) < 8));
            
            //
            // Find weak pawns... pawns that are isolated are weak as
            // are pawns that have advanced beyond their support.
            // Both of these can tie up one or more pieces in order to
            // defend them.
            //
            uUnsupportable = 0;
            uPawnFile = FILE(c) - 1;
            if (pHash->uCountPerFile[uColor][uPawnFile + 1] > 0)
            {
                bb = (pHash->bbPawnLocations[uColor] & 
                      BBFILE[uPawnFile] &
                      BBADJACENT_RANKS[RANK(c)]);
                if (!bb)
                {
                    bb = (pHash->bbPawnLocations[FLIP(uColor)] &
                          BBFILE[uPawnFile]);
                    while(IS_ON_BOARD(cSquare = 
                                      CoorFromBitBoardRank8ToRank1(&bb)))
                    {
                        ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
                        if (uColor == WHITE)
                        {
                            if ((cSquare & 0xF0) >= (c & 0xF0))
                            {
                                ASSERT(RANK(cSquare) <= RANK(c));
                                uUnsupportable = 1;
                                break;
                            }
                        }
                        else
                        {
                            ASSERT(uColor == BLACK);
                            if ((cSquare & 0xF0) <= (c & 0xF0))
                            {
                                ASSERT(RANK(cSquare) >= RANK(c));
                                uUnsupportable = 1;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                uUnsupportable = 1;           // no friend pawn on that file
            }
            if (0 == uUnsupportable)
            {
                goto fast_skip;
            }
            ASSERT(1 == uUnsupportable);

            uPawnFile = FILE(c) + 1;
            if (pHash->uCountPerFile[uColor][uPawnFile + 1] > 0)
            {
                bb = (pHash->bbPawnLocations[uColor] & 
                      BBFILE[uPawnFile] &
                      BBADJACENT_RANKS[RANK(c)]);
                if (!bb)
                {
                    bb = (pHash->bbPawnLocations[FLIP(uColor)] &
                          BBFILE[uPawnFile]);
                    while(IS_ON_BOARD(cSquare = 
                                      CoorFromBitBoardRank8ToRank1(&bb)))
                    {
                        ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
                        if (uColor == WHITE)
                        {
                            if ((cSquare & 0xF0) >= (c & 0xF0))
                            {
                                ASSERT(RANK(cSquare) <= RANK(c));
                                uUnsupportable = 2;
                                break;
                            }
                        }
                        else
                        {
                            ASSERT(uColor == BLACK);
                            if ((cSquare & 0xF0) <= (c & 0xF0))
                            {
                                ASSERT(RANK(cSquare) >= RANK(c));
                                uUnsupportable = 2;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                uUnsupportable = 2;
            }
            
            //
            // If this pawn is not supportable from either side then
            // it is either isolated or has been pushed too far ahead
            // of its support.  Either way it's a target -- penalize
            // it.
            //
            if (2 == uUnsupportable)
            {
                uPawnFile = FILE(c) + 1;
                EVAL_TERM(uColor,
                          PAWN,
                          c,
                          pHash->iScore[uColor],
                          ISOLATED_PAWN_BY_PAWNFILE[uPawnFile],
                          "target/isolated pawn");

                //
                // Isolated + exposed?  Extra penalty.
                //
                ASSERT(ISOLATED_EXPOSED_PAWN < 0);
                EVAL_TERM(uColor,
                          PAWN,
                          c,
                          pHash->iScore[uColor],
                          ((pHash->uCountPerFile[FLIP(uColor)][uPawnFile]==0)* 
                           ISOLATED_EXPOSED_PAWN),
                          "exposed target");
                        
                //
                // Exponential penalty term
                //
                uIsolated[uColor] += 1;
                ASSERT(uIsolated[uColor] > 0);
                ASSERT(uIsolated[uColor] <= 8);
                        
                //
                // Isolated + doubled?  Extra penalty.
                //
                ASSERT(ISOLATED_DOUBLED_PAWN < 0);
                EVAL_TERM(uColor,
                          PAWN,
                          c,
                          pHash->iScore[uColor],
                          ((pHash->uCountPerFile[uColor][uPawnFile] > 1) *
                           ISOLATED_DOUBLED_PAWN),
                          "doubled + target");
            }

 fast_skip:
            uPawnFile = FILE(c) + 1;
            ASSERT((1 <= uPawnFile) && (uPawnFile <= 8));
            ASSERT(CountBits(pHash->bbPawnLocations[uColor] & BBFILE[FILE(c)])
                   == pHash->uCountPerFile[uColor][uPawnFile]);

            //
            // Keep count of doubled pawns
            //
            uDoubled[uColor] += (pHash->uCountPerFile[uColor][uPawnFile] > 1);
            ASSERT((0 <= uDoubled[uColor]) && (uDoubled[uColor] <= 8));

            //
            // We can detect pawn duos (triads, etc...) and backward pawns
            // by considering the control of the pawn's stopsquare.
            //
            cSquare = c + d1;
            ASSERT(IS_ON_BOARD(cSquare));
            ASSERT((cSquare + 8) == (cSquare | 8));
            if (pos->rgSquare[cSquare|8].bvAttacks[uColor].uWholeThing)
            {
                //
                // Count pawn duos.  See "Pawn Power in Chess" pp 10-16
                //
#ifdef DEBUG
                t = ((uColor * RANK(cSquare)) +
                     (FLIP(uColor) * (9 - RANK(cSquare))));
                if (uColor == WHITE)
                {
                    ASSERT(t == RANK(cSquare));
                }
                else
                {
                    ASSERT(uColor == BLACK);
                    ASSERT(t == (9 - RANK(cSquare)));
                }
#endif
                iDuos[uColor] += ((uColor * RANK(cSquare)) +
                                  (FLIP(uColor) * (9 - RANK(cSquare))));
                ASSERT(iDuos[uColor] <= (7 * 8));
            }
            else if (pos->rgSquare[cSquare|8].bvAttacks[FLIP(uColor)].uWholeThing)
            {
                //
                // Detect backwards pawns.  See "Pawn Power in Chess" pp 25-27
                // 
                if (!IS_PAWN(pos->rgSquare[cSquare].pPiece))
                {
                    p = (BLACK_PAWN | uColor);
                    if (((WHITE == uColor) && (RANK(c) < 4)) ||
                        ((BLACK == uColor) && (RANK(c) > 5)))
                    {
                        if ((IS_ON_BOARD(cSquare - 1) &&
                             pos->rgSquare[cSquare - 1].pPiece == p) ||
                            (IS_ON_BOARD(cSquare + 1) &&
                             pos->rgSquare[cSquare + 1].pPiece == p))
                        {
                            //
                            // The pawn at c is backward.  Determine
                            // whether it is shielded or exposed.
                            //
                            pHash->bbStationaryPawns[uColor] |= COOR_TO_BB(c);
                            ASSERT(CountBits(pHash->bbStationaryPawns[uColor])
                                   <= 8);
                            if (pHash->uCountPerFile[FLIP(uColor)][uPawnFile])
                            {
                                ASSERT(BACKWARD_SHIELDED_BY_LOCATION[c] < 0);
                                EVAL_TERM(uColor,
                                          PAWN,
                                          c,
                                          pHash->iScore[uColor],
                                          BACKWARD_SHIELDED_BY_LOCATION[c],
                                          "shielded backward pawn");
                            }
                            else
                            {
                                ASSERT(BACKWARD_EXPOSED_BY_LOCATION[c] < 0);
                                EVAL_TERM(uColor,
                                          PAWN,
                                          c,
                                          pHash->iScore[uColor],
                                          BACKWARD_EXPOSED_BY_LOCATION[c],
                                          "exposed backward pawn");
                            }
                        }
                    }
                }
                //
                // TODO: Think about backward doubled pawns; read Kauffman's
                // paper "All About Doubled Pawns".
                // 
            }

            //
            // Handle passers / candidate passers
            // 
            _EvaluateCandidatePasser(pos, pHash, c);
        }
    }
    
    //
    // Reward pawn duos, see "Pawn Power in Chess" pp. 10-16
    // 
    ASSERT(iDuos[WHITE] >= 0);
    ASSERT(iDuos[WHITE] <= 56);
    ASSERT(iDuos[BLACK] >= 0);
    ASSERT(iDuos[BLACK] <= 56);
    EVAL_TERM(WHITE,
              PAWN,
              ILLEGAL_COOR,
              pHash->iScore[WHITE],
              iDuos[WHITE] / 2,
              "pawn duos");
    EVAL_TERM(BLACK,
              PAWN,
              ILLEGAL_COOR,
              pHash->iScore[BLACK],
              iDuos[BLACK] / 2,
              "pawn duos");

    //
    // The penalty for doubled pawns is scaled based on two primary
    // factors: the presence of major pieces for the side with doubled
    // pawns and the number of doubled pawns on the board.  The
    // presence of major pieces makes the doubled pawns less severe
    // while the presence of many doubled pawns makes the penalty for
    // each more severe.
    //
    ASSERT((uDoubled[WHITE] >= 0) && (uDoubled[WHITE] <= 8));
    ASSERT((uDoubled[BLACK] >= 0) && (uDoubled[BLACK] <= 8));
    u = pos->uNonPawnCount[BLACK][ROOK] + pos->uNonPawnCount[BLACK][QUEEN] * 2;
    u = MINU(u, 3);
    EVAL_TERM(BLACK,
              PAWN,
              ILLEGAL_COOR,
              pHash->iScore[BLACK],
              DOUBLED_PAWN_PENALTY_BY_COUNT[u][uDoubled[BLACK]],
              "exponential doubled");
    u = pos->uNonPawnCount[WHITE][ROOK] + pos->uNonPawnCount[WHITE][QUEEN] * 2;
    u = MINU(u, 3);
    EVAL_TERM(WHITE,
              PAWN,
              ILLEGAL_COOR,
              pHash->iScore[WHITE],
              DOUBLED_PAWN_PENALTY_BY_COUNT[u][uDoubled[WHITE]],
              "exponential doubled");

    ASSERT(uIsolated[WHITE] >= 0);
    ASSERT(uIsolated[WHITE] <= 8);
    ASSERT(uIsolated[BLACK] >= 0);
    ASSERT(uIsolated[BLACK] <= 8);
    EVAL_TERM(BLACK,
              PAWN,
              ILLEGAL_COOR,
              pHash->iScore[BLACK],
              ISOLATED_PAWN_PENALTY_BY_COUNT[uIsolated[BLACK]],
              "exponential isolated");
    EVAL_TERM(WHITE,
              PAWN,
              ILLEGAL_COOR,
              pHash->iScore[WHITE],
              ISOLATED_PAWN_PENALTY_BY_COUNT[uIsolated[WHITE]],
              "exponential isolated");
    
    //
    // Look for connected, supported and outside passed pawns to give 
    // extra bonuses.
    //
    _EvaluateConnectedSupportedOutsidePassers(pos, pHash);

    //
    // TODO: recognize quartgrips and stonewalls
    //
    return(pHash);
}


ULONG 
CountKingSafetyDefects(IN OUT POSITION *pos,
                       IN ULONG uSide)
/**

Routine description:

    Determine how many defects uSide's king position has _quickly_.
    TODO: add more knowledge as cheaply as possible...

Parameters:

    POSITION *pos,
    ULONG uSide

Return value:

    ULONG

**/
{
    ULONG uCounter = 0;
    ULONG xSide = FLIP(uSide);
    COOR cKing;
    COOR c;
    int i;
    PIECE p;
    ULONG u;
    
    //
    // Don't count king safety defects if the real eval code in
    // _EvalKing would not...
    // 
    if (pos->uNonPawnMaterial[xSide] < DO_KING_SAFETY_THRESHOLD) {
        return 0;
    }

    cKing = pos->cNonPawns[uSide][0];
    uCounter = KING_INITIAL_COUNTER_BY_LOCATION[uSide][cKing] >> 1;
    ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));
    ASSERT(pos->rgSquare[cKing].uIndex == 0);
    ASSERT(IS_ON_BOARD(cKing));
    ASSERT(GET_COLOR(pos->rgSquare[cKing].pPiece) == uSide);
    
    //
    // Make sure cKing - 1, cKing and cKing + 1 are on the board
    //
    cKing += (!IS_ON_BOARD(cKing - 1));
    cKing -= (!IS_ON_BOARD(cKing + 1));
    ASSERT(IS_ON_BOARD(cKing));
    ASSERT(IS_ON_BOARD(cKing + 1));
    ASSERT(IS_ON_BOARD(cKing - 1));

    //
    // Consider all enemy pieces except the king (not including pawns)
    //
    for (u = 1;
         u < pos->uNonPawnCount[xSide][0];
         u++)
    {
        c = pos->cNonPawns[xSide][u];
        ASSERT(IS_ON_BOARD(c));
        i = (int)c - (int)(cKing + 1);
        ASSERT((i >= -128) && (i <= 125));
        p = pos->rgSquare[c].pPiece;
        ASSERT(pos->rgSquare[c].uIndex == u);
        ASSERT(!IS_KING(p));
        ASSERT(GET_COLOR(p) == xSide);
        p = 1 << PIECE_TYPE(p);

        uCounter += ((i == 0) | (i == -2) |
                     ((CHECK_VECTOR_WITH_INDEX(i, xSide) & p) != 0) |
                     ((CHECK_VECTOR_WITH_INDEX(i + 1, xSide) & p) != 0) |
                     ((CHECK_VECTOR_WITH_INDEX(i + 2, xSide) & p) != 0));
    }
    ASSERT(uCounter < 15);
    
    // Save the number of enemy pieces pointing at this king for later use.
    pos->uPiecesPointingAtKing[uSide] = 
      (uCounter - (KING_INITIAL_COUNTER_BY_LOCATION[uSide][cKing] >> 1));
    return uCounter;
}


static void
_QuicklyEstimateKingSafetyTerm(IN POSITION *pos, 
                               IN OUT SCORE *piAlphaMargin,
                               IN OUT SCORE *piBetaMargin)
/**

Routine description:

    Before doing an early lazy eval, look at the position and try to
    quickly see if there is a large king safety positional component
    to the score.

Parameters:

    POSITION *pos

Return value:

    void

**/
{
    //
    // IDEA: make this stuff dynamic like ctx->uPositional
    //
    static const SCORE iScoreByDefectCount[15] = {
        33, 50, 88, 146, 215, 245, 280, 430, 550, 630, 646, 646, 646, 646, 646
    };
    SCORE iPenaltyEst[2];
    SCORE iRet;

    ASSERT(CountKingSafetyDefects(pos, pos->uToMove) < 15);
    iPenaltyEst[WHITE] = 
        iScoreByDefectCount[CountKingSafetyDefects(pos, WHITE)];
    ASSERT(iPenaltyEst[WHITE] > 0);

    ASSERT(CountKingSafetyDefects(pos, BLACK) < 15);
    iPenaltyEst[BLACK] = 
        iScoreByDefectCount[CountKingSafetyDefects(pos, BLACK)];
    ASSERT(iPenaltyEst[BLACK] > 0);

    //
    // For the beta margin, assume our king safety penalty will stand
    // and our opponents will be less severe than we guess... net bonus
    // to them / our bonus not as high.
    //
    iRet = iPenaltyEst[pos->uToMove] - iPenaltyEst[FLIP(pos->uToMove)] / 2;
    iRet = MAX0(iRet);
    ASSERT(iRet >= 0);
    *piBetaMargin += iRet;

    //
    // For the alpha margin, assume our king safety penalty will not
    // be too bad and our opponents will be as bad as we guess... net
    // bonus to us / their offsetting bonus not as high.
    //
    iRet = iPenaltyEst[FLIP(pos->uToMove)] - iPenaltyEst[pos->uToMove] / 2;
    iRet = MAX0(iRet);
    ASSERT(iRet >= 0);
    *piAlphaMargin += iRet;
}


static void
_QuicklyEstimatePasserBonuses(POSITION *pos,
                              PAWN_HASH_ENTRY *pHash,
                              SCORE *piAlphaMargin,
                              SCORE *piBetaMargin) 
{
    SCORE iBonusEst[2] = {0, 0};
    SCORE iRet;
    ULONG u;

    //
    // Guess about the passer bonuses
    //
    if (pHash->bbPasserLocations[WHITE])
    {
        u = pos->uNonPawnMaterial[BLACK] - VALUE_KING;
        u /= VALUE_PAWN;
        u = MINU(31, u);
        ASSERT(PASSER_BONUS_AS_MATERIAL_COMES_OFF[u] >= 0);
        iBonusEst[WHITE] = 
            (CountBits(pHash->bbPasserLocations[WHITE]) *
             PASSER_BONUS_AS_MATERIAL_COMES_OFF[u]);
    }
    if (pHash->bbPasserLocations[BLACK])
    {
        u = pos->uNonPawnMaterial[WHITE] - VALUE_KING;
        u /= VALUE_PAWN;
        u = MINU(31, u);
        ASSERT(PASSER_BONUS_AS_MATERIAL_COMES_OFF[u] >= 0);
        iBonusEst[BLACK] = 
            (CountBits(pHash->bbPasserLocations[BLACK]) *
             PASSER_BONUS_AS_MATERIAL_COMES_OFF[u]);
    }

    //
    // For the beta margin, assume our passer bonus gets reduced and
    // our opponent's is enhanced... net bonus to them / our bonus
    // not as high.
    //
    iRet = iBonusEst[pos->uToMove] / 2 - iBonusEst[FLIP(pos->uToMove)];
    iRet = MAX0(iRet);
    *piBetaMargin += iRet;

    //
    // For the alpha bonus, assume our passer bonus gets doubled and
    // our opponent's is reduced.  Net bonus to us / their bonus not 
    // as high.
    //
    iRet = iBonusEst[pos->uToMove] - iBonusEst[FLIP(pos->uToMove)] / 2;
    iRet = MAX0(iRet);
    *piAlphaMargin += iRet;
}


static void 
_InvalidEvaluator(UNUSED POSITION *pos, 
                  UNUSED COOR c, 
                  UNUSED PAWN_HASH_ENTRY *pHash)
/**

Routine description:

    This code should never be called

Parameters:

    POSITION *pos,
    COOR c,
    PAWN_HASH_ENTRY *pHash,

Return value:

    void

**/
{
    UtilPanic(SHOULD_NOT_GET_HERE,
              NULL, NULL, NULL, NULL,
              __FILE__, __LINE__);
}

//
// ======================================================================
//

static FLAG FASTCALL  
_InvalidMobilityHelper(UNUSED POSITION *pos,
                       UNUSED COOR c,
                       UNUSED ULONG *puMobility,
                       UNUSED ULONG *puBit)
/**

Routine description:

    This code should never be called

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
    UtilPanic(SHOULD_NOT_GET_HERE,
              NULL, NULL, NULL, NULL,
              __FILE__, __LINE__);
    return(TRUE);
}

// ----------------------------------------------------------------------

static FLAG FASTCALL 
_BMinorToEnemyLess(IN POSITION *pos,
                   IN COOR c,
                   IN OUT ULONG *puMobility,
                   UNUSED ULONG *puBit)
/**

Routine description:

    Consider a black minor (knight | bishop) taking an enemy pawn

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL : always TRUE, stop scanning in this direction

**/
{
#ifdef DEBUG
    PIECE pMinor = pos->rgSquare[pos->cPiece].pPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(!IS_EMPTY(p));
    ASSERT(PIECE_VALUE(p) < VALUE_BISHOP);
    ASSERT(!IS_EMPTY(pMinor));
    ASSERT(IS_PAWN(p));
    ASSERT(IS_BISHOP(pMinor) || IS_KNIGHT(pMinor));
    ASSERT(OPPOSITE_COLORS(p, pMinor));
    ASSERT(GET_COLOR(pMinor) == BLACK);
#endif
    *puMobility += (!UNSAFE_FOR_MINOR(pos->rgSquare[c|8].bvAttacks[WHITE]));
    ASSERT(*puMobility <= 8);
    return(TRUE);                             // stop scanning this dir
}

static FLAG FASTCALL 
_WMinorToEnemyLess(IN POSITION *pos,
                   IN COOR c,
                   IN OUT ULONG *puMobility,
                   UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE pMinor = pos->rgSquare[pos->cPiece].pPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(!IS_EMPTY(p));
    ASSERT(IS_PAWN(p));
    ASSERT(PIECE_VALUE(p) < VALUE_BISHOP);
    ASSERT(!IS_EMPTY(pMinor));
    ASSERT(IS_BISHOP(pMinor) || IS_KNIGHT(pMinor));
    ASSERT(OPPOSITE_COLORS(p, pMinor));
    ASSERT(GET_COLOR(pMinor) == WHITE);
#endif
    *puMobility += (!UNSAFE_FOR_MINOR(pos->rgSquare[c|8].bvAttacks[BLACK]));
    ASSERT(*puMobility <= 8);
    return(TRUE);                             // stop scanning this dir
}

static FLAG FASTCALL 
_WRookToEnemyLess(POSITION *pos,
                  COOR c,
                  ULONG *puMobility,
                  UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ROOK(pRook));
    ASSERT(!IS_EMPTY(p));
    ASSERT(OPPOSITE_COLORS(p, pRook));
    ASSERT(PIECE_VALUE(p) < VALUE_ROOK);
    ASSERT(WHITE == GET_COLOR(pRook));
#endif
    *puMobility += (!UNSAFE_FOR_ROOK(pos->rgSquare[c|8].bvAttacks[BLACK]));
    ASSERT(*puMobility <= 7);
    return(TRUE);                             // stop scanning this dir
}

static FLAG FASTCALL 
_BRookToEnemyLess(IN POSITION *pos,
                  IN COOR c,
                  IN OUT ULONG *puMobility,
                  UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_ROOK(pRook));
    ASSERT(OPPOSITE_COLORS(p, pRook));
    ASSERT(PIECE_VALUE(p) < VALUE_ROOK);
    ASSERT(BLACK == GET_COLOR(pRook));
#endif
    *puMobility += (!UNSAFE_FOR_ROOK(pos->rgSquare[c|8].bvAttacks[WHITE]));
    ASSERT(*puMobility <= 7);
    return(TRUE);                             // stop scanning this dir
}


static FLAG FASTCALL 
_BQueenToEnemyLess(IN POSITION *pos,
                   IN COOR c,
                   IN OUT ULONG *puMobility,
                   UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(PIECE_VALUE(p) < VALUE_QUEEN);
    ASSERT(GET_COLOR(pQueen) == BLACK);
    ASSERT(OPPOSITE_COLORS(pQueen, p));
#endif
    *puMobility += (!UNSAFE_FOR_QUEEN(pos->rgSquare[c|8].bvAttacks[WHITE]));
    ASSERT(*puMobility <= 27);
    return(TRUE);                             // stop scanning this dir
}


static FLAG FASTCALL 
_WQueenToEnemyLess(IN POSITION *pos,
                   IN COOR c,
                   IN OUT ULONG *puMobility,
                   UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(PIECE_VALUE(p) < VALUE_QUEEN);
    ASSERT(GET_COLOR(pQueen) == WHITE);
    ASSERT(OPPOSITE_COLORS(pQueen, p));
#endif
    *puMobility += (!UNSAFE_FOR_QUEEN(pos->rgSquare[c|8].bvAttacks[BLACK]));
    ASSERT(*puMobility <= 27);
    return(TRUE);                             // stop scanning this dir
}

// ----------------------------------------------------------------------

static FLAG FASTCALL 
_EMinorToEnemySame(IN POSITION *pos,
                   IN COOR c,
                   IN OUT ULONG *puMobility,
                   UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE pMinor = pos->rgSquare[pos->cPiece].pPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(!IS_EMPTY(p));
    ASSERT(!IS_EMPTY(pMinor));
    ASSERT(OPPOSITE_COLORS(p, pMinor));
    
    if (IS_BISHOP(pMinor))
    {
        ASSERT(PIECE_VALUE(p) == VALUE_BISHOP);
    }
    else
    {
        ASSERT(IS_KNIGHT(pMinor));
        ASSERT(PIECE_VALUE(p) >= VALUE_KNIGHT);
    }
#endif
    *puMobility += 1;
    ASSERT(*puMobility <= 8);
    return(TRUE);                             // stop scanning this dir
}


static FLAG FASTCALL 
_ERookToEnemySame(IN POSITION *pos,
                  IN COOR c,
                  IN OUT ULONG *puMobility,
                  UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_ROOK(pRook));
    ASSERT(!IS_EMPTY(p));
    ASSERT(IS_ROOK(p));
    ASSERT(OPPOSITE_COLORS(p, pRook));
#endif
    *puMobility += 1;
    ASSERT(*puMobility <= 7);
    return(TRUE);                             // stop scanning this dir
}


static FLAG FASTCALL 
_EQueenToEnemyGreaterEqual(IN POSITION *pos,
                           IN COOR c,
                           IN OUT ULONG *puMobility,
                           UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(!IS_EMPTY(p));
    ASSERT(PIECE_VALUE(p) >= VALUE_QUEEN);
    ASSERT(OPPOSITE_COLORS(pQueen, p));
#endif
    *puMobility += 1;
    ASSERT(*puMobility <= 27);
    return(TRUE);                             // stop scanning this dir
}

// ----------------------------------------------------------------------

static FLAG FASTCALL 
_EMinorToEnemyGreater(IN POSITION *pos,
                      IN COOR c,
                      IN OUT ULONG *puMobility,
                      IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE pMinor = pos->rgSquare[pos->cPiece].pPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(!IS_EMPTY(p));
    ASSERT(PIECE_VALUE(p) > VALUE_BISHOP);
    ASSERT(IS_BISHOP(pMinor) || IS_KNIGHT(pMinor));
    ASSERT(OPPOSITE_COLORS(p, pMinor));
#endif
    *puMobility += 1;
    ASSERT(*puMobility <= 8);
    *puBit = MINOR_XRAY_BIT;
    return(FALSE);                            // keep scanning this dir
}

static FLAG FASTCALL 
_ERookToEnemyGreater(IN POSITION *pos,
                     IN COOR c,
                     IN OUT ULONG *puMobility,
                     IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[pos->cPiece].pPiece;

    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_ROOK(pRook));
    ASSERT(!IS_EMPTY(p));
    ASSERT(OPPOSITE_COLORS(p, pRook));
    ASSERT(PIECE_VALUE(p) > VALUE_ROOK);
#endif
    *puMobility += 1;
    ASSERT(*puMobility <= 7);
    *puBit = ROOK_XRAY_BIT;
    return(FALSE);                            // keep scanning this dir
}

// ----------------------------------------------------------------------

static FLAG FASTCALL 
_AnythingToFriendNoXray(IN POSITION *pos,
                        IN COOR c,
                        UNUSED ULONG *puMobility,
                        UNUSED ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    COOR cMover = pos->cPiece;
    PIECE pMover = pos->rgSquare[cMover].pPiece;
    PIECE pFriend = pos->rgSquare[c].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(cMover));
    ASSERT(!IS_EMPTY(pFriend));
    ASSERT(!IS_EMPTY(pMover));
    ASSERT(GET_COLOR(pMover) == GET_COLOR(pFriend));
    if (IS_BISHOP(pMover))
    {
        ASSERT(!IS_BISHOP(pFriend));
        ASSERT(!IS_QUEEN(pFriend));
    }
    else if (IS_ROOK(pMover))
    {
        ASSERT(!IS_QUEEN(pFriend));
        ASSERT(!IS_ROOK(pFriend));
    }
    else if (IS_QUEEN(pMover))
    {
        ASSERT(!IS_BISHOP(pFriend));
        ASSERT(!IS_ROOK(pFriend));
        ASSERT(!IS_QUEEN(pFriend));
    }
#endif
    return(TRUE);                             // stop scanning this dir
}

// ----------------------------------------------------------------------

static FLAG FASTCALL 
_EMinorToFriendXray(IN POSITION *pos,
                    IN COOR c,
                    IN OUT ULONG *puMobility,
                    IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE pMinor = pos->rgSquare[pos->cPiece].pPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    
    ASSERT(!IS_EMPTY(p));
    ASSERT(!IS_EMPTY(pMinor));
    ASSERT(IS_BISHOP(pMinor));
    ASSERT(GET_COLOR(p) == GET_COLOR(pMinor));
#endif
    *puBit = MINOR_XRAY_BIT;
    return(FALSE);
}

static FLAG FASTCALL 
_BRookToFriendRook(IN POSITION *pos,
                   IN COOR c,
                   IN OUT ULONG *puMobility,
                   IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
    COOR cRook = pos->cPiece;
    FLAG fHoriz = ((c & 0xF0) == (cRook & 0xF0));
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[cRook].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(cRook));
    ASSERT(IS_ROOK(pRook));
    ASSERT(!IS_EMPTY(p));
    ASSERT(IS_ROOK(p));
    ASSERT(GET_COLOR(p) == GET_COLOR(pRook));
    ASSERT(GET_COLOR(p) == BLACK);
    ASSERT((fHoriz && (RANK(c) == RANK(pos->cPiece))) ||
           (!fHoriz && (FILE(c) == FILE(pos->cPiece))));
#endif
    EVAL_TERM(BLACK,
              ROOK,
              c,
              pos->iScore[BLACK],
              (ROOK_CONNECTED_HORIZ * fHoriz + 
               ROOK_CONNECTED_VERT * FLIP(fHoriz)),
              "rook connected");
    *puBit = ROOK_XRAY_BIT;
    return(FALSE);
}


static FLAG FASTCALL 
_WRookToFriendRook(IN POSITION *pos,
                   IN COOR c,
                   IN OUT ULONG *puMobility,
                   IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
    COOR cRook = pos->cPiece;
    FLAG fHoriz = ((c & 0xF0) == (cRook & 0xF0));
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[cRook].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(cRook));
    ASSERT(IS_ROOK(pRook));
    ASSERT(!IS_EMPTY(p));
    ASSERT(IS_ROOK(p));
    ASSERT(GET_COLOR(p) == GET_COLOR(pRook));
    ASSERT(GET_COLOR(p) == WHITE);
    ASSERT((fHoriz && (RANK(c) == RANK(pos->cPiece))) ||
           (!fHoriz && (FILE(c) == FILE(pos->cPiece))));
#endif
    EVAL_TERM(WHITE,
              ROOK,
              c,
              pos->iScore[WHITE],
              (ROOK_CONNECTED_HORIZ * fHoriz + 
               ROOK_CONNECTED_VERT * FLIP(fHoriz)),
              "rook connected");
    *puBit = ROOK_XRAY_BIT;
    return(FALSE);
}


static FLAG FASTCALL 
_ERookToFriendQueen(IN POSITION *pos,
                    IN COOR c,
                    IN OUT ULONG *puMobility,
                    IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_ROOK(pRook));
    ASSERT(!IS_EMPTY(p));
    ASSERT(IS_QUEEN(p));
    ASSERT(GET_COLOR(p) == GET_COLOR(pRook));
#endif
    *puBit = ROOK_XRAY_BIT;
    return(FALSE);
}

static FLAG FASTCALL 
_EQueenToFriendBishop(IN POSITION *pos,
                      IN COOR c,
                      IN OUT ULONG *puMobility,
                      IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
    COOR cQueen = pos->cPiece;
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[cQueen].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(cQueen));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(IS_BISHOP(p));
    ASSERT(GET_COLOR(p) == GET_COLOR(pQueen));
#endif
    *puBit = QUEEN_XRAY_BIT;
    return((((c & 0xF0) == (cQueen & 0xF0)) ||
            ((c & 0x0F) == (cQueen & 0x0F))));
}


static FLAG FASTCALL 
_EQueenToFriendRook(IN POSITION *pos,
                    IN COOR c,
                    IN OUT ULONG *puMobility,
                    IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
    COOR cQueen = pos->cPiece;
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[cQueen].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(cQueen));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(IS_ROOK(p));
    ASSERT(GET_COLOR(p) == GET_COLOR(pQueen));
#endif
    *puBit = QUEEN_XRAY_BIT;
    return(FLIP((((c & 0xF0) == (cQueen & 0xF0)) ||
                 ((c & 0x0F) == (cQueen & 0x0F)))));
}


static FLAG FASTCALL 
_EQueenToFriendQueen(IN POSITION *pos,
                     IN COOR c,
                     IN OUT ULONG *puMobility,
                     IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    COOR cQueen = pos->cPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[cQueen].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(cQueen));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(IS_QUEEN(p));
    ASSERT(GET_COLOR(p) == GET_COLOR(pQueen));
#endif
    *puBit = QUEEN_XRAY_BIT;
    return(FALSE);
}


// ----------------------------------------------------------------------

static FLAG FASTCALL 
_BMinorToEmpty(IN POSITION *pos,
               IN COOR c,
               IN OUT ULONG *puMobility,
               IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE pMinor = pos->rgSquare[pos->cPiece].pPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    
    ASSERT(IS_EMPTY(p));
    ASSERT(!IS_EMPTY(pMinor));
    ASSERT(IS_BISHOP(pMinor) || IS_KNIGHT(pMinor));
    ASSERT(GET_COLOR(pMinor) == BLACK);
#endif
    *puMobility += (!UNSAFE_FOR_MINOR(pos->rgSquare[c|8].bvAttacks[WHITE]));
    ASSERT(*puMobility <= 8);
    return(FALSE);
}

static FLAG FASTCALL 
_WMinorToEmpty(IN POSITION *pos,
               IN COOR c,
               IN OUT ULONG *puMobility,
               IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE pMinor = pos->rgSquare[pos->cPiece].pPiece;
    PIECE p = pos->rgSquare[c].pPiece;
    
    ASSERT(IS_EMPTY(p));
    ASSERT(!IS_EMPTY(pMinor));
    ASSERT(IS_BISHOP(pMinor) || IS_KNIGHT(pMinor));
    ASSERT(GET_COLOR(pMinor) == WHITE);
#endif
    *puMobility += (!UNSAFE_FOR_MINOR(pos->rgSquare[c|8].bvAttacks[BLACK]));
    ASSERT(*puMobility <= 8);
    return(FALSE);
}

static FLAG FASTCALL 
_WRookToEmpty(IN POSITION *pos,
              IN COOR c,
              IN OUT ULONG *puMobility,
              IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_ROOK(pRook));
    ASSERT(IS_EMPTY(p));
    ASSERT(GET_COLOR(pRook) == WHITE);
#endif
    *puMobility += (!UNSAFE_FOR_ROOK(pos->rgSquare[c|8].bvAttacks[BLACK]));
    ASSERT(*puMobility <= 7);
    return(FALSE);
}

static FLAG FASTCALL 
_BRookToEmpty(IN POSITION *pos,
              IN COOR c,
              IN OUT ULONG *puMobility,
              IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pRook = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_ROOK(pRook));
    ASSERT(IS_EMPTY(p));
    ASSERT(GET_COLOR(pRook) == BLACK);
#endif
    *puMobility += (!UNSAFE_FOR_ROOK(pos->rgSquare[c|8].bvAttacks[WHITE]));
    ASSERT(*puMobility <= 7);
    return(FALSE);
}

static FLAG FASTCALL 
_BQueenToEmpty(IN POSITION *pos,
               IN COOR c,
               IN OUT ULONG *puMobility,
               IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(IS_EMPTY(p));
    ASSERT(GET_COLOR(pQueen) == BLACK);
#endif
    *puMobility += (!UNSAFE_FOR_QUEEN(pos->rgSquare[c|8].bvAttacks[WHITE]));
    ASSERT(*puMobility <= 27);
    return(FALSE);
}

static FLAG FASTCALL 
_WQueenToEmpty(IN POSITION *pos,
               IN COOR c,
               IN OUT ULONG *puMobility,
               IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE p = pos->rgSquare[c].pPiece;
    PIECE pQueen = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_QUEEN(pQueen));
    ASSERT(IS_EMPTY(p));
    ASSERT(GET_COLOR(pQueen) == WHITE);
#endif
    *puMobility += (!UNSAFE_FOR_QUEEN(pos->rgSquare[c|8].bvAttacks[BLACK]));
    ASSERT(*puMobility <= 27);
    return(FALSE);
}



static FLAG FASTCALL 
_EBishopToFriendPawn(IN POSITION *pos,
                     IN COOR c,
                     IN OUT ULONG *puMobility,
                     IN OUT ULONG *puBit)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    ULONG *puMobility,
    ULONG *puBit

Return value:

    static FLAG FASTCALL

**/
{
#ifdef DEBUG
    PIECE pPawn = pos->rgSquare[c].pPiece;
    PIECE pBishop = pos->rgSquare[pos->cPiece].pPiece;
    
    ASSERT(IS_ON_BOARD(c));
    ASSERT(IS_ON_BOARD(pos->cPiece));
    ASSERT(IS_BISHOP(pBishop));
    ASSERT(IS_PAWN(pPawn));
    ASSERT(GET_COLOR(pBishop) == GET_COLOR(pPawn));
#endif
    *puMobility += ((pos->bb & COOR_TO_BB(c)) != 0);
    return(TRUE);
}




//
// ======================================================================
//


static void
_EvalBishop(IN OUT POSITION *pos,
            IN COOR c, 
            IN PAWN_HASH_ENTRY *pHash)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    PAWN_HASH_ENTRY *pHash,

Return value:

    FLAG

**/
{
    static const BITBOARD bbColorSq[2] = { 0x55aa55aa55aa55aaULL,
                                           0xaa55aa55aa55aa55ULL };
    static const PMOBILITY_HELPER BMobJumpTable[2][14] = 
    {
        {// (black)
            _BMinorToEmpty,                    // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _EBishopToFriendPawn,              // BLACK_PAWN    (2)
            _BMinorToEnemyLess,                // WHITE_PAWN    (3)
            _AnythingToFriendNoXray,           // BLACK_KNIGHT  (4)
            _EMinorToEnemySame,                // WHITE_KNIGHT  (5)
            _EMinorToFriendXray,               // BLACK_BISHOP  (6)
            _EMinorToEnemySame,                // WHITE_BISHOP  (7)
            _AnythingToFriendNoXray,           // BLACK_ROOK    (8)
            _EMinorToEnemyGreater,             // WHITE_ROOK    (9)
            _EMinorToFriendXray,               // BLACK_QUEEN   (10)
            _EMinorToEnemyGreater,             // WHITE_QUEEN   (11)
            _AnythingToFriendNoXray,           // BLACK_KING    (12)
            _EMinorToEnemyGreater,             // WHITE_KING    (13)
        },
        {// (white)
            _WMinorToEmpty,                    // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _WMinorToEnemyLess,                // BLACK_PAWN    (2)
            _EBishopToFriendPawn,              // WHITE_PAWN    (3)
            _EMinorToEnemySame,                // BLACK_KNIGHT  (4)
            _AnythingToFriendNoXray,           // WHITE_KNIGHT  (5)
            _EMinorToEnemySame,                // BLACK_BISHOP  (6)
            _EMinorToFriendXray,               // WHITE_BISHOP  (7)
            _EMinorToEnemyGreater,             // BLACK_ROOK    (8)
            _AnythingToFriendNoXray,           // WHITE_ROOK    (9)
            _EMinorToEnemyGreater,             // BLACK_QUEEN   (10)
            _EMinorToFriendXray,               // WHITE_QUEEN   (11)
            _EMinorToEnemyGreater,             // BLACK_KING    (12)
            _AnythingToFriendNoXray,           // WHITE_KING    (13)
        }
    };
    static const COOR cBishopAtHome[2][2] = 
    {
        { C8, F8 }, // BLACK
        { C1, F1 }  // WHITE
    };
    ULONG uColor;
    BITBOARD bb;
    BITBOARD bbMask;
    BITBOARD bbPc;
    COOR cSquare;
    SCORE i;
    ULONG u;
    ULONG uTotalMobility;
    ULONG uMaxMobility;
    ULONG uCurrentMobility;
    ULONG uBit;
    PIECE p;
    PMOBILITY_HELPER pFun;
    
    ASSERT(IS_ON_BOARD(c));
    p = pos->rgSquare[c].pPiece;
    ASSERT(p && IS_BISHOP(p));
    uColor = GET_COLOR(p);

#ifdef DEBUG
    pos->cPiece = c;
#endif

    //
    // Unmoved piece
    //
    pos->uMinorsAtHome[uColor] += (c == cBishopAtHome[uColor][0]);
    pos->uMinorsAtHome[uColor] += (c == cBishopAtHome[uColor][1]);
    ASSERT(pos->uMinorsAtHome[uColor] <= 4);

    //
    // Good and bad bishops
    //
    i = 16;
    bbMask = bbColorSq[IS_SQUARE_WHITE(c)];
    bb = pHash->bbStationaryPawns[uColor] & bbMask;
    ASSERT(CountBits(bb) <= 16);
    while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&bb)))
    {
        //
        // Consider stationary (rammed or backward) pawns of the same
        // color as the bishop.
        //
        ASSERT(pos->rgSquare[cSquare].pPiece);
        ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[cSquare].pPiece) == uColor);
        i += STATIONARY_PAWN_ON_BISHOP_COLOR[cSquare];
    }
    EVAL_TERM(uColor,
              BISHOP,
              c,
              pos->iScore[uColor],
              i,
              "good/bad stationary pawns ++");

    i = 0;
    bbPc = ~(pHash->bbStationaryPawns[WHITE] | 
             pHash->bbStationaryPawns[BLACK]);
    bbPc &= bbMask;
    bb = pHash->bbPawnLocations[uColor] & bbPc;
    while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&bb)))
    {
        ASSERT(pos->rgSquare[cSquare].pPiece);
        ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[cSquare].pPiece) == uColor);
        ASSERT(pHash->bbPawnLocations[uColor] & COOR_TO_BB(cSquare));
        i += TRANSIENT_PAWN_ON_BISHOP_COLOR[cSquare];
    }

    bb = pHash->bbPawnLocations[FLIP(uColor)] & bbPc;
    while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&bb)))
    {
        //
        // N.B. Only count enemy pawns that are supported by another
        // pawn.
        //
        ASSERT(pos->rgSquare[cSquare].pPiece);
        ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[cSquare].pPiece) == FLIP(uColor));
        ASSERT(pHash->bbPawnLocations[FLIP(uColor)] & COOR_TO_BB(cSquare));
        i += 
         (pos->rgSquare[cSquare|8].bvAttacks[FLIP(uColor)].small.uPawn != 0) *
            TRANSIENT_PAWN_ON_BISHOP_COLOR[cSquare] / 2;
    }
    EVAL_TERM(uColor,
              BISHOP,
              c,
              pos->iScore[uColor],
              i,
              "good/bad transient pawns");

    //
    // Bonus to bishops in very open positions.
    //
    ASSERT(pos->uClosedScaler <= 32);
    EVAL_TERM(uColor,
              BISHOP,
              c,
              pos->iScore[uColor],
              BISHOP_IN_CLOSED_POSITION[pos->uClosedScaler],
              "in closed/open position");
    
    //
    // Bishop mobility (and update attack table bits)
    //
    u = uMaxMobility = uTotalMobility = 0;
    pos->bb = bbPc;
    ASSERT(g_iBDeltas[u] != 0);
    do
    {
        uCurrentMobility = 0;
        uBit = MINOR_BIT;
        cSquare = c + g_iBDeltas[u];
        
        while(IS_ON_BOARD(cSquare))
        {
            //
            // Always toggle attack table bits.
            //
            ASSERT((cSquare|8) == (cSquare + 8));
            pos->rgSquare[cSquare|8].bvAttacks[uColor].uWholeThing |= uBit;

            //
            // What did we hit?
            //
            p = pos->rgSquare[cSquare].pPiece;
            pFun = BMobJumpTable[uColor][p];
            ASSERT(pFun);
            if (TRUE == (*pFun)(pos,
                                cSquare,
                                &uCurrentMobility,
                                &uBit))
            {
                break;
            }
            cSquare += g_iBDeltas[u];
        }
        uTotalMobility += uCurrentMobility;
        ASSERT((uMaxMobility & 0x80000000) == 0);
        ASSERT((uCurrentMobility & 0x80000000) == 0);
        uMaxMobility = MAXU(uMaxMobility, uCurrentMobility);
        ASSERT(uMaxMobility <= 7);
        u++;
    }
    while(g_iBDeltas[u] != 0);
    ASSERT(uTotalMobility <= 13);
    EVAL_TERM(uColor,
              BISHOP,
              c,
              pos->iScore[uColor],
              BISHOP_MOBILITY_BY_SQUARES[uTotalMobility],
              "bishop mobility");
    EVAL_TERM(uColor,
              BISHOP,
              c,
              pos->iScore[uColor],
              BISHOP_MAX_MOBILITY_IN_A_ROW_BONUS[uMaxMobility],
              "consecutive bishop mobility");

    //
    // Look for bishops with no mobility who are under attack.  These
    // pieces are trapped!
    //
    if (uTotalMobility == 0)
    {
        pos->cTrapped[uColor] = c;
    }
    uTotalMobility /= 2;
    ASSERT((pos->uMinMobility[uColor] & 0x80000000) == 0);
    ASSERT((uTotalMobility & 0x80000000) == 0);
    pos->uMinMobility[uColor] = MINU(pos->uMinMobility[uColor], 
                                     uTotalMobility);
    
    //
    // Look for "active bad bishops".  See if square c is safe from
    // enemy pawns.
    //
    bb = pHash->bbPawnLocations[FLIP(uColor)] &
        (~pHash->bbStationaryPawns[FLIP(uColor)]);
    if (TRUE == _IsSquareSafeFromEnemyPawn(pos, c, bb))
    {
        //
        // Give a bonus based on distance from enemy king
        //
        if (pos->rgSquare[c|8].bvAttacks[uColor].small.uPawn)
        {
#ifdef DEBUG
            p = BLACK_PAWN | uColor;
            ASSERT(((IS_ON_BOARD(c + 17 * g_iBehind[uColor]) &&
                     (pos->rgSquare[c + 17 * g_iBehind[uColor]].pPiece==p)) ||
                    ((IS_ON_BOARD(c + 15 * g_iBehind[uColor])) &&
                     (pos->rgSquare[c + 15 * g_iBehind[uColor]].pPiece==p))));
#endif
            u = DISTANCE(c, pos->cNonPawns[FLIP(uColor)][0]);
            i = BISHOP_UNASSAILABLE_BY_DIST_FROM_EKING[u];
            EVAL_TERM(uColor,
                      BISHOP,
                      c,
                      pos->iReducedMaterialDownScaler[uColor],
                      i,
                      "[scaled] unassailable");
        }
    }
}

static void
_EvalKnight(IN OUT POSITION *pos, 
            IN COOR c, 
            IN PAWN_HASH_ENTRY *pHash)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    PAWN_HASH_ENTRY *pHash,

Return value:

    FLAG

**/
{
    static const int iPawnStart[2] = { -17, +15 };
    static const PMOBILITY_HELPER NMobJumpTable[2][14] = 
    {
        {// (black)
            _BMinorToEmpty,                    // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _AnythingToFriendNoXray,           // BLACK_PAWN    (2)
            _BMinorToEnemyLess,                // WHITE_PAWN    (3)
            _AnythingToFriendNoXray,           // BLACK_KNIGHT  (4)
            _EMinorToEnemySame,                // WHITE_KNIGHT  (5)
            _AnythingToFriendNoXray,           // BLACK_BISHOP  (6)
            _EMinorToEnemySame,                // WHITE_BISHOP  (7)
            _AnythingToFriendNoXray,           // BLACK_ROOK    (8)
            _EMinorToEnemySame,                // WHITE_ROOK    (9)
            _AnythingToFriendNoXray,           // BLACK_QUEEN   (10)
            _EMinorToEnemySame,                // WHITE_QUEEN   (11)
            _AnythingToFriendNoXray,           // BLACK_KING    (12)
            _EMinorToEnemySame,                // WHITE_KING    (13)
        },
        {// (white)
            _WMinorToEmpty,                    // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _WMinorToEnemyLess,                // BLACK_PAWN    (2)
            _AnythingToFriendNoXray,           // WHITE_PAWN    (3)
            _EMinorToEnemySame,                // BLACK_KNIGHT  (4)
            _AnythingToFriendNoXray,           // WHITE_KNIGHT  (5)
            _EMinorToEnemySame,                // BLACK_BISHOP  (6)
            _AnythingToFriendNoXray,           // WHITE_BISHOP  (7)
            _EMinorToEnemySame,                // BLACK_ROOK    (8)
            _AnythingToFriendNoXray,           // WHITE_ROOK    (9)
            _EMinorToEnemySame,                // BLACK_QUEEN   (10)
            _AnythingToFriendNoXray,           // WHITE_QUEEN   (11)
            _EMinorToEnemySame,                // BLACK_KING    (12)
            _AnythingToFriendNoXray,           // WHITE_KING    (13)
        }
    };
    static const COOR cKnightAtHome[2][2] = 
    {
        { B8, G8 }, // BLACK
        { B1, G1 }  // WHITE
    };
    PIECE p;
    COOR cSquare;
    BITBOARD bb;
    ULONG uColor;
    ULONG uPawnsSupporting;
    ULONG u;
    ULONG uPawnFile;
    ULONG uMobilitySquares;
    SCORE i;
    ULONG uDist;
    PMOBILITY_HELPER pFun;

    p = pos->rgSquare[c].pPiece;
    ASSERT(p && IS_KNIGHT(p));
    uColor = GET_COLOR(p);
    ASSERT(IS_VALID_COLOR(uColor));
#ifdef DEBUG
    pos->cPiece = c;
#endif

    //
    // Unmoved piece
    //
    pos->uMinorsAtHome[uColor] += (c == cKnightAtHome[uColor][0]);
    pos->uMinorsAtHome[uColor] += (c == cKnightAtHome[uColor][1]);
    ASSERT(pos->uMinorsAtHome[uColor] <= 4);

    //
    // TODO: Don't block unmoved E2/D2 pawns
    // 
    
    //
    // Centrality bonus
    //
    EVAL_TERM(uColor,
              KNIGHT,
              c,
              pos->iScore[uColor],
              KNIGHT_CENTRALITY_BONUS[c],
              "board centrality");

    //
    // Give a bonus to knights on a closed / busy board
    //
    ASSERT(pos->uClosedScaler <= 32);
    EVAL_TERM(uColor,
              KNIGHT,
              c,
              pos->iScore[uColor],
              KNIGHT_IN_CLOSED_POSITION[pos->uClosedScaler],
              "in closed/open position");

    //
    // See if square c is safe from enemy pawns; if so give bonus for
    // outposted knight which increases the closer it is to the enemy
    // king and the more pawns it has supporting it.
    //
    uDist = DISTANCE(c, pos->cNonPawns[FLIP(uColor)][0]);
    ASSERT((uDist > 0) && (uDist <= 8));
    bb = pHash->bbPawnLocations[FLIP(uColor)] &
        (~pHash->bbStationaryPawns[FLIP(uColor)]);
    if (TRUE == _IsSquareSafeFromEnemyPawn(pos, c, bb))
    {
        //
        // Count the number of supporting pawns the knight has
        //
        uPawnsSupporting = 0;
        p = BLACK_PAWN | uColor;
        cSquare = c + iPawnStart[uColor];
        uPawnsSupporting = (IS_ON_BOARD(cSquare) && 
                            (pos->rgSquare[cSquare].pPiece == p));
        cSquare += 2;
        uPawnsSupporting += (IS_ON_BOARD(cSquare) &&
                             (pos->rgSquare[cSquare].pPiece == p));
        ASSERT(uPawnsSupporting <= 2);
        
        //
        // Give a bonus based on distance from enemy king
        //
        i = KNIGHT_UNASSAILABLE_BY_DIST_FROM_EKING[uDist] +
            KNIGHT_WITH_N_PAWNS_SUPPORTING[uPawnsSupporting];
        EVAL_TERM(uColor,
                  KNIGHT,
                  c,
                  pos->iReducedMaterialDownScaler[uColor],
                  i,
                  "[scaled] unassailable/support");
        
        //
        // Now, if there's zero or one supporter see if the enemy has a
        // bishop the color of the knight it could capture with.
        //
        if (uPawnsSupporting != 2)
        {
            if (IS_SQUARE_WHITE(c))
            {
                if (pos->uWhiteSqBishopCount[FLIP(uColor)])
                {
                    EVAL_TERM(uColor,
                              KNIGHT,
                              c,
                              pos->iScore[uColor],
                              -(i / 2),
                              "not safe from enemy B");
                }
            }
            else
            {
                if (pos->uNonPawnCount[FLIP(uColor)][BISHOP] - 
                    pos->uWhiteSqBishopCount[FLIP(uColor)])
                {
                    EVAL_TERM(uColor,
                              KNIGHT,
                              c,
                              pos->iScore[uColor],
                              -(i / 2),
                              "not safe from enemy B");
                }
            }
        }
    } 
    else // not outposted
    {
        //
        // Still good to be close to the enemy king.
        //
        i = KNIGHT_KING_TROPISM_BONUS[uDist];
        EVAL_TERM(uColor,
                  KNIGHT,
                  c,
                  pos->iReducedMaterialDownScaler[uColor],
                  i,
                  "[scaled] enemy king tropism");
    }
    
    //
    // Give a bonus for blockading an enemy backward pawn.  Note:
    // we also reward pieces for blocking enemy passers in the
    // passer code.
    //
    cSquare = c + 16 * g_iAhead[uColor];
    bb = pHash->bbStationaryPawns[FLIP(uColor)];
    if (bb & COOR_TO_BB(cSquare))
    {
        ASSERT(pos->rgSquare[cSquare].pPiece);
        ASSERT(IS_PAWN(pos->rgSquare[cSquare].pPiece));
        EVAL_TERM(uColor,
                  KNIGHT,
                  c,
                  pos->iScore[uColor],
                  KNIGHT_ON_INTERESTING_SQUARE_BY_RANK[uColor][RANK(c)],
                  "blockades enemy pawn");
    }

    //
    // A knight with an open file behind it is good.
    // 
    uPawnFile = FILE(c) + 1;
    EVAL_TERM(uColor,
              KNIGHT,
              c,
              pos->iScore[uColor],
              ((pHash->uCountPerFile[uColor][uPawnFile] == 0) *
               KNIGHT_ON_INTERESTING_SQUARE_BY_RANK[uColor][RANK(c)]),
              "open file behind");
    
    //
    // Do mobilility and piece relevance.  Also update attack tables.
    //
    uMobilitySquares = 0;
    u = 0;
    ASSERT(g_iNDeltas[u] != 0);
    do
    {
        cSquare = c + g_iNDeltas[u];
        if (IS_ON_BOARD(cSquare))
        {
            //
            // Always update the attack bits
            //
            ASSERT((cSquare + 8) == (cSquare | 8));
            pos->rgSquare[cSquare|8].bvAttacks[uColor].small.uMinor = 1;

            //
            // See what we hit
            //
            p = pos->rgSquare[cSquare].pPiece;
            pFun = NMobJumpTable[uColor][p];
            if (pFun != _AnythingToFriendNoXray)
            {
                (void)(*pFun)(pos,
                              cSquare,
                              &uMobilitySquares,
                              NULL);
            }

            //
            // IDEA: bonus for hitting friendly pawn?
            //
        }
        u++;
    }
    while(g_iNDeltas[u] != 0);
    ASSERT(uMobilitySquares >= 0);
    ASSERT(uMobilitySquares <= 8);
    EVAL_TERM(uColor,
              KNIGHT,
              c,
              pos->iScore[uColor],
              KNIGHT_MOBILITY_BY_COUNT[uMobilitySquares],
              "knight mobility");
    if (uMobilitySquares == 0)
    {
        pos->cTrapped[uColor] = c;
    }
    ASSERT((pos->uMinMobility[uColor] & 0x80000000) == 0);
    ASSERT((uMobilitySquares & 0x80000000) == 0);
    pos->uMinMobility[uColor] = MINU(pos->uMinMobility[uColor], 
                                     uMobilitySquares);
}

static void
_EvalRook(IN OUT POSITION *pos, 
          IN COOR c, 
          IN PAWN_HASH_ENTRY *pHash)
/**

Routine description:

Parameters:

    IN POSITION *pos,
    IN COOR c,
    IN PAWN_HASH_ENTRY *pHash,

Return value:

    FLAG

**/
{
    static const PMOBILITY_HELPER RMobJumpTable[2][14] = 
    {
        {// (black)
            _BRookToEmpty,                     // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _AnythingToFriendNoXray,           // BLACK_PAWN    (2)
            _BRookToEnemyLess,                 // WHITE_PAWN    (3)
            _AnythingToFriendNoXray,           // BLACK_KNIGHT  (4)
            _BRookToEnemyLess,                 // WHITE_KNIGHT  (5)
            _AnythingToFriendNoXray,           // BLACK_BISHOP  (6)
            _BRookToEnemyLess,                 // WHITE_BISHOP  (7)
            _BRookToFriendRook,                // BLACK_ROOK    (8)
            _ERookToEnemySame,                 // WHITE_ROOK    (9)
            _ERookToFriendQueen,               // BLACK_QUEEN   (10)
            _ERookToEnemyGreater,              // WHITE_QUEEN   (11)
            _AnythingToFriendNoXray,           // BLACK_KING    (12)
            _ERookToEnemyGreater,              // WHITE_KING    (13)
        },
        {// (white)
            _WRookToEmpty,                     // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _WRookToEnemyLess,                 // BLACK_PAWN    (2)
            _AnythingToFriendNoXray,           // WHITE_PAWN    (3)
            _WRookToEnemyLess,                 // BLACK_KNIGHT  (4)
            _AnythingToFriendNoXray,           // WHITE_KNIGHT  (5)
            _WRookToEnemyLess,                 // BLACK_BISHOP  (6)
            _AnythingToFriendNoXray,           // WHITE_BISHOP  (7)
            _ERookToEnemySame,                 // BLACK_ROOK    (8)
            _WRookToFriendRook,                // WHITE_ROOK    (9)
            _ERookToEnemyGreater,              // BLACK_QUEEN   (10)
            _ERookToFriendQueen,               // WHITE_QUEEN   (11)
            _ERookToEnemyGreater,              // BLACK_KING    (12)
            _AnythingToFriendNoXray,           // WHITE_KING    (13)
        },
    };

    PIECE p;
    PIECE pFriendRook;
    ULONG uPawnFile = FILE(c) + 1;
    ULONG uColor;
    ULONG u;
    ULONG uCurrentMobility;
    ULONG uMaxMobility;
    ULONG uTotalMobility;
    COOR cSquare;
    BITBOARD bb;
    ULONG uBit;
    PMOBILITY_HELPER pFun;
    SCORE i;
    
    ASSERT(IS_ON_BOARD(c));
    p = pos->rgSquare[c].pPiece;
    ASSERT(p && IS_ROOK(p));
    uColor = GET_COLOR(p);
    ASSERT(IS_VALID_COLOR(uColor));
    pos->cPiece = c;
    
    //
    // Reward being on a half open or full open file.
    //
    if (0 == pHash->uCountPerFile[uColor][uPawnFile])
    {
        u = FILE_DISTANCE(c, pos->cNonPawns[FLIP(uColor)][0]);
        if (0 == pHash->uCountPerFile[FLIP(uColor)][uPawnFile])
        {
            //
            // Full open
            //
            EVAL_TERM(uColor,
                      ROOK,
                      c,
                      pos->iScore[uColor],
                      ROOK_ON_FULL_OPEN_BY_DIST_FROM_EKING[u],
                      "on full open");
        }
        else
        {
            //
            // Half open, no friendly, just enemy.
            //
            EVAL_TERM(uColor,
                      ROOK,
                      c,
                      pos->iScore[uColor],
                      ROOK_ON_HALF_OPEN_WITH_ENEMY_BY_DIST_FROM_EKING[u],
                      "on half open");
            
            //
            // Added bonus if the enemy pawn is a passer.
            //
            bb = pHash->bbPasserLocations[FLIP(uColor)] & BBFILE[FILE(c)];
            if (bb)
            {
                cSquare = CoorFromBitBoardRank8ToRank1(&bb);
                EVAL_TERM(uColor,
                          ROOK, 
                          c,
                          pos->iScore[uColor],
                          PASSER_BY_RANK[FLIP(uColor)][RANK(cSquare)] / 4,
                          "hassles enemy passer");
            }
        }
    }
    else if (0 == pHash->uCountPerFile[FLIP(uColor)][uPawnFile])
    {
        //
        // Half open, no enemy pawn, just a friendly
        //
        u = FILE_DISTANCE(c, pos->cNonPawns[FLIP(uColor)][0]);
        EVAL_TERM(uColor,
                  ROOK,
                  c,
                  pos->iScore[uColor],
                  ROOK_ON_HALF_OPEN_WITH_FRIEND_BY_DIST_FROM_EKING[u],
                  "on half open");

        //
        // See if the friend is a passed pawn and if the rook's in
        // front of it or behind it.
        //
        bb = pHash->bbPasserLocations[uColor] & BBFILE[FILE(c)];
        if (bb)
        {
            //
            // IDEA: Rook behind candidates, helpers or sentries is good
            // because the file may open soon.
            //
            while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&bb)))
            {
                if (uColor == WHITE) 
                {
                    if (cSquare < c)
                    {
                        EVAL_TERM(WHITE,
                                  ROOK,
                                  c,
                                  pos->iScore[WHITE],
                                  ROOK_BEHIND_PASSER_BY_PASSER_RANK[WHITE]
                                  [RANK(cSquare)],
                                  "behind own passer");
                    }
                    else
                    {
                        EVAL_TERM(WHITE,
                                  ROOK,
                                  c,
                                  pos->iScore[WHITE],
                                  ROOK_LEADS_PASSER_BY_PASSER_RANK[WHITE]
                                  [RANK(cSquare)],
                                  "in the way of passer");
                    }
                } else {
                    ASSERT(uColor == BLACK);
                    if (cSquare > c)
                    {
                        EVAL_TERM(BLACK,
                                  ROOK,
                                  c,
                                  pos->iScore[BLACK],
                                  ROOK_BEHIND_PASSER_BY_PASSER_RANK[BLACK]
                                  [RANK(cSquare)],
                                  "behind own passer");
                    }
                    else
                    {
                        EVAL_TERM(BLACK,
                                  ROOK,
                                  c,
                                  pos->iScore[BLACK],
                                  ROOK_LEADS_PASSER_BY_PASSER_RANK[BLACK]
                                  [RANK(cSquare)],
                                  "in the way of passer");
                    }
                }
            }
        }
    }
    
    //
    // Look for rooks that are trapping enemy kings.
    // 
    if (uColor == WHITE) 
    {
        pFriendRook = WHITE_ROOK;

        //
        // Rook on 7th/8th rank with an enemy king back there.
        // 
        if ((c < A6) && (pos->cNonPawns[BLACK][0] < A6))
        {
            ASSERT(RANK(c) > 6);
            ASSERT(RANK(pos->cNonPawns[BLACK][0]) > 6);
            
            EVAL_TERM(WHITE,
                      ROOK,
                      c,
                      pos->iScore[WHITE],
                      ROOK_TRAPPING_EKING,
                      "rook trapping enemy king");
        }
    }
    else 
    {
        pFriendRook = BLACK_ROOK;

        //
        // Rook on 7th/8th rank with an enemy king back there.
        // 
        if ((c > H3) && (pos->cNonPawns[WHITE][0] > H3))
        {
            ASSERT(RANK(c) < 3);
            ASSERT(RANK(pos->cNonPawns[WHITE][0]) < 3);
            EVAL_TERM(BLACK,
                      ROOK,
                      c,
                      pos->iScore[BLACK],
                      ROOK_TRAPPING_EKING,
                      "rook trapping enemy king");
        }
    }
    
    //
    // Rooks increase in value as pawns come off:
    // 
    // "A further refinement would be to raise the knight's value by
    // 1/16 and lower the rook's value by 1/8 for each pawn above five
    // of the side being valued, with the opposite adjustment for each
    // pawn short of five."
    //                                             --Larry Kaufman, IM
    //                               "Evaluation of Material Imbalance"
    // 
    EVAL_TERM(uColor,
              ROOK,
              c,
              pos->iScore[uColor],
              ROOK_VALUE_AS_PAWNS_COME_OFF[pos->uPawnCount[uColor] +
                                           pos->uPawnCount[FLIP(uColor)]],
              "increase in value/pawns");
    
    //
    // Rook mobility
    //
    u = uMaxMobility = uTotalMobility = 0;
    ASSERT(g_iRDeltas[u] != 0);
    do
    {
        uCurrentMobility = 0;
        uBit = ROOK_BIT;
        cSquare = c + g_iRDeltas[u];
        
        while(IS_ON_BOARD(cSquare))
        {
            //
            // Twiddle our attack table bits.
            //
            pos->rgSquare[cSquare|8].bvAttacks[uColor].uWholeThing |= uBit;
            ASSERT((cSquare | 8) == (cSquare + 8));

            //
            // What did we hit?
            //
            p = pos->rgSquare[cSquare].pPiece;
            pFun = RMobJumpTable[uColor][p];
            if (TRUE == (*pFun)(pos,
                                cSquare,
                                &uCurrentMobility,
                                &uBit))
            {
                break;
            }
            cSquare += g_iRDeltas[u];
        }
        uTotalMobility += uCurrentMobility;
        ASSERT(uTotalMobility <= 14);
        ASSERT((uMaxMobility & 0x80000000) == 0);
        ASSERT((uCurrentMobility & 0x80000000) == 0);
        uMaxMobility = MAXU(uCurrentMobility, uMaxMobility);
        ASSERT(uMaxMobility <= 7);
        u++;
    }
    while(g_iRDeltas[u] != 0);
    ASSERT(uTotalMobility <= 14);

    ASSERT(pos->uArmyScaler[FLIP(uColor)] <= 31);
    ASSERT(REDUCED_MATERIAL_UP_SCALER[pos->uArmyScaler[FLIP(uColor)]] <= 8);
    i = ROOK_MOBILITY_BY_SQUARES[uTotalMobility];
    i *= (int)(REDUCED_MATERIAL_UP_SCALER[pos->uArmyScaler[FLIP(uColor)]] + 1);
    i /= 8;
    EVAL_TERM(uColor,
              ROOK,
              c,
              pos->iScore[uColor],
              i,
              "rook mobility");
    EVAL_TERM(uColor,
              ROOK,
              c,
              pos->iScore[uColor],
              ROOK_MAX_MOBILITY_IN_A_ROW_BONUS[uMaxMobility],
              "consecutive rook mobility");

    ASSERT((pos->uMinMobility[uColor] & 0x80000000) == 0);
    ASSERT((uTotalMobility & 0x80000000) == 0);
    pos->uMinMobility[uColor] = MINU(pos->uMinMobility[uColor], 
                                     uTotalMobility);
    if (uTotalMobility < 3)
    {
        //
        // Look for rooks with no mobility who are under attack.  These
        // pieces are trapped!
        //
        if (uTotalMobility == 0)
        {
            pos->cTrapped[uColor] = c;
        }

        //
        // Rook trapped in the corner by a stupid friendly king?
        // 
        ASSERT(IS_VALID_COLOR(uColor));
        if (uColor == WHITE)
        {
            ASSERT(IS_ON_BOARD(c));
            if (RANK1(c))
            {
                cSquare = pos->cNonPawns[WHITE][0];
                ASSERT(IS_ON_BOARD(cSquare));
                if (RANK1(cSquare))
                {
                    if (((cSquare > E1) && (c > cSquare)) ||
                        ((cSquare < D1) && (c < cSquare)))
                    {
                        EVAL_TERM(WHITE,
                                  ROOK,
                                  c,
                                  pos->iScore[WHITE],
                                  KING_TRAPPING_ROOK,
                                  "king trapping rook");
                    }
                }
            }
        }
        else 
        {
            ASSERT(uColor == BLACK);
            ASSERT(IS_ON_BOARD(c));
            if (RANK8(c))
            {
                cSquare = pos->cNonPawns[BLACK][0];
                ASSERT(IS_ON_BOARD(cSquare));
                
                if (RANK8(cSquare))
                {
                    if (((cSquare > E8) && (c > cSquare)) ||
                        ((cSquare < D8) && (c < cSquare)))
                    {
                        EVAL_TERM(BLACK,
                                  ROOK,
                                  c,
                                  pos->iScore[BLACK],
                                  KING_TRAPPING_ROOK,
                                  "king trapping rook");
                    }
                }
            }
        }
    } // if low mobility
}


static void
_EvalQueen(IN OUT POSITION *pos, 
           IN COOR c, 
           IN PAWN_HASH_ENTRY *pHash)
/**

Parameters:

    POSITION *pos,
    COOR c,
    PAWN_HASH_ENTRY *pHash,

Return value:

    FLAG

**/
{
    static const PMOBILITY_HELPER QMobJumpTable[2][14] = 
    {
        {
            _BQueenToEmpty,                    // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _AnythingToFriendNoXray,           // BLACK_PAWN    (2)
            _BQueenToEnemyLess,                // WHITE_PAWN    (3)
            _AnythingToFriendNoXray,           // BLACK_KNIGHT  (4)
            _BQueenToEnemyLess,                // WHITE_KNIGHT  (5)
            _EQueenToFriendBishop,             // BLACK_BISHOP  (6)
            _BQueenToEnemyLess,                // WHITE_BISHOP  (7)
            _EQueenToFriendRook,               // BLACK_ROOK    (8)
            _BQueenToEnemyLess,                // WHITE_ROOK    (9)
            _EQueenToFriendQueen,              // BLACK_QUEEN   (10)
            _EQueenToEnemyGreaterEqual,        // WHITE_QUEEN   (11)
            _AnythingToFriendNoXray,           // BLACK_KING    (12)
            _EQueenToEnemyGreaterEqual,        // WHITE_KING    (13)
        },
        {
            _WQueenToEmpty,                    // EMPTY_SQUARE  (0)
            _InvalidMobilityHelper,            // INVALID_PIECE (1)
            _WQueenToEnemyLess,                // BLACK_PAWN    (2)
            _AnythingToFriendNoXray,           // WHITE_PAWN    (3)
            _WQueenToEnemyLess,                // BLACK_KNIGHT  (4)
            _AnythingToFriendNoXray,           // WHITE_KNIGHT  (5)
            _WQueenToEnemyLess,                // BLACK_BISHOP  (6)
            _EQueenToFriendBishop,             // WHITE_BISHOP  (7)
            _WQueenToEnemyLess,                // BLACK_ROOK    (8)
            _EQueenToFriendRook,               // WHITE_ROOK    (9)
            _EQueenToEnemyGreaterEqual,        // BLACK_QUEEN   (10)
            _EQueenToFriendQueen,              // WHITE_QUEEN   (11)
            _EQueenToEnemyGreaterEqual,        // BLACK_KING    (12)
            _AnythingToFriendNoXray,           // WHITE_KING    (13)
        },
    };

    PIECE p = pos->rgSquare[c].pPiece;
    ULONG uColor;
    COOR cSquare;
    ULONG uTotalMobility;
    ULONG u;
    ULONG uBit;
    PMOBILITY_HELPER pFun;
    ULONG uNearKing = 0;
    COOR cKing;

    ASSERT(IS_ON_BOARD(c));
    ASSERT(p && IS_QUEEN(p));
    uColor = GET_COLOR(p);
    ASSERT(IS_VALID_COLOR(uColor));
    pos->cPiece = c;
    cKing = pos->cNonPawns[FLIP(uColor)][0];
    ASSERT(IS_ON_BOARD(cKing));
    ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));
    
    if ((FALSE == pos->fCastled[uColor]) && (pos->uMinorsAtHome[uColor] > 1))
    {
        //
        // Discourage queen being out too early
        // 
        if (!RANK1(c) && !RANK8(c))
        {
            ASSERT(QUEEN_OUT_EARLY[pos->uMinorsAtHome[uColor]] < 0);
            ASSERT(pos->uMinorsAtHome[uColor] <= 4);
            EVAL_TERM(uColor,
                      QUEEN,
                      c,
                      pos->iScore[uColor],
                      QUEEN_OUT_EARLY[pos->uMinorsAtHome[uColor]],
                      "queen out too early");
        }
    }
    else
    {
        //
        // Encourage enemy king tropism
        //
        u = DISTANCE(c, cKing);
        ASSERT((u <= 7) && (u > 0));
        EVAL_TERM(uColor,
                  QUEEN,
                  c,
                  pos->iScore[uColor],
                  QUEEN_KING_TROPISM[u],
                  "enemy king tropism");
    }

    //
    // Do queen mobility
    //
    uTotalMobility = 0;
    u = 0;
    ASSERT(g_iQKDeltas[u] != 0);
    do
    {
        uBit = QUEEN_BIT;
        cSquare = c + g_iQKDeltas[u];
        while(IS_ON_BOARD(cSquare))
        {
            //
            // Toggle attack table bits.
            //
            pos->rgSquare[cSquare|8].bvAttacks[uColor].uWholeThing |= uBit;
            
            //
            // What did we hit?
            //
            p = pos->rgSquare[cSquare].pPiece;
            uNearKing += (DISTANCE(cSquare, cKing) <= 1);
            pFun = QMobJumpTable[uColor][p];
            if (TRUE == (*pFun)(pos,
                                cSquare,
                                &uTotalMobility,
                                &uBit))
            {
                break;
            }
            cSquare += g_iQKDeltas[u];
        }
        u++;
    }
    while(g_iQKDeltas[u] != 0);

    ASSERT(uTotalMobility <= 27);
    EVAL_TERM(uColor,
              QUEEN,
              c,
              pos->iScore[uColor],
              QUEEN_MOBILITY_BY_SQUARES[uTotalMobility],
              "queen mobility");

    //
    // Look for queens with no mobility who are under attack.  These
    // pieces are trapped!
    //
    if (uTotalMobility == 0)
    {
        pos->cTrapped[uColor] = c;
    }
    ASSERT((pos->uMinMobility[uColor] & 0x80000000) == 0);
    ASSERT((uTotalMobility & 0x80000000) == 0);
    pos->uMinMobility[uColor] = MINU(pos->uMinMobility[uColor], 
                                     uTotalMobility);

    //
    // If queen points near enemy king, that's a good thing too.
    //
    ASSERT(uNearKing <= 6);
    EVAL_TERM(uColor,
              QUEEN,
              c,
              pos->iScore[uColor],
              (uNearKing * QUEEN_ATTACKS_SQ_NEXT_TO_KING),
              "pointing near enemy K");
}

static void
_EvalKing(IN OUT POSITION *pos, 
          IN const COOR c, 
          IN const PAWN_HASH_ENTRY *pHash)
/**

Routine description:

Parameters:

    POSITION *pos,
    COOR c,
    PAWN_HASH_ENTRY *pHash,

Return value:

    FLAG

**/
{
    PIECE p;
    ULONG uColor, ufColor;
    COOR cSquare;
    ULONG u, v;
    ULONG uCounter;
    BITV bvAttack;
    BITV bvXray;
    BITV bvDefend;
    BITV bvPattern = 0;
    ULONG uFlightSquares;
    BITBOARD bb;
    SCORE i;
    SCORE iKingScore = 0;

    static ULONG KingFlightDefects[9] =
    {
        +4, +2, +1, 0, 0, 0, 0, 0, 0
    };
    
    static ULONG KingStormingPawnDefects[8] =
    {
        +0, +4, +3, +1, +0, +0, +0, +0
    };

    static ULONG KingFileDefects[3] = 
    {
        +2, +1, +0
    };

    static INT KingSafetyDeltas[11] = 
    {
        -17, -16, -15,
        -1,        +1,
        +15, +16, +17, -2, +2, 0
    };
    
    static ULONG EmptyAttackedSquare[2][11] = 
    {
        { 
            1, 1, 1,
            1,    1,
            2, 2, 2, 1, 1, 0
        },
        {
            2, 2, 2,
            1,    1,
            1, 1, 1, 1, 1, 0
        },
    };

    static ULONG EmptyUnattackedSquare[2][11] = 
    {
        { 
            0, 0, 0,
            0,    0,
            1, 1, 1, 0, 0, 0
        },
        {
            1, 1, 1,
            0,    0,
            0, 0, 0, 0, 0, 0
        },
    };
    
    static ULONG OccupiedAttackedSquare[2] = 
    {
        1,
        2
    };
   
    ASSERT(IS_ON_BOARD(c));
    p = pos->rgSquare[c].pPiece;
    ASSERT(p && IS_KING(p));
    uColor = GET_COLOR(p);
    ufColor = FLIP(uColor);
    ASSERT(IS_VALID_COLOR(uColor));
    
    u = pos->uNonPawnMaterial[ufColor];
    ASSERT(u >= VALUE_KING);
    if (u < DO_KING_SAFETY_THRESHOLD)
    {
        u = 0;
        ASSERT(g_iQKDeltas[u] != 0);
        do
        {
            cSquare = c + g_iQKDeltas[u];
            if (IS_ON_BOARD(cSquare))
            {
                pos->rgSquare[cSquare|8].bvAttacks[uColor].small.uKing = 1;
            }
            u++;
        }
        while(g_iQKDeltas[u] != 0);
        goto skip_safety;
    }

    // Prepare to do king safety
    uCounter = (u >= KEEP_KING_AT_HOME_THRESHOLD) * 
        KING_INITIAL_COUNTER_BY_LOCATION[uColor][c];
#ifdef EVAL_DUMP
    Trace("%s Initial KS Counter: %u\n", COLOR_NAME(uColor), uCounter);
#endif

    uCounter += pos->uPiecesPointingAtKing[uColor] / 2;
#ifdef EVAL_DUMP
    Trace("%s KS Counter after pieces pointing: %u\n", 
          COLOR_NAME(uColor), uCounter);
#endif
    uFlightSquares = 0;
    u = 0;
    ASSERT(KingSafetyDeltas[u] != 0);
    do
    {
        cSquare = c + KingSafetyDeltas[u];
        if (IS_ON_BOARD(cSquare))
        {
            p = pos->rgSquare[cSquare].pPiece;
            
            cSquare |= 8;
            bvAttack = pos->rgSquare[cSquare].bvAttacks[ufColor].uSmall;
            bvXray = pos->rgSquare[cSquare].bvAttacks[ufColor].uXray;
            pos->rgSquare[cSquare].bvAttacks[uColor].small.uKing = 1;
            bvDefend = pos->rgSquare[cSquare].bvAttacks[uColor].uSmall;
            
            if (bvAttack != 0)
            {
                bvPattern |= (bvAttack | bvXray);
                if (bvXray || (bvAttack & (bvAttack - 1)))
                {
                    bvDefend &= ~8;
                }
                if (IS_EMPTY(p))
                {
                    ASSERT(u < 11);
                    uCounter += EmptyAttackedSquare[uColor][u];
                }
                else
                {
                    uCounter += 
                        OccupiedAttackedSquare[OPPOSITE_COLORS(p, uColor)];
                }
                uCounter += (bvDefend == 0);
            }
            else
            {
                uCounter += (bvXray != 0);
                ASSERT(u < 11);
                uCounter += EmptyUnattackedSquare[uColor][u];
                ASSERT((IS_EMPTY(p) == 0) || (IS_EMPTY(p) == 1));
                uFlightSquares += ((IS_EMPTY(p)) && (u < 8));
            }
        }
        u++;
    }
    while(KingSafetyDeltas[u] != 0);
#ifdef EVAL_DUMP
    Trace("%s KS Counter post-squares: %u\n", COLOR_NAME(uColor), uCounter);
#endif

    if (IS_ON_BOARD(c - 1))
    {
        u = FILE(c - 1) + 1;
        v = (pHash->uCountPerFile[WHITE][u] > 0) +
            (pHash->uCountPerFile[BLACK][u] > 0);
        ASSERT((v >= 0) && (v <= 2));

        // Note: open rook files are worse than open interior files
        uCounter += (KingFileDefects[v] + ((v < 2) && ((u == 1) || (u == 8))));

        bb = pHash->bbPawnLocations[ufColor] & BBFILE[u - 1];
        if (bb)
        {
            if (uColor == WHITE)
            {
                cSquare = CoorFromBitBoardRank1ToRank8(&bb);
            }
            else
            {
                cSquare = CoorFromBitBoardRank8ToRank1(&bb);
            }
            ASSERT(IS_ON_BOARD(cSquare));
            uCounter += KingStormingPawnDefects[DISTANCE(cSquare, c)];
        }
    }
    
    u = FILE(c) + 1;
    v = (pHash->uCountPerFile[WHITE][u] > 0) +
        (pHash->uCountPerFile[BLACK][u] > 0);
    ASSERT((v >= 0) && (v <= 2));

    // Note: open rook files are worse than open interior files
    uCounter += (KingFileDefects[v] + ((v < 2) && ((u == 1) || (u == 8))));
    bb = pHash->bbPawnLocations[ufColor] & BBFILE[u - 1];
    if (bb)
    {
        if (uColor == WHITE)
        {
            cSquare = CoorFromBitBoardRank1ToRank8(&bb);
        }
        else
        {
            cSquare = CoorFromBitBoardRank8ToRank1(&bb);
        }
        ASSERT(IS_ON_BOARD(cSquare));
        uCounter += KingStormingPawnDefects[DISTANCE(cSquare, c)];
    }

    if (IS_ON_BOARD(c + 1))
    {
        ASSERT((FILE(c + 1) + 1) == (u + 1));
        u += 1;
        v = (pHash->uCountPerFile[WHITE][u] > 0) +
            (pHash->uCountPerFile[BLACK][u] > 0);
        ASSERT((v >= 0) && (v <= 2));
        
        // Note: open rook files are worse than open interior files
        uCounter += (KingFileDefects[v] + ((v < 2) && ((u == 1) || (u == 8))));
        
        bb = pHash->bbPawnLocations[ufColor] & BBFILE[u - 1];
        if (bb)
        {
            if (uColor == WHITE)
            {
                cSquare = CoorFromBitBoardRank1ToRank8(&bb);
            }
            else
            {
                cSquare = CoorFromBitBoardRank8ToRank1(&bb);
            }
            ASSERT(IS_ON_BOARD(cSquare));
            uCounter += KingStormingPawnDefects[DISTANCE(cSquare, c)];
        }
    }    
#ifdef EVAL_DUMP
    Trace("%s KS Counter post-open file/stormers: %u\n", COLOR_NAME(uColor), 
          uCounter);
#endif

    bvPattern >>= 3;
    ASSERT(bvPattern >= 0);
    ASSERT(bvPattern < 32);
    v = KING_COUNTER_BY_ATTACK_PATTERN[bvPattern];
    uCounter += v;
#ifdef EVAL_DUMP
    Trace("%s KS Counter post-attack pattern: %u\n", COLOR_NAME(uColor), 
          uCounter);
#endif
        
    ASSERT(uFlightSquares <= 8);
    uCounter += KingFlightDefects[uFlightSquares] * (v > 1);
#ifdef EVAL_DUMP
    Trace("%s KS Counter post-flight sq: %u\n", COLOR_NAME(uColor), uCounter);
#endif

    //
    // Note: can't use pos->iReducedMaterialDownScaler here because we
    // are scaling it based on the _other_ side's material.
    //
    uCounter = MINU(uCounter, 41);
    i = KING_SAFETY_BY_COUNTER[uCounter];
    i *= REDUCED_MATERIAL_DOWN_SCALER[pos->uArmyScaler[ufColor]];
    i /= 8;
    EVAL_TERM(uColor,
              KING,
              c,
              iKingScore,
              i,
              "king safety");

    //
    // Bonus for castling / penalty for loss of castle.  Also, if side has
    // not yet castled, be concerned with undeveloped minor pieces.
    //
    if (FALSE == pos->fCastled[uColor])
    {
        if (pos->uNonPawnCount[uColor][0] > 4)
        {
            ASSERT(pos->uMinorsAtHome[uColor] <= 4);
            EVAL_TERM(uColor,
                     0,
                     ILLEGAL_COOR,
                     pos->iScore[uColor],
                     UNDEVELOPED_MINORS_IN_OPENING[pos->uMinorsAtHome[uColor]],
                     "development");
            if (uColor == BLACK)
            {
                EVAL_TERM(BLACK,
                          KING,
                          c,
                          iKingScore,
                          (KING_MISSING_ONE_CASTLE_OPTION *
                           ((CASTLE_BLACK_SHORT & pos->bvCastleInfo) == 0)),
                          "can't castle short");
                EVAL_TERM(BLACK,
                          KING,
                          c,
                          iKingScore,
                          (KING_MISSING_ONE_CASTLE_OPTION *
                           ((CASTLE_BLACK_LONG & pos->bvCastleInfo) == 0)),
                          "can't castle long");
            }
            else
            {
                ASSERT(uColor == WHITE);
                EVAL_TERM(WHITE,
                          KING,
                          c,
                          iKingScore,
                          (KING_MISSING_ONE_CASTLE_OPTION *
                           ((CASTLE_WHITE_SHORT & pos->bvCastleInfo) == 0)),
                          "can't castle short");
                EVAL_TERM(WHITE,
                          KING,
                          c,
                          iKingScore,
                          (KING_MISSING_ONE_CASTLE_OPTION *
                           ((CASTLE_WHITE_LONG & pos->bvCastleInfo) == 0)),
                          "can't castle long");
            }
        }
    }
    
 skip_safety:
    //
    // Special code for kings in the endgame
    // 
    u = pos->uNonPawnCount[WHITE][0] + pos->uNonPawnCount[BLACK][0];
    if (u < 8)
    {
        //
        // Encourage kings to come to the center
        //
        i = KING_TO_CENTER[c];
        EVAL_TERM(uColor,
                  KING,
                  c,
                  iKingScore,
                  i,
                  "centralize king");

        //
        // Kings in front of passers in the late endgame are strong...
        // 
        cSquare = FILE(c);
        bb = pHash->bbPasserLocations[uColor] & 
            (BBADJACENT_FILES[cSquare] | BBFILE[cSquare]);
        if (bb)
        {
            while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&bb)))
            {
                if (DISTANCE(cSquare, c) == 1)
                {
                    ASSERT(abs((INT)FILE(c) - (INT)FILE(cSquare)) <= 1);
                    ASSERT(abs((INT)RANK(c) - (INT)RANK(cSquare)) <= 1);
                    
                    i = 2;
                    i += SUPPORTED_PASSER_BY_RANK[uColor][RANK(cSquare)];
                    switch(uColor)
                    {
                        case WHITE:
                            i += 8 * (c < (cSquare - 1));
                            break;
                            
                        case BLACK:
                            i += 8 * (c > (cSquare + 1));
                            break;
                    }
                    EVAL_TERM(uColor,
                              KING,
                              c,
                              iKingScore,
                              i,
                              "supporting own passer");
                }
            }
        }
        
        //
        // Detect kings way out of the action in KP endgames:
        // 
        // 8/8/1p6/p6k/P3K3/8/1P6/8 w - - 0 11
        // 
        if ((u == 2) && (uColor == WHITE))
        {
            i = 0;
            bb = (pHash->bbPawnLocations[WHITE] |
                  pHash->bbPawnLocations[BLACK]);
            while(IS_ON_BOARD(cSquare = CoorFromBitBoardRank8ToRank1(&bb)))
            {
                i += (DISTANCE(cSquare, pos->cNonPawns[BLACK][0]) -
                      DISTANCE(cSquare, pos->cNonPawns[WHITE][0]));
            }
            EVAL_TERM(WHITE,
                      KING,
                      c,
                      iKingScore,
                      i * 12,
                      "in/out of \"action\"");
        }
    }
    pos->iScore[uColor] += iKingScore;
    pos->iTempScore = iKingScore;
}


FLAG
EvalPasserRaces(IN OUT POSITION *pos, 
                IN PAWN_HASH_ENTRY *pHash)
/**

Routine description:

    Determine if one side has a pawn that is unstoppable.

    This is broken for positions like:

Parameters:

    POSITION *pos,
    PAWN_HASH_ENTRY *pHash,

Return value:

    void

**/
{
    BITBOARD bb[2];
    ULONG uColor;
    int d1;
    COOR cQueen;
    COOR cKing, c;
    ULONG uKingDist, uPawnDist, uFriendDist;
    ULONG uRacerDist[2] = { 99, 99 };
    FLAG fDontCountMeOut[2] = { ((RANK(pos->cNonPawns[BLACK][0]) <= 3) &&
                                 (pos->uPawnCount[BLACK] != 0)),
                                ((RANK(pos->cNonPawns[WHITE][0]) >= 6) &&
                                 (pos->uPawnCount[WHITE] != 0)) };
    bb[BLACK] = pHash->bbPasserLocations[BLACK];
    bb[WHITE] = pHash->bbPasserLocations[WHITE];
    if (!(bb[BLACK] | bb[WHITE]))
    {
        return(FALSE);
    }
    ASSERT((pos->uNonPawnCount[WHITE][0] == 1) ||
           (pos->uNonPawnCount[BLACK][0] == 1))
    FOREACH_COLOR(uColor)
    {
        if (pos->uNonPawnCount[FLIP(uColor)][0] == 1)
        {
            d1 = 16 * g_iAhead[uColor];
            ASSERT((d1 * 2) == (32 * g_iAhead[uColor]));
            if (bb[uColor])
            {
                while(IS_ON_BOARD(c = CoorFromBitBoardRank8ToRank1(
                                      &(bb[uColor]))))
                {
                    // If the enemy king can take this passer and is
                    // on the move, all bets are off!
                    cKing = pos->cNonPawns[FLIP(uColor)][0];
                    ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));
                    ASSERT(GET_COLOR(pos->rgSquare[cKing].pPiece) != uColor);
                    if (pos->uToMove == FLIP(uColor) &&
                        (DISTANCE(cKing, c) == 1))
                    {
                        // TODO: something clever here about protected pawns
                        continue;
                    }

                    cQueen = FILE_RANK_TO_COOR(FILE(c), QUEENING_RANK[uColor]);
                    ASSERT(FILE(cQueen) == FILE(c));
                    uPawnDist = RANK_DISTANCE(cQueen, c);
                    if ((RANK(c) == JUMPING_RANK[uColor]) &&
                        (IS_EMPTY(pos->rgSquare[c + d1].pPiece) &&
                         IS_EMPTY(pos->rgSquare[c + d1 * 2].pPiece)))
                    {
                        uPawnDist--;
                    }
                    ASSERT((uPawnDist >= 1) && (uPawnDist <= 6));
                    
                    uKingDist = DISTANCE(cKing, cQueen);
                    if (uKingDist > 0) 
                    {
                        uKingDist -= (pos->uToMove == FLIP(uColor));
                    }
                    ASSERT((uKingDist >= 0) && (uPawnDist <= 7));
                    
                    if (uPawnDist < uKingDist)
                    {
                        uRacerDist[uColor] = MINU(uRacerDist[uColor],
                                                  uPawnDist);
                    }
                    
                    cKing = pos->cNonPawns[uColor][0];
                    ASSERT(IS_KING(pos->rgSquare[cKing].pPiece));
                    ASSERT(GET_COLOR(pos->rgSquare[cKing].pPiece) == uColor);
                    
                    uFriendDist = DISTANCE(cKing, cQueen);
                    if ((uFriendDist <= 1) && (DISTANCE(cKing, c) <= 1))
                    {
                        if (FILE(c) != FILE(cKing))
                        {
                            uRacerDist[uColor] = MINU(uRacerDist[uColor],
                                                      uPawnDist);
                        }
                        else if ((FILE(cKing) != A) &&
                                 (FILE(cKing) != H))
                        {
                            uRacerDist[uColor] = MINU(uRacerDist[uColor],
                                                      (uPawnDist + 1));
                        }
                    }
                    fDontCountMeOut[uColor] = TRUE;
                    
                    // 
                    // TODO: winning connected passers
                    //
                }
            }
        }
    }

    if ((uRacerDist[WHITE] < uRacerDist[BLACK]) && 
        (FALSE == fDontCountMeOut[BLACK]))
    {
        EVAL_TERM(WHITE,
                  PAWN,
                  ILLEGAL_COOR,
                  pos->iScore[WHITE],
                  RACER_WINS_RACE - uRacerDist[WHITE] * 4,
                  "winning racer pawn");
        return(TRUE);
    }
    else if ((uRacerDist[BLACK] < uRacerDist[WHITE]) &&
             (FALSE == fDontCountMeOut[WHITE]))
    {
        EVAL_TERM(BLACK,
                  PAWN,
                  ILLEGAL_COOR,
                  pos->iScore[BLACK],
                  RACER_WINS_RACE - uRacerDist[BLACK] * 4,
                  "winning racer pawn");
        return(TRUE);
    }
    return(FALSE);
}


static void 
_EvalPassers(IN OUT POSITION *pos, 
             IN PAWN_HASH_ENTRY *pHash)
/**

Routine description:

Parameters:

    POSITION *pos,
    PAWN_HASH_ENTRY *pHash,

Return value:

    void

**/
{
    ULONG u;
    ULONG uColor;
    COOR c;
    COOR cSquare;
    BITBOARD bb;
    PIECE p;
    SCORE i;
    ULONG uNumPassers[2] = { 0, 0 };
    int d1;

    ASSERT(pHash->bbPasserLocations[WHITE] |
           pHash->bbPasserLocations[BLACK]);
    
    FOREACH_COLOR(uColor)
    {
        bb = pHash->bbPasserLocations[uColor];
        if (bb != 0)
        {
            uNumPassers[uColor] = CountBits(bb);
            ASSERT(uNumPassers[uColor] > 0);
            d1 = 16 * g_iAhead[uColor];
            ASSERT((d1 * 2) == (32 * g_iAhead[uColor]));
            
            while(IS_ON_BOARD(c = CoorFromBitBoardRank8ToRank1(&bb)))
            {
                //
                // Consider the control of the square in front of the passer
                //
                cSquare = c + d1;
                ASSERT(IS_ON_BOARD(cSquare));
                u = _WhoControlsSquareFast(pos, cSquare);
                if (u == FLIP(uColor))
                {
                    EVAL_TERM(uColor,
                              PAWN,
                              c,
                              pos->iScore[uColor],
                              -((PASSER_BY_RANK[uColor][RANK(c)] / 4) +
                                (PASSER_BY_RANK[uColor][RANK(c)] / 16)),
                              "enemy controls sq ahead");
                }
                else if (u == (ULONG)-1)
                {
                    p = pos->rgSquare[cSquare].pPiece;
                    if (!IS_EMPTY(p) && (OPPOSITE_COLORS(p, uColor)))
                    {
                        ASSERT(GET_COLOR(p) != uColor);
                        EVAL_TERM(uColor,
                                  PAWN,
                                  c,
                                  pos->iScore[uColor],
                                  -(PASSER_BY_RANK[uColor][RANK(c)] / 4),
                                  "enemy occupies sq ahead");
                    }
                }
#ifdef DEBUG
                else
                {
                    ASSERT(u == uColor);
                }
#endif
                
                //
                // Consider distance from friend king / enemy king
                //
                i = (8 - DISTANCE(c, pos->cNonPawns[uColor][0]));
                if (((uColor == WHITE) && 
                     ((pos->cNonPawns[BLACK][0] >> 4) <= (c >> 4))) ||
                    ((uColor == BLACK) &&
                     ((pos->cNonPawns[WHITE][0] >> 4) >= (c >> 4)))) {
                    // 
                    // The enemy king is ahead of the passer
                    //
                    i += FILE_DISTANCE(c, pos->cNonPawns[FLIP(uColor)][0]) * 4;
                }
                else
                {
                    // 
                    // The enemy king is behind the passer
                    //
                    i += 10 + DISTANCE(c, pos->cNonPawns[FLIP(uColor)][0]) * 4;
                }
                i *= PASSER_MATERIAL_UP_SCALER[pos->uArmyScaler[FLIP(uColor)]];
                i /= 8;
                EVAL_TERM(uColor,
                          PAWN,
                          c,
                          pos->iScore[uColor],
                          i,
                          "passer distance to kings");
                
                //
                // If a side is a piece down then the other side's passers
                // are even more dangerous.
                //
                if (pos->uNonPawnCount[uColor][0] > 
                    pos->uNonPawnCount[FLIP(uColor)][0])
                {
                    i = (PASSER_BY_RANK[uColor][RANK(c)] / 2);
                    EVAL_TERM(uColor,
                              PAWN,
                              c,
                              pos->iScore[uColor],
                              i,
                              "passer and piece up");
                }

                //
                // TODO: For connected passers vs KR, consult the oracle
                //
            }
        }
    }
    
    //
    // Game stage bonus
    // 
    u = pos->uArmyScaler[BLACK];
    EVAL_TERM(WHITE,
              PAWN,
              ILLEGAL_COOR,
              pos->iScore[WHITE],
              uNumPassers[WHITE] * PASSER_BONUS_AS_MATERIAL_COMES_OFF[u],
              "passer value material");
    ASSERT(PASSER_BONUS_AS_MATERIAL_COMES_OFF[u] >= 0);
    u = pos->uArmyScaler[WHITE];
    EVAL_TERM(BLACK,
              PAWN,
              ILLEGAL_COOR,
              pos->iScore[BLACK],
              uNumPassers[BLACK] * PASSER_BONUS_AS_MATERIAL_COMES_OFF[u],
              "passer value material");
    ASSERT(PASSER_BONUS_AS_MATERIAL_COMES_OFF[u] >= 0);
}


static void 
_EvalBadTrades(IN OUT POSITION *pos)
/**

Routine description:

Parameters:

    POSITION *pos,

Return value:

    void

**/
{
    ULONG uAhead, uBehind;
    ULONG uMagnitude;
    
    //
    // See who is ahead
    //
    ASSERT(pos->uNonPawnMaterial[WHITE] != pos->uNonPawnMaterial[BLACK]);
    uAhead = (pos->uNonPawnMaterial[WHITE] > pos->uNonPawnMaterial[BLACK]);
    uBehind = FLIP(uAhead);
#ifdef DEBUG
    if (pos->uNonPawnMaterial[WHITE] > pos->uNonPawnMaterial[BLACK])
    {
        ASSERT(uAhead == WHITE);
        ASSERT(uBehind == BLACK);
    }
    else
    {
        ASSERT(pos->uNonPawnMaterial[BLACK] > pos->uNonPawnMaterial[WHITE]);
        ASSERT(uAhead == BLACK);
        ASSERT(uBehind == WHITE);
    }
#endif
    uMagnitude = ((pos->uNonPawnMaterial[uAhead] + 
                   pos->uNonPawnCount[uAhead][0] * 128) -
                  (pos->uNonPawnMaterial[uBehind] +
                   pos->uNonPawnCount[uBehind][0] * 128));
    uMagnitude /= 128;
    uMagnitude -= (uMagnitude != 0);
    ASSERT(!(uMagnitude & 0x80000000));
    uMagnitude = MINU(2, uMagnitude);
    ASSERT(uMagnitude <= 2);

    //
    // Encourage the side that is ahead in piece material to continue
    // trading pieces and not to trade pawns.  This also has the
    // effect of making the side that is behind in material want to
    // trade pawns and not pieces.  This also gives the side that is
    // ahead a bit of a "bad trade" bonus.  Note that the bonus is not
    // given if the side "ahead" has no pawns; this is so that the
    // engine will not like positions like KNB vs KRP - the side with
    // the pawn has the winning chances.
    //
    EVAL_TERM(uAhead,
              0,
              ILLEGAL_COOR,
              pos->iScore[uAhead],
              ((pos->uPawnCount[uAhead] != 0) *
               TRADE_PIECES[uMagnitude][pos->uNonPawnCount[uBehind][0]]),
              "trade pieces");
    ASSERT(TRADE_PIECES[uMagnitude][pos->uNonPawnCount[uBehind][0]] > 0);
    EVAL_TERM(uAhead,
              0,
              ILLEGAL_COOR,
              pos->iScore[uAhead],
              DONT_TRADE_PAWNS[uMagnitude][pos->uPawnCount[uAhead]],
              "don't trade pawns");
}

static void
_EvalLookForDanger(IN OUT SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

    This routine looks for squares on the board where the side to move
    has a piece that is en prise.  So far in eval we will have tagged
    pieces in situations where they are attacked by lesser-valued
    enemy pieces as "in danger".  However, because of the order in
    which the attack table was generated, we will have missed
    situations where a piece is undefended (or underdefended) and
    attacked by an enemy piece that is the same (or greater) value.
    
    Note: This routine only considers side to move.
    
    Also Note: this routine does not find pawns that are en prise, only
    pieces.

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    void
    
**/
{
    COOR c;
    ULONG u;
    POSITION *pos = &ctx->sPosition;
    ULONG uSide = pos->uToMove;
#ifdef DEBUG
    PIECE p;
#endif

    for (u = 1;
         u < pos->uNonPawnCount[uSide][0];
         u++)
    {
        c = pos->cNonPawns[uSide][u];
#ifdef DEBUG
        ASSERT(IS_ON_BOARD(c));
        p = pos->rgSquare[c].pPiece;
        ASSERT(p);
        ASSERT(GET_COLOR(p) == uSide);
        ASSERT(!IS_PAWN(p));
#endif
        if (_WhoControlsSquareFast(pos, c) == FLIP(uSide))
        {
            StoreEnprisePiece(pos, c);
        }
    }
}


static void 
_EvalTrappedPieces(IN OUT SEARCHER_THREAD_CONTEXT *ctx)
/**

Routine description:

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx

Return value:

    static void

**/
{
    COOR c;
    ULONG uColor;
    POSITION *pos = &ctx->sPosition;
#ifdef DEBUG
    PIECE p;
#endif

    FOREACH_COLOR(uColor)
    {
        c = pos->cTrapped[uColor];
        if (IS_ON_BOARD(c))
        {
            if (_WhoControlsSquareFast(pos, c) == FLIP(uColor))
            {
#ifdef DEBUG
                p = pos->rgSquare[c].pPiece;
                ASSERT(p);
                ASSERT(!IS_PAWN(p));
                ASSERT(GET_COLOR(p) == uColor);
#endif
                if (OPPOSITE_COLORS(uColor, pos->uToMove))
                {
                    StoreEnprisePiece(pos, c);
                }
				else
				{
                    StoreTrappedPiece(pos, c);
				}
            }
        }
    }
}


static void
_EvalBishopPairs(IN POSITION *pos)
/*++

Routine description:

    Give a bonus to sides that have a viable bishop pair based; scale
    the bonus by the number of pawns remaining on the board.

Parameters:

    IN POSITION *pos - position

Return value:

    static void

--*/
{
    ULONG uPawnSum = pos->uPawnCount[WHITE] + pos->uPawnCount[BLACK];
    FLAG fPair;
    ULONG uBishopCount = pos->uNonPawnCount[BLACK][BISHOP];
    ULONG uWhiteSqBishopCount = pos->uWhiteSqBishopCount[BLACK];
    ASSERT(uPawnSum <= 16);
    ASSERT(uBishopCount <= 10);
    ASSERT(uWhiteSqBishopCount <= 10);
    fPair = ((uBishopCount > 1) &
             (uWhiteSqBishopCount != 0) &
             (uWhiteSqBishopCount != uBishopCount));
    EVAL_TERM(BLACK,
              0,
              ILLEGAL_COOR,
              pos->iScore[BLACK],
              BISHOP_PAIR[fPair][uPawnSum],
              "bishop pair");
    uBishopCount = pos->uNonPawnCount[WHITE][BISHOP];
    uWhiteSqBishopCount = pos->uWhiteSqBishopCount[WHITE];
    ASSERT(uBishopCount <= 10);
    ASSERT(uWhiteSqBishopCount <= 10);
    fPair = ((uBishopCount > 1) &
             (uWhiteSqBishopCount != 0) &
             (uWhiteSqBishopCount != uBishopCount));
    EVAL_TERM(WHITE,
              0,
              ILLEGAL_COOR,
              pos->iScore[WHITE],
              BISHOP_PAIR[fPair][uPawnSum],
              "bishop pair");
}



SCORE 
Eval(IN SEARCHER_THREAD_CONTEXT *ctx, 
     IN SCORE iAlpha, 
     IN SCORE iBeta)
/**

Routine description:

Parameters:

    SEARCHER_THREAD_CONTEXT *ctx,
    SCORE iAlpha,
    SCORE iBeta,

Return value:

    SCORE

**/
{
    static const PEVAL_HELPER JumpTable[] =
    {
        _InvalidEvaluator,                    // EMPTY_SQUARE  (0)
        _InvalidEvaluator,                    // INVALID_PIECE (1)
        _InvalidEvaluator,                    // BLACK_PAWN    (2)
        _InvalidEvaluator,                    // WHITE_PAWN    (3)
        _EvalKnight,                          // BLACK_KNIGHT  (4)
        _EvalKnight,                          // WHITE_KNIGHT  (5)
        _EvalBishop,                          // BLACK_BISHOP  (6)
        _EvalBishop,                          // WHITE_BISHOP  (7)
        _InvalidEvaluator,                    // BLACK_ROOK    (8)
        _InvalidEvaluator,                    // WHITE_ROOK    (9)
        _InvalidEvaluator,                    // BLACK_QUEEN   (10)
        _InvalidEvaluator,                    // WHITE_QUEEN   (11)
        _InvalidEvaluator,                    // BLACK_KING    (12)
        _InvalidEvaluator                     // WHITE_KING    (13)
    };
    ULONG uDefer[2][2];
    COOR cDefer[2][2][10];
    POSITION *pos = &(ctx->sPosition);
    SCORE iScoreForSideToMove;
    SCORE iAlphaMargin, iBetaMargin;
    PAWN_HASH_ENTRY *pHash;
    COOR c;
    PIECE p;
    ULONG u;
    ULONG uColor;
    BITBOARD bb;
    FLAG fDeferred;
#ifdef EVAL_TIME
    UINT64 uTimer = SystemReadTimeStampCounter();
#endif
    ASSERT(IS_VALID_SCORE(iAlpha));
    ASSERT(IS_VALID_SCORE(iBeta));
    ASSERT(iAlpha < iBeta);
    ASSERT((pos->iMaterialBalance[WHITE] * -1) == 
           pos->iMaterialBalance[BLACK]);
    // ASSERT(!InCheck(pos, pos->uToMove));

    pos->uMinMobility[BLACK] = pos->uMinMobility[WHITE] = 100;
    pos->cTrapped[BLACK] = pos->cTrapped[WHITE] = ILLEGAL_COOR;
    
#ifdef EVAL_HASH
    //
    // Check the eval hash
    // 
    iScoreForSideToMove = ProbeEvalHash(ctx);
    if (iScoreForSideToMove != INVALID_SCORE) 
    {
        INC(ctx->sCounters.tree.u64EvalHashHits);
        goto end;
    }
#endif

    pos->iScore[BLACK] = 
        (pos->uPawnMaterial[BLACK] + pos->uNonPawnMaterial[BLACK]);
    pos->iScore[WHITE] = 
        (pos->uPawnMaterial[WHITE] + pos->uNonPawnMaterial[WHITE]);
#ifdef EVAL_DUMP
    EvalTraceClear();
    Trace("Material:\n%d\t\t%d\n", pos->iScore[WHITE], pos->iScore[BLACK]);
#endif

    //
    // Pawn eval.  Note: if fDeferred comes back as TRUE then we have
    // neither cleared nor initialized the attack tables.  This is
    // because we hope that we can get a lazy eval cutoff here and
    // save some time.  BEFORE ANY CODE BELOW TOUCHES THE ATTACK
    // TABLES IT NEEDS TO CLEAR/POPULATE THEM THOUGH!!!
    //
    pHash = _EvalPawns(ctx, &fDeferred);
    ASSERT(NULL != pHash);
    ASSERT(IS_VALID_FLAG(fDeferred));
    pos->iScore[WHITE] += pHash->iScore[WHITE];
    pos->iScore[BLACK] += pHash->iScore[BLACK];
    ASSERT(IS_VALID_SCORE(pos->iScore[WHITE] - pos->iScore[BLACK]));
#ifdef EVAL_DUMP
    Trace("After pawns:\n%d\t\t%d\n", pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    
    //
    // Look for won passers races.  Note: This code cannot trust the
    // state of the attack tables yet!
    //
    if ((pos->uNonPawnCount[WHITE][0] == 1) || 
        (pos->uNonPawnCount[BLACK][0] == 1))
    {
        (void)EvalPasserRaces(pos, pHash);
#ifdef EVAL_DUMP
        Trace("After passer races:\n%d\t\t%d\n", pos->iScore[WHITE], 
              pos->iScore[BLACK]);
#endif
    }
    
    //
    // Bad trade code.  Right.  Note: this code cannot trust the state
    // of the attack tables...
    //
    if (pos->uNonPawnMaterial[WHITE] != pos->uNonPawnMaterial[BLACK])
    {
        _EvalBadTrades(pos);
#ifdef EVAL_DUMP
        Trace("After bad trades:\n%d\t\t%d\n", pos->iScore[WHITE], 
              pos->iScore[BLACK]);
#endif
    }

    //
    // Bishop pairs.  Again... Note: this code cannot trust the state
    // of the attack tables.
    //
    _EvalBishopPairs(pos);
#ifdef EVAL_DUMP
    Trace("After bishop pair bonus:\n%d\t\t%d\n", pos->iScore[WHITE], 
          pos->iScore[BLACK]);
#endif

#ifdef LAZY_EVAL
    //
    // Compute an estimate of the score for the side to move based on the
    // eval terms we have already considered:
    // 
    //     1. Material balance
    //     2. Pawn structure bonuses/penalties (incl passers/candidates)
    //     3. Passer races are detected already
    //     4. "Bad trade" code has already run
    //     5. We've already detected unwinnable endgames
    //     6. Bishop pairs
    //
    iScoreForSideToMove = (pos->iScore[pos->uToMove] - 
                           pos->iScore[FLIP(pos->uToMove)]);
    ASSERT(IS_VALID_SCORE(iScoreForSideToMove));

    //
    // Eval has not considered several potentially large terms:
    // 
    //     1. Piece positional components (such as mobility, trapped)
    //     2. King safety penalties
    //     3. Other miscellaneous bonuses/penalties
    //
    // We build two "margins" to account for these components of the
    // score.
    // 
    iAlphaMargin = iBetaMargin = (50 + (SCORE)ctx->uPositional);
    
    //
    // If (score + alpha_margin) is already > alpha -OR-
    //    (score - beta_margin) is already < beta 
    // 
    // ...then we can stop thinking about lazy eval; the rest of the
    // computation only increases the margin so we know LE will fail
    // and can save some work here.
    // 
    if ((iScoreForSideToMove + iAlphaMargin < iAlpha) ||
        (iScoreForSideToMove - iBetaMargin > iBeta))
    {
        //
        // Ok, we can't say for sure that we won't take a lazy exit
        // yet.  So do the expensive part of lazy eval estimation and
        // widen the margin further for king safety issues and passed
        // pawns.
        // 
        _QuicklyEstimateKingSafetyTerm(pos, &iAlphaMargin, &iBetaMargin);
        _QuicklyEstimatePasserBonuses(pos, pHash, &iAlphaMargin, &iBetaMargin);
        
        if ((iScoreForSideToMove + iAlphaMargin <= iAlpha) ||
            (iScoreForSideToMove - iBetaMargin >= iBeta))
        {
            ctx->uPositional = MINU(200, ctx->uPositional);
            INC(ctx->sCounters.tree.u64LazyEvals);
            goto end;
        }
    }
#endif

    //
    // If we have to clear/populate the attack table, do it now that
    // we know we aren't taking a lazy exit.
    //
    if (TRUE == fDeferred)
    {
        _PopulatePawnAttackBits(pos);
    }
    
    //
    // Pre-compute some common terms used in per-piece evals:
    //
    // This is a scaler based on the size of the army for each side.
    //
    pos->uArmyScaler[BLACK] = pos->uNonPawnMaterial[BLACK] - VALUE_KING;
    pos->uArmyScaler[WHITE] = pos->uNonPawnMaterial[WHITE] - VALUE_KING;
    pos->uArmyScaler[BLACK] /= VALUE_PAWN;
    pos->uArmyScaler[WHITE] /= VALUE_PAWN;
    ASSERT(!(pos->uArmyScaler[BLACK] & 0x80000000));
    ASSERT(!(pos->uArmyScaler[WHITE] & 0x80000000));
    pos->uArmyScaler[BLACK] = MINU(31, pos->uArmyScaler[BLACK]);
    pos->uArmyScaler[WHITE] = MINU(31, pos->uArmyScaler[WHITE]);
    ASSERT(pos->uArmyScaler[BLACK] >= 0);
    ASSERT(pos->uArmyScaler[BLACK] <= 31);
    ASSERT(pos->uArmyScaler[WHITE] >= 0);
    ASSERT(pos->uArmyScaler[WHITE] <= 31);
    pos->iReducedMaterialDownScaler[BLACK] = 
        pos->iReducedMaterialDownScaler[WHITE] = 0;

    //
    // This is a "position is closed|open" number.
    //
    pos->uClosedScaler = (pos->uPawnCount[WHITE] + pos->uPawnCount[BLACK]) -
        (pHash->uNumUnmovedPawns[WHITE] + pHash->uNumUnmovedPawns[BLACK]) +
        (pHash->uNumRammedPawns);
    ASSERT(pos->uClosedScaler >= 0);
    ASSERT(pos->uClosedScaler <= 32);

    //
    // Evaluate individual pieces.
    //
    pos->uMinorsAtHome[BLACK] = pos->uMinorsAtHome[WHITE] = 0;
    uDefer[BLACK][0] = uDefer[BLACK][1] =
        uDefer[WHITE][0] = uDefer[WHITE][1] = 0;
    uColor = pos->uToMove;
    for (u = 1;                               // skips the king
         u < pos->uNonPawnCount[uColor][0];
         u++)
    {
        c = pos->cNonPawns[uColor][u];
        ASSERT(IS_ON_BOARD(c));
        p = pos->rgSquare[c].pPiece;
        ASSERT(IS_VALID_PIECE(p));
        ASSERT(GET_COLOR(p) == uColor);
        ASSERT(!IS_PAWN(p) && !IS_KING(p));

        if (p & 0x4)
        {
            ASSERT(IS_BISHOP(p) || IS_KNIGHT(p));
            
            //
            // Note: The side on move's pieces of value X are
            // evaluated and have their mobility counted and added to
            // the attack table before the other side's pieces.  Also,
            // we do not care if JumpTable tells us this piece is in
            // danger b/c we will find out later on in
            // EvalLookForDanger anyway.
            //
            ASSERT(JumpTable[p]);
            (void)(JumpTable[p])(pos, c, pHash);
#ifdef EVAL_DUMP
            Trace("After %s:\n%d\t\t%d\n", PieceAbbrev(p),
                  pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
        }
        else
        {
            ASSERT(IS_ROOK(p) || IS_QUEEN(p));
            cDefer[uColor][IS_QUEEN(p)][uDefer[uColor][IS_QUEEN(p)]++] = c;
        }
    }
    
    uColor = FLIP(uColor);
    ASSERT(uColor != pos->uToMove);
    for (u = 1;
         u < pos->uNonPawnCount[uColor][0];
         u++)
    {
        c = pos->cNonPawns[uColor][u];
        ASSERT(IS_ON_BOARD(c));
        p = pos->rgSquare[c].pPiece;
        ASSERT(IS_VALID_PIECE(p));
        ASSERT(GET_COLOR(p) == uColor);
        ASSERT(!IS_PAWN(p) && !IS_KING(p));
        if (p & 0x4)
        {
            //
            // This time we are evaluating a piece from the side not
            // on move.  We will not go back later and look at this
            // piece again so if JumpTable tells us it's in danger
            // then record it.  The drawback, of course, is that the
            // JumpTable routines only detect danger if a piece is
            // attacked by an enemy of lesser value!
            //
            ASSERT(JumpTable[p]);
            (void)(JumpTable[p])(pos, c, pHash);
#ifdef EVAL_DUMP
            Trace("After %s:\n%d\t\t%d\n", PieceAbbrev(p),
                  pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
        }
        else
        {
            ASSERT(IS_ROOK(p) || IS_QUEEN(p));
            cDefer[uColor][IS_QUEEN(p)][uDefer[uColor][IS_QUEEN(p)]++] = c;
        }
    }
    
    //
    // Evaluate any rook(s) for side on move then for side not on move.
    // 
    uColor = FLIP(uColor);
    for (u = 0;
         u < uDefer[uColor][0];
         u++)
    {
        c = cDefer[uColor][0][u];
        ASSERT(IS_ON_BOARD(c));
        ASSERT(IS_ROOK(pos->rgSquare[c].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[c].pPiece) == uColor);
        _EvalRook(pos, c, pHash);
#ifdef EVAL_DUMP
        p = BLACK_ROOK | uColor;
        Trace("After %s:\n%d\t\t%d\n", PieceAbbrev(p),
              pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    }
    
    uColor = FLIP(uColor);
    for (u = 0;
         u < uDefer[uColor][0];
         u++)
    {
        c = cDefer[uColor][0][u];
        ASSERT(IS_ON_BOARD(c));
        ASSERT(IS_ROOK(pos->rgSquare[c].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[c].pPiece) == uColor);
        _EvalRook(pos, c, pHash);
#ifdef EVAL_DUMP
        p = BLACK_ROOK | uColor;
        Trace("After %s:\n%d\t\t%d\n", PieceAbbrev(p),
              pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    }

    //
    // Evaluate any queen(s) for side on move then for side not on move.
    // 
    uColor = FLIP(uColor);
    for (u = 0;
         u < uDefer[uColor][1];
         u++)
    {
        c = cDefer[uColor][1][u];
        ASSERT(IS_ON_BOARD(c));
        ASSERT(IS_QUEEN(pos->rgSquare[c].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[c].pPiece) == uColor);
        _EvalQueen(pos, c, pHash);
#ifdef EVAL_DUMP
        p = BLACK_ROOK | uColor;
        Trace("After %s:\n%d\t\t%d\n", PieceAbbrev(p),
              pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    }
    
    uColor = FLIP(uColor);
    for (u = 0;
         u < uDefer[uColor][1];
         u++)
    {
        c = cDefer[uColor][1][u];
        ASSERT(IS_ON_BOARD(c));
        ASSERT(IS_QUEEN(pos->rgSquare[c].pPiece));
        ASSERT(GET_COLOR(pos->rgSquare[c].pPiece) == uColor);
        _EvalQueen(pos, c, pHash);
#ifdef EVAL_DUMP
        p = BLACK_ROOK | uColor;
        Trace("After %s:\n%d\t\t%d\n", PieceAbbrev(p),
              pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    }
    
    //
    // Evaluate the two kings last.
    //
    c = pos->cNonPawns[BLACK][0];
#ifdef DEBUG
    ASSERT(IS_ON_BOARD(c));
    p = pos->rgSquare[c].pPiece;
    ASSERT(IS_VALID_PIECE(p));
    ASSERT(GET_COLOR(p) == BLACK);
    ASSERT(IS_KING(p));
    ASSERT(JumpTable[p]);
#endif
    _EvalKing(pos, c, pHash);
    ctx->sPlyInfo[ctx->uPly].iKingScore[BLACK] = pos->iTempScore;
#ifdef EVAL_DUMP
    Trace("After *k:\n%d\t\t%d\n", pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    
    c = pos->cNonPawns[WHITE][0];
#ifdef DEBUG
    ASSERT(IS_ON_BOARD(c));
    p = pos->rgSquare[c].pPiece;
    ASSERT(IS_VALID_PIECE(p));
    ASSERT(GET_COLOR(p) == WHITE);
    ASSERT(IS_KING(p));
    ASSERT(JumpTable[p]);
#endif
    _EvalKing(pos, c, pHash);
    ctx->sPlyInfo[ctx->uPly].iKingScore[WHITE] = pos->iTempScore;
#ifdef EVAL_DUMP
    Trace("After .k:\n%d\t\t%d\n", pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    
    //
    // Now that we have the whole attack table generated, think about
    // passed pawns identified by the pawn eval routine again.  Also
    // see if the side not on move has a trapped piece.
    //
    bb = (pHash->bbPasserLocations[WHITE] | pHash->bbPasserLocations[BLACK]);
    if (0 != bb)
    {
        _EvalPassers(pos, pHash);
#ifdef EVAL_DUMP
        Trace("After passers:\n%d\t\t%d\n", pos->iScore[WHITE], 
              pos->iScore[BLACK]);
#endif
    }

    //
    // Make one more pass over the piece list for the side on move now
    // that the full attack table is computed to detect
    // under/unprotected pieces en prise to more valuable enemy pieces
    // which we missed when building the attack table incrementally.
    //
    ctx->sPlyInfo[ctx->uPly].uMinMobility[BLACK] = pos->uMinMobility[BLACK];
    ctx->sPlyInfo[ctx->uPly].uMinMobility[WHITE] = pos->uMinMobility[WHITE];
    _EvalLookForDanger(ctx);
    _EvalTrappedPieces(ctx);

    //
    // B over N in the endgame with 2 pawn wings.
    // 
    if ((pos->uNonPawnCount[WHITE][0] <= 2) &&
        (pos->uNonPawnCount[BLACK][0] <= 2))
    {
        if ((pos->uNonPawnCount[WHITE][BISHOP] > 0) &&
            (pos->uNonPawnCount[BLACK][BISHOP] > 0))
        {
            if ((pos->uNonPawnCount[BLACK][0] == 2) &&
                (pos->uNonPawnCount[WHITE][0] == 2))
            {
                ASSERT(pos->uNonPawnCount[BLACK][BISHOP] == 1);
                ASSERT(pos->uNonPawnCount[WHITE][BISHOP] == 1);
                if (pos->uWhiteSqBishopCount[BLACK] +
                    pos->uWhiteSqBishopCount[WHITE] == 1)
                {
                    pos->iScore[WHITE] /= 2;
                    pos->iScore[BLACK] /= 2;
                }
            }        
        }
        else
        {
            //
            // At least one side has no bishop.  Look for positions
            // with two pawn wings where having a bishop is an
            // advantage.
            //
            bb = (pHash->bbPawnLocations[WHITE] | 
                  pHash->bbPawnLocations[BLACK]);
            ASSERT((BBFILE[A] | BBFILE[B] | BBFILE[C]) == 
                   0x0707070707070707ULL);
            ASSERT((BBFILE[F] | BBFILE[G] | BBFILE[H]) == 
                   0xe0e0e0e0e0e0e0e0ULL);
            if ((bb & 0x0707070707070707ULL) &&
                (bb & 0xe0e0e0e0e0e0e0e0ULL))
            {
                if ((pos->uNonPawnCount[BLACK][BISHOP] == 0) &&
                    (pos->uNonPawnCount[WHITE][BISHOP] > 0))
                {
                    EVAL_TERM(WHITE,
                              BISHOP,
                              0x88,
                              pos->iScore[WHITE],
                              BISHOP_OVER_KNIGHT_IN_ENDGAME *
                              pos->uNonPawnCount[WHITE][BISHOP],
                              "endgame w/ 2 pawn flanks");
                }
                else if ((pos->uNonPawnCount[BLACK][BISHOP] > 0) &&
                         (pos->uNonPawnCount[WHITE][BISHOP] == 0))
                {
                    EVAL_TERM(BLACK,
                              BISHOP,
                              0x88,
                              pos->iScore[BLACK],
                              BISHOP_OVER_KNIGHT_IN_ENDGAME *
                              pos->uNonPawnCount[BLACK][BISHOP],
                              "endgame w/ 2 pawn flanks");
                }
            }
        }
#ifdef EVAL_DUMP
        Trace("After BOOC / BvsN:\n%d\t\t%d\n", 
              pos->iScore[WHITE], pos->iScore[BLACK]);
#endif
    }

    //
    // Roll in the reduced material down scaler terms.
    //
    ASSERT(pos->iReducedMaterialDownScaler[BLACK] > -200);
    ASSERT(pos->iReducedMaterialDownScaler[BLACK] < +200);   
    iAlphaMargin = (pos->iReducedMaterialDownScaler[BLACK] *
                    (SCORE)REDUCED_MATERIAL_DOWN_SCALER[pos->uArmyScaler[BLACK]]) / 8;
    pos->iScore[BLACK] += iAlphaMargin;
    ASSERT(pos->iReducedMaterialDownScaler[WHITE] > -200);
    ASSERT(pos->iReducedMaterialDownScaler[WHITE] < +200);   
    iAlphaMargin = (pos->iReducedMaterialDownScaler[WHITE] *
                    (SCORE)REDUCED_MATERIAL_DOWN_SCALER[pos->uArmyScaler[WHITE]]) / 8;
    pos->iScore[WHITE] += iAlphaMargin;

    //
    // Almost done
    //
    iScoreForSideToMove = (pos->iScore[pos->uToMove] - 
                           pos->iScore[FLIP(pos->uToMove)]);
#ifdef EVAL_DUMP
    Trace("At the end:\n%d\t\t%d\n", pos->iScore[WHITE], 
          pos->iScore[BLACK]);
#endif
    
    //
    // TODO: detect and discourage blocked positions?
    //

    //
    // TODO: drive the score towards zero as we approach a 50 move w/o
    // progress draw.
    //

    //
    // Adjust dynamic positional component.
    //
    iAlphaMargin = abs(pos->iMaterialBalance[pos->uToMove] - iScoreForSideToMove);
    if ((ULONG)iAlphaMargin > ctx->uPositional)
    {
        ctx->uPositional = (ULONG)iAlphaMargin;
    }
    else
    {
        ctx->uPositional -= (ctx->uPositional - (ULONG)iAlphaMargin) / 4;
        ctx->uPositional = MAXU(ctx->uPositional, VALUE_PAWN);
    }
    g_Options.iLastEvalScore = iScoreForSideToMove;
    
#ifdef EVAL_TIME
    ctx->sCounters.tree.u64CyclesInEval += (SystemReadTimeStampCounter() - 
                                            uTimer);
#endif
    INC(ctx->sCounters.tree.u64FullEvals);

#ifdef EVAL_HASH
    //
    // Store in eval hash.
    //
    StoreEvalHash(ctx, iScoreForSideToMove);
#endif

 end:
    ASSERT(IS_VALID_SCORE(iScoreForSideToMove));
    ASSERT((iScoreForSideToMove > -NMATE) && 
           (iScoreForSideToMove < +NMATE));
    ASSERT(abs(ctx->sPlyInfo[ctx->uPly].iKingScore[BLACK]) < 700);
    ASSERT(abs(ctx->sPlyInfo[ctx->uPly].iKingScore[WHITE]) < 700);
    ASSERT(ctx->sPlyInfo[ctx->uPly].uMinMobility[BLACK] <= 100);
    ASSERT(ctx->sPlyInfo[ctx->uPly].uMinMobility[WHITE] <= 100);
    ASSERT((ctx->uPositional < 2500) && (ctx->uPositional >= VALUE_PAWN));
    return(iScoreForSideToMove);
}
