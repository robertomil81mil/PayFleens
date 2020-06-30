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
#include "board.h"
#include "data.h"
#include "defs.h"
#include "evaluate.h"
#include "hashkeys.h"
#include "init.h"
#include "movegen.h"
#include "search.h"
#include "ttable.h"

uint64_t PassedPawnMasks[COLOUR_NB][64];
uint64_t OutpostSquareMasks[COLOUR_NB][64];
uint64_t PawnAttacks[COLOUR_NB][64];
uint64_t KingAreaMasks[64];
uint64_t IsolatedMask[64];

uint64_t kingAreaMasks(int sq) {
    ASSERT(0 <= sq && sq < 64);
    return KingAreaMasks[sq];
}

uint64_t outpostSquareMasks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return OutpostSquareMasks[colour][sq];
}

uint64_t pawnAttacks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return PawnAttacks[colour][sq];
}

void KingAreaMask() {

	int sq, t_sq;

    for (sq = 0; sq < 64; ++sq)
		KingAreaMasks[sq] = 0ull;

    for (sq = 0; sq < BRD_SQ_NUM; ++sq) {
		if (!SQOFFBOARD(sq)) {
			for (int i = 0; i < NumDir[wK]; ++i) {
				t_sq = sq + PceDir[wK][i];
				KingAreaMasks[SQ64(sq)] |= (1ull << SQ64(sq));
				if (!SQOFFBOARD(t_sq))
					KingAreaMasks[SQ64(sq)] |= (1ull << SQ64(t_sq));
			}
		}	
	}
}

void PawnAttacksMasks() {

	int sq, t_sq, pce, index;

	for (sq = 0; sq < 64; ++sq) {
		PawnAttacks[WHITE][sq] = 0ull;
		PawnAttacks[BLACK][sq] = 0ull;
	}

	pce = wP;
	for (sq = 0; sq < BRD_SQ_NUM; ++sq) {
		if (!SQOFFBOARD(sq)) {
			for (index = 0; index < NumDir[pce]; ++index) {
				t_sq = sq + PceDir[pce][index];
				if (!SQOFFBOARD(t_sq))
					PawnAttacks[WHITE][SQ64(sq)] |= (1ull << SQ64(t_sq));
			}
		}	
	}

	pce = bP;
	for (sq = 0; sq < BRD_SQ_NUM; ++sq) {
		if (!SQOFFBOARD(sq)) {
			for (index = 0; index < NumDir[pce]; ++index) {
				t_sq = sq + PceDir[pce][index];
				if (!SQOFFBOARD(t_sq))
					PawnAttacks[BLACK][SQ64(sq)] |= (1ull << SQ64(t_sq));
			}
		}	
	}
}

void setSquaresNearKing() {

    for (int i = 0; i < 120; ++i) {
        for (int j = 0; j < 120; ++j) {

            e.sqNearK[i][j] = 0;
            // e.sqNearK [ KingSq[!side] ] [t_sq] 

            if (!SQOFFBOARD(i) && !SQOFFBOARD(j)) {

            	if (j == i)
            		e.sqNearK[i][j] = 1;

                /* squares constituting the ring around both kings */
                if (   j == i + 10 || j == i - 10 
					|| j == i + 1  || j == i - 1 
					|| j == i + 9  || j == i - 9 
					|| j == i + 11 || j == i - 11)
                	e.sqNearK[i][j] = 1;
            }
        }
    }
}

void InitEvalMasks() {

	for (int sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = adjacentFiles(sq);
		PassedPawnMasks[WHITE][sq] = passedPawnSpan(WHITE, sq);
		PassedPawnMasks[BLACK][sq] = passedPawnSpan(BLACK, sq);
		OutpostSquareMasks[WHITE][sq] = PassedPawnMasks[WHITE][sq] & ~FilesBB[file_of(sq)];
		OutpostSquareMasks[BLACK][sq] = PassedPawnMasks[BLACK][sq] & ~FilesBB[file_of(sq)];
    }
}

void AllInit() {
	InitSq120To64();
	InitHashKeys();
	InitFilesRanksBrd();
	InitEvalMasks();
	initLMRTable();
	setSquaresNearKing();
	KingAreaMask();
	PawnAttacksMasks();
	setPcsq32();
	initTTable(16);
}