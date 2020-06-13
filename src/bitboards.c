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

// bitboards.c

#include <stdbool.h>
#include <stdio.h>

#include "bitboards.h"
#include "defs.h"

const uint64_t FilesBB[FILE_NB] = {FileABB, FileBBB, FileCBB, FileDBB, FileEBB, FileFBB, FileGBB, FileHBB};
const uint64_t RanksBB[RANK_NB] = {Rank1BB, Rank2BB, Rank3BB, Rank4BB, Rank5BB, Rank6BB, Rank7BB, Rank8BB};

const int Files[FILE_NB] = {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
const int Ranks[RANK_NB] = {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

int file_of(int sq) {
    ASSERT(0 <= sq && sq < 64);
    return sq % FILE_NB;
}

int rank_of(int sq) {
    ASSERT(0 <= sq && sq < 64);
    return sq / FILE_NB;
}

int map_to_queenside(int file) {
    ASSERT(0 <= file && file < FILE_NB);
    return MIN(file, FILE_H - file);
}

uint64_t adjacentFiles(int sq) {
	ASSERT(0 <= sq && sq < 64);
	return (  (FilesBB[file_of(sq)] & ~FilesBB[FILE_H]) << 1
			| (FilesBB[file_of(sq)] & ~FilesBB[FILE_A]) >> 1);
}

uint64_t forwardRanks(int colour, int sq) {
	ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
	return colour == WHITE ? ~RanksBB[RANK_1] << 8 * (rank_of(sq) - RANK_1)
                    	   : ~RanksBB[RANK_8] >> 8 * (RANK_8 - rank_of(sq));
}

uint64_t forwardFile(int colour, int sq) {
	ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
	return forwardRanks(colour, sq) & FilesBB[file_of(sq)];
}

uint64_t passedPawnSpan(int colour, int sq) {
	ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
	return forwardRanks(colour, sq) & (adjacentFiles(sq) | FilesBB[file_of(sq)]);
}

int relativeRank(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return colour == WHITE ? rank_of(sq) : 7 - rank_of(sq);
}

int relativeSquare(int colour, int sq) {
	ASSERT(0 <= colour && colour < COLOUR_NB);
	ASSERT(0 <= sq && sq < 120);
    return colour == WHITE ? sq : Mirror120[SQ64(sq)];
}

int relativeSquare32(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return 4 * relativeRank(colour, sq) + map_to_queenside(file_of(sq));
}

int distanceBetween(int s1, int s2) {
	ASSERT(0 <= s1 && s1 < 120);
    ASSERT(0 <= s2 && s2 < 120);
    return SquareDistance[s1][s2];
}

int distanceByFile(int s1, int s2) {
	ASSERT(0 <= s1 && s1 < 120);
    ASSERT(0 <= s2 && s2 < 120);
    return FileDistance[s1][s2];
}

int distanceByRank(int s1, int s2) {
	ASSERT(0 <= s1 && s1 < 120);
    ASSERT(0 <= s2 && s2 < 120);
    return RankDistance[s1][s2];
}

int clamp(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

int frontmost(int colour, U64 b) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    return colour == WHITE ? getmsb(b) : getlsb(b);
}

int backmost(int colour, U64 b) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    return colour == WHITE ? getlsb(b) : getmsb(b);
}

int getlsb(U64 bb) {
    return __builtin_ctzll(bb);
}

int getmsb(U64 bb) {
    return __builtin_clzll(bb) ^ 63;
}

int poplsb(uint64_t *bb) {
    int lsb = getlsb(*bb);
    *bb &= *bb - 1;
    return lsb;
}

int popmsb(uint64_t *bb) {
    int msb = getmsb(*bb);
    *bb ^= 1ull << msb;
    return msb;
}

int popcount(U64 bb) {
    return __builtin_popcountll(bb);
}

bool several(U64 bb) {
    return bb & (bb - 1);
}

bool onlyOne(U64 bb) {
    return bb && !several(bb);
}

bool opposite_colors(int s1, int s2) {
    ASSERT(0 <= s1 && s1 < 64);
    ASSERT(0 <= s2 && s2 < 64);
    return (s1 + rank_of(s1) + s2 + rank_of(s2)) & 1;
}

int PopBit(U64 *bb) {
	U64 b = *bb ^ (*bb - 1);
	unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
	*bb &= (*bb - 1);
	return BitTable[(fold * 0x783a9b23) >> 26];
}

int CountBits(U64 b) {
  int r;
  for(r = 0; b; r++, b &= b - 1);
  return r;
}

void PrintBitBoard(U64 bb) {

	U64 shiftMe = 1ULL;
	
	int rank = 0, file = 0, sq = 0, sq64 = 0;
	
	printf("\n");
	for(rank = RANK_8; rank >= RANK_1; --rank) {
		for(file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file,rank);	// 120 based		
			sq64 = SQ64(sq); // 64 based
			
			if((shiftMe << sq64) & bb) 
				printf("X");
			else 
				printf("-");
				
		}
		printf("\n");
	}  
    printf("\n\n");
}