/*
 *  PayFleens is a UCI chess engine by Roberto Martinez.
 * 
 *  Copyright (C) 2019 Roberto Martinez
 *
 *  PayFleens is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  PayFleens is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG

#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n) \
if(!(n)) { \
printf("%s - Failed",#n); \
printf("On %s ",__DATE__); \
printf("At %s ",__TIME__); \
printf("In File %s ",__FILE__); \
printf("At Line %d\n",__LINE__); \
exit(1);}
#endif

typedef unsigned long long U64;

#define PRIu64 "I64u"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define FEN3 "4rrk1/1p3qbp/p2n1p2/2NP2p1/1P1B4/3Q1R2/P5PP/5RK1 b - - 7 30"
#define FEN4 "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

#define NAME "PayFleens 1.5.1"

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

enum {
	BRD_SQ_NUM = 120,
	MAXPOSITIONMOVES = 256, 
	MAXGAMEMOVES = 2048
};

enum {
	MAXDEPTH = 64, 
	MAX_PLY = 128,
	KNOWN_WIN = 10000,
    INFINITE = 32000,
    MATE_IN_MAX = INFINITE - MAX_PLY,
    MATED_IN_MAX = MAX_PLY - INFINITE,
    VALUE_NONE = 32001
};

enum { NOMOVE = 0, NULL_MOVE = 22 };

enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK  };

enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };

enum { WHITE, BLACK, BOTH };
enum { UCIMODE, XBOARDMODE, CONSOLEMODE };
enum {
  A1 = 21, B1, C1, D1, E1, F1, G1, H1,
  A2 = 31, B2, C2, D2, E2, F2, G2, H2,
  A3 = 41, B3, C3, D3, E3, F3, G3, H3,
  A4 = 51, B4, C4, D4, E4, F4, G4, H4,
  A5 = 61, B5, C5, D5, E5, F5, G5, H5,
  A6 = 71, B6, C6, D6, E6, F6, G6, H6,
  A7 = 81, B7, C7, D7, E7, F7, G7, H7,
  A8 = 91, B8, C8, D8, E8, F8, G8, H8, NO_SQ, OFFBOARD
};

enum { NonPV, PVNode};

enum { FALSE, TRUE };

enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

typedef struct evalInfo evalInfo;
typedef struct evalData evalData;
typedef struct PVariation PVariation;
typedef struct TT_Entry TT_Entry;
typedef struct TT_Cluster TT_Cluster;
typedef struct TTable TTable;
typedef struct Limits Limits;
typedef struct EngineOptions EngineOptions;

struct PVariation {
    int line[MAX_PLY];
    int length;
};

typedef struct {
	int move;
	int score;
} S_MOVE;

typedef struct {
	S_MOVE moves[MAXPOSITIONMOVES];
	int count;
	int quiets;
} S_MOVELIST;

typedef struct {

	int move;
	int castlePerm;
	int enPas;
	int fiftyMove;
	U64 posKey;

} S_UNDO;

typedef struct {

	int pieces[BRD_SQ_NUM];
	U64 pawns[3];
	int pawn_ctrl[2][120];

	int KingSq[2];

	int side;
	int enPas;
	int fiftyMove;

	int ply;
	int hisPly;
	int gamePly;
	int reps;

	int castlePerm;

	U64 posKey;

	int pceNum[13];
	int bigPce[2];
	int majPce[2];
	int minPce[2];
	int mPhases[2];
	int material[2];
	int materialeg[2];

	S_UNDO history[MAXGAMEMOVES];

	// piece list
	int pList[13][10];

	int PSQT[2];

	PVariation pv;

	int searchHistory[13][BRD_SQ_NUM];
	int searchKillers[2][MAX_PLY];

} S_BOARD;

typedef struct {

	double startTime, optimumTime, maximumTime;
	double previousTimeReduction, bestMoveChanges;

	int depth, seldepth;
	int quit, stop, timeset;

	int values[MAX_PLY];
	int currentMove[MAX_PLY];
	int staticEval[MAX_PLY];

	uint64_t nodes;
	float fh, fhf;

	int nullCut, probCut, TTCut;

	int GAME_MODE;
	int POST_THINKING;

} S_SEARCHINFO;

/* GAME MOVE */

/*
0000 0000 0000 0000 0000 0111 1111 -> From 0x7F
0000 0000 0000 0011 1111 1000 0000 -> To >> 7, 0x7F
0000 0000 0011 1100 0000 0000 0000 -> Captured >> 14, 0xF
0000 0000 0100 0000 0000 0000 0000 -> EP 0x40000
0000 0000 1000 0000 0000 0000 0000 -> Pawn Start 0x80000
0000 1111 0000 0000 0000 0000 0000 -> Promoted Piece >> 20, 0xF
0001 0000 0000 0000 0000 0000 0000 -> Castle 0x1000000
*/

/*		2	C
0000 0000 0000 0000 0000 0011 1111 -> From 0x6F
0000 0000 0000 0000 1111 1100 0000 -> To >> 6, 0x6F
0000 0000 0000 1111 0000 0000 0000 -> Captured >> 9, 0xF
0000 0000 0001 0000 0000 0000 0000 -> EP 0x10000
0000 0000 0010 0000 0000 0000 0000 -> Pawn Start 0x20000
0000 0011 1100 0000 0000 0000 0000 -> Promoted Piece >> 14, 0xF
0000 0100 0000 0000 0000 0000 0000 -> Castle 0x400000
*/


/*#define FROMSQ(m) (((m)>>0) & 0x6F)
#define TOSQ(m) (((m)>>6) & 0x6F) //(((m)>>7) & 0x7F)
#define CAPTURED(m) (((m)>>9) & 0xF)
#define PROMOTED(m) (((m)>>14)& 0xF)

#define MFLAGEP 0x10000
#define MFLAGPS 0x20000
#define MFLAGCA 0x400000

#define MFLAGCAP 0x2F000
#define MFLAGPROM 0x2C0000*/

#define MFLAGCAP 0x7C000
#define MFLAGPROM 0xF00000

#define FROMSQ(m) ((m) & 0x7F)
#define TOSQ(m) (((m)>>7) & 0x7F)
#define CAPTURED(m) (((m)>>14) & 0xF)
#define PROMOTED(m) (((m)>>20) & 0xF)

#define MFLAGEP 0x40000
#define MFLAGPS 0x80000
#define MFLAGCA 0x1000000

#define MFLAGCAP 0x7C000
#define MFLAGPROM 0xF00000

/* MACROS */

#define FR2SQ(f,r) ( (21 + (f) ) + ( (r) * 10 ) )
#define SQOFFBOARD(sq) (FilesBrd[(sq)]==OFFBOARD)
#define SQ64(sq120) (Sq120ToSq64[(sq120)])
#define SQ120(sq64) (Sq64ToSq120[(sq64)])
#define POP(b) PopBit(b)
#define CNT(b) CountBits(b)
#define CLRBIT(bb,sq) ((bb) &= ClearMask[(sq)])
#define SETBIT(bb,sq) ((bb) |= SetMask[(sq)])

#define IsBQ(p) (PieceBishopQueen[(p)])
#define IsRQ(p) (PieceRookQueen[(p)])
#define IsKn(p) (PieceKnight[(p)])
#define IsKi(p) (PieceKing[(p)])

#define MIRROR64(sq) (Mirror64[(sq)])

#define NORTH 10
#define SOUTH -10
#define EAST  1
#define WEST  -1
#define NE    11
#define SW    -11
#define NW    9
#define SE    -9

/* GLOBALS */

extern int Sq120ToSq64[BRD_SQ_NUM];
extern int Sq64ToSq120[64];
extern U64 SetMask[64];
extern U64 ClearMask[64];
extern U64 PieceKeys[13][120];
extern U64 SideKey;
extern U64 CastleKeys[16];
extern char PceChar[];
extern char SideChar[];
extern char RankChar[];
extern char FileChar[];

extern int PieceBig[13];
extern int PieceMaj[13];
extern int PieceMin[13];
extern int PieceVal[13];
extern int PieceValPhases[13];
extern int PieceValEG[13];
extern int PieceCol[13];
extern int PiecePawn[13];
extern int PiecePawnW[13];
extern int PiecePawnB[13];

extern int FilesBrd[BRD_SQ_NUM];
extern int RanksBrd[BRD_SQ_NUM];

extern int SquareDistance[120][120];
extern int FileDistance[120][120];
extern int RankDistance[120][120];

extern int PieceKnight[13];
extern int PieceBishop[13];
extern int PieceRook[13];
extern int PieceKing[13];
extern int PieceRookQueen[13];
extern int PieceBishopQueen[13];
extern int PieceSlides[13];

extern int Mirror64[64];
extern int Mirror120[64];

extern U64 FileBBMask[8];
extern U64 RankBBMask[8];

extern U64 PassedPawnMasks[2][64];
extern U64 OutpostSquareMasks[2][64];
extern U64 IsolatedMask[64];
extern U64 PawnAttacks[2][64];

extern U64 KingAreaMasks[BOTH][64];
extern void KingAreaMask();
extern U64 kingAreaMasks(int colour, int sq);
extern void PawnAttacksMasks();
extern U64 pawnAttacks(int colour, int sq);
extern U64 outpostSquareMasks(int colour, int sq);

/* FUNCTIONS */

// init.c
extern void AllInit();

// bitboards.c
extern void PrintBitBoard(U64 bb);
extern int PopBit(U64 *bb);
extern int CountBits(U64 b);

// hashkeys.c
extern U64 GeneratePosKey(const S_BOARD *pos);

// board.c
extern void ResetBoard(S_BOARD *pos);
extern int ParseFen(char *fen, S_BOARD *pos);
extern void PrintBoard(const S_BOARD *pos);
extern void UpdateListsMaterial(S_BOARD *pos);
extern int CheckBoard(const S_BOARD *pos);
extern void MirrorBoard(S_BOARD *pos);
extern void PrintNonBits(const S_BOARD *pos, int side);

// attack.c
extern int SqAttacked(const int sq, const int side, const S_BOARD *pos);
extern int SqAttackedByPawn(const int sq, const int side, const S_BOARD *pos);
extern int SqAttackedByBishopQueen(const int sq, const int side, const S_BOARD *pos);
extern int SqAttackedByRookQueen(const int sq, const int side, const S_BOARD *pos);
extern int SqAttackedByKnight(const int sq, const int side, const S_BOARD *pos);

// io.c
extern char *PrMove(const int move);
extern char *PrSq(const int sq);
extern void PrintMoveList(const S_MOVELIST *list);
extern int ParseMove(char *ptrChar, S_BOARD *pos);

//validate.c
extern int SqOnBoard(const int sq);
extern int SideValid(const int side);
extern int FileRankValid(const int fr);
extern int PieceValidEmpty(const int pce);
extern int PieceValid(const int pce);
extern void MirrorEvalTest(S_BOARD *pos);
extern int SqIs120(const int sq);
extern int PceValidEmptyOffbrd(const int pce);
extern int MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos);

// movegen.c
extern void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list);
extern void GenerateAllCaps(const S_BOARD *pos, S_MOVELIST *list);
extern int MoveExists(S_BOARD *pos, const int move);
extern void InitMvvLva();
extern void setSquaresNearKing();
extern int moveBestCaseValue(const S_BOARD *pos);

// makemove.c
extern int MakeMove(S_BOARD *pos, int move);
extern void TakeMove(S_BOARD *pos);
extern void MakeNullMove(S_BOARD *pos);
extern void TakeNullMove(S_BOARD *pos);

// perft.c
extern void PerftTest(int depth, S_BOARD *pos);

/*
// uci.c
extern void Uci_Loop(S_BOARD *pos, S_SEARCHINFO *info);

// xboard.c
extern void XBoard_Loop(S_BOARD *pos, S_SEARCHINFO *info);
extern void Console_Loop(S_BOARD *pos, S_SEARCHINFO *info);
extern int ThreeFoldRep(const S_BOARD *pos);
extern int DrawMaterial(const S_BOARD *pos);*/

// polybook.c
extern int GetBookMove(S_BOARD *board);
extern void CleanPolyBook();
extern void InitPolyBook();