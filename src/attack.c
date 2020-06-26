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

// attack.c

#include <stdio.h>

#include "attack.h"
#include "defs.h"
#include "board.h"
#include "data.h"
#include "validate.h"

int SqAttacked(const int sq, const int side, const Board *pos) {

	int pce, index, t_sq;
	
	ASSERT(SqOnBoard(sq));
	ASSERT(SideValid(side));
	ASSERT(CheckBoard(pos));
	
	// pawns
	if (side == WHITE) {
		if (   pos->pieces[sq-11] == wP
			|| pos->pieces[sq-9] == wP) return 1;
	}

	else {
		if (   pos->pieces[sq+11] == bP
			|| pos->pieces[sq+9] == bP) return 1;
	}
	
	// knights
	for (index = 0; index < 8; ++index) {
		pce = pos->pieces[sq + KnDir[index]];
		ASSERT(PceValidEmptyOffbrd(pce));
		if (pce != OFFBOARD && IsKn(pce) && PieceCol[pce] == side)
			return 1;
	}
	
	// rooks, queens
	for (index = 0; index < 4; ++index) {
		t_sq = sq + RkDir[index];
		ASSERT(SqIs120(t_sq));
		pce = pos->pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));
		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsRQ(pce) && PieceCol[pce] == side)
					return 1;

				break;
			}
			t_sq += RkDir[index];
			ASSERT(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}
	
	// bishops, queens
	for(index = 0; index < 4; ++index) {
		t_sq = sq + BiDir[index];
		ASSERT(SqIs120(t_sq));
		pce = pos->pieces[t_sq];
		ASSERT(PceValidEmptyOffbrd(pce));
		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsBQ(pce) && PieceCol[pce] == side)
					return 1;
				
				break;
			}
			t_sq += BiDir[index];
			ASSERT(SqIs120(t_sq));
			pce = pos->pieces[t_sq];
		}
	}
	
	// kings
	for (index = 0; index < 8; ++index) {		
		pce = pos->pieces[sq + KiDir[index]];
		ASSERT(PceValidEmptyOffbrd(pce));
		if (pce != OFFBOARD && IsKi(pce) && PieceCol[pce] == side)
			return 1;
	}
	
	return 0;
}

int KingSqAttacked(const Board *pos) {
	return SqAttacked(pos->KingSq[pos->side], !pos->side, pos);
}

int KnightAttack(int side, int sq, const Board *pos) {
    int Knight = side == WHITE ? wN : bN, t_sq;

    for (int index = 0; index < 8; ++index) {
        t_sq = sq + PceDir[wN][index];
        if (!SQOFFBOARD(t_sq) && pos->pieces[t_sq] == Knight)
            return 1;
    }
    return 0;
}

int BishopAttack(int side, int sq, int dir, const Board *pos) {
    int Bishop = side == WHITE ? wB : bB;
    int t_sq = sq + dir;

    while (!SQOFFBOARD(t_sq)) {
        if (pos->pieces[t_sq] != EMPTY) {
            if (pos->pieces[t_sq] == Bishop)
                return 1;
            return 0;
        }
        t_sq += dir;
    }
    return 0;
}