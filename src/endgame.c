#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bitbase.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "defs.h"
#include "endgame.h"
#include "evaluate.h"
#include "init.h"

Material_Table Table;

U64 MaterialKeys[COLOUR_NB][ENDGAME_NB];

void endgameInit(S_BOARD *pos) {

	Bitbases_init();
	endgameAdd(pos, KPK, "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
	endgameAdd(pos, KBNK, "4k3/8/8/8/8/8/8/4KBN1 w - - 0 1");
	endgameAdd(pos, KQKR, "3rk3/8/8/8/8/8/8/4KQ2 w - - 0 1");
	endgameAdd(pos, KQKP, "4k3/5p2/8/8/8/8/8/4KQ2 w - - 0 1");
	endgameAdd(pos, KRKP, "4k3/5p2/8/8/8/8/8/4KR2 w - - 0 1");
	endgameAdd(pos, KRKB, "4kb2/8/8/8/8/8/8/4KR2 w - - 0 1");
	endgameAdd(pos, KRKN, "4kn2/8/8/8/8/8/8/4KR2 w - - 0 1");
	endgameAdd(pos, KNNK, "4k3/8/8/8/8/8/5N2/4KN2 w - - 0 1");
	endgameAdd(pos, KNNKP, "4k3/5p2/8/8/8/8/5N2/4KN2 w - - 0 1");
	endgameAdd(pos, KPKP, "4k3/4p3/8/8/8/8/4P3/4K3 w - - 0 1");
	endgameAdd(pos, KBPKB, "4kb2/8/8/8/8/8/4P3/4KB2 w - - 0 1");
	endgameAdd(pos, KBPPKB, "4kb2/8/8/8/8/8/4PP2/4KB2 w - - 0 1");
	endgameAdd(pos, KRPKB, "4kb2/8/8/8/8/8/7P/4K2R w K - 0 1");
	endgameAdd(pos, KRPKR, "4K3/4P1k1/8/8/8/8/4R3/3r4 w - - 0 1");
	endgameAdd(pos, KRPPKRP, "8/6k1/6p1/6P1/5PK1/8/4R3/3r4 w - - 0 1");
}

void endgameAdd(S_BOARD *pos, int eg, char *fen) {

	ParseFen(fen, pos);
	MaterialKeys[WHITE][eg] = pos->materialKey;
	MirrorBoard(pos);
	MaterialKeys[BLACK][eg] = pos->materialKey;
}

Material_Entry* Material_probe(const S_BOARD *pos, Material_Table *materialTable) {

	U64 key = pos->materialKey;
	Material_Entry *entry = &materialTable->entry[key >> MT_HASH_SHIFT];

    // Search for a matching key signature,
    // if eval exist compute it and return.
    if (entry->key == key) {

		if (entry->evalExists)
			entry->eval = EndgameValue_probe(pos, key);

		entry->factor = EndgameFactor_probe(pos, key);

		return entry;
    }

	memset(entry, 0, sizeof(Material_Entry));

	entry->key = key;

	entry->eval = EndgameValue_probe(pos, key);

	if ((entry->evalExists = entry->eval != VALUE_NONE) != 0)
    	return entry;

	entry->gamePhase = 24 - 4 * (pos->pceNum[wQ] + pos->pceNum[bQ])
               		      - 2 * (pos->pceNum[wR] + pos->pceNum[bR])
               		      - 1 * (pos->pceNum[wN] + pos->pceNum[bN] 
               		      	   + pos->pceNum[wB] + pos->pceNum[bB]);

    entry->gamePhase = (entry->gamePhase * 256 + 12) / 24;

    entry->factor = EndgameFactor_probe(pos, key);

    if (entry->factor != SCALE_NORMAL)
    	return entry;

    const int pieceCount[2][6] = {
        { pos->pceNum[wB] > 1, pos->pceNum[wP], pos->pceNum[wN],
          pos->pceNum[wB]    , pos->pceNum[wR], pos->pceNum[wQ] },
        { pos->pceNum[bB] > 1, pos->pceNum[bP], pos->pceNum[bN],
          pos->pceNum[bB]    , pos->pceNum[bR], pos->pceNum[bQ] }
    };

    entry->imbalance = imbalance(pieceCount, WHITE) - imbalance(pieceCount, BLACK);
    return entry;
}

int EndgameValue_probe(const S_BOARD *pos, U64 key) {

	if ((pos->bigPce[WHITE] + pos->bigPce[BLACK]) <= 5) {

		const int egScore = egScore(pos->mPhases[WHITE] - pos->mPhases[BLACK]);
		const int strongSide = egScore > 0 ? WHITE : BLACK;

		for (int eg = KPK; eg <= KNNKP; eg++) {
			if (key == MaterialKeys[strongSide][eg]) {
				switch(eg) {
					case KPK:   return EndgameKPK(pos, strongSide);
					case KBNK:  return EndgameKBNK(pos, strongSide);
					case KQKR:  return EndgameKQKR(pos, strongSide);
					case KQKP:  return EndgameKQKP(pos, strongSide);
					case KRKP:  return EndgameKRKP(pos, strongSide);
					case KRKB:  return EndgameKRKB(pos, strongSide);
					case KRKN:  return EndgameKRKN(pos, strongSide);
					case KNNK:  return EndgameKNNK(pos, strongSide);
					case KNNKP: return EndgameKNNKP(pos, strongSide);
					default : exit(EXIT_FAILURE);
				}
			}
		}
	}

	for (int c = WHITE; c < COLOUR_NB; c++)
		if (is_KXK(pos, c)) {
			return EndgameKXK(pos, c);
		}

	return VALUE_NONE;
}

int EndgameFactor_probe(const S_BOARD *pos, U64 key) {

	if ((pos->bigPce[WHITE] + pos->bigPce[BLACK]) <= 7) {

		const int egScore = egScore(pos->mPhases[WHITE] - pos->mPhases[BLACK]);
		const int strongSide = egScore > 0 ? WHITE : BLACK;

		for (int eg = KPKP; eg <= KRPPKRP; eg++) {
			if (key == MaterialKeys[strongSide][eg]) {
				switch(eg) {
					case KPKP:    return EndgameKPKP(pos, strongSide);
					case KBPKB:   return EndgameKBPKB(pos, strongSide);
					case KBPPKB:  return EndgameKBPPKB(pos, strongSide);
					case KRPKB:   return EndgameKRPKB(pos, strongSide);
					case KRPKR:   return EndgameKRPKR(pos, strongSide);
					case KRPPKRP: return EndgameKRPPKRP(pos, strongSide);
					default     : exit(EXIT_FAILURE);
				}
			}
		}
	}

	for (int c = WHITE; c < COLOUR_NB; ++c) {
		if (is_KBPsK(pos, c)) {
		    return EndgameKBPsK(pos, c);
		}
		if (is_KPsK(pos, c)) {
		    return EndgameKPsK(pos, c);
		}
	}

	return SCALE_NORMAL;
}

int EndgameKXK(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

    ASSERT(pos->material[weakSide] == PieceValue[EG][wK]);

    int winnerKSq, loserKSq, result, queen, rook, bishop, knight;

    winnerKSq = pos->KingSq[strongSide];
    loserKSq  = pos->KingSq[weakSide];

    result =  pos->material[strongSide]
            + PushToEdges[SQ64(loserKSq)]
            + PushClose[distanceBetween(winnerKSq, loserKSq)];

    queen  = strongSide == WHITE ? wQ : bQ;
    rook   = strongSide == WHITE ? wR : bR;
    bishop = strongSide == WHITE ? wB : bB;
    knight = strongSide == WHITE ? wN : bN;

    if (   pos->pceNum[queen ]
    	|| pos->pceNum[rook  ]
    	||(pos->pceNum[bishop] && pos->pceNum[knight])
    	|| (   opposite_colors(SQ64(pos->pList[bishop][0]), SQ64(A1))
        	&& opposite_colors(SQ64(pos->pList[bishop][1]), SQ64(A8))) )
        result = MIN(result + KNOWN_WIN, MATE_IN_MAX - 1);

    return strongSide == pos->side ? result : -result;
};

int EndgameKPK(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wK] + PieceValue[EG][wP]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wK]);

 	int pawn = strongSide == WHITE ? wP : bP;

	// Assume strongSide is white and the pawn is on files A-D
	int wksq = fixSquare(pos, strongSide, SQ64(pos->KingSq[strongSide]));
	int bksq = fixSquare(pos, strongSide, SQ64(pos->KingSq[weakSide]));
	int psq  = fixSquare(pos, strongSide, SQ64(pos->pList[pawn][0]));

	int us = strongSide == pos->side ? WHITE : BLACK;

	if (!Bitbases_probe(wksq, psq, bksq, us))
    	return VALUE_DRAW;

	int result = KNOWN_WIN + PieceValue[EG][wP] + rank_of(SQ64(pos->pList[pawn][0]));

	return strongSide == pos->side ? result : -result;
}

int EndgameKBNK(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wB] + PieceValue[EG][wN]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wK]);

 	int winnerKSq, loserKSq, bishopSq, bishop, result;

 	bishop = strongSide == WHITE ? wB : bB;

  	winnerKSq = pos->KingSq[strongSide];
  	loserKSq  = pos->KingSq[weakSide];
  	bishopSq  = pos->pList[bishop][0];

  	// If our Bishop does not attack A1/H8, we flip the enemy king square
  	// to drive to opposite corners (A8/H1).

  	result =  KNOWN_WIN
            + PushClose[distanceBetween(winnerKSq, loserKSq)]
            + PushToCorners[opposite_colors(SQ64(bishopSq), SQ64(A1)) ? SQ64(loserKSq) ^ 56
            														  : SQ64(loserKSq)];

	ASSERT(abs(result) < MATE_IN_MAX);
	return strongSide == pos->side ? result : -result;
}

int EndgameKQKR(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wQ]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wR]);

 	int winnerKSq, loserKSq, result;

	winnerKSq = pos->KingSq[strongSide];
	loserKSq  = pos->KingSq[weakSide];

	result =  PieceValue[EG][wQ]
            - PieceValue[EG][wR]
            + PushToEdges[SQ64(loserKSq)]
            + PushClose[distanceBetween(winnerKSq, loserKSq)];

	return strongSide == pos->side ? result : -result;
}

int EndgameKQKP(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wQ]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wP]);

 	int winnerKSq, loserKSq, pawnSq, pawn, result;

 	pawn = weakSide == WHITE ? wP : bP;

 	winnerKSq = pos->KingSq[strongSide];
	loserKSq  = pos->KingSq[weakSide];
	pawnSq    = pos->pList[pawn][0];

	result = PushClose[distanceBetween(winnerKSq, loserKSq)];

	if (   relativeRank(weakSide, SQ64(pawnSq)) != RANK_7
		|| distanceBetween(loserKSq, pawnSq) != 1
		|| !((FileABB | FileCBB | FileFBB | FileHBB) & SQ64(pawnSq)))
    	result += PieceValue[EG][wQ] - PieceValue[EG][wP];

	return strongSide == pos->side ? result : -result;
}

int EndgameKRKP(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wR]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wP]);

 	int wksq, bksq, rsq, psq, queeningSq, rook, pawn, result;

 	rook = strongSide == WHITE ? wR : bR;
    pawn = weakSide   == WHITE ? wP : bP;

	wksq = relativeSquare(strongSide, pos->KingSq[strongSide]);
	bksq = relativeSquare(strongSide, pos->KingSq[weakSide]);
	rsq  = relativeSquare(strongSide, pos->pList[rook][0]);
	psq  = relativeSquare(strongSide, pos->pList[pawn][0]);

	queeningSq = FR2SQ(file_of(SQ64(psq)), RANK_1);

	// If the stronger side's king is in front of the pawn, it's a win
	if (   file_of(SQ64(psq)) == file_of(SQ64(wksq))
		&& rank_of(SQ64(psq)) >  rank_of(SQ64(wksq)))
		result = PieceValue[EG][wR] - distanceBetween(wksq, psq);

	// If the weaker side's king is too far from the pawn,
	// and the rook can stop the pawn, the it's a win.
	else if (   distanceBetween(bksq, psq) >= 3 + (pos->side == weakSide)
       		 && distanceBetween(bksq, rsq) >= 2
       		 && !(adjacentFiles(SQ64(psq)) & SQ64(rsq)))
    	result = PieceValue[EG][wR] - distanceBetween(wksq, psq);

	// If the pawn is far advanced and supported by the defending king,
	// the position is drawish
	else if (   rank_of(SQ64(bksq)) <= RANK_3
    		 && distanceBetween(bksq, psq) == 1
             && rank_of(SQ64(wksq)) >= RANK_4
             && distanceBetween(wksq, psq) > 2 + (pos->side == strongSide))
    	result = 44 - 4 * distanceBetween(wksq, psq);

	else
		result = 110 - 4 * (  distanceBetween(wksq, psq + SOUTH)
                            - distanceBetween(bksq, psq + SOUTH)
                            - distanceBetween(psq, queeningSq));

	return strongSide == pos->side ? result : -result;
}

int EndgameKRKB(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wR]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wB]);

	int result = PushToEdges[SQ64(pos->KingSq[weakSide])];
	return strongSide == pos->side ? result : -result;
}

int EndgameKRKN(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wR]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wN]);

 	int weakKSq, weakNSq, knight, result;

 	knight = weakSide == WHITE ? wN : bN;

	weakKSq = pos->KingSq[weakSide];
	weakNSq = pos->pList[knight][0];

	result  =  PushToEdges[SQ64(weakKSq)]
			 + PushAway[distanceBetween(weakKSq, weakNSq)];

	return strongSide == pos->side ? result : -result;
}

int EndgameKNNK(const S_BOARD *pos, int strongSide) {

	ASSERT(pos->material[strongSide] == 2 * PieceValue[EG][wN]);

	return strongSide == pos->side ? VALUE_DRAW : VALUE_DRAW;
}

int EndgameKNNKP(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == 2 * PieceValue[EG][wN]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wP]);

 	int pawn, psq, result;

 	pawn = weakSide == WHITE ? wP : bP;
	psq  = pos->pList[pawn][0];

    result =      PieceValue[EG][wP]
            + 2 * PushToEdges[SQ64(pos->KingSq[weakSide])]
            - 5 * relativeRank(weakSide, SQ64(psq));

	return strongSide == pos->side ? result : -result;
}

int EndgameKPsK(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[weakSide] == PieceValue[EG][wK]);
	ASSERT((  pos->material[strongSide] - PieceValue[EG][wP]
		    * popcount(pos->pawns[strongSide]) == PieceValue[EG][wK])
		   && popcount(pos->pawns[strongSide]) >= 2);

	int ksq = pos->KingSq[weakSide];
	U64 pawns = pos->pawns[strongSide];

	// If all pawns are ahead of the king, on a single rook file and
	// the king is within one file of the pawns, it's a draw.
	if (   !(pawns & ~forwardRanks(weakSide, SQ64(ksq)))
		&& !((pawns & ~FileABB) && (pawns & ~FileHBB))
		&&  distanceByFile(ksq, SQ120(getlsb(pawns))) <= 1)
		return SCALE_FACTOR_DRAW;

	return SCALE_NORMAL;
}

int EndgameKPKP(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wK] + PieceValue[EG][wP]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wK] + PieceValue[EG][wP]);

 	int pawn = strongSide == WHITE ? wP : bP;

	// Assume strongSide is white and the pawn is on files A-D
	int wksq = fixSquare(pos, strongSide, SQ64(pos->KingSq[strongSide]));
	int bksq = fixSquare(pos, strongSide, SQ64(pos->KingSq[weakSide]));
	int psq  = fixSquare(pos, strongSide, SQ64(pos->pList[pawn][0]));

	int us = strongSide == pos->side ? WHITE : BLACK;

	// If the pawn has advanced to the fifth rank or further, and is not a
	// rook pawn, it's too dangerous to assume that it's at least a draw.
	if (rank_of(psq) >= RANK_5 && file_of(psq) != FILE_A)
	    return SCALE_NORMAL;

	// Probe the KPK bitbase with the weakest side's pawn removed. If it's a draw,
	// it's probably at least a draw even with the pawn.
	return Bitbases_probe(wksq, psq, bksq, us) ? SCALE_NORMAL : SCALE_FACTOR_DRAW;
}

int EndgameKBPsK(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT((  pos->material[strongSide] - PieceValue[EG][wP]
		    * popcount(pos->pawns[strongSide]) == PieceValue[EG][wB])
		   && popcount(pos->pawns[strongSide]) >= 1);

	U64 strongpawns = pos->pawns[strongSide];
	U64 weakpawns   = pos->pawns[weakSide];
	U64 allpawns    = pos->pawns[COLOUR_NB];

	// All pawns are on a single rook file?
	if (!(strongpawns & ~FileABB) || !(strongpawns & ~FileHBB)) {

		int bishop = strongSide == WHITE ? wB : bB;
    	int bishopSq = pos->pList[bishop][0];
    	int queeningSq = relativeSquare(strongSide, FR2SQ(file_of(getlsb(strongpawns)), RANK_8));
    	int weakKingSq = pos->KingSq[weakSide];

    	if (   opposite_colors(SQ64(queeningSq), SQ64(bishopSq))
        	&& distanceBetween(queeningSq, weakKingSq) <= 1)
        	return SCALE_FACTOR_DRAW;
	}

	// If all the pawns are on the same B or G file, then it's potentially a draw
	if ((!(allpawns & ~FileBBB) || !(allpawns & ~FileGBB))
    	&& pos->material[weakSide] - PieceValue[EG][wP] * popcount(weakpawns) == 0
    	&& popcount(weakpawns) >= 1) {

    	// Get weakSide pawn that is closest to the home rank
    	int weakPawnSq = frontmost(strongSide, weakpawns);

    	int strongKingSq = pos->KingSq[strongSide];
    	int weakKingSq = pos->KingSq[weakSide];
    	int bishop = strongSide == WHITE ? wB : bB;
    	int bishopSq = pos->pList[bishop][0];

    	int push = weakSide == WHITE ? 8 : -8;

    	// There's potential for a draw if our pawn is blocked on the 7th rank,
    	// the bishop cannot attack it or they only have one pawn left
    	if (   relativeRank(strongSide, weakPawnSq) == RANK_7
        	&& (strongpawns & (weakPawnSq + push))
        	&& (opposite_colors(SQ64(bishopSq), SQ64(weakPawnSq)) || onlyOne(strongpawns))) {

        	int strongKingDist = distanceBetween(SQ120(weakPawnSq), strongKingSq);
        	int weakKingDist = distanceBetween(SQ120(weakPawnSq), weakKingSq);

        	// It's a draw if the weak king is on its back two ranks, within 2
        	// squares of the blocking pawn and the strong king is not
        	// closer. (I think this rule only fails in practically
        	// unreachable positions such as 5k1K/6p1/6P1/8/8/3B4/8/8 w
        	// and positions where qsearch will immediately correct the
        	// problem such as 8/4k1p1/6P1/1K6/3B4/8/8/8 w)
        	if (   relativeRank(strongSide, SQ64(weakKingSq)) >= RANK_7
            	&& weakKingDist <= 2
            	&& weakKingDist <= strongKingDist)
            	return SCALE_FACTOR_DRAW;
    	}
	}

	return SCALE_NORMAL;
}

int EndgameKBPKB(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wB] + PieceValue[EG][wP]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wB]);

 	int pawn         = strongSide == WHITE ? wP : bP;
 	int strongBishop = strongSide == WHITE ? wB : bB;
 	int weakBishop   = weakSide   == WHITE ? wB : bB;

	int pawnSq = SQ64(pos->pList[pawn][0]);
 	int strongBishopSq = SQ64(pos->pList[strongBishop][0]);
 	int weakBishopSq = SQ64(pos->pList[weakBishop][0]);
 	int weakKingSq = SQ64(pos->KingSq[weakSide]);

	// Case 1: Defending king blocks the pawn, and cannot be driven away
	if (   file_of(weakKingSq) == file_of(pawnSq)
    	&& relativeRank(strongSide, pawnSq) < relativeRank(strongSide, weakKingSq)
    	&& (   opposite_colors(weakKingSq, strongBishopSq)
        	|| relativeRank(strongSide, weakKingSq) <= RANK_6))
    	return SCALE_FACTOR_DRAW;

	// Case 2: Opposite colored bishops
	if (opposite_colors(strongBishopSq, weakBishopSq))
    	return SCALE_FACTOR_DRAW;

	return SCALE_NORMAL;
}

int EndgameKBPPKB(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wB] + 2 * PieceValue[EG][wP]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wB]);

 	int strongBishop = strongSide == WHITE ? wB : bB;
 	int weakBishop   = weakSide   == WHITE ? wB : bB;

	int strongBishopSq = SQ64(pos->pList[strongBishop][0]);
 	int weakBishopSq = SQ64(pos->pList[weakBishop][0]);

	if (!opposite_colors(strongBishopSq, weakBishopSq))
    	return SCALE_NORMAL;

    int pawn = strongSide == WHITE ? wP : bP;

	int weakKingSq = SQ64(pos->KingSq[weakSide]);
 	int psq1 = SQ64(pos->pList[pawn][0]);
 	int psq2 = SQ64(pos->pList[pawn][1]);

 	int push = strongSide == WHITE ? 8 : -8;
 	int blockSq1, blockSq2;

	if (relativeRank(strongSide, psq1) > relativeRank(strongSide, psq2)) {
    	blockSq1 = psq1 + push;
    	blockSq2 = makeSq(file_of(psq2), rank_of(psq1));
	}
	else {
    	blockSq1 = psq2 + push;
    	blockSq2 = makeSq(file_of(psq1), rank_of(psq2));
	}

	switch (distanceByFile(SQ120(psq1), SQ120(psq2))) {

		case 0:
	    // Both pawns are on the same file. It's an easy draw if the defender firmly
	    // controls some square in the frontmost pawn's path.
	    	if (   file_of(weakKingSq) == file_of(blockSq1)
	        	&& relativeRank(strongSide, weakKingSq) >= relativeRank(strongSide, blockSq1)
	        	&& opposite_colors(weakKingSq, strongBishopSq))
	        	return SCALE_FACTOR_DRAW;
	    	else
	        	return SCALE_NORMAL;

	 	case 1:
		    // Pawns on adjacent files. It's a draw if the defender firmly controls the
		    // square in front of the frontmost pawn's path.
		    if (   weakKingSq == blockSq1
		        && opposite_colors(weakKingSq, strongBishopSq)
		        && (   weakBishopSq == blockSq2
		            || distanceByRank(SQ120(psq1), SQ120(psq2)) >= 2))
		        return SCALE_FACTOR_DRAW;

		    else if (   weakKingSq == blockSq2
		             && opposite_colors(weakKingSq, strongBishopSq)
		             && weakBishopSq == blockSq1)
		        return SCALE_FACTOR_DRAW;
		    else
	        	return SCALE_NORMAL;

 		default:
    		// The pawns are not on the same file or adjacent files. No scaling.
    		return SCALE_NORMAL;
	}
}

int EndgameKRPKB(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wR] + PieceValue[EG][wP]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wB]);

	// Test for a rook pawn
	if (pos->pawns[strongSide] & (FileABB | FileHBB)) {

		int bishop = weakSide   == WHITE ? wB : bB;
 		int pawn   = strongSide == WHITE ? wP : bP;

    	int ksq = pos->KingSq[weakSide];
    	int bsq = pos->pList[bishop][0];
    	int psq = pos->pList[pawn][0];
    	int r = relativeRank(strongSide, SQ64(psq));
    	int push = strongSide == WHITE ? 10 : -10;

    	// If the pawn is on the 5th rank and the pawn (currently) is on
    	// the same color square as the bishop then there is a chance of
    	// a fortress. Depending on the king position give a moderate
    	// reduction or a stronger one if the defending king is near the
    	// corner but not trapped there.
    	if (r == RANK_5 && !opposite_colors(SQ64(bsq), SQ64(psq))) {

        	int d = distanceBetween(psq + 3 * push, ksq);

        	if (d <= 2 && !(d == 0 && ksq == pos->KingSq[strongSide] + 2 * push))
            	return 48;
        	else
            	return 96;
    	}

    	// When the pawn has moved to the 6th rank we can be fairly sure
    	// it's drawn if the bishop attacks the square in front of the
    	// pawn from a reasonable distance and the defending king is near
    	// the corner
    	if (   r == RANK_6
        	&& distanceBetween(psq + 2 * push, ksq) <= 1
        	&& distanceByFile(bsq, psq) >= 2) {

    		for (int index = 0; index < NumDir[bishop]; ++index) {
        		int t_sq = bsq + PceDir[bishop][index];

        		while (!SQOFFBOARD(t_sq)) {

            		if (pos->pieces[t_sq] == EMPTY) {

            			if (t_sq == (psq + push))
            				return 16;
            		}
            		else
            			break;
            	}

            	t_sq += PceDir[bishop][index];
        	}
    	}
	}

	return SCALE_NORMAL;
}

int EndgameKRPKR(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wR] + PieceValue[EG][wP]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wR]);

    int rookW = strongSide == WHITE ? wR : bR;
 	int rookB = weakSide   == WHITE ? wR : bR;
 	int pawn  = strongSide == WHITE ? wP : bP;

 	// Assume strongSide is white and the pawn is on files A-D
	int wksq = fixSquare(pos, strongSide, SQ64(pos->KingSq[strongSide]));
	int bksq = fixSquare(pos, strongSide, SQ64(pos->KingSq[weakSide]));
	int wrsq = fixSquare(pos, strongSide, SQ64(pos->pList[rookW][0]));
	int wpsq = fixSquare(pos, strongSide, SQ64(pos->pList[pawn][0]));
	int brsq = fixSquare(pos, strongSide, SQ64(pos->pList[rookB][0]));

	// Convert the squares to 120
	int wksq120 = SQ120(wksq);
	int bksq120 = SQ120(bksq);
	int wrsq120 = SQ120(wrsq);
	int wpsq120 = SQ120(wpsq);
	int brsq120 = SQ120(brsq);

	int f = file_of(wpsq);
	int r = rank_of(wpsq);
	int queeningSq = FR2SQ(f, RANK_8);
	int blockSq = wpsq120 + 10;
	int tempo = pos->side == strongSide;

	// If the pawn is not too far advanced and the defending king defends the
	// queening square, it's a draw.
	if (   r <= RANK_5
    	&& distanceBetween(bksq120, queeningSq) <= 1
    	&& rank_of(wksq) <= RANK_5
    	&& (rank_of(brsq) == RANK_6 || (r <= RANK_3 && rank_of(wrsq) != RANK_6)))
    	return SCALE_FACTOR_DRAW;

    // If the defending king blocks the pawn and the attacking king is too far
	// away, it's a draw.
	if (   r <= RANK_5
    	&& bksq == wpsq + NORTH
    	&& distanceBetween(wksq120, wpsq120) - tempo >= 2
    	&& distanceBetween(wksq120, brsq120) - tempo >= 2)
    	return SCALE_FACTOR_DRAW;

	// The defending side saves a draw by checking from behind in case the pawn
	// has advanced to the 6th rank with the king behind.
	if (   r == RANK_6
    	&& distanceBetween(bksq120, queeningSq) <= 1
    	&& rank_of(wksq) + tempo <= RANK_6
    	&& (rank_of(brsq) == RANK_1 || (!tempo && distanceByFile(brsq120, wpsq120) >= 3)))
    	return SCALE_FACTOR_DRAW;

	if (   r >= RANK_6
    	&& bksq120 == queeningSq
    	&& rank_of(brsq) == RANK_1
    	&& (!tempo || distanceBetween(wksq120, wpsq120) >= 2))
    	return SCALE_FACTOR_DRAW;

	// White pawn on a7 and rook on a8 is a draw if black's king is on g7 or h7
	// and the black rook is behind the pawn.
	if (   wpsq120 == A7
		&& wrsq120 == A8
    	&& (bksq120 == H7 || bksq120 == G7)
    	&& file_of(brsq) == FILE_A
    	&& (rank_of(brsq) <= RANK_3 || file_of(wksq) >= FILE_D || rank_of(wksq) <= RANK_5))
    	return SCALE_FACTOR_DRAW;

	// Pawn on the 7th rank supported by the rook from behind usually wins if the
	// attacking king is closer to the queening square than the defending king,
	// and the defending king cannot gain tempo by threatening the attacking rook.
	if (   r == RANK_7
    	&& f != FILE_A
    	&& file_of(wrsq) == f
    	&& wrsq120 != queeningSq
    	&& (distanceBetween(wksq120, queeningSq) < distanceBetween(bksq120, queeningSq) - 2 + tempo)
    	&& (distanceBetween(wksq120, queeningSq) < distanceBetween(bksq120, wrsq120) + tempo))
    	return SCALE_MAX - 4 * distanceBetween(wksq120, queeningSq);

	// Similar to the above, but with the pawn further back
	if (   f != FILE_A
    	&& file_of(wrsq) == f
    	&& wrsq < wpsq
    	&& (distanceBetween(wksq120, queeningSq) < distanceBetween(bksq120, queeningSq) - 2 + tempo)
    	&& (distanceBetween(wksq120, blockSq) < distanceBetween(bksq120, blockSq) - 2 + tempo)
    	&& (  distanceBetween(bksq120, wrsq120) + tempo >= 3
        	|| (    distanceBetween(wksq120, queeningSq) < distanceBetween(bksq120, wrsq120) + tempo
            	&& (distanceBetween(wksq120, blockSq) < distanceBetween(bksq120, wrsq120) + tempo))))
    	return SCALE_MAX
               - 16 * distanceBetween(wpsq120, queeningSq)
               -  4 * distanceBetween(wksq120, queeningSq);

	// If the pawn is not far advanced and the defending king is somewhere in
	// the pawn's path, it's probably a draw.
	if (r <= RANK_4 && bksq > wpsq) {

    	if (file_of(bksq) == file_of(wpsq))
        	return 20;

    	if (   distanceByFile(bksq120, wpsq120) == 1
        	&& distanceBetween(wksq120, bksq120) > 2)
        	return 48 - 4 * distanceBetween(wksq120, bksq120);
	}

	return SCALE_NORMAL;
}

int EndgameKRPPKRP(const S_BOARD *pos, int strongSide) {

	const int weakSide = !strongSide;

	ASSERT(pos->material[strongSide] == PieceValue[EG][wR] + 2 * PieceValue[EG][wP]);
 	ASSERT(pos->material[weakSide] == PieceValue[EG][wR] + 1 * PieceValue[EG][wP]);

	int pawn = strongSide == WHITE ? wP : bP;

	int wpsq1 = SQ64(pos->pList[pawn][0]);
	int wpsq2 = SQ64(pos->pList[pawn][1]);
	int bksq = pos->KingSq[weakSide];

	// Does the stronger side have a passed pawn?
	if (   !(PassedPawnMasks[strongSide][wpsq1] & pos->pawns[weakSide])
		|| !(PassedPawnMasks[strongSide][wpsq2] & pos->pawns[weakSide]))
    	return SCALE_NORMAL;

	int r = MAX(relativeRank(strongSide, wpsq1), relativeRank(strongSide, wpsq2));

	if (   distanceByFile(bksq, SQ120(wpsq1)) <= 1
    	&& distanceByFile(bksq, SQ120(wpsq2)) <= 1
    	&& relativeRank(strongSide, SQ64(bksq)) > r) {

    	ASSERT(r > RANK_1 && r < RANK_7);
    	return KRPPKRPScaleFactors[r];
	}

	return SCALE_NORMAL;
}

int fixSquare(const S_BOARD *pos, int strongSide, int sq) {

    ASSERT(onlyOne(pos->pawns[strongSide]));
    ASSERT(0 <= sq && sq < 64);

    int pawn = strongSide == WHITE ? wP : bP;
	
	if (file_of(SQ64(pos->pList[pawn][0])) >= FILE_E)
		sq = sq ^ FILE_H;

    return strongSide == WHITE ? sq : sq ^ 56;
}

bool is_KXK(const S_BOARD *pos, int strongSide) {
	return (   pos->material[ strongSide] >= PieceValue[EG][wR]
		    && pos->material[!strongSide] == PieceValue[EG][wK]);
}

bool is_KPsK(const S_BOARD *pos, int strongSide) {
	return ((  pos->material[strongSide] - PieceValue[EG][wP]
		     * popcount(pos->pawns[strongSide]) == PieceValue[EG][wK])
		    && popcount(pos->pawns[strongSide]) >= 2
		    && pos->material[!strongSide] == PieceValue[EG][wK]);
}

bool is_KBPsK(const S_BOARD *pos, int strongSide) {
    return ((  pos->material[strongSide] - PieceValue[EG][wP]
		     * popcount(pos->pawns[strongSide]) == PieceValue[EG][wB])
    		&& popcount(pos->pawns[strongSide]) >= 1);
}