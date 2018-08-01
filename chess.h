/**

Copyright (c) Scott Gasch

Module Name:

    chess.h

Abstract:

Author:

    Scott Gasch (scott.gasch@gmail.com) 7 Apr 2004

Revision History:

    $Id: chess.h 354 2008-06-30 05:10:08Z scott $

**/

#ifndef CHESS_
#define CHESS_

#include "compiler.h"

//
// Datatype wrappers
//
#define MIN_CHAR                   (0x80)
#define MAX_CHAR                   (0x7f)
typedef char                       CHAR;

#define MIN_BYTE                   (0x00)
#define MAX_BYTE                   (0xff)
typedef unsigned char              BYTE;

#define MIN_UCHAR                  (0x00)
#define MAX_UCHAR                  (0xff)
typedef unsigned char              UCHAR;
#define CAN_FIT_IN_UCHAR(x)        ((x) <= MAX_UCHAR)

#define MIN_SHORT                  (0x8000)
#define MAX_SHORT                  (0x7fff)
typedef signed short               SHORT;

#define MIN_USHORT                 (0x0000)
#define MAX_USHORT                 (0xffff)
typedef unsigned short             USHORT;
#define CAN_FIT_IN_USHORT(x)       ((x) <= MAX_USHORT)

#define MIN_WORD                   (0x0000)
#define MAX_WORD                   (0xffff)
typedef unsigned short             WORD;

#define MIN_INT                    (0x80000000)
#define MAX_INT                    (0x7fffffff)
typedef signed int                 INT;

#define MIN_DWORD                  (0x00000000)
#define MAX_DWORD                  (0xffffffff)
typedef unsigned int               DWORD;

#define MIN_UINT                   (0x00000000)
#define MAX_UINT                   (0xffffffff)
typedef unsigned int               UINT;

#define MIN_ULONG                  (0x00000000)
#define MAX_ULONG                  (0xffffffff)
typedef unsigned int               ULONG;

#define MIN_INT64                  (0x8000000000000000)
#define MAX_INT64                  (0x7fffffffffffffff)
typedef signed COMPILER_LONGLONG   INT64;

#define MIN_UINT64                 (0x0000000000000000)
#define MAX_UINT64                 (0xffffffffffffffff)
typedef unsigned COMPILER_LONGLONG UINT64;

#define MIN_BITBOARD               (0x0000000000000000)
#define MAX_BITBOARD               (0xffffffffffffffff)
typedef UINT64                     BITBOARD;

#define MIN_BITV                   MIN_UINT
#define MAX_BITV                   MAX_UINT
typedef unsigned int               BITV;

#define MIN_FLAG                   MIN_UINT
#define MAX_FLAG                   MAX_UINT
typedef unsigned int               FLAG;
#define IS_VALID_FLAG(x)           (((x) == TRUE) || ((x) == FALSE))

typedef unsigned int               COOR;
typedef unsigned int               PIECE;
typedef signed int                 SCORE;

#define BIT1                              (0x1)
#define BIT2                              (0x2)
#define BIT3                              (0x4)
#define BIT4                              (0x8)
#define BIT5                             (0x10)
#define BIT6                             (0x20)
#define BIT7                             (0x40)
#define BIT8                             (0x80)
#define BIT9                            (0x100)
#define BIT10                           (0x200)
#define BIT11                           (0x400)
#define BIT12                           (0x800)
#define BIT13                          (0x1000)
#define BIT14                          (0x2000)
#define BIT15                          (0x4000)
#define BIT16                          (0x8000)
#define BIT17                         (0x10000)
#define BIT18                         (0x20000)
#define BIT19                         (0x40000)
#define BIT20                         (0x80000)
#define BIT21                        (0x100000)
#define BIT22                        (0x200000)
#define BIT23                        (0x400000)
#define BIT24                        (0x800000)
#define BIT25                       (0x1000000)
#define BIT26                       (0x2000000)
#define BIT27                       (0x4000000)
#define BIT28                       (0x8000000)
#define BIT29                      (0x10000000)
#define BIT30                      (0x20000000)
#define BIT31                      (0x40000000)
#define BIT32                      (0x80000000)

typedef struct _DLIST_ENTRY
{
    struct _DLIST_ENTRY *pFlink;
    struct _DLIST_ENTRY *pBlink;
} DLIST_ENTRY;

//
// Constants
//
#define YES                        (1)
#define NO                         (0)
#ifndef TRUE
#define TRUE                       (YES)
#endif
#ifndef FALSE
#define FALSE                      (NO)
#endif

//
// Calculate the length of an array (i.e. number of elements)
//
#define ARRAY_LENGTH(x)            (sizeof(x) / sizeof((x)[0]))
#define COMMAND(x) \
    void (x)(CHAR *szInput, ULONG argc, CHAR *argv[], POSITION *pos)
#define MB (1024 * 1024)

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//
#ifndef OFFSET_OF
#define OFFSET_OF(field, type) \
    (ULONG)(&((type *)0)->field)
#endif
#ifndef CONTAINING_STRUCT
#define CONTAINING_STRUCT(address, type, field) \
    ((type *)((BYTE *)(address) - (BYTE *)(OFFSET_OF(field, type))))
#endif

#define WHITE                      (1)
#define BLACK                      (0)
#define BAD_COLOR                  (2)
#define IS_VALID_COLOR(x)          (((x) == WHITE) || ((x) == BLACK))
#define FOREACH_COLOR(x)           for((x) = BLACK; (x) < BAD_COLOR; (x)++)
#define RANDOM_COLOR               (rand() & 1)
#define FLIP(color)                ((color) ^ 1)
#define COLOR_NAME(color)          (((color) == WHITE) ? "white" : "black")

#define MAX_MOVES_PER_PLY          (218)
#define MAX_PLY_PER_SEARCH         (64)
#define MAX_MOVES_PER_GAME         (1024)
#define SMALL_STRING_LEN_CHAR      (256)
#define MEDIUM_STRING_LEN_CHAR     (8192)
#define BIG_STRING_LEN_CHAR        (16384)

#define QUARTER_PLY                16
#define HALF_PLY                   32
#define THREE_QUARTERS_PLY         48
#define ONE_PLY                    64
#define TWO_PLY                    128
#define THREE_PLY                  192
#define FOUR_PLY                   256
#define MAX_DEPTH_PER_SEARCH       (MAX_PLY_PER_SEARCH * ONE_PLY)

#define IS_VALID_DEPTH(x)          (((x) >= 0) && \
                                   ((x) <= MAX_DEPTH_PER_SEARCH) && \
                                   (((x) & 0xfffff00f) == 0))

#define INFINITY                   (MAX_SHORT)
#define INVALID_SCORE              (INFINITY + 1)
#define IS_VALID_SCORE(x)          (((x) >= -INFINITY) && \
                                    ((x) <= +INFINITY))
#define NMATE                      (+INFINITY - 200)
#define MATED_SCORE(ply)           (-INFINITY + (ply))

#define IS_A_POWER_OF_2(x)         (((x) & (x - 1)) == 0)
//
// Program version number
//
#define VERSION                    "1.00"
#define REVISION                   "$Id: chess.h 354 2008-06-30 05:10:08Z scott $\n"

//
// Function decorators
//
#define IN
#define OUT
#define INOUT
#define UNUSED
#define NOTHING

// ----------------------------------------------------------------------
//
// PIECE:
//
// a piece (4 bits) = type (3 bits) + color (1 bit)
//
//  3   1 0
//  . . . .
// |     | |
//  type  C
//
#define KING                       (6)        // 110X
#define QUEEN                      (5)        // 101X
#define ROOK                       (4)        // 100X
#define BISHOP                     (3)        // 011X
#define KNIGHT                     (2)        // 010X
#define PAWN                       (1)        // 001X
#define EMPTY                      (0)        // 000X

// (WHATEVER << 1)
#define BLACK_PAWN                 (2)        // 0010
#define BLACK_KNIGHT               (4)        // 0100
#define BLACK_BISHOP               (6)        // 0110
#define BLACK_ROOK                 (8)        // 1000
#define BLACK_QUEEN                (10)       // 1010
#define BLACK_KING                 (12)       // 1100

// (WHATEVER << 1) | WHITE
#define WHITE_PAWN                 (3)        // 0011
#define WHITE_KNIGHT               (5)        // 0101
#define WHITE_BISHOP               (7)        // 0111
#define WHITE_ROOK                 (9)        // 1001
#define WHITE_QUEEN                (11)       // 1011
#define WHITE_KING                 (13)       // 1101

#define PIECE_TYPE(p)              ((p) >> 1)
#define PIECE_COLOR(p)             ((p) & WHITE)
#define RANDOM_PIECE               (rand() % 12) + 2;
#define IS_PAWN(p)                 (((p) & 0xE) == BLACK_PAWN)
#define IS_KNIGHT(p)               (((p) & 0xE) == BLACK_KNIGHT)
#define IS_BISHOP(p)               (((p) & 0xE) == BLACK_BISHOP)
#define IS_ROOK(p)                 (((p) & 0xE) == BLACK_ROOK)
#define IS_QUEEN(p)                (((p) & 0xE) == BLACK_QUEEN)
#define IS_KING(p)                 (((p) & 0xE) == BLACK_KING)
#define IS_KNIGHT_OR_KING(p)       (((p) & 0x6) == 0x4)
#define IS_VALID_PIECE(p)          ((PIECE_TYPE((p)) >= PAWN) && \
                                    (PIECE_TYPE((p)) <= KING))
#define IS_WHITE_PIECE(p)          ((p) & WHITE)
#define IS_BLACK_PIECE(p)          (!IS_WHITE(p))
#define OPPOSITE_COLORS(p, q)      (((p) ^ (q)) & WHITE)
#define SAME_COLOR(p,q)            (!OPPOSITE_COLORS(p, q))
#define GET_COLOR(p)               ((p) & WHITE)

// ----------------------------------------------------------------------
//
// a coordinate (8 bits) = rank + file (0x88)
//
//  7     4 3     0
//  . . . . . . . .
// |       |       |
//   rank     file
//     |        |
//     |        +--- 0-7 = file A thru file H
//     |
//     +------------ 0-7 = rank 1 thru rank 8
//
// given a coordinate, C, if C & 0x88 then it is illegal (off board)
//
//
//     0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07
//    +---------------------------------------+
//  8 | 00 |:01:| 02 |:03:| 04 |:05:| 06 |:07:| 0x00
//  7 |:10:| 11 |:12:| 13 |:14:| 15 |:16:| 17 | 0x10
//  6 | 20 |:21:| 22 |:23:| 24 |:25:| 26 |:27:| 0x20
//  5 |:30:| 31 |:32:| 33 |:34:| 35 |:36:| 37 | 0x30
//  4 | 40 |:41:| 42 |:43:| 44 |:45:| 46 |:47:| 0x40
//  3 |:50:| 51 |:52:| 53 |:54:| 55 |:56:| 57 | 0x50
//  2 | 60 |:61:| 62 |:63:| 64 |:65:| 66 |:67:| 0x60
//  1 |:70:| 71 |:72:| 73 |:74:| 75 |:76:| 77 | 0x70
//    +---------------------------------------+
//       A    B    C    D    E    F    G    H
//
#define A                          (0)
#define B                          (1)
#define C                          (2)
#define D                          (3)
#define E                          (4)
#define F                          (5)
#define G                          (6)
#define H                          (7)

#define A8                         (0x00)
#define B8                         (0x01)
#define C8                         (0x02)
#define D8                         (0x03)
#define E8                         (0x04)
#define F8                         (0x05)
#define G8                         (0x06)
#define H8                         (0x07)

#define A7                         (0x10)
#define B7                         (0x11)
#define C7                         (0x12)
#define D7                         (0x13)
#define E7                         (0x14)
#define F7                         (0x15)
#define G7                         (0x16)
#define H7                         (0x17)

#define A6                         (0x20)
#define B6                         (0x21)
#define C6                         (0x22)
#define D6                         (0x23)
#define E6                         (0x24)
#define F6                         (0x25)
#define G6                         (0x26)
#define H6                         (0x27)

#define A5                         (0x30)
#define B5                         (0x31)
#define C5                         (0x32)
#define D5                         (0x33)
#define E5                         (0x34)
#define F5                         (0x35)
#define G5                         (0x36)
#define H5                         (0x37)

#define A4                         (0x40)
#define B4                         (0x41)
#define C4                         (0x42)
#define D4                         (0x43)
#define E4                         (0x44)
#define F4                         (0x45)
#define G4                         (0x46)
#define H4                         (0x47)

#define A3                         (0x50)
#define B3                         (0x51)
#define C3                         (0x52)
#define D3                         (0x53)
#define E3                         (0x54)
#define F3                         (0x55)
#define G3                         (0x56)
#define H3                         (0x57)

#define A2                         (0x60)
#define B2                         (0x61)
#define C2                         (0x62)
#define D2                         (0x63)
#define E2                         (0x64)
#define F2                         (0x65)
#define G2                         (0x66)
#define H2                         (0x67)

#define A1                         (0x70)
#define B1                         (0x71)
#define C1                         (0x72)
#define D1                         (0x73)
#define E1                         (0x74)
#define F1                         (0x75)
#define G1                         (0x76)
#define H1                         (0x77)

// TODO: if use this anywhere that matters, consider double loop
// and unrolling the inside loop to prevent the multiple
// continue for invalid squares
#define FOREACH_SQUARE(x)          for((x) = (A8); (x) <= (H1); (x)++)

#define IS_ON_BOARD(c)             (!((c) & 0x88))
#define RANDOM_COOR                (rand() & 0x77)
#define FILE_RANK_TO_COOR(f, r)    (((8 - (r)) << 4) | (f))
#define RANK(c)                    (8 - ((c) >> 4))
#define RANK1(c)                   (((c) & 0xF0) == 0x70)
#define RANK2(c)                   (((c) & 0xF0) == 0x60)
#define RANK3(c)                   (((c) & 0xF0) == 0x50)
#define RANK4(c)                   (((c) & 0xF0) == 0x40)
#define RANK5(c)                   (((c) & 0xF0) == 0x30)
#define RANK6(c)                   (((c) & 0xF0) == 0x20)
#define RANK7(c)                   (((c) & 0xF0) == 0x10)
#define RANK8(c)                   (((c) & 0xF0) == 0x00)
#define FILE(c)                    ((c) & 0x0F)
#define FILEA(c)                   (((c) & 0x0F) == 0x00)
#define FILEB(c)                   (((c) & 0x0F) == 0x01)
#define FILEC(c)                   (((c) & 0x0F) == 0x02)
#define FILED(c)                   (((c) & 0x0F) == 0x03)
#define FILEE(c)                   (((c) & 0x0F) == 0x04)
#define FILEF(c)                   (((c) & 0x0F) == 0x05)
#define FILEG(c)                   (((c) & 0x0F) == 0x06)
#define FILEH(c)                   (((c) & 0x0F) == 0x07)
#define IS_WHITE_SQUARE_FR(f, r)   (((f) + (r) - 1) & 1)
#define IS_WHITE_SQUARE_COOR(c)    (IS_WHITE_SQUARE_FR(FILE(c), RANK(c)))
#define VALID_EP_SQUARE(c)         (((c) == ILLEGAL_COOR) || ((c) == 0) || \
                                    (RANK3(c)) || \
                                    (RANK6(c)))

// ----------------------------------------------------------------------
//
// a move (4 bytes) = from/to, moved, captured, promoted and special
//
//  31           24 23           16 15   12 11    8 7     4 3     0
//  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
// |     from      |       to      | moved | capt  | prom  | flags |
//         |               |          |       |       |
//         |               |          |       |       |
//         |               |          |       |       |
//         |               |          +-------+-------+-- type and
//         |               |                              color of
//         |               |                              piece moved
//         |               |                              captured and
//         |               |                              promote targ
//         +---------------+-- board location where
//                             move begins and ends
//
//
// Special flag asserted on special move types:
//
// if (special)
// {
//    if (moved == PAWN)
//    {
//       if (promotion)
//       {
//          move is a promote (w/ or w/o capture)
//       }
//       else if (capture)
//       {
//          move is an en passant catpure
//       }
//       else
//       {
//          move is a double jump
//       }
//    }
//    else if (moved == KING)
//    {
//       move is a castle (long or short)
//    }
//    else
//    {
//       invalid use of special flag
//    }
// }
//
typedef union _MOVE
{
    ULONG uMove;
    struct
    {
        UCHAR cFrom        : 8;
        UCHAR cTo          : 8;
        UCHAR pMoved       : 4;
        UCHAR pCaptured    : 4;
        UCHAR pPromoted    : 4;
        UCHAR bvFlags      : 4;
    };
} MOVE;

#define IS_SAME_MOVE(a, b) \
    (((a).uMove & 0x0FFFFFFF) == (((b).uMove & 0x0FFFFFFF)))

#define MAKE_MOVE(from,to,piece,cap,prom,flags) \
    (((ULONG)(from)) | \
     ((ULONG)(to) << 8) | \
     ((ULONG)(piece) << 16) | \
     ((ULONG)(cap) << 20) | \
     ((ULONG)(prom) << 24) | \
     ((ULONG)(flags) << 28))

#define MAKE_MOVE_WITH_NO_PROM_OR_FLAGS(from,to,piece,cap) \
    (((ULONG)(from)) | \
     ((ULONG)(to) << 8) | \
     ((ULONG)(piece) << 16) | \
     ((ULONG)(cap) << 20))

//
// Move flag values:
//
#define MOVE_FLAG_SPECIAL          (1)        // en pass, prom, castle, double
#define IS_SPECIAL_MOVE(mv)         \
    ((mv).uMove & 0x10000000)

#define IS_CASTLE(mv)               \
    (IS_SPECIAL_MOVE(mv) && IS_KING((mv).pMoved))

#ifdef DEBUG
#define IS_PROMOTION(mv)            \
    (IS_SPECIAL_MOVE(mv) && ((mv).pPromoted))
#else
#define IS_PROMOTION(mv)            \
    ((mv).pPromoted)
#endif

#define IS_CAPTURE_OR_PROMOTION(mv) \
    ((mv).uMove & 0x0FF00000)

#define IS_ENPASSANT(mv)            \
    (IS_SPECIAL_MOVE(mv) && (mv.pCaptured) && !IS_PROMOTION(mv))

#define IS_DOUBLE_JUMP(mv)          \
    (IS_SPECIAL_MOVE(mv) && !IS_CAPTURE_OR_PROMOTION(mv))

#define MOVE_FLAG_KILLERMATE       (2)        // killer mate
#define IS_KILLERMATE_MOVE(mv)     ((mv).uMove & 0x20000000)

#define MOVE_FLAG_ESCAPING_CHECK   (4)        // escaping check
#define IS_ESCAPING_CHECK(mv)      ((mv).uMove & 0x40000000)

#define MOVE_FLAG_CHECKING         (8)        // checking move
#define IS_CHECKING_MOVE(mv)       ((mv).uMove & 0x80000000)

// ----------------------------------------------------------------------
//
// Move list
//
#define MAX_MOVE_LIST              (MAX_MOVES_PER_GAME + MAX_PLY_PER_SEARCH)

// ----------------------------------------------------------------------

#pragma pack(1)
typedef union _ATTACK_BITV
{
    ULONG uWholeThing;
    struct
    {
        union
        {
            UCHAR uSmall;
            struct
            {
                UCHAR uNumAttacks : 3;            // 0..2
                UCHAR uKing : 1;                  // 4..7
                UCHAR uQueen : 1;
                UCHAR uRook : 1;
                UCHAR uMinor : 1;
                UCHAR uPawn : 1;
            } small;
        };

        // --------------------
        union
        {
            USHORT uBig;
            struct
            {
                USHORT uKing : 1;                  // 8..23
                USHORT uQueens : 4;
                USHORT uRooks : 4;
                USHORT uMinors : 4;
                USHORT uPawns : 2;
                USHORT uUnusedFlag1 : 1;
            } big;
        };

        // -------------------
        union
        {
            UCHAR uXray;
            struct
            {
                UCHAR uNumXrays : 3;
                UCHAR uUnusedFlag2 : 1;
                UCHAR uQueen : 1;                 // 24..28
                UCHAR uRook : 1;
                UCHAR uBishop : 1;
                UCHAR uUnusedFlag3 : 1;
            } xray;
        };
    };
}
ATTACK_BITV;

#define UNSAFE_FOR_MINOR(x) ((ULONG)((x).uWholeThing) & 0x00000080UL)
#define UNSAFE_FOR_ROOK(x)  ((ULONG)((x).uWholeThing) & 0x000000C0UL)
#define UNSAFE_FOR_QUEEN(x) ((ULONG)((x).uWholeThing) & 0x000000E0UL)

#define PAWN_BIT       0x00000080UL
#define MINOR_BIT      0x00000040UL
#define MINOR_XRAY_BIT 0x40000000UL
#define ROOK_BIT       0x00000020UL
#define ROOK_XRAY_BIT  0x20000000UL
#define QUEEN_BIT      0x00000010UL
#define QUEEN_XRAY_BIT 0x10000000UL

#define INVALID_PIECE_INDEX (17)
#define IS_VALID_PIECE_INDEX(x) ((x) < INVALID_PIECE_INDEX)

typedef union _SQUARE
{
    struct
    {
        PIECE pPiece;
        ULONG uIndex;
    };
    ATTACK_BITV bvAttacks[2];
}
SQUARE;
#pragma pack()

//
// POSITION
//
typedef struct _POSITION
{
    SQUARE rgSquare[128];                  // where the pieces are,
                                           // also, the attack table
    UINT64 u64NonPawnSig;                  // hash signature
    UINT64 u64PawnSig;                     // pawn hash signature
    ULONG uToMove;                         // whose turn?
    ULONG uFifty;                          // 50 moves w/o progress = draw
    FLAG fCastled[2];                      // record when sides have castled
    BITV bvCastleInfo;                     // who can castle how?
    COOR cEpSquare;                        // en-passant capture square

    COOR cPawns[2][8];                     // location of pawns on the board
    ULONG uPawnMaterial[2];                // pawn material of each side
    ULONG uPawnCount[2];                   // number of pawns for each side

    COOR cNonPawns[2][16];                 // location of pieces on the board
    ULONG uNonPawnMaterial[2];             // piece material of each side
    ULONG uNonPawnCount[2][8];             // number of non-pawns / type
                                           // 0 and 1 are the sum,
                                           // 2..6 are per PIECE_TYPE

    ULONG uWhiteSqBishopCount[2];          // num bishops on white squares
    SCORE iMaterialBalance[2];             // material balance

    // temporary storage space for use in eval
    COOR cTrapped[2];
    ULONG uArmyScaler[2];
    ULONG uClosedScaler;
    SCORE iScore[2];
    SCORE iReducedMaterialDownScaler[2];
    SCORE iTempScore;
    ULONG uMinMobility[2];
    COOR cPiece;
    ULONG uMinorsAtHome[2];
    BITBOARD bb;
    ULONG uPiecesPointingAtKing[2];
}
POSITION;

//
// Castling permission bitvector flags.
//
#define CASTLE_WHITE_SHORT                (1)
#define CASTLE_WHITE_LONG                 (2)
#define WHITE_CAN_CASTLE                  \
    (CASTLE_WHITE_SHORT | CASTLE_WHITE_LONG)
#define CASTLE_BLACK_SHORT                (4)
#define CASTLE_BLACK_LONG                 (8)
#define BLACK_CAN_CASTLE                  \
    (CASTLE_BLACK_SHORT | CASTLE_BLACK_LONG)
#define CASTLE_NONE_POSSIBLE              (0)
#define CASTLE_ALL_POSSIBLE               \
    (WHITE_CAN_CASTLE | BLACK_CAN_CASTLE)

// ----------------------------------------------------------------------
//
// Move stack
//
#define MAX_MOVE_STACK             (MAX_PLY_PER_SEARCH * \
                                    MAX_MOVES_PER_PLY)

//
// Main part of the move stack, triads of moves, their values and flags
// that denote where the values came from
//
#define MVF_MOVE_SEARCHED    (1)
#define MVF_EXTEND_MOVE      (2)
#define MVF_REDUCE_MOVE      (4)
#define MVF_PRUNE_SUBTREE    (8)

typedef struct _MOVE_STACK_MOVE_VALUE_FLAGS
{
    SCORE iValue;
    MOVE mv;
    BITV bvFlags;
}
MOVE_STACK_MOVE_VALUE_FLAGS;

typedef struct _KEY_POINTER
{
    int iPointer;
    ULONG uKey;
}
KEY_POINTER;

typedef union _GENERATOR_FLAGS 
{
    struct {
        UCHAR uMoveCount;
        UCHAR uKingMoveCount;
        UCHAR uCheckingPieces;
        UCHAR uUnused;
    };
    ULONG uAllGenFlags;
}
GENERATOR_FLAGS;

//
// A move stack
//
typedef struct _MOVE_STACK
{
#ifdef DEBUG
    POSITION board[MAX_PLY_PER_SEARCH];
#endif
    //
    // Unblocked squares map, used for check detection
    //
    ULONG uUnblockedKeyValue[MAX_PLY_PER_SEARCH];
    KEY_POINTER sUnblocked[MAX_PLY_PER_SEARCH][128];

    //
    // The main move list, a long series of moves, their values and some
    // flag bits to tell search what the values are based upon.
    //
    MOVE_STACK_MOVE_VALUE_FLAGS mvf[MAX_MOVE_STACK];

    //
    // uBegin[ply] and uEnd[ply] specify the start and end of moves gen-
    // erated for a position at ply distance from the root.
    //
    ULONG uPly;
    ULONG uBegin[MAX_PLY_PER_SEARCH];
    ULONG uEnd[MAX_PLY_PER_SEARCH];
    MOVE mvHash[MAX_PLY_PER_SEARCH];
    GENERATOR_FLAGS sGenFlags[MAX_PLY_PER_SEARCH];
}
MOVE_STACK;

#define GENERATE_NO_MOVES                              \
    ctx->sMoveStack.uEnd[ctx->uPly] =                  \
        ctx->sMoveStack.uBegin[ctx->uPly] + 1;         \
    ctx->sMoveStack.uBegin[ctx->uPly + 1] =            \
        ctx->sMoveStack.uEnd[ctx->uPly]

#define GENERATE_ALL_MOVES             (1)
#define GENERATE_ESCAPES               (2)
#define GENERATE_CAPTURES_PROMS_CHECKS (3)
#define GENERATE_CAPTURES_PROMS        (4)
#define GENERATE_DONT_SCORE            (5)
#ifdef TEST
#define GENERATE_ALL_MOVES_CHECK_OK    (6)
#endif

#define MOVE_COUNT(ctx, x)             \
    (((ctx)->sMoveStack.uEnd[(x)]) - (ctx)->sMoveStack.uBegin[(x)])
#define ONE_LEGAL_MOVE(ctx, x) \
    (MOVE_COUNT(ctx, x) == 1)
#define NUM_KING_MOVES(ctx, x) \
    ((ctx)->sMoveStack.sGenFlags[(x)].uKingMoveCount)
#define NUM_CHECKING_PIECES(ctx, x) \
    ((ctx)->sMoveStack.sGenFlags[(x)].uCheckingPieces)


// ----------------------------------------------------------------------
//
// Accumulators
//
typedef struct _COUNTERS
{
    struct
    {
        UINT64 u64Probes;
        UINT64 u64OverallHits;
        UINT64 u64UsefulHits;
        UINT64 u64UpperBoundHits;
        UINT64 u64LowerBoundHits;
        UINT64 u64ExactScoreHits;
    }
    hash;

    struct
    {
        UINT64 u64Probes;
        UINT64 u64Hits;
    }
    pawnhash;

    struct
    {
        UINT64 u64TotalNodeCount;
        UINT64 u64QNodeCount;
        UINT64 u64LeafCount;
        UINT64 u64TerminalPositionCount;
        UINT64 u64BetaCutoffs;
        UINT64 u64BetaCutoffsOnFirstMove;
        UINT64 u64NullMoves;
        UINT64 u64NullMoveSuccess;
#ifdef TEST_NULL
        UINT64 u64QuickNullSuccess;
        UINT64 u64QuickNullDeferredSuccess;
        UINT64 u64QuickNullFailures;
        UINT64 u64AvoidNullSuccess;
        UINT64 u64AvoidNullFailures;
#endif
        UINT64 u64EvalHashHits;
        UINT64 u64LazyEvals;
        UINT64 u64FullEvals;
        UINT64 u64CyclesInEval;
    }
    tree;

    struct
    {
        ULONG uNumSplits;
        ULONG uNumSplitsTerminated;
    }
    parallel;

    struct
    {
        ULONG uPawnPush;
        ULONG uCheck;
        ULONG uMateThreat;
        ULONG uRecapture;
        ULONG uOneLegalMove;
        ULONG uNoLegalKingMoves;
        ULONG uMultiCheck;
        ULONG uSingularMove;
        ULONG uZugzwang;
        ULONG uSingularReplyToCheck;
        ULONG uEndgame;
        ULONG uBotvinnikMarkoff;
        ULONG uQExtend;
    }
    extension;

    struct
    {
        ULONG uProbes;
        ULONG uHits;
    }
    egtb;
}
COUNTERS;

typedef struct _CUMULATIVE_SEARCH_FLAGS
{
    // search
    ULONG uNumChecks[2];                      // not used
    FLAG fInSuspiciousBranch;                 // not used
    FLAG fInReducedDepthBranch;               // not used
    FLAG fAvoidNullmove;                      // restore
    FLAG fVerifyNullmove;                     // restore

    // qsearch
    ULONG uQsearchDepth;                      // restore
    ULONG uQsearchNodes;                      // incremental
    ULONG uQsearchCheckDepth;                 // restore
    FLAG fCouldStandPat[2];                   // restore
}
CUMULATIVE_SEARCH_FLAGS;

// ----------------------------------------------------------------------
//
// Searcher thread record
//
typedef struct _SPLIT_INFO
{
    // locks / counters
    volatile ULONG uLock;                     // lock for this split node
    volatile ULONG uNumThreadsHelping;        // num threads in this node
    volatile FLAG fTerminate;                 // signal helpers to terminate

    // moves in the split node
    ULONG uRemainingMoves;                    // num moves remaining to do
    ULONG uOnDeckMove;                        // next move to do
    ULONG uNumMoves;
    ULONG uAlreadyDone;
    MOVE_STACK_MOVE_VALUE_FLAGS
        mvf[MAX_MOVES_PER_PLY];               // the moves to search

    // input to the split node
    POSITION sRootPosition;                   // the root position
    MOVE mvPathToHere[MAX_PLY_PER_SEARCH];    // path from root to split
    MOVE mvLast;                              // last move before split
    ULONG uDepth;                             // remaining depth at split
    INT iPositionExtend;                      // positional extension
    SCORE iAlpha;                             // original alpha at split
    SCORE iBeta;                              // beta at split
    ULONG uSplitPositional;                   // pos->uPositional at split
    CUMULATIVE_SEARCH_FLAGS sSearchFlags;     // flags at split time

    // output from the split node
    MOVE mvBest;                              // the best move
    SCORE iBestScore;                         // it's score
    COUNTERS sCounters;                       // updated counters
    MOVE PV[MAX_PLY_PER_SEARCH];

#ifdef DEBUG
    ULONG uSplitPly;
    POSITION sSplitPosition;
#endif
}
SPLIT_INFO;

typedef struct _PLY_INFO
{
#ifdef DEBUG
    POSITION sPosition;
    SCORE iAlpha;
    SCORE iBeta;
#endif
    SCORE iEval;
    INT iExtensionAmount;
    FLAG fInCheck;
    FLAG fInQsearch;
    MOVE mv;
    MOVE mvBest;
    MOVE PV[MAX_PLY_PER_SEARCH];

    SCORE iKingScore[2];
    ULONG uMinMobility[2];
    UINT64 u64NonPawnSig;
    UINT64 u64PawnSig;
    UINT64 u64Sig;
    ULONG uFifty;
    FLAG fCastled[2];
    ULONG uTotalNonPawns;
    BITV bvCastleInfo;
    COOR cEpSquare;
}
PLY_INFO;

#define EVAL_HASH
#ifdef EVAL_HASH
#define EVAL_HASH_TABLE_SIZE (2097152) // 32Mb (per thread)
typedef struct _EVAL_HASH_ENTRY
{
    UINT64 u64Key;
    SCORE iEval;
    ULONG uPositional;
    COOR cTrapped[2];

} EVAL_HASH_ENTRY;
#endif

#define PAWN_HASH_TABLE_SIZE (131072) // 5.5Mb (per thread)
typedef struct _PAWN_HASH_ENTRY
{
    UINT64 u64Key;
    BITBOARD bbPawnLocations[2];
    BITBOARD bbPasserLocations[2];
    BITBOARD bbStationaryPawns[2];
    SHORT iScore[2];
    UCHAR uCountPerFile[2][10];
    UCHAR uNumRammedPawns;
    UCHAR uNumUnmovedPawns[2];
}
PAWN_HASH_ENTRY;

#define NUM_SPLIT_PTRS_IN_CONTEXT (8)

typedef struct _SEARCHER_THREAD_CONTEXT
{
    ULONG uPly;                               // its distance from root
    ULONG uPositional;                        // positional component of score
    POSITION sPosition;                       // the board
    MOVE_STACK sMoveStack;                    // the move stack
    CUMULATIVE_SEARCH_FLAGS sSearchFlags;
    PLY_INFO sPlyInfo[MAX_PLY_PER_SEARCH+1];
    MOVE mvKiller[MAX_PLY_PER_SEARCH][2];
    MOVE mvKillerEscapes[MAX_PLY_PER_SEARCH][2];
    MOVE mvNullmoveRefutations[MAX_PLY_PER_SEARCH];
    COUNTERS sCounters;
    ULONG uThreadNumber;
    SPLIT_INFO *pSplitInfo[NUM_SPLIT_PTRS_IN_CONTEXT];
    MOVE mvRootMove;
    SCORE iRootScore;
    ULONG uRootDepth;
    PAWN_HASH_ENTRY rgPawnHash[PAWN_HASH_TABLE_SIZE];
#ifdef EVAL_HASH
    EVAL_HASH_ENTRY rgEvalHash[EVAL_HASH_TABLE_SIZE];
#endif
    CHAR szLastPV[SMALL_STRING_LEN_CHAR];
}
SEARCHER_THREAD_CONTEXT;

//
// When I added the pawn hash table and eval hash tables to searcher
// thread contexts they became too heavy to just allocate on the fly
// for things like checking the legality of SAN moves or seeing if an
// opening book line leads to a draw.  This
// LIGHTWEIGHT_SEARCHER_CONTEXT structure can be cast into a full
// SEARCHER_THREAD_CONTEXT and passed safely into the Generate,
// MakeMove and UnMakeMove functions because they presently only
// need:
//
//     1. uPly
//     2. sPosition
//     3. sPlyInfo
//     4. sMoveStack
//
typedef struct _LIGHTWEIGHT_SEARCHER_CONTEXT
{
    ULONG uPly;
    ULONG uPositional;
    POSITION sPosition;
    MOVE_STACK sMoveStack;
    CUMULATIVE_SEARCH_FLAGS sSearchFlags;
    PLY_INFO sPlyInfo[MAX_PLY_PER_SEARCH+1];
}
LIGHTWEIGHT_SEARCHER_CONTEXT;

// ----------------------------------------------------------------------
//
// Global game options
//
typedef struct _GAME_OPTIONS
{
    ULONG uMyClock;
    ULONG uOpponentsClock;
    FLAG fGameIsRated;
    FLAG fOpponentIsComputer;
    ULONG uSecPerMove;
    ULONG uMaxDepth;
    UINT64 u64MaxNodeCount;
    FLAG fShouldPonder;
    FLAG fPondering;
    FLAG fThinking;
    FLAG fSuccessfulPonder;
    MOVE mvPonder;
    FLAG fShouldPost;
    FLAG fForceDrawWorthZero;
    ULONG uMyIncrement;
    ULONG uMovesPerTimePeriod;
    CHAR szAnalyzeProgressReport[SMALL_STRING_LEN_CHAR];
    FLAG fShouldAnnounceOpening;
    SCORE iLastEvalScore;
    UINT64 u64NodesSearched;
    CHAR szLogfile[SMALL_STRING_LEN_CHAR];
    CHAR szEGTBPath[SMALL_STRING_LEN_CHAR];
    CHAR szBookName[SMALL_STRING_LEN_CHAR];
    ULONG uNumProcessors;
    ULONG uNumHashTableEntries;
    FLAG fNoInputThread;
    FLAG fVerbosePosting;
    FLAG fRunningUnderXboard;
    FLAG fStatusLine;
    FLAG fFastScript;
    INT iResignThreshold;

    enum
    {
        CLOCK_NORMAL               = 0,
        CLOCK_FIXED,
        CLOCK_INCREMENT,
        CLOCK_NONE
    }
    eClock;

    enum
    {
        GAME_UNKNOWN               = 0,
        GAME_BULLET,
        GAME_BLITZ,
        GAME_STANDARD
    }
    eGameType;

    enum
    {
        I_PLAY_WHITE               = 0,
        I_PLAY_BLACK,
        FORCE_MODE,
        EDIT_MODE,
        ANALYZE_MODE
    }
    ePlayMode;
}
GAME_OPTIONS;
extern GAME_OPTIONS g_Options;

// ----------------------------------------------------------------------
//
// Move timer
//

#define TIMER_SEARCHING_FIRST_MOVE       (0x1)
#define TIMER_SEARCHING_IMPORTANT_MOVE   (0x2)
#define TIMER_RESOLVING_ROOT_FL          (0x4)
#define TIMER_RESOLVING_ROOT_FH          (0x8)
#define TIMER_JUST_OUT_OF_BOOK           (0x10)
#define TIMER_CURRENT_OBVIOUS            (0x20)
#define TIMER_CURRENT_WONT_UNBLOCK       (0x40)
#define TIMER_ROOT_POSITION_CRITICAL     (0x80)
#define TIMER_MOVE_IMMEDIATELY           (0x100)
#define TIMER_MANY_ROOT_FLS              (0x200)
#define TIMER_STOPPING                   (0x400)
#define TIMER_SPLIT_FAILED               (0x800)

typedef struct _MOVE_TIMER
{
    double dStartTime;
    double dEndTime;
    double dSoftTimeLimit;
    double dHardTimeLimit;
    ULONG uNodeCheckMask;
    volatile BITV bvFlags;
}
MOVE_TIMER;

// ----------------------------------------------------------------------
//
// Special move tag literals
//
#define ILLEGALMOVE                ((ULONG)0x1DDD8888)

//
// useful macros
//
#ifdef DEBUG
void
_assert(CHAR *szFile, ULONG uLine);

#define ASSERT(x)                  if (x)    \
                                   { ; }     \
                                   else      \
                                   { _assert(__FILE__, __LINE__); }
#define VERIFY(x)                  ASSERT(x)
#else
#define ASSERT(x)                   ;
#define VERIFY(x)                  x;
#endif // DEBUG

#ifdef PERF_COUNTERS
#define INC(x)                     ((x) += 1)
#else
#define INC(x)
#endif

#define BREAKPOINT                 SystemDebugBreakpoint()

#define MIN(x, y)                  (((x) < (y)) ? (x) : (y))
#define MAX(x, y)                  (((x) > (y)) ? (x) : (y))

#ifdef _X86_
//
// Note: MAXU, MINU and ABS_DIFF require arguments with the high order
// bit CLEAR to work right.
//
// These are branchless constructs.  MAXU and MINU are equivalent to
// MIN and MAX (with the above restriction on inputs)
//
// MIN0 and MAX0 are equivalent to MAX(0, x) and MIN(0, x).  These
// macros have no restiction on argument type.
//
// ABS_DIFF is equivalent to abs(x - y).  Again, x and y must have
// their high-order bits CLEAR for this to work.
//
// Note: gcc generates code with cmovs so no need for MAXU/MINU on
// that compiler.
//
#define MINU(x, y)                 \
    (((((int)((x)-(y)))>>31) & ((x)-(y)))+(y))
#define MAXU(x, y)                 \
    (((((int)((x)-(y)))>>31) & ((y)-(x)))+(x))
#define MIN0(x)                    \
    ((x) & (((int)(x)) >> 31))
#define MAX0(x)                    \
    ((x) & ~(((int)(x)) >> 31))


#define ABS_DIFF(a, b)             \
    (((b)-(a)) - ((((b) - (a)) & (((int)((b) - (a))) >> 31) ) << 1))

#endif // _X86_

#ifndef MINU
#define MINU(x, y)                 (MIN((x), (y)))
#endif

#ifndef MAXU
#define MAXU(x, y)                 (MAX((x), (y)))
#endif

#ifndef MIN0
#define MIN0(x)                    (MIN((x), 0))
#endif

#ifndef MAX0
#define MAX0(x)                    (MAX((x), 0))
#endif

#ifndef ABS_DIFF
#define ABS_DIFF(a, b)             (abs((a) - (b)))
#endif

#define FILE_DISTANCE(a, b)        (ABS_DIFF(FILE((a)), FILE((b))))
#define RANK_DISTANCE(a, b)        (ABS_DIFF(((a) & 0xF0), ((b) & 0xF0)) >> 4)
#define REAL_DISTANCE(a, b)        (MAXU(FILE_DISTANCE((a), (b)), \
                                         RANK_DISTANCE((a), (b))))
#ifdef DEBUG
#define DISTANCE(a, b)             DistanceBetweenSquares((a), (b))
#else
#define DISTANCE(a, b)             g_pDistance[(a) - (b)]
#endif // DEBUG

#define    IS_EMPTY( square )      (!(square))
#define IS_OCCUPIED( square )      ((square))

#define IS_DEAD( listindex )       ((listindex) > 119)
#define IS_ALIVE( listindex )      ((listindex) <= 119)
#define ILLEGAL_COOR               (0x88)
#define EDGE_DISTANCE(c)           (MIN(MIN(abs(RANK(c) - 7), RANK(c)), \
                                        MIN(abs(FILE(c) - 7), FILE(c))))
#define NOT_ON_EDGE(c)             ((((c) & 0xF0) != 0x00) && \
                                       (((c) & 0xF0) != 0x70) && \
                                       (((c) & 0x0F) != 0x00) && \
                                       (((c) & 0x0F) != 0x07))
#define ON_EDGE(c)                 (RANK8(c) || \
                                    RANK1(c) || \
                                    FILEA(c) || \
                                    FILEH(c))
#define CORNER_DISTANCE(c)         (MAX(MIN((ULONG)abs(RANK(c) - 7), RANK(c)),\
                                        MIN((ULONG)abs(FILE(c) - 7), FILE(c))))
#define IN_CORNER(c)               (((c) == A8) || \
                                       ((c) == A1) || \
                                       ((c) == H8) || \
                                       ((c) == H1))
#define WHITE_CORNER_DISTANCE(c)   (MIN(DISTANCE((c), A8), \
                                        DISTANCE((c), H1)))
#define BLACK_CORNER_DISTANCE(c)   (MIN(DISTANCE((c), H8), \
                                        DISTANCE((c), A1)))
#define SYM_SQ(c)                  ((7 - (((c) & 0xF0) >> 4)) << 4) \
                                       | (7 - ((c) & 0x0F))
#define ARRAY_SIZE(a)              (sizeof((a)) / sizeof((a[0])))
#define MAKE_PSQT(a, b, c, d)      (((a) << 24) | ((b) << 16) | \
                                       ((c) << 8) | (d))
#define TO64(x)                    ((x) & 0x7) + ((0x7 - ((x)>>4)) << 3)
#define COOR_TO_BIT_NUMBER(c)      (((((c) & 0x70) >> 1) | ((c) & 0x7)))
#define SLOWCOOR_TO_BB(c)          (1ULL << COOR_TO_BIT_NUMBER(c))
#define COOR_TO_BB(c)              (BBSQUARE[COOR_TO_BIT_NUMBER(c)])
#define SLOW_BIT_NUMBER_TO_COOR(b) ((((b) / 8) << 4) + ((b) & 7))
#define BIT_NUMBER_TO_COOR(b)      ((((b) & 0xF8) << 1) | ((b) & 7))

#define BBRANK88 \
    SLOWCOOR_TO_BB(A8) | SLOWCOOR_TO_BB(B8) | SLOWCOOR_TO_BB(C8) | \
    SLOWCOOR_TO_BB(D8) | SLOWCOOR_TO_BB(E8) | SLOWCOOR_TO_BB(F8) | \
    SLOWCOOR_TO_BB(G8) | SLOWCOOR_TO_BB(H8)

#define BBRANK77 \
    SLOWCOOR_TO_BB(A7) | SLOWCOOR_TO_BB(B7) | SLOWCOOR_TO_BB(C7) | \
    SLOWCOOR_TO_BB(D7) | SLOWCOOR_TO_BB(E7) | SLOWCOOR_TO_BB(F7) | \
    SLOWCOOR_TO_BB(G7) | SLOWCOOR_TO_BB(H7)

#define BBRANK66 \
    SLOWCOOR_TO_BB(A6) | SLOWCOOR_TO_BB(B6) | SLOWCOOR_TO_BB(C6) | \
    SLOWCOOR_TO_BB(D6) | SLOWCOOR_TO_BB(E6) | SLOWCOOR_TO_BB(F6) | \
    SLOWCOOR_TO_BB(G6) | SLOWCOOR_TO_BB(H6)

#define BBRANK55 \
    SLOWCOOR_TO_BB(A5) | SLOWCOOR_TO_BB(B5) | SLOWCOOR_TO_BB(C5) | \
    SLOWCOOR_TO_BB(D5) | SLOWCOOR_TO_BB(E5) | SLOWCOOR_TO_BB(F5) | \
    SLOWCOOR_TO_BB(G5) | SLOWCOOR_TO_BB(H5)

#define BBRANK44 \
    SLOWCOOR_TO_BB(A4) | SLOWCOOR_TO_BB(B4) | SLOWCOOR_TO_BB(C4) | \
    SLOWCOOR_TO_BB(D4) | SLOWCOOR_TO_BB(E4) | SLOWCOOR_TO_BB(F4) | \
    SLOWCOOR_TO_BB(G4) | SLOWCOOR_TO_BB(H4)

#define BBRANK33 \
    SLOWCOOR_TO_BB(A3) | SLOWCOOR_TO_BB(B3) | SLOWCOOR_TO_BB(C3) | \
    SLOWCOOR_TO_BB(D3) | SLOWCOOR_TO_BB(E3) | SLOWCOOR_TO_BB(F3) | \
    SLOWCOOR_TO_BB(G3) | SLOWCOOR_TO_BB(H3)

#define BBRANK22 \
    SLOWCOOR_TO_BB(A2) | SLOWCOOR_TO_BB(B2) | SLOWCOOR_TO_BB(C2) | \
    SLOWCOOR_TO_BB(D2) | SLOWCOOR_TO_BB(E2) | SLOWCOOR_TO_BB(F2) | \
    SLOWCOOR_TO_BB(G2) | SLOWCOOR_TO_BB(H2)

#define BBRANK11 \
    SLOWCOOR_TO_BB(A1) | SLOWCOOR_TO_BB(B1) | SLOWCOOR_TO_BB(C1) | \
    SLOWCOOR_TO_BB(D1) | SLOWCOOR_TO_BB(E1) | SLOWCOOR_TO_BB(F1) | \
    SLOWCOOR_TO_BB(G1) | SLOWCOOR_TO_BB(H1)

#define BBFILEA \
    SLOWCOOR_TO_BB(A1) | SLOWCOOR_TO_BB(A2) | SLOWCOOR_TO_BB(A3) | \
    SLOWCOOR_TO_BB(A4) | SLOWCOOR_TO_BB(A5) | SLOWCOOR_TO_BB(A6) | \
    SLOWCOOR_TO_BB(A7) | SLOWCOOR_TO_BB(A8)

#define BBFILEB \
    SLOWCOOR_TO_BB(B1) | SLOWCOOR_TO_BB(B2) | SLOWCOOR_TO_BB(B3) | \
    SLOWCOOR_TO_BB(B4) | SLOWCOOR_TO_BB(B5) | SLOWCOOR_TO_BB(B6) | \
    SLOWCOOR_TO_BB(B7) | SLOWCOOR_TO_BB(B8)

#define BBFILEC \
    SLOWCOOR_TO_BB(C1) | SLOWCOOR_TO_BB(C2) | SLOWCOOR_TO_BB(C3) | \
    SLOWCOOR_TO_BB(C4) | SLOWCOOR_TO_BB(C5) | SLOWCOOR_TO_BB(C6) | \
    SLOWCOOR_TO_BB(C7) | SLOWCOOR_TO_BB(C8)

#define BBFILED \
    SLOWCOOR_TO_BB(D1) | SLOWCOOR_TO_BB(D2) | SLOWCOOR_TO_BB(D3) | \
    SLOWCOOR_TO_BB(D4) | SLOWCOOR_TO_BB(D5) | SLOWCOOR_TO_BB(D6) | \
    SLOWCOOR_TO_BB(D7) | SLOWCOOR_TO_BB(D8)

#define BBFILEE \
    SLOWCOOR_TO_BB(E1) | SLOWCOOR_TO_BB(E2) | SLOWCOOR_TO_BB(E3) | \
    SLOWCOOR_TO_BB(E4) | SLOWCOOR_TO_BB(E5) | SLOWCOOR_TO_BB(E6) | \
    SLOWCOOR_TO_BB(E7) | SLOWCOOR_TO_BB(E8)

#define BBFILEF \
    SLOWCOOR_TO_BB(F1) | SLOWCOOR_TO_BB(F2) | SLOWCOOR_TO_BB(F3) | \
    SLOWCOOR_TO_BB(F4) | SLOWCOOR_TO_BB(F5) | SLOWCOOR_TO_BB(F6) | \
    SLOWCOOR_TO_BB(F7) | SLOWCOOR_TO_BB(F8)

#define BBFILEG \
    SLOWCOOR_TO_BB(G1) | SLOWCOOR_TO_BB(G2) | SLOWCOOR_TO_BB(G3) | \
    SLOWCOOR_TO_BB(G4) | SLOWCOOR_TO_BB(G5) | SLOWCOOR_TO_BB(G6) | \
    SLOWCOOR_TO_BB(G7) | SLOWCOOR_TO_BB(G8)

#define BBFILEH \
    SLOWCOOR_TO_BB(H1) | SLOWCOOR_TO_BB(H2) | SLOWCOOR_TO_BB(H3) | \
    SLOWCOOR_TO_BB(H4) | SLOWCOOR_TO_BB(H5) | SLOWCOOR_TO_BB(H6) | \
    SLOWCOOR_TO_BB(H7) | SLOWCOOR_TO_BB(H8)

#define BBRANK72 \
    BBRANK77 | BBRANK66 | BBRANK55 | BBRANK44 | BBRANK33 | BBRANK22

#define BBRANK62 \
    BBRANK66 | BBRANK55 | BBRANK44 | BBRANK33 | BBRANK22

#define BBRANK52 \
    BBRANK55 | BBRANK44 | BBRANK33 | BBRANK22

#define BBRANK42 \
    BBRANK44 | BBRANK33 | BBRANK22

#define BBRANK32 \
    BBRANK33 | BBRANK22

#define BBRANK27 \
    BBRANK22 | BBRANK33 | BBRANK44 | BBRANK55 | BBRANK66 | BBRANK77

#define BBRANK37 \
    BBRANK33 | BBRANK44 | BBRANK55 | BBRANK66 | BBRANK77

#define BBRANK47 \
    BBRANK44 | BBRANK55 | BBRANK66 | BBRANK77

#define BBRANK57 \
    BBRANK55 | BBRANK66 | BBRANK77

#define BBRANK67 \
    BBRANK66 | BBRANK77

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//
// util.c
//
COMMAND(LearnPsqtFromPgn);

COMMAND(GeneratePositionAndBestMoveSuite);

CHAR *
WalkPV(SEARCHER_THREAD_CONTEXT *ctx);

CHAR *
ReadNextGameFromPgnFile(FILE *pf);

COOR
DistanceBetweenSquares(COOR a, COOR b);

void CDECL
Trace(CHAR *szMessage, ...);

void CDECL
Log(CHAR *szMessage, ...);

void CDECL
Bug(CHAR *szMessage, ...);

char *
FindChunk(char *sz, ULONG uTargetChunk);

FLAG
BufferIsZeroed(BYTE *p, ULONG u);

char *
ColorToString(ULONG u);

char *
CoorToString(COOR c);

char *
ScoreToString(SCORE iScore);

char *
TimeToString(double d);

ULONG
AcquireSpinLock(volatile ULONG *pSpinlock);

FLAG
TryAcquireSpinLock(volatile ULONG *pSpinlock);

void
ReleaseSpinLock(volatile ULONG *pSpinLock);

BYTE
Checksum(BYTE *p, ULONG uSize);

FLAG
BackupFile(CHAR *szFile);

void
UtilPrintPV(SEARCHER_THREAD_CONTEXT *ctx,
            SCORE iAlpha,
            SCORE iBeta,
            SCORE iScore,
            MOVE mv);

#define CANNOT_INITIALIZE_SPLIT           (1)
#define INCONSISTENT_POSITION             (2)
#define UNEXPECTED_SYSTEM_CALL_FAILURE    (3)
#define SHOULD_NOT_GET_HERE               (4)
#define GOT_ILLEGAL_MOVE_WHILE_PONDERING  (5)
#define CANNOT_OFFICIALLY_MAKE_MOVE       (6)
#define DETECTED_INCORRECT_INITIALIZATION (7)
#define INCONSISTENT_STATE                (8)
#define FATAL_ACCESS_DENIED               (9)
#define TESTCASE_FAILURE                  (10)
#define INITIALIZATION_FAILURE            (11)

void
UtilPanic(ULONG uPanicCode,
          POSITION *pos,
          void *a1,
          void *a2,
          void *a3,
          char *file, ULONG line);


//
// main.c
//
#define LOGFILE_NAME                "typhoon.log"
extern FILE *g_pfLogfile;

void
Banner(void);

FLAG
PreGameReset(FLAG fResetBoard);

//
// system dependent exports (see win32.c or fbsd.c)
//
typedef ULONG (THREAD_ENTRYPOINT)(ULONG);

#ifdef DEBUG
ULONG
GetHeapMemoryUsage(void);
#endif

void
SystemDebugBreakpoint(void);

CHAR *
SystemStrDup(CHAR *p);

double
SystemTimeStamp(void);

FLAG
SystemDoesFileExist(CHAR *szFilename);

void
SystemDeferExecution(ULONG uMs);

void
SystemFreeMemory(void *pMem);

void *
SystemAllocateMemory(ULONG dwSizeBytes);

FLAG
SystemMakeMemoryReadOnly(void *pMemory, ULONG dwSizeBytes);

FLAG
SystemMakeMemoryNoAccess(void *pMemory, ULONG dwSizeBytes);

FLAG
SystemMakeMemoryReadWrite(void *pMemory, ULONG dwSizeBytes);

FLAG
SystemDependentInitialization(void);

UINT64 FASTCALL
SystemReadTimeStampCounter(void);

//
// This routine _must_ implement a full memory fence -- it is used in
// util.c to implement spinlocks.
//
FLAG
SystemCreateThread(THREAD_ENTRYPOINT *pEntry, ULONG uParam, ULONG *puHandle);

FLAG
SystemWaitForThreadToExit(ULONG uThreadHandle);

FLAG
SystemGetThreadExitCode(ULONG uThreadHandle, ULONG *puCode);

FLAG
SystemDeleteThread(ULONG uThreadHandle);

CHAR *
SystemGetDateString(void);

CHAR *
SystemGetTimeString(void);

FLAG
SystemCopyFile(CHAR *szSource, CHAR *szDest);

FLAG
SystemDeleteFile(CHAR *szFile);

ULONG
SystemCreateLock(void);

FLAG
SystemDeleteLock(ULONG u);

FLAG
SystemBlockingWaitForLock(ULONG u);

FLAG
SystemReleaseLock(ULONG u);

ULONG
SystemCreateSemaphore(ULONG u);

FLAG
SystemDeleteSemaphore(ULONG u);

void
SystemReleaseSemaphoreResource(ULONG u);

void
SystemObtainSemaphoreResource(ULONG u);


//
// fen.c
//
#define STARTING_POSITION_IN_FEN \
    "rnbqkbnr/pppppppp/--------/8/8/--------/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

FLAG
LooksLikeFen(char *szFen);

char *
PositionToFen(POSITION *p);

FLAG
FenToPosition(POSITION *p, char *szFen);

//
// testfen.c
//
#ifdef TEST
void
TestFenCode(void);
#endif

//
// piece.c
//
// INVERTED VALUE == (VALUE_QUEEN / VALUE_PIECE) * VALUE_PAWN;
#define VALUE_PAWN            100
#define INVERT_PAWN           900
#define VALUE_KNIGHT          300
#define INVERT_KNIGHT         300
#define VALUE_BISHOP          300
#define INVERT_BISHOP         300
#define VALUE_ROOK            500
#define INVERT_ROOK           180
#define VALUE_QUEEN           975
#define INVERT_QUEEN          100
#define VALUE_KING            (INFINITY)
#define INVERT_KING           1

#define VALUE_FULL_ARMY       (8 * VALUE_PAWN) + (2 * VALUE_KNIGHT) + \
                              (2 * VALUE_BISHOP) + (2 * VALUE_ROOK) + \
                              VALUE_QUEEN + VALUE_KING
#define VALUE_MAX_ARMY        (9 * VALUE_QUEEN) + (2 * VALUE_KNIGHT) + \
                              (2 * VALUE_BISHOP) + (2 * VALUE_ROOK) + \
                              VALUE_KING

typedef struct _PIECE_DATA
{
    ULONG uValue;
    ULONG uValueOver100;
    ULONG uInvertedValue;
    CHAR *szName;
} PIECE_DATA;

extern PIECE_DATA g_PieceData[8];

#define PIECE_VALUE_OVER_100(p) (g_PieceData[PIECE_TYPE(p)].uValueOver100)
extern ULONG
PieceValueOver100(PIECE p);

#define PIECE_VALUE(p)          (g_PieceData[PIECE_TYPE(p)].uValue)
extern ULONG
PieceValue(PIECE p);

#define INVERTED_PIECE_VALUE(p) (g_PieceData[PIECE_TYPE(p)].uInvertedValue)
extern ULONG
PieceInvertedValue(PIECE p);

CHAR *
PieceAbbrev(PIECE p);

//
// board.c
//
FLAG
VerifyPositionConsistency(POSITION *pos, FLAG fContinueOnError);

FLAG
PositionsAreEquivalent(POSITION *p1, POSITION *p2);

CHAR *
DrawTextBoardFromPosition(POSITION *pos);

void
DumpPosition(POSITION *pos);

CHAR *
CastleInfoString(BITV bv);

void
SetRootToInitialPosition(void);

//
// move.c
//
void
SlidePiece(POSITION *pos, COOR cFrom, COOR cTo);

PIECE
LiftPiece(POSITION *pos, COOR cSquare);

void
PlacePiece(POSITION *pos, COOR cSquare, PIECE pPiece);

FLAG
MakeMove(SEARCHER_THREAD_CONTEXT *ctx,
         MOVE mv);

void
UnmakeMove(SEARCHER_THREAD_CONTEXT *ctx,
           MOVE mv);

FLAG
MakeUserMove(SEARCHER_THREAD_CONTEXT *ctx, MOVE mvUser);

void
DumpMove(ULONG u);

//
// movesup.c
//
#define MOVE_TO_INDEX(mv)  (((mv).uMove & 0xFFFF) + \
                            (0x10000 * GET_COLOR(mv.pMoved)))

COOR
FasterExposesCheck(POSITION *pos,
                   COOR cRemove,
                   COOR cLocation);

COOR
ExposesCheck(POSITION *pos,
             COOR cRemove,
             COOR cLocation);

COOR
ExposesCheckEp(POSITION *pos,
               COOR cTest,
               COOR cIgnore,
               COOR cBlock,
               COOR cKing);

FLAG
IsAttacked(COOR cTest, POSITION *pos, ULONG uSide);

FLAG
InCheck(POSITION *pos, ULONG uSide);

FLAG
SanityCheckMove(POSITION *pos, MOVE mv);

FLAG
LooksLikeFile(CHAR c);

FLAG
LooksLikeRank(CHAR c);

FLAG
LooksLikeCoor(CHAR *szData);

CHAR *
StripMove(CHAR *szMove);

ULONG
LooksLikeMove(CHAR *szData);

void FASTCALL
SelectBestWithHistory(SEARCHER_THREAD_CONTEXT *ctx, ULONG u);

void FASTCALL
SelectBestNoHistory(SEARCHER_THREAD_CONTEXT *ctx, ULONG u);

void FASTCALL
SelectMoveAtRoot(SEARCHER_THREAD_CONTEXT *ctx, ULONG u);

#define NOT_MOVE 0
#define MOVE_ICS 1
#define MOVE_SAN 2

COMMAND(PerftCommand);


//
// testmove.c
//
#ifdef TEST
void
TestLiftPlaceSlidePiece(void);

void
TestExposesCheck(void);

void
TestIsAttacked(void);

void
TestMakeUnmakeMove(void);
#endif

//
// generate.c
//
#define SORT_THESE_FIRST (0x40000000)
#define FIRST_KILLER     (0x20000000)
#define SECOND_KILLER    (0x10000000)
#define THIRD_KILLER     (0x08000000)
#define FOURTH_KILLER    (0x04000000)
#define GOOD_MOVE        (0x02000000)
#define STRIP_OFF_FLAGS  (0x00FFFFFF)

extern const int g_iQKDeltas[9];
extern const int g_iNDeltas[9];
extern const int g_iBDeltas[5];
extern const int g_iRDeltas[5];

void
GenerateMoves(SEARCHER_THREAD_CONTEXT *ctx,
              MOVE mvOrderFirst,
              ULONG uType);

FLAG
WouldGiveCheck(IN SEARCHER_THREAD_CONTEXT *ctx,
               IN MOVE mv);

//
// testgenerate.c
//
#ifdef TEST

void
PlyTest(SEARCHER_THREAD_CONTEXT *ctx,
        ULONG uDepth,
        FLAG fRootPositionInCheck);

void
TestMoveGenerator(void);

void
TestLegalMoveGenerator(void);

#endif

//
// sig.c
//
extern UINT64 g_u64SigSeeds[128][7][2];
extern UINT64 g_u64PawnSigSeeds[128][2];
extern UINT64 g_u64CastleSigSeeds[16];
extern UINT64 g_u64EpSigSeeds[9];

void
InitializeSigSystem(void);

UINT64
ComputePawnSig(POSITION *pos);

UINT64
ComputeSig(POSITION *pos);

//
// mersenne.c
//
void
seedMT(unsigned int seed);

unsigned int
reloadMT(void);

unsigned int
randomMT(void);

//
// data.c
//
typedef struct _VECTOR_DELTA
{
    UCHAR iVector[2];
    signed char iDelta;
    signed char iNegDelta;
}
VECTOR_DELTA;

extern ULONG g_uDistance[256];
extern ULONG *g_pDistance;
extern VECTOR_DELTA g_VectorDelta[256];
extern VECTOR_DELTA *g_pVectorDelta;
extern CHAR g_SwapTable[14][32][32];
extern SCORE _PSQT[14][128];
extern ULONG g_uSearchSortLimits[];
extern MOVE NULLMOVE;
extern MOVE HASHMOVE;
extern MOVE RECOGNMOVE;
extern MOVE DRAWMOVE;
extern MOVE MATEMOVE;
extern FLAG g_fIsWhiteSquare[128];
extern BITBOARD BBFILE[8];
extern BITBOARD BBRANK[9];
extern BITBOARD BBWHITESQ;
extern BITBOARD BBBLACKSQ;
extern BITBOARD BBLEFTSIDE;
extern BITBOARD BBRIGHTSIDE;
extern BITBOARD BBSQUARE[64];
extern BITBOARD BBROOK_PAWNS;
extern BITBOARD BBPRECEEDING_RANKS[8][2];
extern BITBOARD BBADJACENT_FILES[8];
extern BITBOARD BBADJACENT_RANKS[9];

void
InitializeWhiteSquaresTable(void);

void
InitializeVectorDeltaTable(void);

void
InitializeSwapTable(void);

void
InitializeDistanceTable(void);

void
InitializeSearchDepthArray(void);

ULONG
GetSearchSortLimit(ULONG);

#ifdef DEBUG
#define SEARCH_SORT_LIMIT(x)    (GetSearchSortLimit((x)))
#else
#define SEARCH_SORT_LIMIT(x)    (g_uSearchSortLimits[(x)])
#endif

#ifdef DEBUG
ULONG CheckVectorWithIndex(int i, ULONG uColor);
#define CHECK_VECTOR_WITH_INDEX(i, color) \
    CheckVectorWithIndex(i, color)

int DirectionBetweenSquaresWithIndex(int i);
#define CHECK_DELTA_WITH_INDEX(i) \
    DirectionBetweenSquaresWithIndex(i)

int DirectionBetweenSquaresFromTo(COOR, COOR);
#define DIRECTION_BETWEEN_SQUARES(from, to) \
    DirectionBetweenSquaresFromTo(from, to)

int NegativeDirectionBetweenSquaresWithIndex(int i);
#define NEG_DELTA_WITH_INDEX(i) \
    NegativeDirectionBetweenSquaresWithIndex(i)

FLAG IsSquareWhite(COOR c);
#define IS_SQUARE_WHITE(c) \
    IsSquareWhite(c)

#else
#define CHECK_VECTOR_WITH_INDEX(i, color) \
    (g_pVectorDelta[(i)].iVector[(color)])

#define CHECK_DELTA_WITH_INDEX(i) \
    (g_pVectorDelta[(i)].iDelta)

#define DIRECTION_BETWEEN_SQUARES(cFrom, cTo) \
    CHECK_DELTA_WITH_INDEX((int)(cFrom) - (int)(cTo))

#define NEG_DELTA_WITH_INDEX(i) \
    (g_pVectorDelta[(i)].iNegDelta)

#define IS_SQUARE_WHITE(c) \
    (g_fIsWhiteSquare[(c)])
#endif

//
// san.c
//
MOVE
ParseMoveSan(CHAR *szInput,
             POSITION *pos);

CHAR *
MoveToSan(MOVE mv, POSITION *pos);

//
// testsan.c
//
void
TestSan(void);

//
// list.c
//
void
InitializeListHead(IN DLIST_ENTRY *pListHead);

FLAG
IsListEmpty(IN DLIST_ENTRY *pListHead);

FLAG
RemoveEntryList(IN DLIST_ENTRY *pEntry);

DLIST_ENTRY *
RemoveHeadList(IN DLIST_ENTRY *pListHead);

DLIST_ENTRY *
RemoveTailList(IN DLIST_ENTRY *pListHead);

void
InsertTailList(IN DLIST_ENTRY *pListHead,
               IN DLIST_ENTRY *pEntry);

void
InsertHeadList(IN DLIST_ENTRY *pListHead,
               IN DLIST_ENTRY *pEntry);

//
// command.c
//
void
ParseUserInput(FLAG fSearching);

FLAG
InitializeCommandSystem(void);

void
CleanupCommandSystem(void);

//
// input.c
//
void
InitInputSystemInBatchMode(void);

ULONG
InitInputSystemWithDedicatedThread(void);

void
PushNewInput(CHAR *buf);

CHAR *
PeekNextInput(void);

CHAR *
ReadNextInput(void);

CHAR *
BlockingReadInput(void);

ULONG
NumberOfPendingInputEvents(void);

volatile extern FLAG g_fExitProgram;

//
// ics.c
//
MOVE
ParseMoveIcs(CHAR *szInput, POSITION *pos);

CHAR *
MoveToIcs(MOVE mv);

//
// testics.c
//
void
TestIcs(void);

//
// gamelist.c
//
typedef enum _ERESULT
{
    RESULT_BLACK_WON = -1,
    RESULT_DRAW = 0,
    RESULT_WHITE_WON = 1,
    RESULT_IN_PROGRESS,
    RESULT_ABANDONED,
    RESULT_UNKNOWN
} ERESULT;

typedef struct _GAME_RESULT
{
    ERESULT eResult;
    CHAR szDescription[256];
} GAME_RESULT;

typedef struct _GAME_PLAYER
{
    CHAR *szName;
    CHAR *szDescription;
    FLAG fIsComputer;
    ULONG uRating;
}
GAME_PLAYER;

typedef struct _GAME_HEADER
{
    CHAR *szGameDescription;
    CHAR *szLocation;
    GAME_PLAYER sPlayer[2];
    FLAG fGameIsRated;
    UINT64 u64OpeningSig;
    GAME_RESULT result;
    CHAR *szInitialFen;
}
GAME_HEADER;

typedef struct _GAME_MOVE
{
    DLIST_ENTRY links;
    ULONG uNumber;
    MOVE mv;
    CHAR *szComment;
    CHAR *szDecoration;
    CHAR *szMoveInSan;
    CHAR *szMoveInIcs;
    CHAR *szUndoPositionFen;
    SCORE iMoveScore;
    UINT64 u64PositionSigAfterMove;
    UINT64 u64PositionSigBeforeMove;
    volatile FLAG fInUse;
}
GAME_MOVE;

typedef struct _GAME_DATA
{
    GAME_HEADER sHeader;
    DLIST_ENTRY sMoveList;
}
GAME_DATA;

extern GAME_DATA g_GameData;

POSITION *
GetRootPosition(void);

FLAG
SetRootPosition(CHAR *szFen);

void
ResetGameList(void);

ULONG
GetMoveNumber(ULONG uColor);

void
SetGameResultAndDescription(GAME_RESULT result);

GAME_RESULT
GetGameResult(void);

void
SetMyName(void);

void
SetOpponentsName(CHAR *sz);

void
SetMyRating(ULONG u);

void
SetOpponentsRating(ULONG u);

void
DumpGameList(void);

void
TellGamelistThatIPlayColor(ULONG u);

void
DumpPgn(void);

FLAG
LoadPgn(CHAR *szPgn);

ULONG
CountOccurrancesOfSigInOfficialGameList(UINT64 u64Sig);

FLAG
DoesSigAppearInOfficialGameList(UINT64 u64Sig);

FLAG
OfficiallyTakebackMove(void);

FLAG
OfficiallyMakeMove(MOVE mv, SCORE iMoveScore, FLAG fFast);

GAME_MOVE *
GetNthOfficialMoveRecord(ULONG n);

void
MakeStatusLine(void);

FLAG
IsLegalDrawByRepetition(void);

//
// script.c
//
COMMAND(ScriptCommand);

COMMAND(SolutionCommand);

COMMAND(AvoidCommand);

COMMAND(IdCommand);

void
PostMoveTestSuiteReport(SEARCHER_THREAD_CONTEXT *);

FLAG
CheckTestSuiteMove(MOVE mv, SCORE iScore, ULONG uDepth);

FLAG
WeAreRunningASuite(void);


//
// vars.c
//
COMMAND(SetCommand);

//
// root.c
//
extern ULONG g_uSoftExtendLimit;
extern ULONG g_uHardExtendLimit;
extern volatile MOVE_TIMER g_MoveTimer;
extern ULONG g_uExtensionReduction[MAX_PLY_PER_SEARCH];

#ifdef PERF_COUNTERS
#define KEEP_TRACK_OF_FIRST_MOVE_FHs(x)                \
    ctx->sCounters.tree.u64BetaCutoffs++;              \
    if (x)                                             \
    {                                                  \
       ctx->sCounters.tree.u64BetaCutoffsOnFirstMove++;\
    }
#else
#define KEEP_TRACK_OF_FIRST_MOVE_FHs(x)
#endif

#define GAME_NOT_OVER 0
#define GAME_WHITE_WON 1
#define GAME_BLACK_WON 2
#define GAME_DRAW_STALEMATE 3
#define GAME_DRAW_REPETITION 4
#define GAME_ONE_LEGAL_MOVE 5
#define GAME_DRAW_FIFTY_MOVES_WO_PROGRESS 6

GAME_RESULT
Think(POSITION *pos);

GAME_RESULT
Ponder(POSITION *pos);

GAME_RESULT
Iterate(SEARCHER_THREAD_CONTEXT *ctx);

void
SetMoveTimerForSearch(FLAG fSwitchOver, ULONG uColor);

void
SetMoveTimerToThinkForever(void);

void
ClearRootNodecountHash(void);

//
// draw.c
//
FLAG
IsDraw(SEARCHER_THREAD_CONTEXT *ctx);


//
// search.c
//
#define QPLIES_OF_NON_CAPTURE_CHECKS (1)
#define FUTILITY_BASE_MARGIN         (50) // + ctx->uPositional (min 100)
#define DO_IID
#define IID_R_FACTOR                 (TWO_PLY + HALF_PLY)

#ifndef MP
#define WE_SHOULD_STOP_SEARCHING (g_MoveTimer.bvFlags & TIMER_STOPPING)
#else
#define WE_SHOULD_STOP_SEARCHING ((g_MoveTimer.bvFlags & TIMER_STOPPING) || \
                                  (ThreadUnderTerminatingSplit(ctx)))
#endif

SCORE FASTCALL
QSearch(SEARCHER_THREAD_CONTEXT *ctx,
        SCORE iAlpha,
        SCORE iBeta);

SCORE FASTCALL
Search(SEARCHER_THREAD_CONTEXT *ctx,
       SCORE iAlpha,
       SCORE iBeta,
       ULONG uDepth);


//
// searchsup.c
//
SCORE
ComputeMoveScore(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN MOVE mv,
                 IN ULONG uMoveNum);

FLAG
ThreadUnderTerminatingSplit(SEARCHER_THREAD_CONTEXT *);

FLAG
WeShouldDoHistoryPruning(IN SCORE iRoughEval,
                         IN SCORE iAlpha,
                         IN SCORE iBeta,
                         IN SEARCHER_THREAD_CONTEXT *ctx,
                         IN ULONG uRemainingDepth,
                         IN ULONG uLegalMoves,
                         IN MOVE mv,
                         IN ULONG uMoveNum,
                         IN INT iExtend);

FLAG
WeShouldTryNullmovePruning(IN SEARCHER_THREAD_CONTEXT *ctx,
                           IN SCORE iAlpha,
                           IN SCORE iBeta,
                           IN SCORE iRoughEval,
                           IN ULONG uNullDepth);

FLAG
TryNullmovePruning(IN OUT SEARCHER_THREAD_CONTEXT *ctx,
                   IN OUT FLAG *pfThreat,
                   IN SCORE iAlpha,
                   IN SCORE iBeta,
                   IN ULONG uNullDepth,
                   IN OUT INT *piOrigExtend,
                   OUT SCORE *piNullScore);

void
UpdatePV(SEARCHER_THREAD_CONTEXT *ctx, MOVE mv);

FLAG
CheckInputAndTimers(IN SEARCHER_THREAD_CONTEXT *ctx);

void
ComputeReactionToCheckExtension(IN OUT SEARCHER_THREAD_CONTEXT *ctx,
                                IN ULONG uGenFlags,
                                IN ULONG uMoveNum,
                                IN SCORE iRoughEval,
                                IN SCORE iAlpha,
                                IN OUT INT *piExtend,
                                IN OUT INT *piOrigExtend);

void
ComputeMoveExtension(IN OUT SEARCHER_THREAD_CONTEXT *ctx,
                     IN SCORE iAlpha,
                     IN SCORE iBeta,
                     IN ULONG uMoveNum,
                     IN SCORE iRoughEval,
                     IN ULONG uDepth,
                     IN OUT INT *piExtend);

SCORE
RescoreMovesViaSearch(IN SEARCHER_THREAD_CONTEXT *ctx,
                      IN ULONG uDepth,
                      IN SCORE iAlpha,
                      IN SCORE iBeta);

FLAG
MateDistancePruningCutoff(IN ULONG uPly,
                          IN FLAG fInCheck,
                          IN OUT SCORE *piBestScore,
                          IN OUT SCORE *piAlpha,
                          IN OUT SCORE *piBeta);

FLAG
CommonSearchInit(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN OUT SCORE *piAlpha,
                 IN OUT SCORE *piBeta,
                 IN OUT SCORE *piScore);

ULONG
SelectNullmoveRFactor(IN SEARCHER_THREAD_CONTEXT *ctx,
                      IN INT uDepth);

#define VERIFY_BEFORE (1)
#define VERIFY_AFTER  (2)

FLAG
SanityCheckMoves(IN SEARCHER_THREAD_CONTEXT *ctx,
                 IN ULONG uCurrent,
                 IN ULONG uType);


//
// testsearch.c
//
FLAG
TestSearch(void);

//
// see.c
//
#define SEE_HEAPS

SCORE
SEE(POSITION *pos,
    MOVE mv);

typedef struct _SEE_THREESOME
{
    PIECE pPiece;
    COOR cLoc;
    ULONG uVal;
} SEE_THREESOME;

typedef struct _SEE_LIST
{
    ULONG uCount;
    SEE_THREESOME data[16];
}
SEE_LIST;

SCORE
ControlsSquareMinusPiece(ULONG uSide,
                         POSITION *pos,
                         COOR c,
                         COOR cIgnore);

//
// testsee.c
//
SCORE
DebugSEE(POSITION *pos,
         MOVE mv);

void
TestGetAttacks(void);

//
// hash.c
//
#define NUM_HASH_ENTRIES_PER_LINE     4
#define HASH_FLAG_EXACT               0x1
#define HASH_FLAG_LOWER               0x2
#define HASH_FLAG_UPPER               0x4
#define HASH_FLAG_VALID_BOUNDS        0x7
#define HASH_FLAG_THREAT              0x8
#define HASH_FLAG_DIRTY               0xF0

typedef struct _HASH_ENTRY
{
    MOVE mv;                         // 0 1 2 3
    UCHAR uDepth;                    // 4
    UCHAR bvFlags;                   // 5 ==> d  d  d  d | thr up low exact
    signed short iValue;             // 6 7
    UINT64 u64Sig;                   // 8 9 A B C D E F == 16 bytes
} HASH_ENTRY;

FLAG
InitializeHashSystem(void);

void
CleanupHashSystem(void);

void
ClearHashTable(void);

void
DirtyHashTable(void);

void
StoreLowerBound(MOVE mvBestMove,
           POSITION *pos,
           SCORE iValue,
           ULONG uDepth,
           FLAG fThreat);
void
StoreExactScore(MOVE mvBestMove,
            POSITION *pos,
          SCORE iValue,
          ULONG uDepth,
          FLAG fThreat,
          ULONG uPly);

void
StoreUpperBound(//MOVE mvBestMove,
           POSITION *pos,
           SCORE iValue,
           ULONG uDepth,
           FLAG fThreat);

HASH_ENTRY *
HashLookup(SEARCHER_THREAD_CONTEXT *ctx,
           ULONG uDepth,
           ULONG uNextDepth,
           SCORE iAlpha,
           SCORE iBeta,
           FLAG *pfThreat,
           FLAG *pfAvoidNull,
           MOVE *pHashMove,
           SCORE *piScore);

COOR
CheckHashForDangerSquare(POSITION *pos);

MOVE
GetPonderMove(POSITION *pos);

extern ULONG g_uHashTableSizeEntries;
extern ULONG g_uHashTableSizeBytes;
extern HASH_ENTRY *g_pHashTable;

//
// testhash.c
//
void
AnalyzeFullHashTable(void);

//
// positionhash.c
//
// IDEA: store "mate threat" flag in here?
// IDEA: store "king safety" numbers in here?
//
typedef struct _POSITION_HASH_ENTRY {
    UINT64 u64Sig;
    UCHAR cEnprise[2];
    UCHAR uEnpriseCount[2];
    UCHAR cTrapped[2];
} POSITION_HASH_ENTRY;

void
InitializePositionHashSystem(void);

void
CleanupPositionHashSystem(void);

void
StoreEnprisePiece(POSITION *pos, COOR cSquare);

void
StoreTrappedPiece(POSITION *pos, COOR cSquare);

COOR
GetEnprisePiece(POSITION *pos, ULONG uSide);

ULONG
GetEnpriseCount(POSITION *pos, ULONG uSide);

COOR
GetTrappedPiece(POSITION *pos, ULONG uSide);

FLAG
SideCanStandPat(POSITION *pos, ULONG uSide);

ULONG
ValueOfMaterialInTroubleDespiteMove(POSITION *pos, ULONG uSide);

ULONG
ValueOfMaterialInTroubleAfterNull(POSITION *pos, ULONG uSide);

//
// pawnhash.c
//
#define COOR_TO_BIT(x)       ((x) & 0x7) + ((0x7 - ((x) >> 4)) << 3)

void
ClearPawnHashStats(void);

void
ReportPawnHashStats(void);

PAWN_HASH_ENTRY *
PawnHashLookup(SEARCHER_THREAD_CONTEXT *ctx);

//
// eval.c
//
#define LAZY_EVAL
#define LAZE_EVAL_BASE_SCORE 10
extern const int g_iAhead[2];
extern const int g_iBehind[2];

ULONG 
DNABufferSizeBytes();

char *
ExportEvalDNA();

FLAG
WriteEvalDNA(char *szFilename);

FLAG
ImportEvalDNA(char *p);

FLAG
ReadEvalDNA(char *szFilename);



SCORE
Eval(SEARCHER_THREAD_CONTEXT *, SCORE, SCORE);

FLAG
EvalPasserRaces(POSITION *,
                PAWN_HASH_ENTRY *);

ULONG
CountKingSafetyDefects(POSITION *pos,
                       ULONG uSide);

//
// testeval.c
//
#ifdef EVAL_DUMP
void
TestEval(void);

void
EvalTraceReport(void);

void
EvalTrace(ULONG uColor, PIECE p, COOR c, SCORE iVal, CHAR *szMessage);

SCORE
EvalSigmaForPiece(POSITION *pos, COOR c);

void
EvalTraceClear(void);

void
TestEvalWithSymmetry(void);

#define EVAL_TERM(COL, PIE, COO, VAL, ADJ, MESS) \
    (VAL) += (ADJ); \
    EvalTrace((COL), (PIE), (COO), (ADJ), (MESS));
#else
#define EVAL_TERM(COL, PIE, COO, VAL, ADJ, MESS) \
    (VAL) += (ADJ);
#endif

//
// bitboard.c
//
void
InitializeBitboards(void);

ULONG CDECL
SlowCountBits(BITBOARD bb);

ULONG CDECL
DeBruijnFirstBit(BITBOARD bb);

ULONG CDECL
SlowFirstBit(BITBOARD bb);

ULONG CDECL
SlowLastBit(BITBOARD bb);

#ifdef CROUTINES
#define CountBits SlowCountBits
#define FirstBit SlowFirstBit
#define LastBit SlowLastBit
#endif

COOR
CoorFromBitBoardRank8ToRank1(BITBOARD *pbb);

COOR
CoorFromBitBoardRank1ToRank8(BITBOARD *pbb);

//
// x86.asm
//
ULONG CDECL
CountBits(BITBOARD bb);

ULONG CDECL
FirstBit(BITBOARD bb);

ULONG CDECL
LastBit(BITBOARD bb);

ULONG CDECL
LockCompareExchange(volatile void *pDest,
                    ULONG uExch,
                    ULONG uComp);

ULONG CDECL
LockIncrement(volatile void *pDest);

ULONG CDECL
LockDecrement(volatile void *pDest);

FLAG CDECL
CanUseParallelOpcodes();

ULONG CDECL
ParallelCompareUlong(ULONG uComparand, void *pComparators);

ULONG CDECL
ParallelCompareVector(void *pComparand, void *pComparators);

void CDECL
GetAttacks(SEE_LIST *pList,
           POSITION *pos,
           COOR cSquare,
           ULONG uSide);

void CDECL
SlowGetAttacks(SEE_LIST *pList,
               POSITION *pos,
               COOR cSquare,
               ULONG uSide);
#ifdef CROUTINES
#define GetAttacks SlowGetAttacks
#endif

#ifdef _X86_
//
// Note: this is most of the stuff that x86.asm assumes about the
// internal data structures.  If any of this fails then either the
// assembly language code needs to be updated or you need to use the C
// version of the routine in see.c instead.
//
#define ASSERT_ASM_ASSUMPTIONS \
    ASSERT(VALUE_PAWN == 100); \
    ASSERT(OFFSET_OF(uCount, SEE_LIST) == 0); \
    ASSERT(OFFSET_OF(data, SEE_LIST) == 4); \
    ASSERT(sizeof(SEE_THREESOME) == 12); \
    ASSERT(OFFSET_OF(pPiece, SEE_THREESOME) == 0); \
    ASSERT(OFFSET_OF(cLoc, SEE_THREESOME) == 4); \
    ASSERT(OFFSET_OF(uVal, SEE_THREESOME) == 8); \
    ASSERT(OFFSET_OF(cNonPawns, POSITION) == 0x478); \
    ASSERT(OFFSET_OF(uNonPawnCount, POSITION) == 0x500); \
    ASSERT(OFFSET_OF(rgSquare, POSITION) == 0); \
    ASSERT(sizeof(SQUARE) == 8); \
    ASSERT(sizeof(VECTOR_DELTA) == 4); \
    ASSERT(sizeof(g_VectorDelta) == 256 * 4); \
    ASSERT(OFFSET_OF(iVector, VECTOR_DELTA) == 0); \
    ASSERT(OFFSET_OF(iDelta, VECTOR_DELTA) == 2); \
    ASSERT(OFFSET_OF(iNegDelta, VECTOR_DELTA) == 3); \
    ASSERT(sizeof(g_PieceData) == 8 * 4 * 4); \
    ASSERT(OFFSET_OF(uValue, PIECE_DATA) == 0);
#else
#define ASSERT_ASM_ASSUMPTIONS
#endif

//
// testbitboard.c
//
void
TestBitboards(void);

//
// dynamic.c
//
extern ULONG g_HistoryCounters[14][128];

ULONG
GetMoveFailHighPercentage(MOVE mv);

void
UpdateDynamicMoveOrdering(SEARCHER_THREAD_CONTEXT *ctx,
                          ULONG uRemainingDepth,
                          MOVE mvBest,
                          SCORE iScore,
                          ULONG uCurrent);

void
NewKillerMove(SEARCHER_THREAD_CONTEXT *ctx, MOVE mv, SCORE iScore);

FLAG
InitializeDynamicMoveOrdering(void);

void
CleanupDynamicMoveOrdering(void);

void
ClearDynamicMoveOrdering(void);

void
MaintainDynamicMoveOrdering(void);

void
IncrementMoveHistoryCounter(MOVE mv, ULONG u);

void
DecrementMoveHistoryCounter(MOVE mv, ULONG u);

//
// split.c
//
extern volatile ULONG g_uNumHelpersAvailable;
extern ULONG g_uNumHelperThreads;

FLAG
InitializeParallelSearch(void);

FLAG
CleanupParallelSearch(void);

void
ClearHelperThreadIdleness(void);

void
DumpHelperIdlenessReport(void);

SCORE
StartParallelSearch(IN SEARCHER_THREAD_CONTEXT *ctx,
                    IN OUT SCORE *piAlpha,
                    IN SCORE iBeta,
                    IN OUT SCORE *piBestScore,
                    IN OUT MOVE *pmvBest,
                    IN ULONG uMoveNum,
                    IN INT iPositionExtend,
                    IN ULONG uDepth);

void
InitializeSearcherContext(POSITION *pos, SEARCHER_THREAD_CONTEXT *ctx);

void
ReInitializeSearcherContext(POSITION *pos, SEARCHER_THREAD_CONTEXT *ctx);

void
InitializeLightweightSearcherContext(POSITION *pos,
                                     LIGHTWEIGHT_SEARCHER_CONTEXT *ctx);

//
// book.c
//

#define BOOK_PROBE_MISS_LIMIT  (7)
#define FLAG_DISABLED          (1)
#define FLAG_ALWAYSPLAY        (2)
#define FLAG_DELETED           (4)

#define BOOKMOVE_SELECT_MOVE   (1)
#define BOOKMOVE_DUMP          (2)

#pragma pack(4)
typedef struct _BOOK_ENTRY
{
    UINT64 u64Sig;                            // 8 bytes
    UINT64 u64NextSig;                        // 8 bytes
    MOVE mvNext;                              // 4 bytes
    ULONG uWins;                              // 4 bytes
    ULONG uDraws;                             // 4 bytes
    ULONG uLosses;                            // 4 bytes
    BITV bvFlags;                             // 4 bytes
}
BOOK_ENTRY;
#pragma pack()

typedef struct _OPENING_NAME_MAPPING
{
    UINT64 u64Sig;
    CHAR *szString;
}
OPENING_NAME_MAPPING;

#define BOOK_EDITING_RECORD       "bkedit.edt"
#define OPENING_LEARNING_FILENAME "bklearn.bin"

typedef struct _OPENING_LEARNING_ENTRY
{
    UINT64 u64Sig;
    ULONG uWhiteWins;
    ULONG uDraws;
    ULONG uBlackWins;
}
OPENING_LEARNING_ENTRY;

FLAG
InitializeOpeningBook(void);

void
ResetOpeningBook(void);

void
CleanupOpeningBook(void);

MOVE
BookMove(POSITION *pos,
         BITV bvFlags);

COMMAND(BookCommand);

extern ULONG g_uBookProbeFailures;
extern FLAG g_fTournamentMode;
extern CHAR *g_szBookName;

//
// bench.c
//
COMMAND(BenchCommand);

//
// testdraw.c
//
void
TestDraw(void);

//
// probe.c
//
FLAG
ProbeEGTB(SEARCHER_THREAD_CONTEXT *ctx, SCORE *score);

void
InitializeEGTB(void);

void
CleanupEGTB(void);

//
// dumptree.c
//
#ifdef DUMP_TREE

void
InitializeTreeDump(void);

void
CleanupTreeDump(void);

void
DTEnterNode(SEARCHER_THREAD_CONTEXT *ctx,
            ULONG uDepth,
            FLAG fIsQNode,
            SCORE iAlpha,
            SCORE iBeta);

void
DTLeaveNode(SEARCHER_THREAD_CONTEXT *ctx,
            FLAG fQNode,
            SCORE iBestScore,
            MOVE mvBestMove);

void CDECL
DTTrace(ULONG uPly, CHAR *szMessage, ...);

#define DTTRACE(...) DTTrace(ctx->uPly, __VA_ARGS__)

#else

#define DTTRACE(...)

#define DTEnterNode(...)

#define DTLeaveNode(...)

#define InitializeTreeDump(...)

#define CleanupTreeDump(...)

#endif

//
// testsup.c
//
FLAG
IsBoardLegal(POSITION *pos);

void
GenerateRandomLegalPosition(POSITION *pos);

void
GenerateRandomLegalSymetricPosition(POSITION *pos);

//
// recogn.c
//
#define UNRECOGNIZED (0)
#define RECOGN_EXACT (1)
#define RECOGN_UPPER (2)
#define RECOGN_LOWER (3)
#define RECOGN_EGTB  (4)

void
InitializeInteriorNodeRecognizers(void);

ULONG
RecognLookup(SEARCHER_THREAD_CONTEXT *ctx,
             SCORE *piScore,
             FLAG fProbeEGTB);

#ifdef EVAL_HASH
void
ClearEvalHashStats(void);

void
ReportEvalHashStats(void);

SCORE
ProbeEvalHash(SEARCHER_THREAD_CONTEXT *ctx);

SCORE
GetRoughEvalScore(IN SEARCHER_THREAD_CONTEXT *ctx,
                  IN SCORE iAlpha,
                  IN SCORE iBeta,
                  IN FLAG fUseHash);

void
StoreEvalHash(SEARCHER_THREAD_CONTEXT *ctx, SCORE iScore);
#endif // EVAL_HASH

#endif // CHESS
