// evaluate.c

#include "stdio.h"
#include "defs.h"
#include "evaluate.h"

eval_info ei[1];

// sjeng 11.2
//setboard 8/6R1/2k5/6P1/8/8/4nP2/6K1 w - - 1 41 
int MaterialDraw(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));
	
    if (!pos->pceNum[wR] && !pos->pceNum[bR] && !pos->pceNum[wQ] && !pos->pceNum[bQ]) {
	  if (!pos->pceNum[bB] && !pos->pceNum[wB]) {
	      if (pos->pceNum[wN] < 3 && pos->pceNum[bN] < 3) {  return TRUE; }
	  } else if (!pos->pceNum[wN] && !pos->pceNum[bN]) {
	     if (abs(pos->pceNum[wB] - pos->pceNum[bB]) < 2) { return TRUE; }
	  } else if ((pos->pceNum[wN] < 3 && !pos->pceNum[wB]) || (pos->pceNum[wB] == 1 && !pos->pceNum[wN])) {
	    if ((pos->pceNum[bN] < 3 && !pos->pceNum[bB]) || (pos->pceNum[bB] == 1 && !pos->pceNum[bN]))  { return TRUE; }
	  }
	} else if (!pos->pceNum[wQ] && !pos->pceNum[bQ]) {
        if (pos->pceNum[wR] == 1 && pos->pceNum[bR] == 1) {
            if ((pos->pceNum[wN] + pos->pceNum[wB]) < 2 && (pos->pceNum[bN] + pos->pceNum[bB]) < 2)	{ return TRUE; }
        } else if (pos->pceNum[wR] == 1 && !pos->pceNum[bR]) {
            if ((pos->pceNum[wN] + pos->pceNum[wB] == 0) && (((pos->pceNum[bN] + pos->pceNum[bB]) == 1) || ((pos->pceNum[bN] + pos->pceNum[bB]) == 2))) { return TRUE; }
        } else if (pos->pceNum[bR] == 1 && !pos->pceNum[wR]) {
            if ((pos->pceNum[bN] + pos->pceNum[bB] == 0) && (((pos->pceNum[wN] + pos->pceNum[wB]) == 1) || ((pos->pceNum[wN] + pos->pceNum[wB]) == 2))) { return TRUE; }
        }
    }
  return FALSE;
}

int popcount(U64 bb) {
    return __builtin_popcountll(bb);
}

static int mirrorFile(const int file) {
    static const int Mirror[] = {0,1,2,3,3,2,1,0};
    ASSERT(0 <= file && file < FILE_NONE);
    return Mirror[file];
}

U64 pawnAdvance(U64 pawns, U64 occupied, int colour) {
    return ~occupied & pawns;
}

int pawns_on_same_color_squares(const S_BOARD *pos, int c, int s) {
	return popcount(pos->pawns[c] & ((BLACK_SQUARES & s) ? BLACK_SQUARES : ~BLACK_SQUARES));
}

static int getTropism(const int sq1, const int sq2) {
	return 7 - (abs(FilesBrd[(sq1)] - FilesBrd[(sq2)]) + abs(RanksBrd[(sq1)] - RanksBrd[(sq2)]));
}

static int king_proximity(const int c, const int s, const S_BOARD *pos) {
    return MIN(SquareDistance[pos->KingSq[c]][s], 5);
};

static int distanceBetween(int s1, int s2) {
    return SquareDistance[s1][s2];
}

static int map_to_queenside(const int f) {
	return MIN(f, FILE_H - f);
}

static int clamp(const int v, const int lo, const int hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

static int isPiece(const int color, const int piece, const int sq, const S_BOARD *pos) {
    return ( (pos->pieces[sq] == piece) && (PieceCol[pos->pieces[sq]] == color) );
}

static int REL_SQ(const int sq120, const int side) {
	return side == WHITE ? sq120 : Mirror120[SQ64(sq120)];
}

static int rank_of(int s) {
	return s / 8;
}

static int relative_rank2(const int c, const int r) {
	return c == WHITE ? r : 7 - r;
}

static int relative_rank(const int c, const int s) {
	return relative_rank2(c, rank_of(s));
}

static int relative_rankBB(const int c, const int s) {
	return c == WHITE ? RankBBMask[RanksBrd[s]] : 7 - RankBBMask[RanksBrd[s]];
}

int getlsb(U64 bb) {
    return __builtin_ctzll(bb);
}

int getmsb(U64 bb) {
    return __builtin_clzll(bb) ^ 63;
}

int frontmost(int colour, U64 b) {
    ASSERT(0 <= colour && colour < BOTH);
    return colour == WHITE ? getmsb(b) : getlsb(b);
}
int backmost(int colour, U64 b) {
    ASSERT(0 <= colour && colour < BOTH);
    return colour == WHITE ? getlsb(b) : getmsb(b);
}

int evaluateScaleFactor(S_BOARD *pos) {

    if (   pos->pceNum[wB] == 1
    	&& pos->pceNum[bB] == 1) {

    	if (!pos->pceNum[wN] && !pos->pceNum[bN] ||
    	    !pos->pceNum[wR] && !pos->pceNum[bR] ||
    	    !pos->pceNum[wQ] && !pos->pceNum[bQ]) {
    		return SCALE_OCB_BISHOPS_ONLY;
    	}

    	if (!pos->pceNum[wR] && !pos->pceNum[bR] || 
    		!pos->pceNum[wQ] && !pos->pceNum[bQ] && 
    		pos->pceNum[wN] == 1 && pos->pceNum[bN] == 1 ) {
    		return SCALE_OCB_ONE_KNIGHT;
    	}

    	if (!pos->pceNum[wN] && !pos->pceNum[bN] ||
    	    !pos->pceNum[wQ] && !pos->pceNum[bQ] &&
    	    pos->pceNum[wR] == 1 && pos->pceNum[bR] == 1 ) {
    		return SCALE_OCB_ONE_ROOK;
    	}
    }

    return SCALE_NORMAL;
}

void Pawns(S_BOARD *pos, int side, int pce, int pceNum) {

	int sq;
	int support, supportCount = 0, pawnbrothers, connected;
	U64 opposed;
	U64 PassedMask;
	U64 PassedMaskFlip;
	U64 flag;
	int blockSq, w, i, R;
	int dist;
	int bonus;
	int bonus2;
	int dir, t_sq, index;
	int att = 0;

	int SU, UP, SE1, SW1;

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	R = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);
	SU = (side == WHITE ? -10 : 10);
	UP = (side == WHITE ? 10 : -10);
	SE1 = (side == WHITE ? -9 : 9);
	SW1 = (side == WHITE ? -11 : 11);

	PassedMask = (side == WHITE ? WhitePassedMask[SQ64(sq)] : BlackPassedMask[SQ64(sq)]);
	PassedMaskFlip = (side == WHITE ? BlackPassedMask[SQ64(sq)] : WhitePassedMask[SQ64(sq)]);

	/*if (pos->pieces[sq+SE1] == pce ) {
		supportCount++;
	} else if (pos->pieces[sq+SW1] == pce) {
		supportCount++;
	}*/
	opposed = (FileBBMask[FilesBrd[sq]] & pos->pawns[side^1]);
	support = (pos->pieces[sq+SE1] == pce || pos->pieces[sq+SW1] == pce );
	pawnbrothers = (pos->pieces[sq-1] == pce || pos->pieces[sq+1] == pce);

	//printf("opposed %d\n",bool(opposed) );
	//printf("support %d\n",bool(support) );
	//printf("pawnbrothers %d\n",bool(pawnbrothers) );

	if(pos->pieces[sq+SU] == pce) {
		//printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
		ei->pawnsMG[side] -= Doubled;
		ei->pawnsEG[side] -= DoubledEG;
	}
	if(pos->pieces[sq+SU*2] == pce) {
		//printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
		ei->pawnsMG[side] -= Doubled;
		ei->pawnsEG[side] -= DoubledEG;
	}
	if(pos->pieces[sq+SU*3] == pce) {
		//printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
		ei->pawnsMG[side] -= Doubled;
		ei->pawnsEG[side] -= DoubledEG;
	}

	if( (IsolatedMask[SQ64(sq)] & pos->pawns[side]) == 0) {
		//printf("%c Iso:%s\n",PceChar[pce], PrSq(sq));
		//printf("Iso P %d\n",PawnIsolated + WeakUnopposed * !opposed );
		ei->pawnsMG[side] -= PawnIsolated + WeakUnopposed * !opposed;
		ei->pawnsEG[side] -= PawnIsolatedEG + WeakUnopposedEG * !opposed;
	}

	if ( !(PassedMaskFlip & pos->pawns[side])
    	&& SqAttackedByPawn(sq + UP,side^1,pos)) {
		//printf("%c BackW:%s\n",PceChar[pce], PrSq(sq));
        flag = !(opposed);
        ei->pawnsMG[side] += PawnBackwards[flag];
        ei->pawnsEG[side] += PawnBackwardsEG[flag];
    }

    if( (PassedMask & pos->pawns[side^1]) == 0) {
		//printf("%c Passed:%s\n",PceChar[pce], PrSq(sq));
		ei->pawnsMG[side] += PawnPassed[R];
		ei->pawnsEG[side] += PawnPassedEG[R];

		if(support || pawnbrothers) {
			ei->pawnsMG[side] += Connected[R];
			ei->pawnsEG[side] += Connected[R];
		}

		if(R > RANK_3) {

			blockSq = sq + UP;
			w = 5 * R - 13;

			bonus = ((  king_proximity(side^1, blockSq, pos) * 5
                      - king_proximity(side,   blockSq, pos) * 2) * w);
			
            if (R != RANK_7) {
                bonus -= ( king_proximity(side, blockSq + UP, pos) * w);
            }
            bonus *= 100;
            bonus /= 410;
    
            ei->pawnsEG[side] += bonus;
		}
	}

	/*if(support || pawnbrothers) {

		int v =  Connected[R] * (2 + bool(pawnbrothers) - bool(opposed))
				+ 21 * supportCount;
        v *= 100;
        v /= 510;

        //printf("supportCount %d v %d v eg %d \n",supportCount, v, v * (R - 2) / 4);
		ei->pawnsMG[side] += v;
		ei->pawnsEG[side] += v * (R - 2) / 4;
	}*/

    for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = sq + dir;
		if(!SQOFFBOARD(t_sq)) {
			if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
				att++;
			}    
		}
	}
	
	if(att) {
		//printf("att %d\n", att);
    	ei->attCnt[side] += att;
    	//ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];		
    }
}

void Knights(const S_BOARD *pos, int side, int pce, int pceNum) {
	int sq;
	int index;
	int t_sq;
	int dir;

	int att = 0;
	int mobility = 0;
	int tropism;

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
	//ei->PSQTMG[side] += KnightTableMG[SQ64(sq)];
	//ei->PSQTEG[side] += KnightTableEG[SQ64(sq)];

	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = sq + dir;
		if(!SQOFFBOARD(t_sq) && PieceCol[pos->pieces[t_sq]] != side) {

			if(!pos->pawn_ctrl[side^1][t_sq]) {
				mobility++;
			}
			//printf("t_sq %s pawn_ctrl %d\n", PrMove(t_sq),pos->pawn_ctrl[side^1][t_sq]);
				
			if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
				att++;
			}    
		}
	}
	//printf("%1c mobility %d\n",PceChar[pce],mobility );

    tropism = getTropism(sq, pos->KingSq[side^1]);
	ei->mgTropism[side] += 3 * tropism;
	ei->egTropism[side] += 3 * tropism;	
	ei->mgMob[side] += 4 * (mobility-4);
    ei->egMob[side] += 4 * (mobility-4);

    int Count = distanceBetween(sq, pos->KingSq[side]);
    int P1 = (7 * Count) * 100 / 310;
    int P2 = (8 * Count) * 100 / 310;
    ei->mgTropism[side] -= P1;
    ei->egTropism[side] -= P2;

	if(att) {
    	ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }	
}

void Bishops(const S_BOARD *pos, int side, int pce, int pceNum) {
	int sq;
	int index;
	int t_sq;
	int dir;

	int att = 0;
	int mobility = 0;
	int tropism;

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = sq + dir;
		while(!SQOFFBOARD(t_sq)) {

			if(pos->pieces[t_sq] == EMPTY) {
				if(!pos->pawn_ctrl[side^1][t_sq]) {
					mobility++;
				}
				if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
					att++;
				}
			} else { // non empty sq
				if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
					if(!pos->pawn_ctrl[side^1][t_sq]) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att++;
					}
				}
				break;
			} 
			t_sq += dir;   
		}
	}

	//printf("%1c mobility %d\n",PceChar[pce],mobility );

	tropism = getTropism(sq, pos->KingSq[side^1]);
	ei->mgTropism[side] += 2 * tropism;
	ei->egTropism[side] += 1 * tropism;	
	ei->mgMob[side] += 3 * (mobility-7);
    ei->egMob[side] += 3 * (mobility-7);

    int Count = pawns_on_same_color_squares(pos,side,SQ64(sq));
    int P1 = (BishopPawns * Count) * 100 / 310;
    int P2 = (BishopPawnsEG * Count) * 100 / 310;
   
    ei->mgMob[side] -= P1;
    ei->egMob[side] -= P2;

    Count = distanceBetween(sq, pos->KingSq[side]);
    P1 = (7 * Count) * 100 / 310;
    P2 = (8 * Count) * 100 / 310;
    ei->mgTropism[side] -= P1;
    ei->egTropism[side] -= P2;

    if(att) {
    	ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }
}

void Rooks(const S_BOARD *pos, int side, int pce, int pceNum ) {
	int sq;
	int index;
	int t_sq;
	int dir;

	int att = 0;
	int mobility = 0;
	int tropism;

	int REL_R7 = (side == WHITE ? RANK_7 : RANK_2);
	int REL_R8 = (side == WHITE ? RANK_8 : RANK_1);

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = sq + dir;
		while(!SQOFFBOARD(t_sq)) {

			if(pos->pieces[t_sq] == EMPTY) {
				if(!pos->pawn_ctrl[side^1][t_sq]) {
					mobility++;
				}
				if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
					att++;
				}
			} else { // non empty sq
				if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
					if(!pos->pawn_ctrl[side^1][t_sq]) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att++;
					}
				}
				break;
			} 
			t_sq += dir;   
		}
	}
	//printf("%1c mobility %d\n",PceChar[pce],mobility );

	if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
		ei->mgMob[side] += RookOpenFile;
    	ei->egMob[side] += RookOpenFile;
	    if (abs(RanksBrd[sq] - RanksBrd[pos->KingSq[side^1]] ) < 2) 
			ei->attWeight[side] += 1;
	}
	if(!(pos->pawns[side] & FileBBMask[FilesBrd[sq]])) {
		ei->mgMob[side] += RookSemiOpenFile;
    	ei->egMob[side] += RookSemiOpenFile;
    	if (abs(RanksBrd[sq] - RanksBrd[pos->KingSq[side^1]] ) < 2) 
			ei->attWeight[side] += 2;
	}

	if((RankBBMask[RanksBrd[sq]] == REL_R7 && RankBBMask[RanksBrd[pos->KingSq[side^1]]] == REL_R8)) {
		ei->mgMob[side] += 20;
		ei->egMob[side] += 30;
	}

	tropism = getTropism(sq, pos->KingSq[side^1]);
	ei->mgTropism[side] += 2 * tropism;
	ei->egTropism[side] += 1 * tropism;	
	ei->mgMob[side] += 2 * (mobility-7);
    ei->egMob[side] += 4 * (mobility-7);

	if (att) {
		ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }
}

void Queens(const S_BOARD *pos, int side, int pce, int pceNum) {
	int sq;
	int index;
	int t_sq;
	int dir;

	int att = 0;
	int mobility = 0;
	int tropism;

	int KNIGHT, BISHOP;

	int REL_R7 = (side == WHITE ? RANK_7 : RANK_2);
	int REL_R8 = (side == WHITE ? RANK_8 : RANK_1);
	int REL_R2 = (side == WHITE ? RANK_2 : RANK_7);
	KNIGHT = (side == WHITE ? wN : bN);
	BISHOP = (side == WHITE ? wB : bB);

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = sq + dir;
		while(!SQOFFBOARD(t_sq)) {

			if(pos->pieces[t_sq] == EMPTY) {
				if(!pos->pawn_ctrl[side^1][t_sq]) {
					mobility++;
				}
				if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
					att++;
				}
			} else { // non empty sq
				if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
					if(!pos->pawn_ctrl[side^1][t_sq]) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att++;
					}
				}
				break;
			} 
			t_sq += dir;   
		}
	}
	//printf("%1c mobility %d\n",PceChar[pce],mobility );

	if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
		ei->mgMob[side] += QueenOpenFile;
    	ei->egMob[side] += QueenOpenFile;
	}
	if(!(pos->pawns[side] & FileBBMask[FilesBrd[sq]])) {
		ei->mgMob[side] += QueenSemiOpenFile;
    	ei->egMob[side] += QueenSemiOpenFile;
	}

	if((RankBBMask[RanksBrd[sq]] == REL_R7 && RankBBMask[RanksBrd[pos->KingSq[side^1]]] == REL_R8)) {
		ei->mgMob[side] += 5;
		ei->egMob[side] += 10;
	}

	if (RanksBrd[sq] > REL_R2) {
		if(isPiece(side, KNIGHT, REL_SQ(side,B1), pos)) ei->positionalThemes[side] -= 2;
		if(isPiece(side, BISHOP, REL_SQ(side,C1), pos)) ei->positionalThemes[side] -= 2;
		if(isPiece(side, BISHOP, REL_SQ(side,F1), pos)) ei->positionalThemes[side] -= 2;
		if(isPiece(side, KNIGHT, REL_SQ(side,G1), pos)) ei->positionalThemes[side] -= 2;
	}

	tropism = getTropism(sq, pos->KingSq[side^1]);
	ei->mgTropism[side] += 2 * tropism;
	ei->egTropism[side] += 4 * tropism;	
	ei->mgMob[side] += 1 * (mobility-14);
    ei->egMob[side] += 2 * (mobility-14);

	if (att) {
		ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }
}

void pieces(S_BOARD *pos) {
	int pce;
	pce = wP;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Pawns(pos, WHITE, pce, pceNum);
	}
	pce = bP;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Pawns(pos, BLACK, pce, pceNum);
	}
	pce = wN;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Knights(pos, WHITE, pce, pceNum);
	}
	pce = bN;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Knights(pos, BLACK, pce, pceNum);
	}
	pce = wB;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Bishops(pos, WHITE, pce, pceNum);
	}
	pce = bB;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Bishops(pos, BLACK, pce, pceNum);
	}
	pce = wR;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Rooks(pos, WHITE, pce, pceNum);
	}
	pce = bR;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Rooks(pos, BLACK, pce, pceNum);
	}
	pce = wQ;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Queens(pos, WHITE, pce, pceNum);
	}
	pce = bQ;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		Queens(pos, BLACK, pce, pceNum);
	}
}

void EvaluateKings(const S_BOARD *pos) {
	int score = 0;
	int pce;
	int pceNum2, sq2;
	int sq;
	int ourDist, theirDist, blocked;
	U64 ours;
	U64 theirs;
	int PAWN, PAWNOPPO;
	int side;
	int count;
	int center;

   	//PrintBitBoard(ei->kingAreas[WHITE]);
	//PrintBitBoard(ei->kingAreas[BLACK]);
	if (ei->attckersCnt[BLACK] > 1 - pos->pceNum[bQ]) {
	
		float scaledAttackCounts = 9.0 * ei->attCnt[BLACK] / popcount(ei->kingAreas[WHITE]) + 1;
		count = ei->attckersCnt[BLACK] * ei->attWeight[BLACK];
		count += 44 * scaledAttackCounts
			  + -22 * popcount(pos->pawns[WHITE] & ei->kingAreas[WHITE]);

		//printf("BLACK attacks %d attckersCnt %d attWeight %d \n", ei->attCnt[BLACK], ei->attckersCnt[BLACK], ei->attWeight[BLACK]);
		//printf("scaledAttackCounts %d Panws & KA %d\n",scaledAttackCounts, popcount(pos->pawns[WHITE] & ei->kingAreas[WHITE]) );
		if(count > 0) {
			//printf("count %d\n", count);
			ei->KD[BLACK] += count * count / 720;
			ei->KDE[BLACK] += count / 20;
		}
	}
    if (ei->attckersCnt[WHITE] > 1 - pos->pceNum[wQ]) {
    
    	float scaledAttackCounts = 9.0 * ei->attCnt[WHITE] / popcount(ei->kingAreas[BLACK]) + 1;
		count = ei->attckersCnt[WHITE] * ei->attWeight[WHITE];
		//printf(" WHITE attacks %d attckersCnt %d attWeight %d \n", ei->attCnt[WHITE], ei->attckersCnt[WHITE], ei->attWeight[WHITE]);
		count += 44 * scaledAttackCounts
		      + -22 * popcount(pos->pawns[BLACK] & ei->kingAreas[BLACK]);
		//printf("scaledAttackCounts %d Panws & KA %d\n",scaledAttackCounts, popcount(pos->pawns[BLACK] & ei->kingAreas[BLACK]) );
		if(count > 0) {
			//printf("count2 %d\n",count );
			ei->KD[WHITE] += count * count / 720;
			ei->KDE[WHITE] += count / 20;
		}
    }

	pce = wK;
	side = WHITE;
	sq = pos->pList[pce][0];
	//ei->PSQTMG[WHITE] += KingMG[SQ64(sq)];
	//ei->PSQTEG[WHITE] += KingEG[SQ64(sq)];

	if (!(pos->pawns[BOTH] & KingFlank[FilesBrd[sq]])) { 
		ei->mgMob[side] -= 8;
		ei->egMob[side] -= 44;
	}

	center = clamp(FilesBrd[pos->KingSq[side]], FILE_B, FILE_G);
  	for (int file = center - 1; file <= center + 1; ++file) {
  		//printf("file %c\n",'a' + file);

  		ours = (pos->pawns[side] & FileBBMask[file]);
  		//printf("count %d ours %d\n",popcount(ours),bool(ours));
  		//printf("ours sq %s\n",PrSq(SQ120(ours)) );
  		int ourRank = ours ? relative_rank(side, frontmost(side^1, ours)) : 0;
  		//printf("ourRank %d\n",ourRank );

  	 	theirs = (pos->pawns[side^1] & FileBBMask[file]);
  	 	//printf("count %d theirs %d\n",popcount(theirs),bool(theirs));
  	 	//printf("theirs sq %s\n",PrSq(SQ120(theirs)) );
      	int theirRank = theirs ? relative_rank(side, frontmost(side^1, theirs)) : 0;
      	//printf("theirRank %d\n",theirRank );

      	int d = map_to_queenside(file);
      	int bonus = 5;
      	bonus += ShelterStrength[d][ourRank];
      	//printf("bonus %d\n",bonus );

      	if (ourRank && (ourRank == theirRank - 1)) {
      		bonus -= BlockedStorm * int(theirRank == RANK_3);
        	//printf("bonus BlockedStorm %d\n", bonus);
      	} else {
      		bonus -= UnblockedStorm[d][theirRank];
        	//printf("bonus UnblockedStorm %d\n", bonus);
      	}
      	bonus *= 100;
        bonus /= 410;
      	ei->pkeval[side] += bonus;
  	}

	pce = bK;
	side = BLACK;
	sq = pos->pList[pce][0];
	//ei->PSQTMG[BLACK] += KingMG[MIRROR64(SQ64(sq))];
	//ei->PSQTEG[BLACK] += KingEG[MIRROR64(SQ64(sq))];
	//printf("\n\nBLACK" );
	if (!(pos->pawns[BOTH] & KingFlank[FilesBrd[sq]])) { 
		ei->mgMob[side] -= 8;
		ei->egMob[side] -= 44;
	}

	center = clamp(FilesBrd[pos->KingSq[side]], FILE_B, FILE_G);
  	for (int file = center - 1; file <= center + 1; ++file) {
  		//printf("file %c\n",'a' + file);

  		ours = (pos->pawns[side] & FileBBMask[file]);
  		//printf("count %d ours %d\n",popcount(ours),bool(ours));
  		//printf("ours sq %s\n",PrSq(SQ120(ours)) );
  		int ourRank = ours ? relative_rank(side, frontmost(side^1, ours)) : 0;
  		//printf("ourRank %d\n",ourRank );

  	 	theirs = (pos->pawns[side^1] & FileBBMask[file]);
  	 	//printf("count %d theirs %d\n",popcount(theirs),bool(theirs));
  	 	//printf("theirs sq %s\n",PrSq(SQ120(theirs)) );
      	int theirRank = theirs ? relative_rank(side, frontmost(side^1, theirs)) : 0;
      	//printf("theirRank %d\n",theirRank );

      	int d = map_to_queenside(file);
      	int bonus = 5;
      	bonus += ShelterStrength[d][ourRank];
      	//printf("bonus %d\n",bonus );

      	if (ourRank && (ourRank == theirRank - 1)) {
      		bonus -= BlockedStorm * int(theirRank == RANK_3);
        	//printf("bonus BlockedStorm %d\n", bonus);
      	} else {
      		bonus -= UnblockedStorm[d][theirRank];
        	//printf("bonus UnblockedStorm %d\n", bonus);
      	}
      	bonus *= 100;
        bonus /= 410;
      	ei->pkeval[side] += bonus;
  	}
}

int wKingShield(const S_BOARD *pos) {

    int score = 0;
    int side = WHITE;
    int PAWN;
    side == WHITE ? PAWN = wP : PAWN = bP;

    /* king on the kingside */
    if ( FileBBMask[FilesBrd[pos->KingSq[side]]] > FILE_E ) {
        
        if( isPiece(side, PAWN, F2,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, F3,pos) ) score += SHIELD_3;

        if( isPiece(side, PAWN, G2,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, G3,pos) ) score += SHIELD_3;
        
        if( isPiece(side, PAWN, H2,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, H3,pos) ) score += SHIELD_3;
        
    }

    /* king on the queenside */
    else if ( FileBBMask[FilesBrd[pos->KingSq[side]]] < FILE_D ) {
   
        if( isPiece(side, PAWN, A2,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, A3,pos)) score += SHIELD_3;

        if( isPiece(side, PAWN, B2,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, B3,pos)) score += SHIELD_3;
        
        if( isPiece(side, PAWN, C2,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, C3,pos)) score += SHIELD_3;
        
    }

    return score;
}

int bKingShield(const S_BOARD *pos) {
    int score = 0;

    int side = BLACK;
    int PAWN;
    side == WHITE ? PAWN = wP : PAWN = bP;
	/* king on the kingside */
    if ( FileBBMask[FilesBrd[pos->KingSq[side]]] > FILE_E ) {
      
        if( isPiece(side, PAWN, F7,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, F6,pos) ) score += SHIELD_3;

        if( isPiece(side, PAWN, G7,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, G6,pos) ) score += SHIELD_3;
        
        if( isPiece(side, PAWN, H7,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, H6,pos) ) score += SHIELD_3;
        
    }

    /* king on the queenside */
    else if ( FileBBMask[FilesBrd[pos->KingSq[side]]] < FILE_D ) {

        if( isPiece(side, PAWN, A7,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, A6,pos)) score += SHIELD_3;

        if( isPiece(side, PAWN, B7,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, B6,pos)) score += SHIELD_3;
        
        if( isPiece(side, PAWN, C7,pos)) score += SHIELD_2;
        else if( isPiece(side, PAWN, C6,pos)) score += SHIELD_3;
        
    }

    return score;
}

void blockedPieces(int side, const S_BOARD *pos) {

	int oppo = side^1;
	int PAWN, BISHOP, KNIGHT, ROOK, KING, PAWNOPPO;

	side == WHITE ? PAWN = wP : PAWN = bP;
    side == WHITE ? BISHOP = wB : BISHOP = bB;
    side == WHITE ? KNIGHT = wN : KNIGHT = bN;
    side == WHITE ? ROOK = wR : ROOK = bR;
    side == WHITE ? KING = wK : KING = bK;

    side == WHITE ? PAWNOPPO = bP : PAWNOPPO = wP;

    // central pawn blocked, bishop hard to develop
    if (isPiece(side, BISHOP, REL_SQ(side,C1),pos) 
	&& isPiece(side, PAWN, REL_SQ(side,D2),pos) 
	&& pos->pieces[REL_SQ(side,D3)] != EMPTY)
       ei->blockages[side] -= P_BLOCK_CENTRAL_PAWN;

	if (isPiece(side, BISHOP, REL_SQ(side,F1),pos) 
	&& isPiece(side, PAWN, REL_SQ(side,E2),pos) 
	&& pos->pieces[REL_SQ(side,E3)] != EMPTY)
	   ei->blockages[side] -= P_BLOCK_CENTRAL_PAWN;

	// trapped knight
	if ( isPiece(side, KNIGHT, REL_SQ(side,A8),pos) 
	&& ( isPiece(oppo, PAWNOPPO, REL_SQ(side,A7),pos) || isPiece(oppo, PAWNOPPO, REL_SQ(side,C7),pos)))
		ei->blockages[side] -= P_KNIGHT_TRAPPED_A8;

	if ( isPiece(side, KNIGHT, REL_SQ(side,H8),pos)
	&& ( isPiece(oppo, PAWNOPPO, REL_SQ(side,H7),pos) || isPiece(oppo, PAWNOPPO, REL_SQ(side,F7),pos)))
	    ei->blockages[side] -= P_KNIGHT_TRAPPED_A8;
 
	if (isPiece(side, KNIGHT, REL_SQ(side, A7),pos)
	&&  isPiece(oppo, PAWNOPPO, REL_SQ(side,A6),pos)
	&&  isPiece(oppo, PAWNOPPO, REL_SQ(side,B7),pos))
	    ei->blockages[side] -= P_KNIGHT_TRAPPED_A7;

	if (isPiece(side, KNIGHT, REL_SQ(side, H7),pos)
	&&  isPiece(oppo, PAWNOPPO, REL_SQ(side,H6),pos)
	&&  isPiece(oppo, PAWNOPPO, REL_SQ(side,G7),pos))
	    ei->blockages[side] -= P_KNIGHT_TRAPPED_A7;

	 // knight blocking queenside pawns
	if (isPiece(side, KNIGHT, REL_SQ(side, C3),pos)
	 && isPiece(side, PAWN, REL_SQ(side, C2),pos)
	 && isPiece(side, PAWN, REL_SQ(side, D4),pos)
	 && !isPiece(side, PAWN, REL_SQ(side, E4),pos) ) 
	    ei->blockages[side] -= P_C3_KNIGHT;

	 // trapped bishop
	if (isPiece(side, BISHOP, REL_SQ(side,A7),pos)
	 &&  isPiece(oppo, PAWNOPPO, REL_SQ(side,B6),pos)) 
	   	ei->blockages[side] -= P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, REL_SQ(side,H7),pos)
	 &&  isPiece(oppo, PAWNOPPO, REL_SQ(side,G6),pos)) 
	   	ei->blockages[side] -= P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, REL_SQ(side,B8),pos)
	 &&  isPiece(oppo, PAWNOPPO, REL_SQ(side,C7),pos)) 
	   	ei->blockages[side] -= P_BISHOP_TRAPPED_A7;   

	if (isPiece(side, BISHOP, REL_SQ(side,G8),pos)
	 &&  isPiece(oppo, PAWNOPPO, REL_SQ(side,F7),pos)) 
	   	ei->blockages[side] -= P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, REL_SQ(side,A6),pos)
	 &&  isPiece(oppo, PAWNOPPO, REL_SQ(side,B5),pos)) 
	   	ei->blockages[side] -= P_BISHOP_TRAPPED_A6;

	if (isPiece(side, BISHOP, REL_SQ(side,H6),pos)
	 &&  isPiece(oppo, PAWNOPPO, REL_SQ(side,G5),pos)) 
	   	ei->blockages[side] -= P_BISHOP_TRAPPED_A6;

	 // bishop on initial square supporting castled king
	if (isPiece(side, BISHOP, REL_SQ(side,F1),pos)
	 && isPiece(side, KING, REL_SQ(side,G1),pos)) 
	   	ei->positionalThemes[side] += RETURNING_BISHOP;

	if (isPiece(side, BISHOP, REL_SQ(side,C1),pos)
	 && isPiece(side, KING, REL_SQ(side,B1),pos)) 
	   	ei->positionalThemes[side] += RETURNING_BISHOP;

    // uncastled king blocking own rook
    if ( ( isPiece(side, KING, REL_SQ(side,F1),pos) || isPiece(side, KING, REL_SQ(side,G1),pos) )
	&&   ( isPiece(side, ROOK, REL_SQ(side,H1),pos) || isPiece(side, ROOK, REL_SQ(side,G1),pos) ) ) 
	   	ei->blockages[side] -= P_KING_BLOCKS_ROOK;

	if ( ( isPiece(side, KING, REL_SQ(side,C1),pos) || isPiece(side, KING, REL_SQ(side,B1),pos) )
	&&   ( isPiece(side, ROOK, REL_SQ(side,A1),pos) || isPiece(side, ROOK, REL_SQ(side,B1),pos) ) ) 
	   	ei->blockages[side] -= P_KING_BLOCKS_ROOK;
}

void blockedPiecesW(const S_BOARD *pos) {

	int side = WHITE;

    // central pawn blocked, bishop hard to develop
    if (pos->pieces[C1] == wB
	&&  pos->pieces[D2] == wP
	&& pos->pieces[D3] != EMPTY) {
       ei->blockages[side] -= P_BLOCK_CENTRAL_PAWN;
	}

	if (pos->pieces[F1] == wB
	&& pos->pieces[E2] == wP
	&& pos->pieces[E3] != EMPTY) {
	   ei->blockages[side] -= P_BLOCK_CENTRAL_PAWN;
	}

	// trapped knight
	if ( pos->pieces[A8] == wN
	&&  ( pos->pieces[A7] == bP || pos->pieces[C7] == bP) ) {
		ei->blockages[side] -= P_KNIGHT_TRAPPED_A8;
	}

	if ( pos->pieces[H8] == wN
	&& ( pos->pieces[H7] == bP || pos->pieces[F7] == bP)) {
		ei->blockages[side] -= P_KNIGHT_TRAPPED_A8;
	}

	if (pos->pieces[A7] == wN
	&&  pos->pieces[A6] == bP
	&&  pos->pieces[B7] == bP) {
	    ei->blockages[side] -= P_KNIGHT_TRAPPED_A7;
	}

	if (pos->pieces[H7] == wN
	&&  pos->pieces[H6] == bP
	&&  pos->pieces[G7] == bP) {
	    ei->blockages[side] -= P_KNIGHT_TRAPPED_A7;
	}

	 // knight blocking queenside pawns
	if (pos->pieces[C3] == wN
	&&  pos->pieces[C2] == wP
	&& (pos->pieces[D4] == wP || pos->pieces[D2] == wP)
	&&  pos->pieces[E4] != wP) {
	    ei->blockages[side] -= P_C3_KNIGHT;
	}

	 // trapped bishop
	if (pos->pieces[A7] == wB
	&&  pos->pieces[B6] == bP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[H7] == wB
	&&  pos->pieces[G6] == bP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[B8] == wB
	&&  pos->pieces[C7] == bP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[G8] == wB
	&&  pos->pieces[F7] == bP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[A6] == wB
	&&  pos->pieces[B5] == bP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A6;
	}

	if (pos->pieces[H6] == wB
	&&  pos->pieces[G5] == bP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A6;
	}

	 // bishop on initial square supporting castled king
	if (pos->pieces[F1] == wB
	&&  pos->pieces[G1] == wK) {
	    ei->positionalThemes[side] += RETURNING_BISHOP;
	}

	if (pos->pieces[C1] == wB
	&&  pos->pieces[B1] == wK) {
	    ei->positionalThemes[side] += RETURNING_BISHOP;
	}

    // uncastled king blocking own rook
	if ( (pos->pieces[F1] == wK || pos->pieces[G1] == wK)
	&&  (pos->pieces[H1] == wR || pos->pieces[G1] == wR)) {
	//&& (pos->pieces[F2] == wP || pos->pieces[G2] == wP || pos->pieces[H2] == wP)) {
	   	ei->blockages[side] -= P_KING_BLOCKS_ROOK;
	}

	if ( (pos->pieces[C1] == wK || pos->pieces[B1] == wK)
	&&  (pos->pieces[A1] == wR || pos->pieces[B1] == wR)) {
	//&& (pos->pieces[F2] == wP || pos->pieces[G2] == wP || pos->pieces[H2] == wP)) {
	   	ei->blockages[side] -= P_KING_BLOCKS_ROOK;
	}
}

void blockedPiecesB(const S_BOARD *pos) {

	int side = BLACK;

    // central pawn blocked, bishop hard to develop
    if (pos->pieces[C8] == bB
	&&  pos->pieces[D7] == bP
	&& pos->pieces[D6] != EMPTY) {
       ei->blockages[side] -= P_BLOCK_CENTRAL_PAWN;
	}

	if (pos->pieces[F8] == bB
	&& pos->pieces[E7] == bP
	&& pos->pieces[E6] != EMPTY) {
	   ei->blockages[side] -= P_BLOCK_CENTRAL_PAWN;
	}

	// trapped knight
	if ( pos->pieces[A1] == bN
	&& ( pos->pieces[A2] == wP || pos->pieces[C2] == wP)) {
		ei->blockages[side] -= P_KNIGHT_TRAPPED_A8;
	}

	if ( pos->pieces[H1] == bN
	&& ( pos->pieces[H2] == wP || pos->pieces[F2] == wP)) {
		ei->blockages[side] -= P_KNIGHT_TRAPPED_A8;
	}

	if (pos->pieces[A2] == bN
	&&  pos->pieces[A3] == wP
	&&  pos->pieces[B2] == wP) {
	    ei->blockages[side] -= P_KNIGHT_TRAPPED_A7;
	}

	if (pos->pieces[H2] == bN
	&&  pos->pieces[H3] == wP
	&&  pos->pieces[G2] == wP) {
	    ei->blockages[side] -= P_KNIGHT_TRAPPED_A7;
	}

	 // knight blocking queenside pawns
	if (pos->pieces[C6] == bN
	&&  pos->pieces[C7] == bP
	&& (pos->pieces[D5] == bP || pos->pieces[D7] == bP)
	&&  pos->pieces[E5] != bP) {
	    ei->blockages[side] -= P_C3_KNIGHT;
	}

	 // trapped bishop
	if (pos->pieces[A2] == bB
	&&  pos->pieces[B3] == wP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[H2] == bB
	&&  pos->pieces[G3] == wP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[B1] == bB
	&&  pos->pieces[C2] == wP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[G1] == bB
	&&  pos->pieces[F2] == wP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A7;
	}

	if (pos->pieces[A3] == bB
	&&  pos->pieces[B4] == wP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A6;
	}

	if (pos->pieces[H3] == bB
	&&  pos->pieces[G4] == wP) {
	    ei->blockages[side] -= P_BISHOP_TRAPPED_A6;
	}

	 // bishop on initial square supporting castled king
	if (pos->pieces[F8] == bB
	&&  pos->pieces[G8] == bK) {
	    ei->positionalThemes[side] += RETURNING_BISHOP;
	}

	if (pos->pieces[C8] == bB
	&&  pos->pieces[B8] == bK) {
	    ei->positionalThemes[side] += RETURNING_BISHOP;
	}

    // uncastled king blocking own rook
	if ((pos->pieces[F8] == bK || pos->pieces[G8] == bK)
 	&& (pos->pieces[H8] == bR || pos->pieces[G8] == bR)) {
	//&& (pos->pieces[F7] == bP || pos->pieces[G7] == bP || pos->pieces[H7] == bP)) {
	   	ei->blockages[side] -= P_KING_BLOCKS_ROOK;
	}

	if ((pos->pieces[C8] == bK || pos->pieces[B8] == bK)
	&& (pos->pieces[A8] == bR || pos->pieces[B8] == bR)) { 
	//&& (pos->pieces[F7] == bP || pos->pieces[G7] == bP || pos->pieces[H7] == bP)) {
	   	ei->blockages[side] -= P_KING_BLOCKS_ROOK;
	}
}

int EvalPosition(S_BOARD *pos) {

	//int startTime = GetTimeMs();

	ASSERT(CheckBoard(pos));

	int pce;
	int pceNum;
	int sq;
	int score = 0;
	int mobility;
	int KingSq;
	int pceNum2, pce2, sq2, QueenSq;
	int opposed, support, pawnbrothers, SupportRank, pawnRank;
	int phase;
	int mgScore = 0, egScore = 0;

	phase = 24 - 4 * (pos->pceNum[wQ] + pos->pceNum[bQ])
               - 2 * (pos->pceNum[wR] + pos->pceNum[bR])
               - 1 * (pos->pceNum[wN] + pos->pceNum[bN] + pos->pceNum[wB] + pos->pceNum[bB]);

    phase = (phase * 256 + 12) / 24;
			
	for(int side = WHITE; side <= BLACK; side++) {
		ei->mgMob[side] = 0;
		ei->egMob[side] = 0;
		ei->attCnt[side] = 0;
		ei->attckersCnt[side] = 0;
		ei->attWeight[side] = 0;
		ei->mgTropism[side] = 0;
		ei->egTropism[side] = 0;
		ei->adjustMaterial[side] = 0;
		ei->blockages[side] = 0;
		ei->positionalThemes[side] = 0;
		ei->pkeval[side] = 0;
		ei->kingShield[side] = 0;
		ei->PSQTMG[side] = 0;
		ei->PSQTEG[side] = 0;
		ei->pawnsMG[side] = 0;
		ei->pawnsEG[side] = 0;
		ei->KD[side] = 0;
		ei->KDE[side] = 0;
		ei->kingAreas[side] = kingAreaMasks(side, SQ64(pos->KingSq[side]));
	} 

	/*if(!pos->pceNum[wP] && !pos->pceNum[bP] && MaterialDraw(pos) == TRUE) {
		return 0;
	}*/

	pieces(pos);
	EvaluateKings(pos);

	mgScore = pos->material[WHITE] + pos->pcsq_mg[WHITE] - pos->material[BLACK] - pos->pcsq_mg[BLACK];
	egScore = pos->materialeg[WHITE] + pos->pcsq_eg[WHITE] - pos->materialeg[BLACK] - pos->pcsq_eg[BLACK];

	//ei->kingShield[WHITE] = wKingShield(pos);
    //ei->kingShield[BLACK] = bKingShield(pos);
    blockedPiecesW(pos);
    blockedPiecesB(pos);

	//mgScore += (ei->kingShield[WHITE] - ei->kingShield[BLACK]);
	mgScore += (ei->pkeval[WHITE] - ei->pkeval[BLACK]);

	mgScore += (ei->mgMob[WHITE] - ei->mgMob[BLACK]);
    egScore += (ei->egMob[WHITE] - ei->egMob[BLACK]);
	mgScore += (ei->mgTropism[WHITE] - ei->mgTropism[BLACK]);
	egScore += (ei->egTropism[WHITE] - ei->egTropism[BLACK]);
	mgScore += (ei->pawnsMG[WHITE] - ei->pawnsMG[BLACK]);
	egScore += (ei->pawnsEG[WHITE] - ei->pawnsEG[BLACK]);
    mgScore += (ei->KD[WHITE] - ei->KD[BLACK]);
    egScore += (ei->KDE[WHITE] - ei->KDE[BLACK]);

	if(pos->pceNum[wB] > 1) ei->adjustMaterial[WHITE] += BishopPair;
	if(pos->pceNum[bB] > 1) ei->adjustMaterial[BLACK] += BishopPair;
	if(pos->pceNum[wN] > 1) ei->adjustMaterial[WHITE] -= KnightPair;
	if(pos->pceNum[bN] > 1) ei->adjustMaterial[BLACK] -= KnightPair;
	ei->adjustMaterial[WHITE] += n_adj[pos->pceNum[wP]] * pos->pceNum[wN];
	ei->adjustMaterial[BLACK] += n_adj[pos->pceNum[bP]] * pos->pceNum[bN];
	ei->adjustMaterial[WHITE] += r_adj[pos->pceNum[wP]] * pos->pceNum[wR];
	ei->adjustMaterial[BLACK] += r_adj[pos->pceNum[bP]] * pos->pceNum[bR];

	int factor = evaluateScaleFactor(pos);

	score = (mgScore * (256 - phase) + egScore * phase * factor / SCALE_NORMAL) / 256;

	score += (ei->blockages[WHITE] - ei->blockages[BLACK]);
    score += (ei->positionalThemes[WHITE] - ei->positionalThemes[BLACK]);
	score += (ei->adjustMaterial[WHITE] - ei->adjustMaterial[BLACK]);

	pos->side == WHITE ? score += TEMPO : score -= TEMPO;

	int stronger,weaker;
	if (score > 0) {
        stronger = WHITE;
        weaker = BLACK;
    } else {
        stronger = BLACK;
        weaker = WHITE;
    }

    int PAWNST, PAWNWK;

    stronger == WHITE ? PAWNST = wP : PAWNST = bP;
    weaker == WHITE ? PAWNWK = wP : PAWNWK = bP;

    if (!pos->pceNum[PAWNST]) {

        if (pos->material[stronger] < 500) return 0;

        if (!pos->pceNum[PAWNWK]
                && (pos->material[stronger] == 2 * PieceVal[wN]))
            return 0;

        if (pos->material[stronger] == PieceVal[wR]
                && pos->material[weaker] == PieceVal[wB]) return 0;

        if (pos->material[stronger] == PieceVal[wR]
                && pos->material[weaker] == PieceVal[wN]) return 0;

        if (pos->material[stronger] == PieceVal[wR] + PieceVal[wB]
                && pos->material[weaker] == PieceVal[wR]) return 0;

        if (pos->material[stronger] == PieceVal[wR] + PieceVal[wN]
                && pos->material[weaker] == PieceVal[wR]) return 0;

        if (pos->material[stronger] == PieceVal[wR]
                && pos->material[weaker] == PieceVal[wR]) return 0;

        if (pos->material[stronger] == PieceVal[wQ]
                && pos->material[weaker] == PieceVal[wQ]) return 0;

        if (pos->material[stronger] == ( PieceVal[wB] || PieceVal[wN] )
                && pos->material[weaker] == ( PieceVal[wB] || PieceVal[wN])) return 0;
    }
	
	//printf("Elapsed %d\n",GetTimeMs() - startTime);

	if(pos->side == WHITE) {
		return score;
	} else {
		return -score;
	}	
}

void printEvalFactor(int wh, int bl) {
    printf("white %4d, black %4d, total: %4d \n", wh, bl, wh - bl);
}

void printEval(S_BOARD *pos) {
    printf("------------------------------------------\n");
    printf("Total value (for side to move): %d \n", EvalPosition(pos) );
    printf("Material balance       : %d \n", pos->material[WHITE] - pos->material[BLACK] );
    printf("Material adjustement   : ");
	printEvalFactor(ei->adjustMaterial[WHITE], ei->adjustMaterial[BLACK]);
    /*printf("Mg Piece/square tables : ");
    printEvalFactor(ei->PSQTMG[WHITE], ei->PSQTMG[BLACK]);
    printf("Eg Piece/square tables : ");
    printEvalFactor(ei->PSQTEG[WHITE], ei->PSQTEG[BLACK]);*/
    printf("Mg PSQT                : ");
    printEvalFactor(pos->pcsq_mg[WHITE], pos->pcsq_mg[BLACK]);
    printf("Eg PSQT                : ");
    printEvalFactor(pos->pcsq_eg[WHITE], pos->pcsq_eg[BLACK]);
    printf("Mg Mobility            : ");
    printEvalFactor(ei->mgMob[WHITE], ei->mgMob[BLACK]);
    printf("Eg Mobility            : ");
    printEvalFactor(ei->egMob[WHITE], ei->egMob[BLACK]);
    printf("Mg Tropism             : ");
    printEvalFactor(ei->mgTropism[WHITE], ei->mgTropism[BLACK]);
    printf("Eg Tropism             : ");
    printEvalFactor(ei->egTropism[WHITE], ei->egTropism[BLACK]);
    printf("Pawn structure Mg      : ");
    printEvalFactor(ei->pawnsMG[WHITE], ei->pawnsMG[BLACK]);
    printf("Pawn structure Eg      : ");
    printEvalFactor(ei->pawnsEG[WHITE], ei->pawnsEG[BLACK]);
    printf("Blockages              : ");
    printEvalFactor(ei->blockages[WHITE], ei->blockages[BLACK]);
    printf("Positional themes      : ");
    printEvalFactor(ei->positionalThemes[WHITE], ei->positionalThemes[BLACK]);
    /*printf("King Safety            : ");
    printEvalFactor(SafetyTable[ei->attWeight[WHITE]], SafetyTable[ei->attWeight[BLACK]]);*/
    printf("King Safety Mg         : ");
  	printEvalFactor(ei->KD[WHITE], ei->KD[BLACK]);
  	printf("King Safety Eg         : ");
  	printEvalFactor(ei->KDE[WHITE], ei->KDE[BLACK]);
    printf("King Shield            : ");
    printEvalFactor(ei->kingShield[WHITE], ei->kingShield[BLACK]);
    printf("Pawn King Eval         : ");
    printEvalFactor(ei->pkeval[WHITE], ei->pkeval[BLACK]);
    printf("Tempo                  : ");
    if ( pos->side == WHITE ) printf("%d", TEMPO);
    else printf("%d", -TEMPO);
    printf("\n");
    //pieces(pos);
    printf("------------------------------------------\n");
    //PrintNonBits(pos, WHITE);
    printf("\n");
    //PrintNonBits(pos, BLACK);
}
