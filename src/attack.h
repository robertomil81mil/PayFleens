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

#include "defs.h"
#include "data.h"

#define IsBQ(p) (PieceBishopQueen[(p)])
#define IsRQ(p) (PieceRookQueen[(p)])
#define IsKn(p) (PieceKnight[(p)])
#define IsKi(p) (PieceKing[(p)])

int SqAttacked(const int sq, const int side, const Board *pos);
int BishopAttack(int side, int sq, int dir, const Board *pos);
int KnightAttack(int side, int sq, const Board *pos);
int KingSqAttacked(const Board *pos);

static const int KnDir[8] = { -8, -19,	-21, -12, 8, 19, 21, 12 };
static const int RkDir[4] = { -1, -10,	1, 10 };
static const int BiDir[4] = { -9, -11, 11, 9 };
static const int KiDir[8] = { -1, -10,	1, 10, -9, -11, 11, 9 };