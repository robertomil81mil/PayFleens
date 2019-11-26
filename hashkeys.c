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

// hashkeys.c
#include "stdio.h"
#include "defs.h"

U64 GeneratePosKey(const S_BOARD *pos) {

	int sq = 0;
	U64 finalKey = 0;
	int piece = EMPTY;
	
	// pieces
	for(sq = 0; sq < BRD_SQ_NUM; ++sq) {
		piece = pos->pieces[sq];
		if(piece!=NO_SQ && piece!=EMPTY && piece != OFFBOARD) {
			ASSERT(piece>=wP && piece<=bK);
			finalKey ^= PieceKeys[piece][sq];
		}		
	}
	
	if(pos->side == WHITE) {
		finalKey ^= SideKey;
	}
		
	if(pos->enPas != NO_SQ) {
		ASSERT(pos->enPas>=0 && pos->enPas<BRD_SQ_NUM);
		ASSERT(SqOnBoard(pos->enPas));
		ASSERT(RanksBrd[pos->enPas] == RANK_3 || RanksBrd[pos->enPas] == RANK_6);
		finalKey ^= PieceKeys[EMPTY][pos->enPas];
	}
	
	ASSERT(pos->castlePerm>=0 && pos->castlePerm<=15);
	
	finalKey ^= CastleKeys[pos->castlePerm];
	
	return finalKey;
}