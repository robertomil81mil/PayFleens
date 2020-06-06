#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bitboards.h"
#include "defs.h"
#include "endgame.h"
#include "evaluate.h"

Material_Table Table;

U64 MaterialKeys[COLOUR_NB][ENDGAME_NB];

void endgameInit(S_BOARD *pos) {

	endgameAdd(pos, KBNK, "4k3/8/8/8/8/8/8/4KBN1 w - - 0 1");
	endgameAdd(pos, KQKR, "3rk3/8/8/8/8/8/8/4KQ2 w - - 0 1");
	endgameAdd(pos, KQKP, "4k3/5p2/8/8/8/8/8/4KQ2 w - - 0 1");
	endgameAdd(pos, KRKP, "4k3/5p2/8/8/8/8/8/4KR2 w - - 0 1");
	endgameAdd(pos, KRKB, "4kb2/8/8/8/8/8/8/4KR2 w - - 0 1");
	endgameAdd(pos, KRKN, "4kn2/8/8/8/8/8/8/4KR2 w - - 0 1");
	endgameAdd(pos, KNNK, "4k3/8/8/8/8/8/5N2/4KN2 w - - 0 1");
	endgameAdd(pos, KNNKP, "4k3/5p2/8/8/8/8/5N2/4KN2 w - - 0 1");
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
			entry->eval = Endgame_probe(pos, key);

		return entry;
    }

	memset(entry, 0, sizeof(Material_Entry));

	entry->key = key;

	entry->eval = Endgame_probe(pos, key);

	if ((entry->evalExists = entry->eval != VALUE_NONE) != 0)
    	return entry;

	entry->gamePhase = 24 - 4 * (pos->pceNum[wQ] + pos->pceNum[bQ])
               		      - 2 * (pos->pceNum[wR] + pos->pceNum[bR])
               		      - 1 * (pos->pceNum[wN] + pos->pceNum[bN] 
               		      	   + pos->pceNum[wB] + pos->pceNum[bB]);

    entry->gamePhase = (entry->gamePhase * 256 + 12) / 24;

    const int pieceCount[2][6] = {
        { pos->pceNum[wB] > 1, pos->pceNum[wP], pos->pceNum[wN],
          pos->pceNum[wB]    , pos->pceNum[wR], pos->pceNum[wQ] },
        { pos->pceNum[bB] > 1, pos->pceNum[bP], pos->pceNum[bN],
          pos->pceNum[bB]    , pos->pceNum[bR], pos->pceNum[bQ] }
    };

    entry->imbalance = imbalance(pieceCount, WHITE) - imbalance(pieceCount, BLACK);
    return entry;
}

int Endgame_probe(const S_BOARD *pos, U64 key) {

	if ((pos->bigPce[WHITE] + pos->bigPce[BLACK]) <= 5) {

		const int egScore = egScore(pos->mPhases[WHITE] - pos->mPhases[BLACK]);
		const int strongSide = egScore > 0 ? WHITE : BLACK;

		for (int eg = KBNK; eg <= KNNKP; eg++) {
			if (key == MaterialKeys[strongSide][eg]) {
				switch(eg) {
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
            + PushToCorners[opposite_colors(SQ64(bishopSq), SQ64(A1)) ? !SQ64(loserKSq) 
            														  :  SQ64(loserKSq)];

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

 	int wksq, bksq, rsq, psq, queeningSq, pfile, rook, pawn, result;

 	rook = strongSide == WHITE ? wR : bR;
    pawn = weakSide   == WHITE ? wP : bP;

	wksq = relativeSquare(strongSide, pos->KingSq[strongSide]);
	bksq = relativeSquare(strongSide, pos->KingSq[weakSide]);
	rsq  = relativeSquare(strongSide, pos->pList[rook][0]);
	psq  = relativeSquare(strongSide, pos->pList[pawn][0]);

	pfile = file_of(SQ64(psq));

	queeningSq = FR2SQ(file_of(SQ64(psq)), RANK_1);

	// If the stronger side's king is in front of the pawn, it's a win
	if (   file_of(SQ64(psq)) == file_of(SQ64(wksq))
		&& rank_of(SQ64(psq)) >  rank_of(SQ64(wksq)))
		result = PieceValue[EG][wR] - distanceBetween(wksq, psq);

	// If the weaker side's king is too far from the pawn,
	// and the rook can stop the pawn, the it's a win.
	else if (   distanceBetween(bksq, psq) >= 3 + (pos->side == weakSide)
       		 && distanceBetween(bksq, rsq) >= 2
       		 && !((FileBBMask[pfile-1] | FileBBMask[pfile+1]) & SQ64(rsq)))
    	result = PieceValue[EG][wR] - distanceBetween(wksq, psq);

	// If the pawn is far advanced and supported by the defending king,
	// the position is drawish
	else if (   rank_of(SQ64(bksq)) <= RANK_3
    		 && distanceBetween(bksq, psq) == 1
             && rank_of(SQ64(wksq)) >= RANK_4
             && distanceBetween(wksq, psq) > 2 + (pos->side == strongSide))
    	result = 44 - 4 * distanceBetween(wksq, psq);

	else
		result = 110 - 4 * (  distanceBetween(wksq, psq - 10)
                            - distanceBetween(bksq, psq - 10)
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

	return strongSide == pos->side ? 0 : 0;
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

bool is_KXK(const S_BOARD *pos, int strongSide) {
	return (   pos->material[ strongSide] >= PieceValue[EG][wR]
		    && pos->material[!strongSide] == PieceValue[EG][wK]);
}