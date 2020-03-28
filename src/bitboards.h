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

enum { NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

enum {
    Rank1BB = 0x00000000000000FFull,
    Rank2BB = 0x000000000000FF00ull,
    Rank3BB = 0x0000000000FF0000ull,
    Rank4BB = 0x00000000FF000000ull,
    Rank5BB = 0x000000FF00000000ull,
    Rank6BB = 0x0000FF0000000000ull,
    Rank7BB = 0x00FF000000000000ull,
    Rank8BB = 0xFF00000000000000ull,

    FileABB = 0x0101010101010101ull,
    FileBBB = 0x0202020202020202ull,
    FileCBB = 0x0404040404040404ull,
    FileDBB = 0x0808080808080808ull,
    FileEBB = 0x1010101010101010ull,
    FileFBB = 0x2020202020202020ull,
    FileGBB = 0x4040404040404040ull,
    FileHBB = 0x8080808080808080ull,

    WHITE_SQUARES = 0x55AA55AA55AA55AAull,
    BLACK_SQUARES = 0xAA55AA55AA55AA55ull,

    QUEEN_FLANK  = FileABB | FileBBB | FileCBB | FileDBB,
    CENTER_FLANK = FileCBB | FileDBB | FileEBB | FileFBB,
    KING_FLANK   = FileEBB | FileFBB | FileGBB | FileHBB,
};

extern int SquareDistance[120][120];
extern int FileDistance[120][120];
extern int RankDistance[120][120];

int file_of(int sq);
int rank_of(int sq);
int map_to_queenside(int file);

int relativeRank(int colour, int sq);
int relativeSquare(int colour, int sq);
int relativeSquare32(int colour, int sq);

int distanceBetween(int s1, int s2);
int distanceByFile(int s1, int s2);
int distanceByRank(int s1, int s2);
bool opposite_colors(int s1, int s2);

int clamp(int v, int lo, int hi);
int frontmost(int colour, U64 b);
int backmost(int colour, U64 b);

int getlsb(U64 bb);
int getmsb(U64 bb);

int popcount(U64 bb);
bool several(U64 bb);
bool onlyOne(U64 bb);

static const U64 KingFlank[8] = {
	QUEEN_FLANK ^ FileDBB, QUEEN_FLANK, QUEEN_FLANK,
	CENTER_FLANK, CENTER_FLANK,
	KING_FLANK, KING_FLANK, KING_FLANK ^ FileEBB
};

static const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
  58, 20, 37, 17, 36, 8
};