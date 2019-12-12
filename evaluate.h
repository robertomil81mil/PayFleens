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

#include <stdint.h>
#include "defs.h"

#ifndef EVAL_H
#define EVAL_H

#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define ScoreMG(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define ScoreEG(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

enum {
    SCALE_OCB_BISHOPS_ONLY =  64,
    SCALE_OCB_ONE_KNIGHT   = 106,
    SCALE_OCB_ONE_ROOK     =  96,
    SCALE_NORMAL           = 128,
};

const int PceDir[13][8] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{  9, 11, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -9, -11, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 }
};

const int NumDir[13] = {
 0, 2, 8, 4, 4, 8, 8, 2, 8, 4, 4, 8, 8
};

const int BishopPair = 42;
const int KnightPair = 11;

const int P_KING_BLOCKS_ROOK   = 24;
const int P_BLOCK_CENTRAL_PAWN = 14;
const int P_BISHOP_TRAPPED_A7  = 150;
const int P_BISHOP_TRAPPED_A6  = 50;
const int P_KNIGHT_TRAPPED_A8  = 150;
const int P_KNIGHT_TRAPPED_A7  = 100;
const int P_C3_KNIGHT = 30;
const int P_NO_FIANCHETTO = 4;
const int RETURNING_BISHOP = 20;
const int FIANCHETTO = 4;
const int TEMPO = 10;

const int Weight[13] = { 0, 4, 16, 10, 8, 2, 0, 4, 16, 10, 8, 2, 0};

const int ShelterStrength[4][8] = {
    {  -6,  81,  93,  58,  39,  18,   25 },
    { -43,  61,  35, -49, -29, -11,  -63 },
    { -10,  75,  23,  -2,  32,   3,  -45 },
    { -39, -13, -29, -52, -48, -67, -166 }
};

const int UnblockedStorm[4][8] = {
    {  89, -285, -185, 93, 57,  45,  51 },
    {  44,  -18,  123, 46, 39,  -7,  23 },
    {   4,   52,  162, 37,  7, -14,  -2 },
    { -10,  -14,   90, 15,  2,  -7, -16 }
};

const int PushToEdges[64] = {
  100, 90, 80, 70, 70, 80, 90, 100,
   90, 70, 60, 50, 50, 60, 70,  90,
   80, 60, 40, 30, 30, 40, 60,  80,
   70, 50, 30, 20, 20, 30, 50,  70,
   70, 50, 30, 20, 20, 30, 50,  70,
   80, 60, 40, 30, 30, 40, 60,  80,
   90, 70, 60, 50, 50, 60, 70,  90,
  100, 90, 80, 70, 70, 80, 90, 100
};

const int PushClose[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

const int n_adj[9] = { -27, -22, -16, -10, -5,  0,  5, 10, 16};
const int r_adj[9] = {  20,  16,  12,  8,  4,  0, -4, -8, -12}; 

#define ENDGAME_MAT (1 * PieceVal[wR] + 2 * PieceVal[wN] + 2 * PieceVal[wP] + PieceVal[wK])

#endif
