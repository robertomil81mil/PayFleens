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

#include "defs.h"
#include "makemove.h"
#include "search.h"

struct Board {

	uint64_t posKey, materialKey, pawns[3];

	int side, enPas, castlePerm, fiftyMove;
	int ply, hisPly, gamePly, plyFromNull;

	int PSQT[COLOUR_NB];
	int KingSq[COLOUR_NB];
	int pieces[BRD_SQ_NUM];
	int pawn_ctrl[COLOUR_NB][BRD_SQ_NUM];

	// piece list
	int pList[PIECE_NB][10];
	int pceNum[PIECE_NB];
	int bigPce[COLOUR_NB];
	int mPhases[COLOUR_NB];
	int material[COLOUR_NB];

	PVariation pv;
	Undo history[MAXGAMEMOVES];

	KillerTable killers;
    CounterMoveTable cmtable;
    HistoryTable mainHistory;
    ContinuationTable continuation;
};

int ParseFen(char *fen, Board *pos);
void UpdateListsMaterial(Board *pos);
void ResetBoard(Board *pos);
void MirrorBoard(Board *pos);
void PrintBoard(const Board *pos);

int IsRepetition(const Board *pos);
int posIsDrawn(const Board *pos, int ply);
int pieceType(int piece);

void InitFilesRanksBrd();
void InitSq120To64();

extern const char PceChar[];
extern const char SideChar[];
extern const char RankChar[];
extern const char FileChar[];

extern int Sq120ToSq64[BRD_SQ_NUM];
extern int Sq64ToSq120[64];
extern int FilesBrd[BRD_SQ_NUM];
extern int RanksBrd[BRD_SQ_NUM];

extern int SquareDistance[120][120];
extern int FileDistance[120][120];
extern int RankDistance[120][120];

#define FR2SQ(f, r) ((21 + (f)) + ((r) * 10))
#define makeSq(f, r) ((f) + ((r) * 8))

#define SQ64(sq120) (Sq120ToSq64[(sq120)])
#define SQ120(sq64) (Sq64ToSq120[(sq64)])
#define SQOFFBOARD(sq) (FilesBrd[(sq)] == OFFBOARD)

#if defined(DEBUG)

int PceListOk(const Board *pos);
int CheckBoard(const Board *pos);

#endif