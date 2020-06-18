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

#include <stdbool.h>

#include "defs.h"

static const int PieceValue[PHASE_NB][PIECE_NB] = {
	{ 0,  70, 433, 459, 714, 1401, 0,  70, 433, 459, 714, 1401, 0 },
	{ 0, 118, 479, 508, 763, 1488, 0, 118, 479, 508, 763, 1488, 0 }
};

static const int PieceCol[13] = { COLOUR_NB, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };

static const int PieceBig[13] = { false, false, true, true, true, true, true, false, true, true, true, true, true };
static const int PieceMaj[13] = { false, false, false, false, true, true, true, false, false, false, true, true, true };
static const int PieceMin[13] = { false, false, true, true, false, false, false, false, true, true, false, false, false };
static const int PiecePawn[13] = { false, true, false, false, false, false, false, true, false, false, false, false, false };
static const int PiecePawnW[13] = { false, true, false, false, false, false, false, false, false, false, false, false, false };	
static const int PiecePawnB[13] = { false, false, false, false, false, false, false, true, false, false, false, false, false };
static const int PieceKnight[13] = { false, false, true, false, false, false, false, false, true, false, false, false, false };
static const int PieceKing[13] = { false, false, false, false, false, false, true, false, false, false, false, false, true };
static const int PieceBishop[13] = { false, false, false, true, false, false, false, false, false, true, false, false, false };
static const int PieceRook[13] = { false, false, false, false, true, false, false, false, false, false, true, false, false };
static const int PieceRookQueen[13] = { false, false, false, false, true, true, false, false, false, false, true, true, false };
static const int PieceBishopQueen[13] = { false, false, false, true, false, true, false, false, false, true, false, true, false };
static const int PieceSlides[13] = { false, false, false, true, true, true, false, false, false, true, true, true, false };

static const int PceDir[13][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    {  9, 11, 0, 0, 0, 0, 0, 0 },
    { -8, -19,  -21, -12, 8, 19, 21, 12 },
    { -9, -11, 11, 9, 0, 0, 0, 0 },
    { -1, -10,  1, 10, 0, 0, 0, 0 },
    { -1, -10,  1, 10, -9, -11, 11, 9 },
    { -1, -10,  1, 10, -9, -11, 11, 9 },
    { -9, -11, 0, 0, 0, 0, 0, 0 },
    { -8, -19,  -21, -12, 8, 19, 21, 12 },
    { -9, -11, 11, 9, 0, 0, 0, 0 },
    { -1, -10,  1, 10, 0, 0, 0, 0 },
    { -1, -10,  1, 10, -9, -11, 11, 9 },
    { -1, -10,  1, 10, -9, -11, 11, 9 }
};

static const int NumDir[13] = {
 0, 2, 8, 4, 4, 8, 8, 2, 8, 4, 4, 8, 8
};

static const int Mirror64[64] = {
56	,	57	,	58	,	59	,	60	,	61	,	62	,	63	,
48	,	49	,	50	,	51	,	52	,	53	,	54	,	55	,
40	,	41	,	42	,	43	,	44	,	45	,	46	,	47	,
32	,	33	,	34	,	35	,	36	,	37	,	38	,	39	,
24	,	25	,	26	,	27	,	28	,	29	,	30	,	31	,
16	,	17	,	18	,	19	,	20	,	21	,	22	,	23	,
8	,	9	,	10	,	11	,	12	,	13	,	14	,	15	,
0	,	1	,	2	,	3	,	4	,	5	,	6	,	7
};

static const int Mirror120[64] = {
	A8	,	B8	,	C8	,	D8	,	E8	,	F8	,	G8	,	H8	,
	A7	,	B7	,	C7	,	D7	,	E7	,	F7	,	G7	,	H7	,
	A6	,	B6	,	C6	,	D6	,	E6	,	F6	,	G6	,	H6	,
	A5	,	B5	,	C5	,	D5	,	E5	,	F5	,	G5	,	H5	,
	A4	,	B4	,	C4	,	D4	,	E4	,	F4	,	G4	,	H4	,
	A3	,	B3	,	C3	,	D3	,	E3	,	F3	,	G3	,	H3	,
	A2	,	B2	,	C2	,	D2	,	E2	,	F2	,	G2	,	H2	,
	A1	,	B1	,	C1	,	D1	,	E1	,	F1	,	G1	,	H1
};
