// movegen.c

#include "stdio.h"
#include "defs.h"

#define MOVE(f,t,ca,pro,fl) ( (f) | ((t) << 7) | ( (ca) << 14 ) | ( (pro) << 20 ) | (fl))
#define SQOFFBOARD(sq) (FilesBrd[(sq)]==OFFBOARD)

EVAL_DATA e[1];

const int LoopSlidePce[8] = {
 wB, wR, wQ, 0, bB, bR, bQ, 0
};

const int LoopNonSlidePce[6] = {
 wN, wK, 0, bN, bK, 0
};

const int LoopSlideIndex[2] = { 0, 4 };
const int LoopNonSlideIndex[2] = { 0, 3 };

const int PceDir[13][8] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 }
};

const int NumDir[13] = {
 0, 0, 8, 4, 4, 8, 8, 0, 8, 4, 4, 8, 8
};

/*
PV Move
Cap -> MvvLVA
Killers
HistoryScore

*/
const int VictimScore[13] = { 0, 100, 450, 470, 670, 1300, 10000, 100, 450, 470, 670, 1300, 10000 };
static int MvvLvaScores[13][13];

void InitMvvLva() {
	int Attacker;
	int Victim;
	for(Attacker = wP; Attacker <= bK; ++Attacker) {
		for(Victim = wP; Victim <= bK; ++Victim) {
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] + 6 - ( VictimScore[Attacker] / 100);
		}
	}
}

int MoveExists(S_BOARD *pos, const int move) {

	S_MOVELIST list[1];
    GenerateAllMoves(pos,list);

    int MoveNum = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        if ( !MakeMove(pos,list->moves[MoveNum].move))  {
            continue;
        }
        TakeMove(pos);
		if(list->moves[MoveNum].move == move) {
			return TRUE;
		}
    }
	return FALSE;
}

static void AddQuietMove( const S_BOARD *pos, int move, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(CheckBoard(pos));
	ASSERT(pos->ply >=0 && pos->ply < MAXDEPTH);

	list->moves[list->count].move = move;
	//list->moves[list->count].mobility = list->count;

	if(pos->searchKillers[0][pos->ply] == move) {
		list->moves[list->count].score = 900000;
	} else if(pos->searchKillers[1][pos->ply] == move) {
		list->moves[list->count].score = 800000;
	} else {
		list->moves[list->count].score = pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];
	}
	list->count++;
	list->quiets++;
}

static void AddCaptureMove( const S_BOARD *pos, int move, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(PieceValid(CAPTURED(move)));
	ASSERT(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[FROMSQ(move)]] + 1000000;
	list->count++;
}

static void AddEnPassantMove( const S_BOARD *pos, int move, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(CheckBoard(pos));
	ASSERT((RanksBrd[TOSQ(move)]==RANK_6 && pos->side == WHITE) || (RanksBrd[TOSQ(move)]==RANK_3 && pos->side == BLACK));

	list->moves[list->count].move = move;
	list->moves[list->count].score = 105 + 1000000;
	list->count++;
}

static void AddWhitePawnCapMove( const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list ) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_7) {
		AddCaptureMove(pos, MOVE(from,to,cap,wQ,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,wR,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,wB,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,wN,0), list);
	} else {
		AddCaptureMove(pos, MOVE(from,to,cap,EMPTY,0), list);
	}
}

static void AddWhitePawnMove( const S_BOARD *pos, const int from, const int to, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_7) {
		AddQuietMove(pos, MOVE(from,to,EMPTY,wQ,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,wR,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,wB,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,wN,0), list);
	} else {
		AddQuietMove(pos, MOVE(from,to,EMPTY,EMPTY,0), list);
	}
}

static void AddBlackPawnCapMove( const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list ) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_2) {
		AddCaptureMove(pos, MOVE(from,to,cap,bQ,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,bR,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,bB,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,bN,0), list);
	} else {
		AddCaptureMove(pos, MOVE(from,to,cap,EMPTY,0), list);
	}
}

static void AddBlackPawnMove( const S_BOARD *pos, const int from, const int to, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_2) {
		AddQuietMove(pos, MOVE(from,to,EMPTY,bQ,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,bR,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,bB,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,bN,0), list);
	} else {
		AddQuietMove(pos, MOVE(from,to,EMPTY,EMPTY,0), list);
	}
}

void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {

	ASSERT(CheckBoard(pos));

	list->count = 0;
	list->quiets = 0;

	int pce = EMPTY;
	int side = pos->side;
	int sq = 0; int t_sq = 0;
	int pceNum = 0;
	int dir = 0;
	int index = 0;
	int pceIndex = 0;

	if(side == WHITE) {

		for(pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if(pos->pieces[sq + 10] == EMPTY) {
				AddWhitePawnMove(pos, sq, sq+10, list);
				if(RanksBrd[sq] == RANK_2 && pos->pieces[sq + 20] == EMPTY) {
					AddQuietMove(pos, MOVE(sq,(sq+20),EMPTY,EMPTY,MFLAGPS),list);
				}
			}

			if(!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq+9, pos->pieces[sq + 9], list);
			}
			if(!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq+11, pos->pieces[sq + 11], list);
			}

			if(pos->enPas != NO_SQ) {
				if(sq + 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq + 9,EMPTY,EMPTY,MFLAGEP), list);
				}
				if(sq + 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq + 11,EMPTY,EMPTY,MFLAGEP), list);
				}
			}
		}

		if(pos->castlePerm & WKCA) {
			if(pos->pieces[F1] == EMPTY && pos->pieces[G1] == EMPTY) {
				if(!SqAttacked(E1,BLACK,pos) && !SqAttacked(F1,BLACK,pos) ) {
					AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

		if(pos->castlePerm & WQCA) {
			if(pos->pieces[D1] == EMPTY && pos->pieces[C1] == EMPTY && pos->pieces[B1] == EMPTY) {
				if(!SqAttacked(E1,BLACK,pos) && !SqAttacked(D1,BLACK,pos) ) {
					AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

	} else {

		for(pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if(pos->pieces[sq - 10] == EMPTY) {
				AddBlackPawnMove(pos, sq, sq-10, list);
				if(RanksBrd[sq] == RANK_7 && pos->pieces[sq - 20] == EMPTY) {
					AddQuietMove(pos, MOVE(sq,(sq-20),EMPTY,EMPTY,MFLAGPS),list);
				}
			}

			if(!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq-9, pos->pieces[sq - 9], list);
			}

			if(!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq-11, pos->pieces[sq - 11], list);
			}
			if(pos->enPas != NO_SQ) {
				if(sq - 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq - 9,EMPTY,EMPTY,MFLAGEP), list);
				}
				if(sq - 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq - 11,EMPTY,EMPTY,MFLAGEP), list);
				}
			}
		}

		// castling
		if(pos->castlePerm &  BKCA) {
			if(pos->pieces[F8] == EMPTY && pos->pieces[G8] == EMPTY) {
				if(!SqAttacked(E8,WHITE,pos) && !SqAttacked(F8,WHITE,pos) ) {
					AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

		if(pos->castlePerm &  BQCA) {
			if(pos->pieces[D8] == EMPTY && pos->pieces[C8] == EMPTY && pos->pieces[B8] == EMPTY) {
				if(!SqAttacked(E8,WHITE,pos) && !SqAttacked(D8,WHITE,pos) ) {
					AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}
	}

	/* Loop for slide pieces */
	pceIndex = LoopSlideIndex[side];
	pce = LoopSlidePce[pceIndex++];
	while( pce != 0) {
		ASSERT(PieceValid(pce));

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				while(!SQOFFBOARD(t_sq)) {
					// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
					if(pos->pieces[t_sq] != EMPTY) {
						if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						}
						break;
					}
					AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
					t_sq += dir;
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];
	}

	/* Loop for non slide */
	pceIndex = LoopNonSlideIndex[side];
	pce = LoopNonSlidePce[pceIndex++];

	while( pce != 0) {
		ASSERT(PieceValid(pce));

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				if(SQOFFBOARD(t_sq)) {
					continue;
				}

				// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
				if(pos->pieces[t_sq] != EMPTY) {
					if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					}
					continue;
				}
				AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}

    ASSERT(MoveListOk(list,pos));
}


void GenerateAllCaps(const S_BOARD *pos, S_MOVELIST *list) {

	ASSERT(CheckBoard(pos));

	list->count = 0;

	int pce = EMPTY;
	int side = pos->side;
	int sq = 0; int t_sq = 0;
	int pceNum = 0;
	int dir = 0;
	int index = 0;
	int pceIndex = 0;

	if(side == WHITE) {

		for(pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if(!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq+9, pos->pieces[sq + 9], list);
			}
			if(!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq+11, pos->pieces[sq + 11], list);
			}

			if(pos->enPas != NO_SQ) {
				if(sq + 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq + 9,EMPTY,EMPTY,MFLAGEP), list);
				}
				if(sq + 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq + 11,EMPTY,EMPTY,MFLAGEP), list);
				}
			}
		}

	} else {

		for(pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if(!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq-9, pos->pieces[sq - 9], list);
			}

			if(!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq-11, pos->pieces[sq - 11], list);
			}
			if(pos->enPas != NO_SQ) {
				if(sq - 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq - 9,EMPTY,EMPTY,MFLAGEP), list);
				}
				if(sq - 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq - 11,EMPTY,EMPTY,MFLAGEP), list);
				}
			}
		}
	}

	/* Loop for slide pieces */
	pceIndex = LoopSlideIndex[side];
	pce = LoopSlidePce[pceIndex++];
	while( pce != 0) {
		ASSERT(PieceValid(pce));

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				while(!SQOFFBOARD(t_sq)) {
					// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
					if(pos->pieces[t_sq] != EMPTY) {
						if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						}
						break;
					}
					t_sq += dir;
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];
	}

	/* Loop for non slide */
	pceIndex = LoopNonSlideIndex[side];
	pce = LoopNonSlidePce[pceIndex++];

	while( pce != 0) {
		ASSERT(PieceValid(pce));

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				if(SQOFFBOARD(t_sq)) {
					continue;
				}

				// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
				if(pos->pieces[t_sq] != EMPTY) {
					if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					}
					continue;
				}
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}
    ASSERT(MoveListOk(list,pos));
}

void fillSq(const int sq, S_BOARD *pos) {

	int piece, color;

    // place a piece on the board
    piece = pos->pieces[sq];
    color = PieceCol[piece];

    ASSERT(color>=WHITE&&color<=BOTH);

    if ( PiecePawn[piece] ) {
      
		// update counter of pawns on a given rank and file
		++pos->pawns_on_file[color][FilesBrd[sq]];
		++pos->pawns_on_rank[color][RanksBrd[sq]];

		// update squares controlled by pawns
		if (color == WHITE) {
			if (!SQOFFBOARD(sq + NE)) pos->pawn_ctrl[WHITE][sq + NE]++;
			if (!SQOFFBOARD(sq + NW)) pos->pawn_ctrl[WHITE][sq + NW]++;
		} else {
			if (!SQOFFBOARD(sq + SE)) pos->pawn_ctrl[BLACK][sq + SE]++;
			if (!SQOFFBOARD(sq + SW)) pos->pawn_ctrl[BLACK][sq + SW]++;
		}
    }

    if(piece != EMPTY) {
    	// update piece-square value
	    pos->pcsq_mg[color] += e->mgPst[piece][sq];
	    pos->pcsq_eg[color] += e->egPst[piece][sq];
    }
}

void clearSq(const int sq, S_BOARD *pos) {

    // set intermediate variables, then do the same
    // as in fillSq(), only backwards

	int piece, color;

    piece = pos->pieces[sq];
    color = PieceCol[piece];
    //ASSERT(PieceValid(piece));
    ASSERT(color>=WHITE&&color<=BOTH);

    if ( PiecePawn[piece] ) {
		// update squares controlled by pawns
		if (color == WHITE) {
			if (!SQOFFBOARD(sq + NE)) pos->pawn_ctrl[WHITE][sq + NE]--;
			if (!SQOFFBOARD(sq + NW)) pos->pawn_ctrl[WHITE][sq + NW]--;
		} else {
			if (!SQOFFBOARD(sq + SE)) pos->pawn_ctrl[BLACK][sq + SE]--;
			if (!SQOFFBOARD(sq + SW)) pos->pawn_ctrl[BLACK][sq + SW]--;
		}

		--pos->pawns_on_file[color][FilesBrd[sq]];
		--pos->pawns_on_rank[color][RanksBrd[sq]];
    }

    if(piece != EMPTY) {
    	pos->pcsq_mg[color] -= e->mgPst[piece][sq];
    	pos->pcsq_eg[color] -= e->egPst[piece][sq];
    }
    
}

static const int SEEPruningDepth = 8;
static const int SEEQuietMargin = -80;
static const int SEENoisyMargin = -18;
static const int SEEPieceValues[] = {
    0, 100,  450,  450,  675,
    1300, 0, 100, 450, 450, 675, 1300, 0
};

int moveBestCaseValue(const S_BOARD *pos) {

    // Assume the opponent has at least a pawn
    int value = SEEPieceValues[wP];
    int sq;
    int piece;

    // Check for a higher value target
    for (piece = bQ; piece >= wP; piece--) { 
    	if(piece == bK || piece == wK) {
    		continue;
    	}
    	for(int pceNum = 0; pceNum < pos->pceNum[piece]; ++pceNum) {
			sq = pos->pList[piece][pceNum];
			ASSERT(SqOnBoard(sq))
			// Check for a potential pawn promotion
			if (   pos->pieces[sq] == wP
		        &&  PieceCol[pos->pieces[sq]] == (pos->side)
		        &&  pos->side == WHITE
		        &&  RanksBrd[sq] == RANK_7)
		        value += SEEPieceValues[wQ] - SEEPieceValues[wP];

		    if (   pos->pieces[sq] == bP
		        &&  PieceCol[pos->pieces[sq]] == (pos->side)
		        && pos->side == BLACK
		        && RanksBrd[sq] == RANK_2)
		        value += SEEPieceValues[bQ] - SEEPieceValues[bP];

			while (PieceCol[pos->pieces[sq]] == (pos->side ^ 1)) {// pos->pieces[piece] && pos->side[!pos->turn] 
            	value = SEEPieceValues[piece];
            	break;
           	}	
		}    
    }
    return value;
}

void setSquaresNearKing() {
    for (int i = 0; i < 120; ++i)
        for (int j = 0; j < 120; ++j)
        {

            e->sqNearK[WHITE][i][j] = 0;
            e->sqNearK[BLACK][i][j] = 0;
            // e->sqNearK[side^1] [ KingSq[side^1] ] [t_sq] 

            if ( !SQOFFBOARD(i) && !SQOFFBOARD(j) ) {

                /* squares constituting the ring around both kings */
                if (j == i + NORTH || j == i + SOUTH 
				||  j == i + EAST  || j == i + WEST 
				||  j == i + NW    || j == i + NE 
				||  j == i + SW    || j == i + SE) {

                    e->sqNearK[WHITE][i][j] = 1;
                    e->sqNearK[BLACK][i][j] = 1;
                }

                /* squares in front of the white king ring */
                /*if (j == i + NORTH + NORTH 
				||  j == i + NORTH + NE 
				||  j == i + NORTH + NW)
                    e->sqNearK[WHITE][i][j] = 1;

                /* squares in front of the black king ring 
                if (j == i + SOUTH + SOUTH 
				||  j == i + SOUTH + SE 
				||  j == i + SOUTH + SW)
                    e->sqNearK[BLACK][i][j] = 1;*/
            }
        }
}

U64 KingAreaMasks[BOTH][64];

void KingAreaMask() {
	int sq;
	int index;
	int MobilityCount = 0;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	int pce;

	for(sq = 0; sq < 64; ++sq) {
		KingAreaMasks[WHITE][sq] = 0ULL;
		KingAreaMasks[BLACK][sq] = 0ULL;
	}
	pce = wK;
	for(sq = 0; sq < BRD_SQ_NUM; ++sq) {
		if(!SQOFFBOARD(sq)) {
			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;
				if(!SQOFFBOARD(t_sq)) {
					KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(t_sq));
					KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(t_sq));
				}
			}
		}	
	}
}

int MobilityCountWhiteKn(const S_BOARD *pos,int pce) {

	int pceNum;
	int sq;
	int index;
	int MobilityCount;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			if(!SQOFFBOARD(t_sq)) {
				//SqAttackedbyThem = SqAttackedByPawn(t_sq,BLACK,pos);
				//if(SqAttackedbyThem == FALSE && PieceCol[pos->pieces[t_sq]] != WHITE) {
				if(PieceCol[pos->pieces[t_sq]] != WHITE) {
					MobilityCount++;
				}
			}
		
		}
		
	}
	//printf("Piece%d:PieceMobility:%d t_sq:%d",pce,MobilityCount,t_sq);
	return MobilityCount;
}

int KingSqAttackedbyPawnB(const S_BOARD *pos) {

	int t_sq;;
	//int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];

	t_sq = SqKIngB - 9; // sq infront left right back to the king
	int SqAttacked = SqAttackedByPawn(t_sq,BLACK,pos);
	if(SqAttacked == TRUE) {
		return TRUE;
	}
	
	t_sq = SqKIngB - 11; // sq infront left right back to the king 
	if(SqAttacked == TRUE) {
		return TRUE;
	}

	t_sq = SqKIngB - 10; // sq infront left right back to the king
	if(SqAttacked == TRUE) {
		return TRUE;
	}
	return FALSE;
}

int KingSqAttackedbyPawnW(const S_BOARD *pos) {

	int t_sq;;
	//int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];

	t_sq = SqKIngB + 9; // sq infront left right back to the king
	int SqAttacked = SqAttackedByPawn(t_sq,WHITE,pos);
	if(SqAttacked == TRUE) {
		return TRUE;
	}
	
	t_sq = SqKIngB + 11; // sq infront left right back to the king 
	if(SqAttacked == TRUE) {
		return TRUE;
	}

	t_sq = SqKIngB + 10; // sq infront left right back to the king
	if(SqAttacked == TRUE) {
		return TRUE;
	}
	return FALSE;
}

int KingSqAttackedbyKnightB(const S_BOARD *pos) {

	int pceNum;
	int sq;
	int index;
	int t_sq;;
	int dir;
	int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];
	int pce = bK;
	
	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = SqKIngW + dir;
		if(!SQOFFBOARD(t_sq)) {  // sq infront left right back to the king
			int SqAttacked = SqAttackedByKnight(t_sq,BLACK,pos);
			if(SqAttacked == TRUE) {
				return TRUE;
			}
		}
	}
	//printf("Piece%d:PieceMobility:%d t_sq:%d",pce,MobilityCount,t_sq);
	return FALSE;
}

int KingSqAttackedbyKnightW(const S_BOARD *pos) {

	int pceNum;
	int sq;
	int index;
	int t_sq;;
	int dir;
	int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];
	int pce = wK;
	
	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = SqKIngB + dir; // sq infront left right back to the king
		if(!SQOFFBOARD(t_sq)) {  // sq infront left right back to the king
			int SqAttacked = SqAttackedByKnight(t_sq,WHITE,pos);
			if(SqAttacked == TRUE) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

int KingSqAttackedbyRookQueenB(const S_BOARD *pos) {

	int pceNum;
	int sq;
	int index;
	int t_sq;;
	int dir;
	int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];
	int pce = bK;
	
	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = SqKIngW + dir; // sq infront left right back to the king
		if(!SQOFFBOARD(t_sq)) {  // sq infront left right back to the king
			int SqAttacked = SqAttackedByRookQueen(t_sq,BLACK,pos);
			if(SqAttacked == TRUE) {
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

int KingSqAttackedbyRookQueenW(const S_BOARD *pos) {

	int pceNum;
	int sq;
	int index;
	int t_sq;;
	int dir;
	int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];
	int pce = wK;
	
	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = SqKIngB + dir; // sq infront left right back to the king
		if(!SQOFFBOARD(t_sq)) {  // sq infront left right back to the king
			int SqAttacked = SqAttackedByRookQueen(t_sq,WHITE,pos);
			if(SqAttacked == TRUE) {
				return TRUE;
			}
		}
	}
	//printf("Piece%d:PieceMobility:%d t_sq:%d",pce,MobilityCount,t_sq);
	return FALSE;
}

int KingSqAttackedbyBishopQueenB(const S_BOARD *pos) {

	int pceNum;
	int sq;
	int index;
	int t_sq;;
	int dir;
	int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];
	int pce = bK;
	
	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = SqKIngW + dir; // sq infront left right back to the king
		if(!SQOFFBOARD(t_sq)) {  // sq infront left right back to the king
			int SqAttacked = SqAttackedByBishopQueen(t_sq,BLACK,pos);
			if(SqAttacked == TRUE) {
				return TRUE;
			}
		}
	}
	//printf("Piece%d:PieceMobility:%d t_sq:%d",pce,MobilityCount,t_sq);
	return FALSE;
}

int KingSqAttackedbyBishopQueenW(const S_BOARD *pos) {

	int pceNum;
	int sq;
	int index;
	int t_sq;;
	int dir;
	int SqKIngW = pos->KingSq[WHITE];
	int SqKIngB = pos->KingSq[BLACK];
	int pce = wK;
	
	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = SqKIngB + dir; // sq infront left right back to the king
		if(!SQOFFBOARD(t_sq)) {  // sq infront left right back to the king
			int SqAttacked = SqAttackedByBishopQueen(t_sq,WHITE,pos);
			if(SqAttacked == TRUE) {
				return TRUE;
			}
		}
	}
	//printf("Piece%d:PieceMobility:%d t_sq:%d",pce,MobilityCount,t_sq);
	return FALSE;
}
int MobilityCountBlackKn(const S_BOARD *pos,int pce) {

	int pceNum;
	int sq;
	int index;
	int MobilityCount;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			if(!SQOFFBOARD(t_sq)) {
				//SqAttackedbyThem = SqAttackedByPawn(t_sq,WHITE,pos);
				//if(SqAttackedbyThem == FALSE && PieceCol[pos->pieces[t_sq]] != BLACK)
				if(PieceCol[pos->pieces[t_sq]] != BLACK) {
					MobilityCount++;
				}
				//}
			}
		
		}
		
	}
	//printf("Piece%d:PieceMobility:%d t_sq:%d",pce,MobilityCount,t_sq);
	return MobilityCount;
}

int MobilityCountWhiteBi(const S_BOARD *pos,int pce) {

	int pceNum;
	int sq;
	int index;
	int MobilityCount;
	int tf_sq;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			tf_sq = sq + dir;
			if(!SQOFFBOARD(tf_sq)) {
				t_sq = tf_sq;
			}
			printf("t_sq:%d",t_sq);
			SqAttackedbyThem = SqAttackedByPawn(t_sq,BLACK,pos);
			if(SqAttackedbyThem == FALSE && PieceCol[pos->pieces[t_sq]] != WHITE) {
				MobilityCount++;
			}		
		}
		
	}
	printf("Piece%d:PieceMobility:%d t_sq:%d",pce,MobilityCount,t_sq);
	return MobilityCount;
}