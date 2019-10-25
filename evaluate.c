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

int mirrorFile(int file) {
    static const int Mirror[] = {0,1,2,3,3,2,1,0};
    ASSERT(0 <= file && file < FILE_NONE);
    return Mirror[file];
}

int getTropism(int sq1, int sq2) {
	return 7 - (abs(FilesBrd[(sq1)] - FilesBrd[(sq2)]) + abs(RanksBrd[(sq1)] - RanksBrd[(sq2)]));
}

int king_proximity(int c, int s, const S_BOARD *pos) {
    return MIN(SquareDistance[pos->KingSq[c]][s], 5);
};

int map_to_queenside(int f) {
	return MIN(f, FILE_H - f);
}

int isPiece(int color, int piece, int sq, const S_BOARD *pos) {
    return ( (pos->pieces[sq] == piece) && (PieceCol[pos->pieces[sq]] == color) );
}

int REL_SQ(int sq120, int side) {
	return side == WHITE ? sq120 : Mirror120[SQ64(sq120)];
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

void EvaluatePawns(S_BOARD *pos) {

	int pce;
	int pceNum;
	int sq;
	int score;
	int mobility;
	int KingSq;
	int pceNum2, pce2, sq2, QueenSq;
	int opposed, support, pawnbrothers, connected;
	int strong, weak;
	int flag;
	int side;
	int blockSq, w, i;
	int mgScore, egScore;
	int bonus;
	int dir, t_sq, index;
	int att[10];
	for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}

	pce = wP;
	side = WHITE;

	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ei->PSQTMG[WHITE] += PawnTable[SQ64(sq)];
		ei->PSQTEG[WHITE] += PawnTable[SQ64(sq)];

    	for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			if(!SQOFFBOARD(t_sq)) {
				if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
					att[pceNum]++;
				}    
			}
		}
		
		if(att[pceNum]) {
    		ei->attCnt[side] += att[pceNum];
    		ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];		
    	}
    	//printf(" attCnt WHITE %d\n", ei->attCnt[side]);
    	//printf(" attckersCnt WHITE %d\n", ei->attckersCnt[side]);
    	//printf(" attWeight WHITE %d\n", ei->attWeight[side]);

		opposed = pos->pieces[sq+10] == bP;

		if( (IsolatedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("wP Iso:%s\n",PrSq(sq));
			ei->pawnsMG[side] -= PawnIsolated + WeakUnopposed * !opposed;
			ei->pawnsEG[side] -= PawnIsolatedEG + WeakUnopposedEG * !opposed;
		}

		if ( !(BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE])
            && SqAttackedByPawn(sq + 10,side^1,pos)) {
			//printf("wP BackW:%s\n",PrSq(sq));
            flag = !(FileBBMask[FilesBrd[sq]] & pos->pawns[BLACK]);
            ei->pawnsMG[side] += PawnBackwards[flag];
            ei->pawnsEG[side] += PawnBackwardsEG[flag];
        }
		
		support = ( pos->pieces[sq-9] == wP || pos->pieces[sq-11] == wP);
		pawnbrothers = (pos->pieces[sq-1] == wP || pos->pieces[sq+1] == wP);

		if( (WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("wP Passed:%s\n",PrSq(sq));
			ei->pawnsMG[side] += PawnPassed[RanksBrd[sq]];
			ei->pawnsEG[side] += PawnPassedEG[RanksBrd[sq]];

			if(support || pawnbrothers) {
				ei->pawnsMG[side] += Connected[RanksBrd[sq]];
				ei->pawnsEG[side] += Connected[RanksBrd[sq]];
			} 
			/*if(RanksBrd[sq] > RANK_3) {

				blockSq = sq + 10;
				w = 2 * RanksBrd[sq] - 5;

				bonus = ((  king_proximity(side^1, blockSq, pos) * 2
                          - king_proximity(side,   blockSq, pos) * 1) * w);
           		//printf("w %d\n",w );
            	//printf("bonus %d\n",bonus );
			
            	if (RanksBrd[sq] != RANK_7) {
                	bonus -= ( king_proximity(side, blockSq + 10, pos) * w);
            		//printf("new bonus %d\n",bonus );
            	}
            	ei->pawnsEG[side] += bonus;//bonus -= 5 * map_to_queenside(FilesBrd[sq]);
			}*/    
		}

		if(pos->pieces[sq-10] == wP) {
			ei->pawnsMG[side] -= Doubled;
			ei->pawnsEG[side] -= DoubledEG;
		}
		if(pos->pieces[sq-20] == wP) {
			ei->pawnsMG[side] -= Doubled;
			ei->pawnsEG[side] -= DoubledEG;
		}
		if(pos->pieces[sq-30] == wP) {
			ei->pawnsMG[side] -= Doubled;
			ei->pawnsEG[side] -= DoubledEG;
		}
	}	

	pce = bP;
	side = BLACK;	
	bonus = 0;
	for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);	
		ei->PSQTMG[BLACK] += PawnTable[MIRROR64(SQ64(sq))];
		ei->PSQTEG[BLACK] += PawnTable[MIRROR64(SQ64(sq))];

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			if(!SQOFFBOARD(t_sq)) {
				if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
					att[pceNum]++;
				}    
			}
		}
		
		if(att[pceNum]) {
    		ei->attCnt[side] += att[pceNum];
    		ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}
    	//printf(" attCnt BLACK %d\n", ei->attCnt[side]);
    	//printf(" attckersCnt BLACK %d\n", ei->attckersCnt[side]);
    	//printf(" attWeight BLACK %d\n", ei->attWeight[side]);

		opposed = pos->pieces[sq-10] == wP;

		if( (IsolatedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("bP Iso:%s\n",PrSq(sq));
			ei->pawnsMG[side] -= PawnIsolated + WeakUnopposed * !opposed;
			ei->pawnsEG[side] -= PawnIsolatedEG + WeakUnopposedEG * !opposed;	
		}

		if ( !(WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK])
            && SqAttackedByPawn(sq - 10,side^1,pos)) {
			//printf("bP BackW:%s\n",PrSq(sq));
            flag = !(FileBBMask[FilesBrd[sq]] & pos->pawns[WHITE]);
            ei->pawnsMG[side] += PawnBackwards[flag];
            ei->pawnsEG[side] += PawnBackwardsEG[flag];
        }

		support = (pos->pieces[sq+9] == bP || pos->pieces[sq+11] == bP);
		pawnbrothers = (pos->pieces[sq-1] == bP || pos->pieces[sq+1] == bP);
		
		if( (BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("bP Passed:%s\n",PrSq(sq));
			ei->pawnsMG[side] += PawnPassed[7 - RanksBrd[sq]];
			ei->pawnsEG[side] += PawnPassedEG[7 - RanksBrd[sq]];

			if(support || pawnbrothers) {
				ei->pawnsMG[side] += Connected[7 - RanksBrd[sq]];
				ei->pawnsEG[side] += Connected[7 - RanksBrd[sq]];
			} 
			/*if(RanksBrd[sq] < RANK_6) {
				blockSq = sq - 10;
				i = 2 * RanksBrd[sq] - 5;

				bonus = ((  king_proximity(side^1, blockSq, pos) * 2
                      	  - king_proximity(side,   blockSq, pos) * 1) * i);

	            if (RanksBrd[sq] != RANK_2) {
	                bonus -= ( king_proximity(side, blockSq - 10, pos) * i);
	                //printf("new bonus %d\n",bonus );
	            }
	            //bonus -= 5 * map_to_queenside(FilesBrd[sq]);
            	ei->pawnsEG[side] += bonus;
			} */
		}

		if(pos->pieces[sq+10] == bP) {
			ei->pawnsMG[side] -= Doubled;
			ei->pawnsEG[side] -= DoubledEG;
		}
		if(pos->pieces[sq+20] == bP) {
			ei->pawnsMG[side] -= Doubled;
			ei->pawnsEG[side] -= DoubledEG;
		}
		if(pos->pieces[sq+30] == bP) {
			ei->pawnsMG[side] -= Doubled;
			ei->pawnsEG[side] -= DoubledEG;
		}
	}
}

void EvaluateKnights(const S_BOARD *pos) {
	int pce;
	int pceNum;
	int sq;
	int score;
	
	int KingSq;
	int pceNum2, pce2, sq2, QueenSq;
	int opposed, support, pawnbrothers;
	int strong, weak;
	int index;
	int MobilityCount = 0;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	int KingSqNum, side;

	int att[10];
	int mobility = 0;
	int tropism;
	for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}
	
	pce = wN;	
	side = WHITE;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ei->PSQTMG[side] += KnightTable[SQ64(sq)];
		ei->PSQTEG[side] += KnightTable[SQ64(sq)];

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			if(!SQOFFBOARD(t_sq) && PieceCol[pos->pieces[t_sq]] != side) {

				if(!SqAttackedByPawn(t_sq,side^1,pos)) {
					mobility++;
				}
				if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
					att[pceNum]++;
				}    
			}
		}

    	tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 3 * tropism;
		ei->egTropism[side] += 3 * tropism;	
		ei->mgMob[side] += 4 * (mobility-4);
    	ei->egMob[side] += 4 * (mobility-4);

		if(att[pceNum]) {
    		ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}
		
		/*if(pos->pieces[sq+10] == bP) {
			ei->positionalThemes[side] += 6;
		}
		if((pos->pieces[sq-9] == wP || pos->pieces[sq-11] == wP) && RankBBMask[RanksBrd[sq]] >= RANK_4) {
			ei->positionalThemes[side] += 6;
		}
		if((pos->pieces[sq-9] == wP || pos->pieces[sq-11] == wP) && RankBBMask[RanksBrd[sq]] >= RANK_6) {
			ei->positionalThemes[side] += 12;
		}*/
	}	

	pce = bN;
	side = BLACK;
	mobility = 0;	
    for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		ei->PSQTMG[side] += KnightTable[MIRROR64(SQ64(sq))];
		ei->PSQTEG[side] += KnightTable[MIRROR64(SQ64(sq))];

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			if(!SQOFFBOARD(t_sq) && PieceCol[pos->pieces[t_sq]] != side) {

				if(!SqAttackedByPawn(t_sq,side^1,pos)) {
					mobility++;
				}
				if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
					att[pceNum]++;
				}    
			}
		}

		tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 3 * tropism;
		ei->egTropism[side] += 3 * tropism;	
		ei->mgMob[side] += 4 * (mobility-4);
    	ei->egMob[side] += 4 * (mobility-4);

		if(att[pceNum]) {
    		ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}
		
		/*if(pos->pieces[sq-10] == wP) {
			ei->positionalThemes[side] += 6;
		}
		if((pos->pieces[sq+9] == bP || pos->pieces[sq+11] == bP) && RankBBMask[RanksBrd[sq]] <= RANK_5) {
			ei->positionalThemes[side] += 6;
		}
		if((pos->pieces[sq+9] == bP || pos->pieces[sq+11] == bP) && RankBBMask[RanksBrd[sq]] <= RANK_3) {
			ei->positionalThemes[side] += 12;
		}*/
	}	
}

void EvaluateBishops(const S_BOARD *pos) {
	int pce;
	int pceNum;
	int sq;
	int score;
	
	int KingSq;
	int pceNum2, pce2, sq2, QueenSq;
	int opposed, support, pawnbrothers;
	int strong, weak;
	int index;
	int MobilityCount = 0;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	int KingSqNum, side;

	int att[10];
	int mobility = 0;
	int tropism;
	for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}

	pce = wB;
	side = WHITE;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ei->PSQTMG[side] += BishopTable[SQ64(sq)];
		ei->PSQTEG[side] += BishopTable[SQ64(sq)];

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			while(!SQOFFBOARD(t_sq)) {

				if(pos->pieces[t_sq] == EMPTY) {
					if(!SqAttackedByPawn(t_sq,side^1,pos)) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att[pceNum]++;
					}
				} else { // non empty sq
					if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
						mobility++;
						if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
							att[pceNum]++;
						}
					}
					break;
				} 
				t_sq += dir;   
			}
		}

		tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 2 * tropism;
		ei->egTropism[side] += 1 * tropism;	
		ei->mgMob[side] += 3 * (mobility-7);
    	ei->egMob[side] += 3 * (mobility-7);

    	if(att[pceNum]) {
    		ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}

		/*if((pos->pieces[sq-9] == wP || pos->pieces[sq-11] == wP) && RankBBMask[RanksBrd[sq]] >= RANK_4) {
			ei->positionalThemes[side] += 4;
		}
		if((pos->pieces[sq-9] == wP || pos->pieces[sq-11] == wP) && RankBBMask[RanksBrd[sq]] >= RANK_6) {
			ei->positionalThemes[side] += 8;
		}*/		
	}	

	pce = bB;
	side = BLACK;	
    for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}
	mobility = 0;

	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		ei->PSQTMG[side] += BishopTable[MIRROR64(SQ64(sq))];
		ei->PSQTEG[side] += BishopTable[MIRROR64(SQ64(sq))];

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			while(!SQOFFBOARD(t_sq)) {

				if(pos->pieces[t_sq] == EMPTY) {
					if(!SqAttackedByPawn(t_sq,side^1,pos)) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att[pceNum]++;
					}
				} else { // non empty sq
					if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
						mobility++;
						if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
							att[pceNum]++;
						}
					}
					break;
				} 
				t_sq += dir;   
			}
		}

    	tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 2 * tropism;
		ei->egTropism[side] += 1 * tropism;	
		ei->mgMob[side] += 3 * (mobility-7);
    	ei->egMob[side] += 3 * (mobility-7);

    	if (att[pceNum]) {
			ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}

		/*if((pos->pieces[sq+9] == bP || pos->pieces[sq-11] == bP) && RankBBMask[RanksBrd[sq]] <= RANK_5) {
			ei->positionalThemes[side] += 4;
		}
		if((pos->pieces[sq+9] == bP || pos->pieces[sq-11] == bP) && RankBBMask[RanksBrd[sq]] <= RANK_3) {
			ei->positionalThemes[side] += 8;
		}*/
	}
}

void EvaluateRooks(const S_BOARD *pos) {
	int pce;
	int pceNum;
	int sq;
	int score;
	
	int KingSq;
	int pceNum2, pce2, sq2, QueenSq;
	int opposed, support, pawnbrothers;
	int strong, weak;
	int index;
	int MobilityCount = 0;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	int KingSqNum, side;

	int att[10];
	int mobility = 0;
	int tropism;
	for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}

	pce = wR;
	side = WHITE;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		
		ei->PSQTMG[side] += RookTable[SQ64(sq)];
		ei->PSQTEG[side] += RookTable[SQ64(sq)];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += RookOpenFile;
    		ei->egMob[side] += RookOpenFile;
    		if (abs(RanksBrd[sq] - RanksBrd[pos->KingSq[side^1]] ) < 2) 
			   ei->attWeight[side] += 1;
		}
		if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += RookSemiOpenFile;
    		ei->egMob[side] += RookSemiOpenFile;
    		if (abs(RanksBrd[sq] - RanksBrd[pos->KingSq[side^1]] ) < 2) 
			   ei->attWeight[side] += 2;
		}

		if((RankBBMask[RanksBrd[sq]] == RANK_7 && RankBBMask[RanksBrd[pos->KingSq[side^1]]] == RANK_8)) {
			ei->mgMob[side] += 20;
			ei->egMob[side] += 30;
		}

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			while(!SQOFFBOARD(t_sq)) {

				if(pos->pieces[t_sq] == EMPTY) {
					if(!SqAttackedByPawn(t_sq,side^1,pos)) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att[pceNum]++;
					}
				} else { // non empty sq
					if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
						mobility++;
						if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
							att[pceNum]++;
						}
					}
					break;
				} 
				t_sq += dir;   
			}
		}

		tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 2 * tropism;
		ei->egTropism[side] += 1 * tropism;	
		ei->mgMob[side] += 2 * (mobility-7);
    	ei->egMob[side] += 4 * (mobility-7);

		if (att[pceNum]) {
			ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}
	}

    for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}
	mobility = 0;

	pce = bR;
	side = BLACK;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		ei->PSQTMG[side] += RookTable[MIRROR64(SQ64(sq))];
		ei->PSQTEG[side] += RookTable[MIRROR64(SQ64(sq))];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += RookOpenFile;
    		ei->egMob[side] += RookOpenFile;
    		if (abs(RanksBrd[sq] - RanksBrd[pos->KingSq[side^1]] ) < 2) 
			   ei->attWeight[side] += 1;
		}
		if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += RookSemiOpenFile;
    		ei->egMob[side] += RookSemiOpenFile;
    		if (abs(RanksBrd[sq] - RanksBrd[pos->KingSq[side^1]] ) < 2) 
			   ei->attWeight[side] += 2;
		}

		if((RankBBMask[RanksBrd[sq]] == RANK_2 && RankBBMask[RanksBrd[pos->KingSq[side^1]]] == RANK_1)) {
			ei->mgMob[side] += 20;
			ei->egMob[side] += 30;
		}

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			while(!SQOFFBOARD(t_sq)) {

				if(pos->pieces[t_sq] == EMPTY) {
					if(!SqAttackedByPawn(t_sq,side^1,pos)) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att[pceNum]++;
					}
				} else { // non empty sq
					if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
						mobility++;
						if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
							att[pceNum]++;
						}
					}
					break;
				} 
				t_sq += dir;   
			}
		}

		tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 2 * tropism;
		ei->egTropism[side] += 1 * tropism;	
		ei->mgMob[side] += 2 * (mobility-7);
    	ei->egMob[side] += 4 * (mobility-7);

		if (att[pceNum]) {
			ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}	
	}
}

void EvaluateQueens(const S_BOARD *pos) {
	int pce;
	int pceNum;
	int sq;
	int score;
	
	int KingSq;
	int pceNum2, pce2, sq2, QueenSq;
	int opposed, support, pawnbrothers;
	int strong, weak;
	int index;
	int MobilityCount = 0;
	int t_sq;
	int SqAttackedbyThem;
	int dir;
	int KingSqNum, side;

	int att[10];
	int mobility = 0;
	int tropism;
	for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}

	int KNIGHT, BISHOP;

	pce = wQ;
	side = WHITE;

	BISHOP = wB;
    KNIGHT = wN;

	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		
		ei->PSQTMG[WHITE] += QueenTable[SQ64(sq)];
		ei->PSQTEG[WHITE] += QueenTable[SQ64(sq)];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += QueenOpenFile;
    		ei->egMob[side] += QueenOpenFile;
		}
		if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += QueenSemiOpenFile;
    		ei->egMob[side] += QueenSemiOpenFile;
		}

		if((RankBBMask[RanksBrd[sq]] == RANK_7 && RankBBMask[RanksBrd[pos->KingSq[side^1]]] == RANK_8)) {
			ei->mgMob[side] += 5;
			ei->egMob[side] += 10;
		}

		if (RanksBrd[sq] > RANK_2) {
			if(isPiece(side, KNIGHT, B1,pos)) ei->positionalThemes[side] -= 2;
			if(isPiece(side, BISHOP, C1,pos)) ei->positionalThemes[side] -= 2;
			if(isPiece(side, BISHOP, F1,pos)) ei->positionalThemes[side] -= 2;
			if(isPiece(side, KNIGHT, G1,pos)) ei->positionalThemes[side] -= 2;
		}

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			while(!SQOFFBOARD(t_sq)) {

				if(pos->pieces[t_sq] == EMPTY) {
					if(!SqAttackedByPawn(t_sq,side^1,pos)) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att[pceNum]++;
					}
				} else { // non empty sq
					if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
						mobility++;
						if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
							att[pceNum]++;
						}
					}
					break;
				} 
				t_sq += dir;   
			}
		}

		tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 2 * tropism;
		ei->egTropism[side] += 4 * tropism;	
		ei->mgMob[side] += 1 * (mobility-14);
    	ei->egMob[side] += 2 * (mobility-14);

		if (att[pceNum]) {
			ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}
	}

	for(int i = 0; i < 10; ++i) {
		att[i] = 0;
	}
	mobility = 0;

	pce = bQ;
	side = BLACK;
	BISHOP = bB;
    KNIGHT = bN;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		ei->PSQTMG[BLACK] += QueenTable[MIRROR64(SQ64(sq))];
		ei->PSQTEG[BLACK] += QueenTable[MIRROR64(SQ64(sq))];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += QueenOpenFile;
    		ei->egMob[side] += QueenOpenFile;
		}
		if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			ei->mgMob[side] += QueenSemiOpenFile;
    		ei->egMob[side] += QueenSemiOpenFile;
		}

		if((RankBBMask[RanksBrd[sq]] == RANK_2 && RankBBMask[RanksBrd[pos->KingSq[side^1]]] == RANK_1)) {
			ei->mgMob[side] += 5;
			ei->egMob[side] += 10;
		}
		
		if (RanksBrd[sq] < RANK_7) {
			if(isPiece(side, KNIGHT, B8,pos)) ei->positionalThemes[side] -= 2;
			if(isPiece(side, BISHOP, C8,pos)) ei->positionalThemes[side] -= 2;
			if(isPiece(side, BISHOP, F8,pos)) ei->positionalThemes[side] -= 2;
			if(isPiece(side, KNIGHT, G8,pos)) ei->positionalThemes[side] -= 2;
		}

		for(index = 0; index < NumDir[pce]; ++index) {
			dir = PceDir[pce][index];
			t_sq = sq + dir;
			while(!SQOFFBOARD(t_sq)) {

				if(pos->pieces[t_sq] == EMPTY) {
					if(!SqAttackedByPawn(t_sq,side^1,pos)) {
						mobility++;
					}
					if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
						att[pceNum]++;
					}
				} else { // non empty sq
					if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
						mobility++;
						if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
							att[pceNum]++;
						}
					}
					break;
				} 
				t_sq += dir;   
			}
		}

		tropism = getTropism(sq, pos->KingSq[side^1]);
		ei->mgTropism[side] += 2 * tropism;
		ei->egTropism[side] += 4 * tropism;	
		ei->mgMob[side] += 1 * (mobility-14);
    	ei->egMob[side] += 2 * (mobility-14);

		if (att[pceNum]) {
			ei->attCnt[side] += att[pceNum];
            ei->attckersCnt[side] += 1;
            ei->attWeight[side] += Weight[pce];
    	}	
	}
}

void EvaluateKings(const S_BOARD *pos) {
	int score = 0;
	int pce;
	int pceNum2, sq2;
	int sq;
	int ours, ourDist, theirs, theirDist, blocked;
	int PAWN, PAWNOPPO;
	int side;
	int count;

   	//PrintBitBoard(ei->kingAreas[WHITE]);
	//PrintBitBoard(ei->kingAreas[BLACK]);
	if (ei->attckersCnt[BLACK] > 1 - pos->pceNum[bQ]) {
	
		float scaledAttackCounts = 9.0 * ei->attCnt[BLACK] / CountBits(ei->kingAreas[WHITE]) + 1;
		count = ei->attckersCnt[BLACK] * ei->attWeight[BLACK];
		count += 44 * scaledAttackCounts
			  + -22 * CountBits(pos->pawns[WHITE] & ei->kingAreas[WHITE]);

		//printf("BLACK attacks %d attckersCnt %d attWeight %d \n", ei->attCnt[BLACK], ei->attckersCnt[BLACK], ei->attWeight[BLACK]);
		if(count > 0) {
			//printf("count %d\n", count);
			ei->KD[BLACK] += count * count / 720;
			ei->KDE[BLACK] += count / 20;
		}
	}
    if (ei->attckersCnt[WHITE] > 1 - pos->pceNum[wQ]) {
    
    	float scaledAttackCounts = 9.0 * ei->attCnt[WHITE] / CountBits(ei->kingAreas[BLACK]) + 1;
		count = ei->attckersCnt[WHITE] * ei->attWeight[WHITE];
		//printf(" WHITE attacks %d attckersCnt %d attWeight %d \n", ei->attCnt[WHITE], ei->attckersCnt[WHITE], ei->attWeight[WHITE]);
		count += 44 * scaledAttackCounts
		      + -22 * CountBits(pos->pawns[BLACK] & ei->kingAreas[BLACK]);
		if(count > 0) {
			//printf("count2 %d\n",count );
			ei->KD[WHITE] += count * count / 720;
			ei->KDE[WHITE] += count / 20;
		}
    }

	pce = wK;
	side = WHITE;
	sq = pos->pList[pce][0];
	ei->PSQTMG[WHITE] += KingO[SQ64(sq)];
	ei->PSQTEG[WHITE] += KingE[SQ64(sq)];
 
	for (int file = MAX(FILE_A, FilesBrd[pos->KingSq[side]] - 1); file <= MIN(FILE_H, FilesBrd[pos->KingSq[side]] + 1); file++) {

		ours = pos->pawns[side] & FileBBMask[FilesBrd[file]];
        ourDist = !ours ? 7 : abs(RanksBrd[pos->KingSq[side]] - RanksBrd[ours]);

        theirs = pos->pawns[side^1] & FileBBMask[FilesBrd[file]];
        theirDist = !theirs ? 7 : abs(RanksBrd[pos->KingSq[side]] - RanksBrd[theirs]);
        
        ei->pkeval[side] += KingShelter[ file == FilesBrd[pos->KingSq[side]] ][file][ourDist];

        blocked = (ourDist != 7 && (ourDist == theirDist - 1));
        ei->pkeval[side] += KingStorm[blocked][mirrorFile(file)][theirDist];
    }

	pce = bK;
	side = BLACK;
	sq = pos->pList[pce][0];
	ei->PSQTMG[BLACK] += KingO[MIRROR64(SQ64(sq))];
	ei->PSQTEG[BLACK] += KingE[MIRROR64(SQ64(sq))];

	for (int file = MAX(FILE_A, FilesBrd[pos->KingSq[side]] - 1); file <= MIN(FILE_H, FilesBrd[pos->KingSq[side]] + 1); file++) {

		ours = pos->pawns[side] & FileBBMask[FilesBrd[file]];
        ourDist = !ours ? 7 : abs(RanksBrd[pos->KingSq[side]] - RanksBrd[ours]);

        theirs = pos->pawns[side^1] & FileBBMask[FilesBrd[file]];
        theirDist = !theirs ? 7 : abs(RanksBrd[pos->KingSq[side]] - RanksBrd[theirs]);
 
        ei->pkeval[side] += KingShelter[file == FilesBrd[pos->KingSq[side]]][file][ourDist];

        blocked = (ourDist != 7 && (ourDist == theirDist - 1));
        ei->pkeval[side] += KingStorm[blocked][mirrorFile(file)][theirDist];
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
	//&&  pos->pieces[D4] == wP
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
	//&&  pos->pieces[D5] == bP
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

	phase = 24 - 4 * pos->pceNum[wQ] + pos->pceNum[bQ]
               - 2 * pos->pceNum[wR] + pos->pceNum[bR]
               - 1 * pos->pceNum[wN] + pos->pceNum[bN] + pos->pceNum[wB] + pos->pceNum[bB];
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
	
	EvaluatePawns(pos);
	EvaluateKnights(pos);
	EvaluateBishops(pos);
	EvaluateRooks(pos);
	EvaluateQueens(pos);
	EvaluateKings(pos);

	mgScore = pos->material[WHITE] + ei->PSQTMG[WHITE] - pos->material[BLACK] - ei->PSQTMG[BLACK];
	egScore = pos->material[WHITE] + ei->PSQTEG[WHITE] - pos->material[BLACK] - ei->PSQTEG[BLACK];

	//ei->kingShield[WHITE] = wKingShield(pos);
    //ei->kingShield[BLACK] = bKingShield(pos);
    blockedPiecesW(pos);
    blockedPiecesB(pos);

	mgScore += (ei->kingShield[WHITE] - ei->kingShield[BLACK]);
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

	//score += SafetyTable[ei->attWeight[WHITE]];
    //score -= SafetyTable[ei->attWeight[BLACK]];

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
    printf("Mg Piece/square tables : ");
    printEvalFactor(ei->PSQTMG[WHITE], ei->PSQTMG[BLACK]);
    printf("Eg Piece/square tables : ");
    printEvalFactor(ei->PSQTEG[WHITE], ei->PSQTEG[BLACK]);
    /*printf("Mg Piece/square tables : ");
    printEvalFactor(pos->pcsq_mg[WHITE], pos->pcsq_mg[BLACK]);
    printf("Eg Piece/square tables : ");
    printEvalFactor(pos->pcsq_eg[BLACK], pos->pcsq_eg[BLACK]);*/
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
    printf("------------------------------------------\n");
}
