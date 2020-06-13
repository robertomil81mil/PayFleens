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

// init.c

#include <stdio.h>
#include <stdlib.h>

#include "bitboards.h"
#include "defs.h"
#include "evaluate.h"
#include "search.h"
#include "ttable.h"

#define RAND_64 	((U64)rand() | \
					(U64)rand() << 15 | \
					(U64)rand() << 30 | \
					(U64)rand() << 45 | \
					((U64)rand() & 0xf) << 60 )

int Sq120ToSq64[BRD_SQ_NUM];
int Sq64ToSq120[64];

U64 SetMask[64];
U64 ClearMask[64];

U64 PieceKeys[13][120];
U64 SideKey;
U64 CastleKeys[16];

int FilesBrd[BRD_SQ_NUM];
int RanksBrd[BRD_SQ_NUM];

U64 PassedPawnMasks[2][64];
U64 OutpostSquareMasks[2][64];
U64 IsolatedMask[64];

int SquareDistance[120][120];
int FileDistance[120][120];
int RankDistance[120][120];

void InitEvalMasks() {

	int sq, tsq;

	for(sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = 0ULL;
		PassedPawnMasks[WHITE][sq] = 0ULL;
		PassedPawnMasks[BLACK][sq] = 0ULL;
		OutpostSquareMasks[WHITE][sq] = 0ULL;
		OutpostSquareMasks[BLACK][sq] = 0ULL;
    }

	for (sq = 0; sq < 64; ++sq) {

		PassedPawnMasks[WHITE][sq] = passedPawnSpan(WHITE, sq);
		PassedPawnMasks[BLACK][sq] = passedPawnSpan(BLACK, sq);

        if (file_of(sq) > FILE_A) {
            IsolatedMask[sq] |= FilesBB[file_of(sq) - 1];

            tsq = sq + 7;
            while(tsq < 64) {
                PassedPawnMasks[WHITE][sq] |= (1ULL << tsq);
                tsq += 8;
            }

            tsq = sq - 9;
            while(tsq >= 0) {
                PassedPawnMasks[BLACK][sq] |= (1ULL << tsq);
                tsq -= 8;
            }
        }

        if (file_of(sq) < FILE_H) {
            IsolatedMask[sq] |= FilesBB[file_of(sq) + 1];

            tsq = sq + 9;
            while(tsq < 64) {
                PassedPawnMasks[WHITE][sq] |= (1ULL << tsq);
                tsq += 8;
            }

            tsq = sq - 7;
            while(tsq >= 0) {
                PassedPawnMasks[BLACK][sq] |= (1ULL << tsq);
                tsq -= 8;
            }
        }
	}

	for (int colour = WHITE; colour < COLOUR_NB; ++colour) { 
        for (sq = 0; sq < 64; ++sq) 
        	OutpostSquareMasks[colour][sq] = PassedPawnMasks[colour][sq] & ~FilesBB[file_of(sq)];
    }
}

void InitFilesRanksBrd() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;
	int s1,s2;

	for(index = 0; index < BRD_SQ_NUM; ++index) {
		FilesBrd[index] = OFFBOARD;
		RanksBrd[index] = OFFBOARD;
	}

	for(rank = RANK_1; rank <= RANK_8; ++rank) {
		for(file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file,rank);
			FilesBrd[sq] = file;
			RanksBrd[sq] = rank;
		}
	}

	for (s1 = 0; s1 < 120; ++s1) {
      	for (s2 = 0; s2 < 120; ++s2) { 
      		FileDistance[s1][s2]   = abs(FilesBrd[s1] - FilesBrd[s2]);
            RankDistance[s1][s2]   = abs(RanksBrd[s1] - RanksBrd[s2]);
            SquareDistance[s1][s2] = MAX(FileDistance[s1][s2], RankDistance[s1][s2]);
      	}
	}
}

void InitHashKeys() {

	int index = 0;
	int index2 = 0;
	for(index = 0; index < 13; ++index) {
		for(index2 = 0; index2 < 120; ++index2) {
			PieceKeys[index][index2] = RAND_64;
		}
	}
	SideKey = RAND_64;
	for(index = 0; index < 16; ++index) {
		CastleKeys[index] = RAND_64;
	}

}

void InitBitMasks() {
	int index = 0;

	for(index = 0; index < 64; index++) {
		SetMask[index] = 0ULL;
		ClearMask[index] = 0ULL;
	}

	for(index = 0; index < 64; index++) {
		SetMask[index] |= (1ULL << index);
		ClearMask[index] = ~SetMask[index];
	}
}

void InitSq120To64() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;
	int sq64 = 0;
	for(index = 0; index < BRD_SQ_NUM; ++index) {
		Sq120ToSq64[index] = 65;
	}

	for(index = 0; index < 64; ++index) {
		Sq64ToSq120[index] = 120;
	}

	for(rank = RANK_1; rank <= RANK_8; ++rank) {
		for(file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file,rank);
			ASSERT(SqOnBoard(sq));
			Sq64ToSq120[sq64] = sq;
			Sq120ToSq64[sq] = sq64;
			sq64++;
		}
	}
}

U64 kingAreaMasks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return KingAreaMasks[colour][sq];
}

U64 outpostSquareMasks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return OutpostSquareMasks[colour][sq];
}

U64 pawnAttacks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return PawnAttacks[colour][sq];
}

void AllInit() {
	InitSq120To64();
	InitBitMasks();
	InitHashKeys();
	InitFilesRanksBrd();
	InitEvalMasks();
	InitMvvLva();
	initLMRTable();
	setSquaresNearKing();
	KingAreaMask();
	PawnAttacksMasks();
	setPcsq32();
	initTTable(16);
}