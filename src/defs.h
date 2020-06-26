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
printf("In File %s ",__FILE__); \
printf("At Line %d\n",__LINE__); \
exit(1);}
#endif

enum { WHITE, BLACK, COLOUR_NB };

enum { NOMOVE = 0, NULL_MOVE = 22 };

enum { MG = 0, EG = 1, PHASE_NB = 2 };

enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK, PIECE_NB };

enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };

enum {
	A1 = 21, B1, C1, D1, E1, F1, G1, H1,
	A2 = 31, B2, C2, D2, E2, F2, G2, H2,
	A3 = 41, B3, C3, D3, E3, F3, G3, H3,
	A4 = 51, B4, C4, D4, E4, F4, G4, H4,
	A5 = 61, B5, C5, D5, E5, F5, G5, H5,
	A6 = 71, B6, C6, D6, E6, F6, G6, H6,
	A7 = 81, B7, C7, D7, E7, F7, G7, H7,
	A8 = 91, B8, C8, D8, E8, F8, G8, H8, NO_SQ, OFFBOARD,
	BRD_SQ_NUM = 120
};

enum {
	MAX_PLY = 128, 
	MAXPOSITIONMOVES = 256, 
	MAXGAMEMOVES = 2048
};

enum {
	VALUE_DRAW = 0,
	KNOWN_WIN  = 10000,
    INFINITE   = 32000,
    VALUE_NONE = 32001,
    MATE_IN_MAX  = INFINITE - MAX_PLY,
    MATED_IN_MAX = MAX_PLY - INFINITE
};

enum {
	NORTH =  8,
	EAST  =  1,
	SOUTH = -NORTH,
	WEST  = -EAST,

	NORTH_EAST = NORTH + EAST,
	SOUTH_EAST = SOUTH + EAST,
	SOUTH_WEST = SOUTH + WEST,
	NORTH_WEST = NORTH + WEST
};

#define PRIu64 "I64u"

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

#define clamp(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))

typedef struct Board Board;
typedef struct EngineOptions EngineOptions;
typedef struct evalInfo evalInfo;
typedef struct evalData evalData;
typedef struct EvalTrace EvalTrace;
typedef struct KPKPos KPKPos;
typedef struct Material_Entry Material_Entry;
typedef struct Material_Table Material_Table;
typedef struct Move Move;
typedef struct MoveList MoveList;
typedef struct Limits Limits;
typedef struct PVariation PVariation;
typedef struct TTable TTable;
typedef struct TT_Cluster TT_Cluster;
typedef struct TT_Entry TT_Entry;
typedef struct SearchInfo SearchInfo;
typedef struct Undo Undo;