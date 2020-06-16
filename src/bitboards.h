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

extern const uint64_t FilesBB[FILE_NB];
extern const uint64_t RanksBB[RANK_NB];
extern const int Files[FILE_NB];
extern const int Ranks[RANK_NB];

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

int frontmost(int colour, uint64_t bb);
int backmost(int colour, uint64_t bb);

int getlsb(uint64_t bb);
int getmsb(uint64_t bb);

int poplsb(uint64_t *bb);
int popmsb(uint64_t *bb);

void setBit(uint64_t *bb, int i);
void clearBit(uint64_t *bb, int i);
bool testBit(uint64_t bb, int i);

int popcount(uint64_t bb);
bool several(uint64_t bb);
bool onlyOne(uint64_t bb);
void PrintBitBoard(uint64_t bb);

uint64_t forwardRanks(int colour, int sq);
uint64_t forwardFile(int colour, int sq);
uint64_t passedPawnSpan(int colour, int sq);
uint64_t shift(uint64_t bb, int D);
uint64_t adjacentFiles(int sq);

static const uint64_t KingFlank[8] = {
	QUEEN_FLANK ^ FileDBB, QUEEN_FLANK, QUEEN_FLANK,
	CENTER_FLANK, CENTER_FLANK,
	KING_FLANK, KING_FLANK, KING_FLANK ^ FileEBB
};