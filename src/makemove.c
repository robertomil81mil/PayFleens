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

// makemove.c

#include <stdio.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "defs.h"
#include "endgame.h"
#include "evaluate.h"
#include "hashkeys.h"
#include "makemove.h"
#include "movegen.h"
#include "validate.h"

const int CastlePerm[120] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

static void ClearPiece(const int sq, Board *pos) {

	ASSERT(SqOnBoard(sq));
	ASSERT(CheckBoard(pos));
	
    int pce = pos->pieces[sq];
	
    ASSERT(PieceValid(pce));
	
	int col = PieceCol[pce];
	int index = 0;
	int t_pceNum = -1;
	
	ASSERT(SideValid(col));
	
    HASH_PCE(pce, sq);
	
	pos->pieces[sq] = EMPTY;
	pos->mPhases[col] -= PieceValPhases[pce];
    pos->material[col] -= PieceValue[EG][pce];
	
	if (PieceBig[pce]) {
		pos->bigPce[col]--;
	} else {
		clearBit(&pos->pawns[col], SQ64(sq));
		clearBit(&pos->pawns[COLOUR_NB], SQ64(sq));

		int ne = (col == WHITE ? 9 : -9);
		int nw = (col == WHITE ? 11 : -11);
		pos->pawn_ctrl[col][sq + ne]--;
		pos->pawn_ctrl[col][sq + nw]--;
	}

	pos->PSQT[col] -= e.PSQT[pce][sq];
    
	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == sq) {
			t_pceNum = index;
			break;
		}
	}
	
	ASSERT(t_pceNum != -1);
	ASSERT(t_pceNum >= 0 && t_pceNum < 10);
	
	pos->pceNum[pce]--;
	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];
}


static void AddPiece(const int sq, Board *pos, const int pce) {

    ASSERT(PieceValid(pce));
    ASSERT(SqOnBoard(sq));
	
	int col = PieceCol[pce];
	ASSERT(SideValid(col));

    HASH_PCE(pce,sq);
	
	pos->pieces[sq] = pce;

    if (PieceBig[pce]) {
		pos->bigPce[col]++;
	} else {
		setBit(&pos->pawns[col], SQ64(sq));
		setBit(&pos->pawns[COLOUR_NB], SQ64(sq));

		int ne = (col == WHITE ? 9 : -9);
		int nw = (col == WHITE ? 11 : -11);
		pos->pawn_ctrl[col][sq + ne]++;
		pos->pawn_ctrl[col][sq + nw]++;
	}

    // update piece-square value
    pos->PSQT[col] += e.PSQT[pce][sq];
	pos->mPhases[col] += PieceValPhases[pce];
	pos->material[col] += PieceValue[EG][pce];
	pos->pList[pce][pos->pceNum[pce]++] = sq;
}

static void MovePiece(const int from, const int to, Board *pos) {

    ASSERT(SqOnBoard(from));
    ASSERT(SqOnBoard(to));
	
	int index = 0;
	int pce = pos->pieces[from];	
	int col = PieceCol[pce];
	ASSERT(SideValid(col));
    ASSERT(PieceValid(pce));
	
#ifdef DEBUG
	int t_PieceNum = 0;
#endif

	HASH_PCE(pce, from);
	pos->pieces[from] = EMPTY;
	pos->PSQT[col] -= e.PSQT[pce][from];

	HASH_PCE(pce, to);
	pos->pieces[to] = pce;

    // update piece-square value
    pos->PSQT[col] += e.PSQT[pce][to];
	
	if (!PieceBig[pce]) {
		clearBit(&pos->pawns[col], SQ64(from));
		clearBit(&pos->pawns[COLOUR_NB], SQ64(from));
		setBit(&pos->pawns[col], SQ64(to));
		setBit(&pos->pawns[COLOUR_NB], SQ64(to));

		int ne = (col == WHITE ? 9 : -9);
		int nw = (col == WHITE ? 11 : -11);
		pos->pawn_ctrl[col][from + ne]--;
		pos->pawn_ctrl[col][from + nw]--;
		pos->pawn_ctrl[col][to + ne]++;
		pos->pawn_ctrl[col][to + nw]++;
	}    
	
	for (index = 0; index < pos->pceNum[pce]; ++index) {
		if (pos->pList[pce][index] == from) {
			pos->pList[pce][index] = to;
#ifdef DEBUG
			t_PieceNum = 1;
#endif
			break;
		}
	}
	ASSERT(t_PieceNum);
}

int MakeMove(Board *pos, int move) {

	ASSERT(CheckBoard(pos));
	
	int from = FROMSQ(move);
    int to = TOSQ(move);
    int side = pos->side;
	
	ASSERT(SqOnBoard(from));
    ASSERT(SqOnBoard(to));
    ASSERT(SideValid(side));
    ASSERT(PieceValid(pos->pieces[from]));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);
	
	pos->history[pos->hisPly].posKey = pos->posKey;
	pos->history[pos->hisPly].materialKey = pos->materialKey;
	
	if (move & MFLAGEP) {
        if (side == WHITE)
            ClearPiece(to - 10, pos);
        else
            ClearPiece(to + 10, pos);
    } else if (move & MFLAGCA) {
        switch(to) {
            case C1:
                MovePiece(A1, D1, pos);
			break;
            case C8:
                MovePiece(A8, D8, pos);
			break;
            case G1:
                MovePiece(H1, F1, pos);
			break;
            case G8:
                MovePiece(H8, F8, pos);
			break;
            default: ASSERT(0); break;
        }
    }	
	
	if (pos->enPas != NO_SQ) HASH_EP;
    HASH_CA;
	
	pos->history[pos->hisPly].move = move;
    pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
    pos->history[pos->hisPly].enPas = pos->enPas;
    pos->history[pos->hisPly].castlePerm = pos->castlePerm;
    pos->history[pos->hisPly].plyFromNull = pos->plyFromNull;

    pos->castlePerm &= CastlePerm[from];
    pos->castlePerm &= CastlePerm[to];
    pos->enPas = NO_SQ;

	HASH_CA;
	
	int captured = CAPTURED(move);
    pos->fiftyMove++;
	
	if (captured != EMPTY) {
        ASSERT(PieceValid(captured));
        ClearPiece(to, pos);
        // Update material hash key and prefetch access to materialTable
      	pos->materialKey ^= PieceKeys[captured][pos->pceNum[captured]];
      	Material_Entry *entry = &Table.entry[pos->materialKey >> MT_HASH_SHIFT];
      	__builtin_prefetch(entry);
        pos->fiftyMove = 0;
    }

    pos->plyFromNull++;
	pos->hisPly++;
	pos->gamePly++;
	pos->ply++;
	
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);
	
	if (PiecePawn[pos->pieces[from]]) {
        pos->fiftyMove = 0;
        if (move & MFLAGPS) {
            if (side == WHITE) {
                pos->enPas = from + 10;
                ASSERT(RanksBrd[pos->enPas] == RANK_3);
            } else {
                pos->enPas = from - 10;
                ASSERT(RanksBrd[pos->enPas] == RANK_6);
            }
            HASH_EP;
        }
    }
	
	MovePiece(from, to, pos);
	
	int prPce = PROMOTED(move);
    if (prPce != EMPTY)   {
        ASSERT(PieceValid(prPce) && !PiecePawn[prPce]);
        ClearPiece(to, pos);
        AddPiece(to, pos, prPce);
        // Update material hash key and prefetch access to materialTable
        pos->materialKey ^=  PieceKeys[prPce][pos->pceNum[prPce]-1]
                           ^ PieceKeys[pos->pieces[to]][pos->pceNum[pos->pieces[to]]];
        
        Material_Entry *entry = &Table.entry[pos->materialKey >> MT_HASH_SHIFT];
      	__builtin_prefetch(entry);
    }
	
	if (PieceKing[pos->pieces[to]])
        pos->KingSq[pos->side] = to;
	
	pos->side ^= 1;
    HASH_SIDE;

    ASSERT(CheckBoard(pos));
		
	if (SqAttacked(pos->KingSq[side],pos->side,pos)) {
        TakeMove(pos);
        return 0;
    }
	
	return 1;
}

void TakeMove(Board *pos) {
	
	ASSERT(CheckBoard(pos));
	
	pos->hisPly--;
	pos->gamePly--;
    pos->ply--;
	
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);
	
    int move = pos->history[pos->hisPly].move;
    int from = FROMSQ(move);
    int to = TOSQ(move);	
	
	ASSERT(SqOnBoard(from));
    ASSERT(SqOnBoard(to));
	
	if (pos->enPas != NO_SQ) HASH_EP;
    HASH_CA;

    pos->plyFromNull = pos->history[pos->hisPly].plyFromNull;
    pos->materialKey = pos->history[pos->hisPly].materialKey;
    pos->castlePerm = pos->history[pos->hisPly].castlePerm;
    pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
    pos->enPas = pos->history[pos->hisPly].enPas;

    if (pos->enPas != NO_SQ) HASH_EP;
    HASH_CA;

    pos->side ^= 1;
    HASH_SIDE;
	
	if (MFLAGEP & move) {
        if (pos->side == WHITE)
            AddPiece(to - 10, pos, bP);
        else
            AddPiece(to + 10, pos, wP);
    } else if (MFLAGCA & move) {
        switch(to) {
            case C1: 
            	MovePiece(D1, A1, pos); 
            break;
            case C8:
            	MovePiece(D8, A8, pos); 
            break;
            case G1:
            	MovePiece(F1, H1, pos); 
            break;
            case G8: 
            	MovePiece(F8, H8, pos); 
            	break;
            default: ASSERT(0); break;
        }
    }
	
	MovePiece(to, from, pos);
	
	if (PieceKing[pos->pieces[from]])
        pos->KingSq[pos->side] = from;
	
	int captured = CAPTURED(move);
    if (captured != EMPTY) {
        ASSERT(PieceValid(captured));
        AddPiece(to, pos, captured);
    }
	
	if (PROMOTED(move) != EMPTY) {
        ASSERT(PieceValid(PROMOTED(move)) && !PiecePawn[PROMOTED(move)]);
        ClearPiece(from, pos);
        AddPiece(from, pos, (PieceCol[PROMOTED(move)] == WHITE ? wP : bP));
    }
	
    ASSERT(CheckBoard(pos));
}

void MakeNullMove(Board *pos) {

    ASSERT(CheckBoard(pos));
    ASSERT(!SqAttacked(pos->KingSq[pos->side],pos->side^1,pos));

    pos->ply++;
    pos->history[pos->hisPly].posKey = pos->posKey;

    if (pos->enPas != NO_SQ) HASH_EP;

    pos->history[pos->hisPly].move = NONE_MOVE;
    pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
    pos->history[pos->hisPly].enPas = pos->enPas;
    pos->history[pos->hisPly].castlePerm = pos->castlePerm;
    pos->history[pos->hisPly].plyFromNull = pos->plyFromNull;
    pos->enPas = NO_SQ;

    pos->side ^= 1;
    pos->hisPly++;
    pos->plyFromNull = 0;
    HASH_SIDE;
   
    ASSERT(CheckBoard(pos));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);

    return;
} // MakeNullMove

void TakeNullMove(Board *pos) {
    ASSERT(CheckBoard(pos));

    pos->hisPly--;
    pos->ply--;

    if (pos->enPas != NO_SQ) HASH_EP;

    pos->plyFromNull = pos->history[pos->hisPly].plyFromNull;
    pos->castlePerm = pos->history[pos->hisPly].castlePerm;
    pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
    pos->enPas = pos->history[pos->hisPly].enPas;

    if (pos->enPas != NO_SQ) HASH_EP;
    pos->side ^= 1;
    HASH_SIDE;
  
    ASSERT(CheckBoard(pos));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAX_PLY);
}

int LegalMoveExist(Board *pos) {

	MoveList list = {0};
    GenerateAllMoves(pos, &list);

    int played = 0;
	for (int MoveNum = 0; MoveNum < list.count; ++MoveNum) {

        if (!MakeMove(pos, list.moves[MoveNum].move))
            continue;

        played += 1;
        TakeMove(pos);

	    if (played >= 1)
	        return 1;
    }

	return 0;
}

int MoveExists(MoveList *list, const int move) {

	for (int i = 0; i < list->count; ++i)
		if (list->moves[i].move == move)
			return 1;

	return 0;
}

int moveIsPseudoLegal(Board *pos, int move) {

    int from = FROMSQ(move);

    // Check against obvious illegal moves
    if (   (move == NONE_MOVE || move == NULL_MOVE)
        || (PieceCol[pos->pieces[from]] != pos->side))
        return 0;

    return 1;
}

int moveIsQuiet(int move) {
	// Check for captures, promotions, or enpassant.
	return !(move & (MFLAGCAP | MFLAGPROM | MFLAGEP));
}

int moveBestCaseValue(const Board *pos) {

	static const int SEEPieceValues[13] = {
    	0, 100, 450, 450, 675, 1300, 
    	0, 100, 450, 450, 675, 1300, 0
	};

    // Assume the opponent has at least a pawn
    int value = SEEPieceValues[wP];

    if (pos->side == WHITE) {
    	// Check for a higher value target
   		for (int piece = bQ; piece >= bP; piece--) { 
    		if (pos->pceNum[piece])
    			{ value = SEEPieceValues[piece]; break; }
    	}
    } else {
    	// Check for a higher value target
    	for (int piece = wQ; piece >= wP; piece--) { 
    		if (pos->pceNum[piece])
    			{ value = SEEPieceValues[piece]; break; }
    	}
    }

    // Check for a potential pawn promotion
    if (pos->pawns[pos->side] & (pos->side == WHITE ? RanksBB[RANK_7] : RanksBB[RANK_2]))
        value += SEEPieceValues[wQ] - SEEPieceValues[wP];

    return value;
}

int see(const Board *pos, int move, int threshold) {

    int from = FROMSQ(move), to = TOSQ(move);
    int nextVictim = pos->pieces[from];

    int balance = PieceValue[EG][pos->pieces[to]] - threshold;

    if (balance < 0)
        return 0;

    balance -= PieceValue[EG][nextVictim];

    if (balance >= 0)
        return 1;

    return (!pos->side) != pos->side;
}

int badCapture(int move, const Board *pos) {

    const int THEM = !pos->side;

    int from = FROMSQ(move);
    int to = TOSQ(move);
    int captured = CAPTURED(move);

    int Knight = pos->side == WHITE ? bN : wN;
    int Bishop = pos->side == WHITE ? bB : wB;

    // Captures by pawn do not lose material
    if (pos->pieces[from] == wP || pos->pieces[from] == bP) return 0;

    // Captures "lower takes higher" (as well as BxN) are good by definition
    if (PieceValue[EG][captured] >= PieceValue[EG][pos->pieces[from]] - 30) return 0;

    if (   pos->pawn_ctrl[THEM][to]
        && PieceValue[EG][captured] + 200 < PieceValue[EG][pos->pieces[from]])
        return 1;

    if (PieceValue[EG][captured] + 500 < PieceValue[EG][pos->pieces[from]]) {
    
        if (pos->pceNum[Knight])
            if (KnightAttack(THEM, to, pos)) return 1;

        if (pos->pceNum[Bishop]) {
            if (BishopAttack(THEM, to, 11, pos)) return 1;
            if (BishopAttack(THEM, to,  9, pos)) return 1;
            if (BishopAttack(THEM, to, -9, pos)) return 1;
            if (BishopAttack(THEM, to,-11, pos)) return 1;
        }
    }

    // If a capture is not processed, it cannot be considered bad
    return 0;
}

int move_canSimplify(int move, const Board *pos) {

    int captured = CAPTURED(move);

    if (  (captured == wP || captured == bP)
        || pos->material[!pos->side] - PieceValue[EG][captured] > ENDGAME_MAT)
        return 0;
    else
        return 1;
}

int advancedPawnPush(int move, const Board *pos) {
    return (   PiecePawn[pos->pieces[FROMSQ(move)]]
            && relativeRank(!pos->side, SQ64(TOSQ(move))) > RANK_5);
}