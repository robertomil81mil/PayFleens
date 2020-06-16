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

#include "board.h"
#include "defs.h"
#include "hashkeys.h"
#include "validate.h"

uint64_t PieceKeys[13][120];
uint64_t SideKey;
uint64_t CastleKeys[16];

uint64_t rand64() {

    // http://vigna.di.unimi.it/ftp/papers/xorshift.pdf

    static uint64_t seed = 1070372ull;

    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 2685821657736338717ull;
}

void InitHashKeys() {

	int index, index2;

	for (index = 0; index < 13; ++index)
		for (index2 = 0; index2 < 120; ++index2)
			PieceKeys[index][index2] = rand64();

	for (index = 0; index < 16; ++index)
		CastleKeys[index] = rand64();

	SideKey = rand64();
}

uint64_t GeneratePosKey(const S_BOARD *pos) {

	uint64_t finalKey = 0;
	int sq, piece;
	
	// pieces
	for (sq = 0; sq < BRD_SQ_NUM; ++sq) {
		piece = pos->pieces[sq];
		if (piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
			ASSERT(piece >= wP && piece <= bK);
			finalKey ^= PieceKeys[piece][sq];
		}		
	}
		
	if (pos->enPas != NO_SQ) {
		ASSERT(pos->enPas >= 0 && pos->enPas < BRD_SQ_NUM);
		ASSERT(SqOnBoard(pos->enPas));
		ASSERT(RanksBrd[pos->enPas] == RANK_3 || RanksBrd[pos->enPas] == RANK_6);
		finalKey ^= PieceKeys[EMPTY][pos->enPas];
	}

	if (pos->side == WHITE)
		finalKey ^= SideKey;
	
	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);
	
	finalKey ^= CastleKeys[pos->castlePerm];
	
	return finalKey;
}

uint64_t GenerateMaterialKey(const S_BOARD *pos) {

	uint64_t materialKey = 0;

	for (int pc = wP; pc <= bK; ++pc) {
		if (pos->pceNum[pc]) {		
			for (int cnt = 0; cnt < pos->pceNum[pc]; ++cnt) {
				ASSERT(cnt >= 0);
				materialKey ^= PieceKeys[pc][cnt];
			}
		}
	}
	
	return materialKey;
}
