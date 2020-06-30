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

static const int LoopSlidePce[8] = {
	wB, wR, wQ, 0, bB, bR, bQ, 0
};

static const int LoopNonSlidePce[6] = {
	wN, wK, 0, bN, bK, 0
};

static const int LoopSlideIndex[2] = { 0, 4 };
static const int LoopNonSlideIndex[2] = { 0, 3 };

static void AddQuietMove(int move, MoveList *list) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));

	list->moves[list->count].move = move;
	list->moves[list->count].score = 0;
	list->count++;
	list->quiets++;
}

static void AddCaptureMove(int move, MoveList *list) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(PieceValid(CAPTURED(move)));

	list->moves[list->count].move = move;
	list->moves[list->count].score = 0;
	list->count++;
}

static void AddEnPassantMove(int move, MoveList *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(RanksBrd[TOSQ(move)] == RANK_6 || RanksBrd[TOSQ(move)] == RANK_3);

	list->moves[list->count].move = move;
	list->moves[list->count].score = 0;
	list->count++;
}

static void AddWhitePawnCapMove(const int from, const int to, const int cap, MoveList *list) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_7) {
		AddCaptureMove(MOVE(from, to, cap, wQ, 0), list);
		AddCaptureMove(MOVE(from, to, cap, wR, 0), list);
		AddCaptureMove(MOVE(from, to, cap, wB, 0), list);
		AddCaptureMove(MOVE(from, to, cap, wN, 0), list);
	} else
		AddCaptureMove(MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddWhitePawnMove(const int from, const int to, MoveList *list) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_7) {
		AddQuietMove(MOVE(from, to, EMPTY, wQ, 0), list);
		AddQuietMove(MOVE(from, to, EMPTY, wR, 0), list);
		AddQuietMove(MOVE(from, to, EMPTY, wB, 0), list);
		AddQuietMove(MOVE(from, to, EMPTY, wN, 0), list);
	} else
		AddQuietMove(MOVE(from, to, EMPTY, EMPTY, 0), list);
}

static void AddBlackPawnCapMove(const int from, const int to, const int cap, MoveList *list ) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_2) {
		AddCaptureMove(MOVE(from, to, cap, bQ, 0), list);
		AddCaptureMove(MOVE(from, to, cap, bR, 0), list);
		AddCaptureMove(MOVE(from, to, cap, bB, 0), list);
		AddCaptureMove(MOVE(from, to, cap, bN, 0), list);
	} else
		AddCaptureMove(MOVE(from, to, cap, EMPTY, 0), list);
}

static void AddBlackPawnMove(const int from, const int to, MoveList *list ) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_2) {
		AddQuietMove(MOVE(from, to, EMPTY, bQ, 0), list);
		AddQuietMove(MOVE(from, to, EMPTY, bR, 0), list);
		AddQuietMove(MOVE(from, to, EMPTY, bB, 0), list);
		AddQuietMove(MOVE(from, to, EMPTY, bN, 0), list);
	} else
		AddQuietMove(MOVE(from, to, EMPTY, EMPTY, 0), list);
}

void GenerateAllMoves(const Board *pos, MoveList *list) {
	list->count = 0;
	genNoisyMoves(pos, list);
	genQuietMoves(pos, list);
}

void genNoisyMoves(const Board *pos, MoveList *list) {

	ASSERT(CheckBoard(pos));

	int pce, side = pos->side;
	int sq, t_sq, pceNum, index, pceIndex;

	if (side == WHITE) {

		for (pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK)
				AddWhitePawnCapMove(sq, sq + 9, pos->pieces[sq + 9], list);
			
			if (!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK)
				AddWhitePawnCapMove(sq, sq + 11, pos->pieces[sq + 11], list);

			if (pos->enPas != NO_SQ) {
				if (sq + 9 == pos->enPas) 
					AddEnPassantMove(MOVE(sq, sq + 9, EMPTY, EMPTY, MFLAGEP), list);
				
				if(sq + 11 == pos->enPas) 
					AddEnPassantMove(MOVE(sq, sq + 11, EMPTY, EMPTY, MFLAGEP), list);
			}
		}

	} else {

		for (pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE)
				AddBlackPawnCapMove(sq, sq - 9, pos->pieces[sq - 9], list);

			if (!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE)
				AddBlackPawnCapMove(sq, sq - 11, pos->pieces[sq - 11], list);
			
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
				t_sq = sq + PceDir[pce][index];

				while (!SQOFFBOARD(t_sq)) {
					if (pos->pieces[t_sq] != EMPTY) {
						if (PieceCol[pos->pieces[t_sq]] == !side)
							AddCaptureMove(MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						
						break;
					}
					t_sq += PceDir[pce][index];
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
				t_sq = sq + PceDir[pce][index];

				if (SQOFFBOARD(t_sq))
					continue;
				
				if (pos->pieces[t_sq] != EMPTY) {
					if (PieceCol[pos->pieces[t_sq]] == !side)
						AddCaptureMove(MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					
					continue;
				}
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}
    ASSERT(MoveListOk(list,pos));
}

void genQuietMoves(const Board *pos, MoveList *list) {

	ASSERT(CheckBoard(pos));

	list->quiets = 0;

	int pce, side = pos->side;
	int sq, t_sq, pceNum, index, pceIndex;

	if (side == WHITE) {

		for (pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (pos->pieces[sq + 10] == EMPTY) {
				AddWhitePawnMove(sq, sq + 10, list);
				if (RanksBrd[sq] == RANK_2 && pos->pieces[sq + 20] == EMPTY)
					AddQuietMove(MOVE(sq, sq + 20, EMPTY, EMPTY, MFLAGPS), list);
			}
		}

		if (pos->castlePerm & WKCA)
			if (pos->pieces[F1] == EMPTY && pos->pieces[G1] == EMPTY)
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos))
					AddQuietMove(MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);

		if (pos->castlePerm & WQCA)
			if (pos->pieces[D1] == EMPTY && pos->pieces[C1] == EMPTY && pos->pieces[B1] == EMPTY)
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos))
					AddQuietMove(MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list);

	} else {

		for (pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (pos->pieces[sq - 10] == EMPTY) {
				AddBlackPawnMove(sq, sq - 10, list);
				if (RanksBrd[sq] == RANK_7 && pos->pieces[sq - 20] == EMPTY)
					AddQuietMove(MOVE(sq, sq - 20, EMPTY, EMPTY, MFLAGPS), list);
			}
		}

		// castling
		if (pos->castlePerm &  BKCA)
			if (pos->pieces[F8] == EMPTY && pos->pieces[G8] == EMPTY)
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos))
					AddQuietMove(MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);

		if (pos->castlePerm &  BQCA)
			if (pos->pieces[D8] == EMPTY && pos->pieces[C8] == EMPTY && pos->pieces[B8] == EMPTY)
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos))
					AddQuietMove(MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);
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
				t_sq = sq + PceDir[pce][index];

				while (!SQOFFBOARD(t_sq)) {
					if (pos->pieces[t_sq] != EMPTY)
						break;
					
					AddQuietMove(MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
					t_sq += PceDir[pce][index];
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
				t_sq = sq + PceDir[pce][index];

				if (SQOFFBOARD(t_sq))
					continue;

				if (pos->pieces[t_sq] != EMPTY)
					continue;
				
				AddQuietMove(MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}

    ASSERT(MoveListOk(list, pos));
}