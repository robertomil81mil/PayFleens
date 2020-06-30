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

struct Move {
	int move;
	int score;
};

struct MoveList {
	Move moves[MAX_MOVES];
	int count;
	int quiets;
};

struct Undo {
	uint64_t posKey, materialKey;
	int move, enPas, castlePerm;
	int fiftyMove, plyFromNull;
};

int MakeMove(Board *pos, int move);
void TakeMove(Board *pos);
void MakeNullMove(Board *pos);
void TakeNullMove(Board *pos);

int LegalMoveExist(Board *pos);
int MoveExists(MoveList *list, const int move);
int moveIsPseudoLegal(Board *pos, int move);
int moveIsQuiet(int move);

int moveBestCaseValue(const Board *pos);
int badCapture(int move, const Board *pos);
int move_canSimplify(int move, const Board *pos);
int advancedPawnPush(int move, const Board *pos);
int see(const Board *pos, int move, int threshold);

/* MOVE

0000 0000 0000 0000 0000 0111 1111 -> From 0x7F
0000 0000 0000 0011 1111 1000 0000 -> To >> 7, 0x7F
0000 0000 0011 1100 0000 0000 0000 -> Captured >> 14, 0xF
0000 0000 0100 0000 0000 0000 0000 -> EP 0x40000
0000 0000 1000 0000 0000 0000 0000 -> Pawn Start 0x80000
0000 1111 0000 0000 0000 0000 0000 -> Promoted Piece >> 20, 0xF
0001 0000 0000 0000 0000 0000 0000 -> Castle 0x1000000
*/

#define FROMSQ(m)    ((m) & 0x7F)
#define TOSQ(m)     (((m) >> 7) & 0x7F)
#define CAPTURED(m) (((m) >> 14) & 0xF)
#define PROMOTED(m) (((m) >> 20) & 0xF)

#define MFLAGCAP 0x7C000
#define MFLAGPROM 0xF00000
#define MFLAGEP 0x40000
#define MFLAGPS 0x80000
#define MFLAGCA 0x1000000