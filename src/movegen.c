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

// movegen.c

#include <stdio.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "defs.h"
#include "evaluate.h"
#include "makemove.h"
#include "movegen.h"
#include "validate.h"

#define MOVE(f, t, ca, pro, fl) ((f) | ((t) << 7) | ((ca) << 14) | ((pro) << 20) | (fl))

const int LoopSlidePce[8] = {
	wB, wR, wQ, 0, bB, bR, bQ, 0
};

const int LoopNonSlidePce[6] = {
	wN, wK, 0, bN, bK, 0
};

const int LoopSlideIndex[2] = { 0, 4 };
const int LoopNonSlideIndex[2] = { 0, 3 };

/*
PV Move
Cap -> MvvLVA
Killers
HistoryScore
*/
const int VictimScore[13] = { 0, 100, 450, 470, 670, 1300, 10000, 100, 450, 470, 670, 1300, 10000 };
static int MvvLvaScores[13][13];

void InitMvvLva() {

	for (int Attacker = wP; Attacker <= bK; ++Attacker)
		for (int Victim = wP; Victim <= bK; ++Victim)
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] + 6 - (VictimScore[Attacker] / 100);
}

static void AddQuietMove( const Board *pos, int move, MoveList *list) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(CheckBoard(pos));
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);

	list->moves[list->count].move = move;

	if (pos->searchKillers[0][pos->ply] == move)
		list->moves[list->count].score = 900000;

	else if(pos->searchKillers[1][pos->ply] == move)
		list->moves[list->count].score = 800000;

	else
		list->moves[list->count].score = pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];

	list->count++;
	list->quiets++;
}

static void AddCaptureMove(const Board *pos, int move, MoveList *list) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(PieceValid(CAPTURED(move)));
	ASSERT(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[FROMSQ(move)]] + 1000000;
	list->count++;
}

static void AddEnPassantMove( int move, MoveList *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(RanksBrd[TOSQ(move)] == RANK_6 || RanksBrd[TOSQ(move)] == RANK_3);

	list->moves[list->count].move = move;
	list->moves[list->count].score = 105 + 1000000;
	list->count++;
}

static void AddWhitePawnCapMove( const Board *pos, const int from, const int to, const int cap, MoveList *list) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if (RanksBrd[from] == RANK_7) {
		AddCaptureMove(pos, MOVE(from, to, cap, wQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wN, 0), list);
	} else
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddWhitePawnMove( const Board *pos, const int from, const int to, MoveList *list) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if (RanksBrd[from] == RANK_7) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, wQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wN, 0), list);
	} else
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
}

static void AddBlackPawnCapMove( const Board *pos, const int from, const int to, const int cap, MoveList *list ) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if (RanksBrd[from] == RANK_2) {
		AddCaptureMove(pos, MOVE(from, to, cap, bQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bN, 0), list);
	} else
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddBlackPawnMove( const Board *pos, const int from, const int to, MoveList *list ) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if (RanksBrd[from] == RANK_2) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, bQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bN, 0), list);
	} else
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
}

void GenerateAllMoves(const Board *pos, MoveList *list) {

	ASSERT(CheckBoard(pos));

	list->count = 0;
	list->quiets = 0;

	int pce = EMPTY, side = pos->side, sq, t_sq;
	int pceNum, dir, index, pceIndex;

	if (side == WHITE) {

		for (pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (pos->pieces[sq + 10] == EMPTY) {
				AddWhitePawnMove(pos, sq, sq+10, list);
				if (RanksBrd[sq] == RANK_2 && pos->pieces[sq + 20] == EMPTY)
					AddQuietMove(pos, MOVE(sq, sq + 20, EMPTY, EMPTY, MFLAGPS), list);
			}

			if (!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK)
				AddWhitePawnCapMove(pos, sq, sq + 9, pos->pieces[sq + 9], list);

			if (!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK)
				AddWhitePawnCapMove(pos, sq, sq + 11, pos->pieces[sq + 11], list);	

			if (pos->enPas != NO_SQ) {
				if (sq + 9 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq + 9, EMPTY, EMPTY, MFLAGEP), list);

				if (sq + 11 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq + 11, EMPTY, EMPTY, MFLAGEP), list);
			}
		}

		if (pos->castlePerm & WKCA)
			if (pos->pieces[F1] == EMPTY && pos->pieces[G1] == EMPTY)
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos))
					AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);

		if (pos->castlePerm & WQCA)
			if (pos->pieces[D1] == EMPTY && pos->pieces[C1] == EMPTY && pos->pieces[B1] == EMPTY)
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos))
					AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list);

	} else {

		for (pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (pos->pieces[sq - 10] == EMPTY) {
				AddBlackPawnMove(pos, sq, sq - 10, list);
				if (RanksBrd[sq] == RANK_7 && pos->pieces[sq - 20] == EMPTY)
					AddQuietMove(pos, MOVE(sq, sq - 20, EMPTY, EMPTY, MFLAGPS), list);
			}

			if (!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE)
				AddBlackPawnCapMove(pos, sq, sq - 9, pos->pieces[sq - 9], list);

			if (!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE)
				AddBlackPawnCapMove(pos, sq, sq - 11, pos->pieces[sq - 11], list);

			if (pos->enPas != NO_SQ) {
				if (sq - 9 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq - 9, EMPTY, EMPTY, MFLAGEP), list);
				
				if (sq - 11 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq - 11, EMPTY, EMPTY, MFLAGEP), list);
			}
		}

		// castling
		if (pos->castlePerm &  BKCA)
			if (pos->pieces[F8] == EMPTY && pos->pieces[G8] == EMPTY)
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
					AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);

		if (pos->castlePerm &  BQCA)
			if (pos->pieces[D8] == EMPTY && pos->pieces[C8] == EMPTY && pos->pieces[B8] == EMPTY)
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
					AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);
	}

	/* Loop for slide pieces */
	pceIndex = LoopSlideIndex[side];
	pce = LoopSlidePce[pceIndex++];
	while (pce != 0) {
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				while (!SQOFFBOARD(t_sq)) {
					if (pos->pieces[t_sq] != EMPTY) {
						if (PieceCol[pos->pieces[t_sq]] == !side)
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						
						break;
					}
					AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
					t_sq += dir;
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];
	}

	/* Loop for non slide */
	pceIndex = LoopNonSlideIndex[side];
	pce = LoopNonSlidePce[pceIndex++];

	while (pce != 0) {
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				if (SQOFFBOARD(t_sq))
					continue;

				// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
				if (pos->pieces[t_sq] != EMPTY) {
					if (PieceCol[pos->pieces[t_sq]] == !side)
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					
					continue;
				}
				AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}

    ASSERT(MoveListOk(list,pos));
}


void GenerateAllCaps(const Board *pos, MoveList *list) {

	ASSERT(CheckBoard(pos));

	list->count = 0;

	int pce = EMPTY, side = pos->side, sq, t_sq;
	int pceNum, dir, index, pceIndex;

	if (side == WHITE) {

		for (pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK)
				AddWhitePawnCapMove(pos, sq, sq + 9, pos->pieces[sq + 9], list);
			
			if (!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK)
				AddWhitePawnCapMove(pos, sq, sq + 11, pos->pieces[sq + 11], list);

			if (pos->enPas != NO_SQ) {
				if (sq + 9 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq + 9, EMPTY, EMPTY, MFLAGEP), list);
				
				if (sq + 11 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq + 11, EMPTY, EMPTY, MFLAGEP), list);
			}
		}

	} else {

		for (pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE)
				AddBlackPawnCapMove(pos, sq, sq-9, pos->pieces[sq - 9], list);

			if (!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE)
				AddBlackPawnCapMove(pos, sq, sq-11, pos->pieces[sq - 11], list);
			
			if (pos->enPas != NO_SQ) {
				if (sq - 9 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq - 9, EMPTY, EMPTY, MFLAGEP), list);
				
				if (sq - 11 == pos->enPas)
					AddEnPassantMove(MOVE(sq, sq - 11, EMPTY, EMPTY, MFLAGEP), list);
			}
		}
	}

	/* Loop for slide pieces */
	pceIndex = LoopSlideIndex[side];
	pce = LoopSlidePce[pceIndex++];
	while (pce != 0) {
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				while (!SQOFFBOARD(t_sq)) {
					if (pos->pieces[t_sq] != EMPTY) {
						if (PieceCol[pos->pieces[t_sq]] == !side)
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						
						break;
					}
					t_sq += dir;
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];
	}

	/* Loop for non slide */
	pceIndex = LoopNonSlideIndex[side];
	pce = LoopNonSlidePce[pceIndex++];

	while (pce != 0) {
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				if (SQOFFBOARD(t_sq))
					continue;

				if (pos->pieces[t_sq] != EMPTY) {
					if (PieceCol[pos->pieces[t_sq]] == !side)
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					
					continue;
				}
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}
    ASSERT(MoveListOk(list,pos));
}