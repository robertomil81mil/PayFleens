// evaluate.c

#include "stdio.h"
#include "defs.h"

const int PawnIsolated = -20;
const int PawnPassed[8] = { 0, 10, 17, 15, 62, 168, 276, 0};
const int PawnPassedNoProtected[8] = { 0, 0, 5, 5, 30, 80, 130, 0};
const int RookOpenFile = 10;
const int RookSemiOpenFile = 5;
const int RookOnPawn = 10;
const int QueenOpenFile = 5;
const int QueenSemiOpenFile = 3;
const int QueenNonOpenFile = 5;
const int KingOpenFile = 20;
const int KingSemiOpenFile = 10;
const int BishopPair = 30;
const int Mobility[16] = { -30, -20, -10, -4, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24};
const int QueenCheck = 3;
const int RookCheck = 5;
const int BishopCheck = 6;  //  Q     R     B   KN|     5     4    4    3
const int KnightCheck = 7;  // 780  1080  635 790 rook 10, kn 7, q 7, b 6

const int PawnTable[64] = {
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,
10	,	10	,   -10	,	-10	,   -10	,   0	,	10	,	10	,
5	,	0	,	0	,	0	,	0	,	0	,	0	,	5	,
5	,	0	,	15	,	20	,	20	,	15	,	0	,	5	,
10	,	5	,	15	,	25	,	25	,	15	,	5	,	10	,
20	,	20	,	20	,	30	,	30	,	20	,	20	,	20	,
30	,	30	,	30	,	35	,	35	,	30	,	30	,	30	,
0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	
};

const int KnightTable[64] = {
-30	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-30	,
-6	,	0	,	0	,	5	,	5	,	0	,	0	,	-6	,
-4	,	5	,	10	,	10	,	10	,	10	,	5	,	-4  ,
0	,	5	,	10	,	20	,	20	,	10	,	5	,	0	,
0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
-4	,	10	,	15	,	20	,	20	,	15	,	10	,	-4	,
-6	,	5	,	10	,	15	,	15	,	10	,	5	,	-6	,
-30	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-30		
};

const int BishopTable[64] = {
-30	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-30	,
-4	,	10	,	5	,	10	,	10	,	5	,	10	,	-4	,
-4	,	5	,	10	,	15	,	15	,	10	,	5	,	-4	,
0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
-4	,	0	,	10	,	15	,	15	,	10	,	0	,	-4	,
-4	,	10	,	5	,	10	,	10	,	5	,	10	,	-4	,
-30	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-30	
};

const int RookTable[64] = {
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
0	,	5	,	5	,	10	,	10	,	5	,	5	,	0	,
25	,	35	,	35	,	25	,	35	,	35	,	35	,	25	,
0	,	0	,	5	,	10	,	10	,	5	,	0	,	0		
};

const int QueenTable[64] = {
-15	,	-5	,	-5	,	-5	,	0	,	-5	,	-5	,	-15	,
0	,	0	,	0	,	3	,	3	,	0	,	0	,	0	,
0	,	3	,	3	,	3	,	3	,	3	,	3	,	0	,
0	,	3	,	5	,	5	,	5	,	5	,	3	,	0	,
0	,	3	,	5	,	5	,	5	,	5	,	3	,	0	,
0	,	5	,	5	,	5	,	5	,	5	,	5	,	0	,
3	,	5	,	5	,	5	,	5	,	5	,	5	,	3	,
-10	,	0	,	0	,	0	,	0	,	0	,	0	,	-10		
};

const int KingE[64] = {	
	-50 ,	-10	,	-5	,	0	,	0	,	-5	,	-10	,	-50	,
	-10 ,	0	,	10	,	10	,	10	,	10	,	0	,	-10	,
	0	,	10	,	20	,	20	,	20	,	20	,	10	,	0	,
	0	,	10	,	20	,	40	,	40	,	20	,	10	,	0	,
	0	,	10	,	20	,	40	,	40	,	20	,	10	,	0	,
	0	,	10	,	20	,	20	,	20	,	20	,	10	,	0	,
	-10 ,	0	,	10	,	10	,	10	,	10	,	0	,	-10	,
	-50	,	-10	,	0	,	0	,	0	,	0	,	-10	,	-50	
};

const int KingO[64] = {	
	5	,	5	,	 0	,	-10	,	-10	,	-10	,	5	,	0	,
	5	,	5	,	-30	,	-30	,	-30	,	-30	,	5	,	5	,
	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,
	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70	,	-70		
};
// sjeng 11.2
//8/6R1/2k5/6P1/8/8/4nP2/6K1 w - - 1 41 
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

#define ENDGAME_MAT (1 * PieceVal[wR] + 2 * PieceVal[wN] + 2 * PieceVal[wP] + PieceVal[wK])

int EvalPosition(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	int pce;
	int pceNum;
	int sq;
	int score = pos->material[WHITE] - pos->material[BLACK];
	int mobility;
	int KingSq;
	//int SqCloseToKIngW = pos->KingSq[WHITE];
	// SqCloseToKIngB = pos->KingSq[BLACK];

	if(!pos->pceNum[wP] && !pos->pceNum[bP] && MaterialDraw(pos) == TRUE) {
		return 0;
	}
	
	pce = wP;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += PawnTable[SQ64(sq)];	
		
		if( (IsolatedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("wP Iso:%s\n",PrSq(sq));
			score += PawnIsolated;
		}
		
		if( (WhitePassedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("wP Passed:%s\n",PrSq(sq));
			score += PawnPassed[RanksBrd[sq]];
		}
		if( (WhitePassedMask[SQ64(sq)] && pos->pieces[sq-11] != wP && pos->pieces[sq-9] != wP & pos->pawns[BLACK]) == 0) {
			//printf("wP Passed:%s\n",PrSq(sq));
			score -= PawnPassedNoProtected[RanksBrd[sq]];
		}

		if(pos->pieces[sq-10] == wP) {
			score -= 10;
		}
		if(pos->pieces[sq-20] == wP) {
			score -= 10;
		}
		/*if(pos->pieces[sq+19] == bN && pos->pieces[sq+9] != bP && pos->pieces[sq+10] != bP || pos->pieces[sq+21] == bN && pos->pieces[sq+11] != bP && pos->pieces[sq+10] != bP) {
			score += 6;
		}
		if(RankBBMask[RanksBrd[sq]] == RANK_4) {
			score += 4;
		}
		if(RankBBMask[RanksBrd[sq]] == RANK_5) {
			score += 8;
		}
		if((RankBBMask[RanksBrd[sq]] >= RANK_5) && (pos->pieces[sq-11] == wP || pos->pieces[sq-9] == wP)) {
			score += 8;
		}
		if((RankBBMask[RanksBrd[sq]] >= RANK_5) && (pos->pieces[sq-11] == wP && pos->pieces[sq-9] == wP)) {
			score += 10;
		}
		if((RankBBMask[RanksBrd[sq]] >= RANK_5) && (pos->pieces[sq-11] != wP || pos->pieces[sq-9] != wP)) {
			score -= 8;
		}*/
			
		//int KingSqC = KingSqAttackedbyPawnW(pos);
		//if(KingSqC == TRUE) {
			//score += 10;
		//}
		
	}	

	pce = bP;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= PawnTable[MIRROR64(SQ64(sq))];	
		
		if( (IsolatedMask[SQ64(sq)] & pos->pawns[BLACK]) == 0) {
			//printf("bP Iso:%s\n",PrSq(sq));
			score -= PawnIsolated;
		}
		
		if( (BlackPassedMask[SQ64(sq)] & pos->pawns[WHITE]) == 0) {
			//printf("bP Passed:%s\n",PrSq(sq));
			score -= PawnPassed[7 - RanksBrd[sq]];
		}
		if( (BlackPassedMask[SQ64(sq)] && pos->pieces[sq+11] != bP && pos->pieces[sq+9] != bP & pos->pawns[WHITE]) == 0) {
			//printf("bP Passed:%s\n",PrSq(sq));
			score += PawnPassedNoProtected[7 - RanksBrd[sq]];
		}  

		if(pos->pieces[sq+10] == bP) {
			score += 10;
		}
		if(pos->pieces[sq+20] == bP) {
			score += 10;
		}
		/*if(pos->pieces[sq-19] == wN && pos->pieces[sq-9] != wP && pos->pieces[sq-10] != wP  || pos->pieces[sq-21] == wN && pos->pieces[sq-11] != wP && pos->pieces[sq-10] != wP) {
			score -= 6;
		}
		if((RankBBMask[RanksBrd[sq]] == RANK_5)) {
			score -= 4;
		}
		if((RankBBMask[RanksBrd[sq]] == RANK_4)) {
			score -= 8;
		}
		if((RankBBMask[RanksBrd[sq]] <= RANK_4) && (pos->pieces[sq+11] == bP || pos->pieces[sq+9] == bP)) {
			score -= 8;
		}
		if((RankBBMask[RanksBrd[sq]] <= RANK_4) && (pos->pieces[sq+11] == bP && pos->pieces[sq+9] == bP)) {
			score -= 10;
		}
		if((RankBBMask[RanksBrd[sq]] <= RANK_4) && (pos->pieces[sq+11] != bP || pos->pieces[sq+9] != bP)) {
			score += 8;
		}*/
		//int KingSqC = KingSqAttackedbyPawnB(pos);
		//if(KingSqC == TRUE) {
			//score -= 10;
		//}
	}
	
	pce = wN;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += KnightTable[SQ64(sq)];
		if((RankBBMask[RanksBrd[sq]] >= RANK_4)) {
			score += 2;
		} 
		if((RankBBMask[RanksBrd[sq]] == RANK_6)) {
			score += 4;
		}
		if((RankBBMask[RanksBrd[sq]] == RANK_7)) {
			score += 6;
		}
		if((RankBBMask[RanksBrd[sq]] == RANK_1)) {
			score -= 2;
		}
		if(pos->pieces[sq+10] == wP && RankBBMask[RanksBrd[sq]] != RANK_2) {
			score += 2;
		}
		if(pos->pieces[sq+10] == bP && pos->pieces[sq+9] != bP && pos->pieces[sq+11] != bP) {
			score += 8;
		}
		if(pos->pieces[sq-9] == wP || pos->pieces[sq-11] == wP && RankBBMask[RanksBrd[sq]] >= RANK_4) {
			score += 4;
		}
		
		//int InCheck = SqAttacked(pos->KingSq[BLACK], WHITE, pos);
		//int KingSqC = KingSqAttackedbyKnightW(pos);

		/*if(InCheck == TRUE) {
			score += 5;
		}*/
		//if(KingSqC == TRUE) {
			//score += 4;
		//}
		//mobility = MobilityCountWhiteKn(pos,wN);
		//score += Mobility[mobility];
		//int InCheck = SqAttacked(pos->KingSq[BLACK],WHITE,pos);
		//if(InCheck == TRUE) {
			//score += KnightCheck;
		//}

	}	

	pce = bN;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= KnightTable[MIRROR64(SQ64(sq))];
		if((RankBBMask[RanksBrd[sq]] <= RANK_5)) {
			score -= 2;
		}
		if((RankBBMask[RanksBrd[sq]] == RANK_3)) {
			score -= 4;
		}
		if((RankBBMask[RanksBrd[sq]] == RANK_2)) {
			score -= 6;
		}
		if((RankBBMask[RanksBrd[sq]] == RANK_8)) {
			score += 2;
		}
		if(pos->pieces[sq-10] == bP && RankBBMask[RanksBrd[sq]] != RANK_7) {
			score -= 2;
		}
		if(pos->pieces[sq-10] == wP && pos->pieces[sq-9] != wP && pos->pieces[sq-11] != wP) {
			score -= 8;
		}
		if(pos->pieces[sq+9] == bP || pos->pieces[sq+11] == bP && RankBBMask[RanksBrd[sq]] <= RANK_5) {
			score -= 4;
		}
		
		//int InCheck = SqAttacked(pos->KingSq[WHITE], BLACK, pos);
		//int KingSqC = KingSqAttackedbyKnightB(pos);
		/*if(InCheck == TRUE) {
			score -= 5;
		}*/
		//if(KingSqC == TRUE) {
			//score -= 4;
		//}
		//mobility = MobilityCountBlackKn(pos,bN);
		//score -= Mobility[mobility];
		/*int InCheck = SqAttacked(pos->KingSq[WHITE],BLACK,pos);
		if(InCheck == TRUE) {
			score -= KnightCheck;
		}*/
	}			
	
	pce = wB;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += BishopTable[SQ64(sq)];
		if((pos->pieces[sq+11] == wP) || (pos->pieces[sq+9] == wP)) {
			score -= 4;
		}
		if((pos->pieces[sq+11] == wP) && (pos->pieces[sq+9] == wP)) {
			score -= 8;
		}
		if((pos->pieces[sq+1] == wP) && (pos->pieces[sq-1] == wP) && (pos->pieces[sq+10] == wP)) {
			score += 8;
		}
		if((pos->pieces[sq+2] == bP) && (pos->pieces[sq-11] == bP)) {
			score -= 220;
		}
		if((pos->pieces[sq-2] == bP) && (pos->pieces[sq-9] == bP)) {
			score -= 220;
		}
		if(pos->pieces[sq-10] == wQ) {
			score -= 2;
		}
		if((pos->pieces[sq-9] == wP) || (pos->pieces[sq-11] == wP) && RankBBMask[RanksBrd[sq]] >= RANK_4) {
			score += 4;
		}
		//int InCheck = SqAttacked(pos->KingSq[BLACK], WHITE, pos);
		//int KingSqC = KingSqAttackedbyBishopQueenW(pos);
		/*if(InCheck == TRUE) {
			score += 4;
		}*/ 
		//if(KingSqC == TRUE) {
			//score += 3;
		//}
		/*int InCheck = SqAttacked(pos->KingSq[BLACK],WHITE,pos);
		if(InCheck == TRUE) {
			score += BishopCheck;
		}*/
	}	

	pce = bB;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= BishopTable[MIRROR64(SQ64(sq))];
		if((pos->pieces[sq-11] == bP) || (pos->pieces[sq-9] == bP)) {
			score += 4;
		}
		if((pos->pieces[sq-11] == bP) && (pos->pieces[sq-9] == bP)) {
			score += 8;
		}
		if((pos->pieces[sq+1] == bP) && (pos->pieces[sq-1] == bP) && (pos->pieces[sq-10] == bP)) {
			score -= 8;
		}
		if((pos->pieces[sq+2] == wP) && (pos->pieces[sq+11] == wP)) {
			score += 220;
		}
		if((pos->pieces[sq-2] == wP) && (pos->pieces[sq+9] == wP)) {
			score += 220;
		}
		if(pos->pieces[sq+10] == bQ) {
			score += 2;
		}
		if((pos->pieces[sq+9] == bP) || (pos->pieces[sq+11] == bP) && RankBBMask[RanksBrd[sq]] <= RANK_5) {
			score -= 4;
		}
		//int InCheck = SqAttacked(pos->KingSq[WHITE], BLACK, pos);
		//int KingSqC = KingSqAttackedbyBishopQueenB(pos);
		/*if(InCheck == TRUE) {
			score -= 4;
		}*/ 
		//if(KingSqC == TRUE) {
			//score -= 3;
		//}
		/*int InCheck = SqAttacked(pos->KingSq[WHITE],BLACK,pos);
		if(InCheck == TRUE) {
			score -= BishopCheck;
		}*/
	}	

	pce = wR;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		//int KingSqC = KingSqAttackedbyRookQueenW(pos);
		//int InCheck = SqAttacked(pos->KingSq[BLACK],WHITE,pos);
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += RookTable[SQ64(sq)];
		
		ASSERT(FileRankValid(FilesBrd[sq]));
		KingSq = pos->KingSq[BLACK];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += RookOpenFile;
		}
		if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += RookSemiOpenFile;
		}
		if((RankBBMask[RanksBrd[sq]] == RankBBMask[RanksBrd[KingSq]])) {
			score += 2;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score += 4;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score += 3;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score += 3;
		}
		if(!(pos->pawns[WHITE]) && (pos->pawns[BLACK]) & FileBBMask[FilesBrd[sq]]) {
			score += 6;
		}
		if(!(pos->pawns[WHITE]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score += 8;
		}
		if(!(pos->pawns[WHITE]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score += 7;
		}
		if(!(pos->pawns[WHITE]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score += 7;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score += 10;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score += 9;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score += 9;
		}
		if((pos->pieces[sq-9] == wP) || (pos->pieces[sq-11] == wP) && RankBBMask[RanksBrd[sq]] >= RANK_5) {
			score += 6;
		} //else if(KingSqC == TRUE) {
			//score += 5;
		//} /*else if(InCheck == TRUE) {
			//score += 5;
		//}*/
	}	

	pce = bR;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		//int KingSqC = KingSqAttackedbyRookQueenB(pos);
		//int InCheck = SqAttacked(pos->KingSq[WHITE],BLACK,pos);
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= RookTable[MIRROR64(SQ64(sq))];
		ASSERT(FileRankValid(FilesBrd[sq]));
		KingSq = pos->KingSq[WHITE];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= RookOpenFile;
		}
		if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= RookSemiOpenFile;
		}
		if((RankBBMask[RanksBrd[sq]] == RankBBMask[RanksBrd[KingSq]])) {
			score -= 2;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score -= 4;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score -= 3;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score -= 3;
		}
		if(!(pos->pawns[BLACK]) && (pos->pawns[WHITE]) & FileBBMask[FilesBrd[sq]]) {
			score -= 6;
		}
		if(!(pos->pawns[BLACK]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score -= 8;
		}
		if(!(pos->pawns[BLACK]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score -= 7;
		}
		if(!(pos->pawns[BLACK]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score -= 7;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score -= 10;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score -= 9;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score -= 9;
		}
		if((pos->pieces[sq+9] == bP) || (pos->pieces[sq+11] == bP) && RankBBMask[RanksBrd[sq]] <= RANK_4) {
			score -= 6;
		}//else if(KingSqC == TRUE) {
			//score -= 5;
		//} /*else if(InCheck == TRUE) {
			//score -= 5;
		//}*/
	}	
	
	pce = wQ;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		//int KingSqC = KingSqAttackedbyBishopQueenW(pos);//int InCheck = SqAttacked(pos->KingSq[BLACK],WHITE,pos);
		//int KingSqC2 = KingSqAttackedbyRookQueenW(pos);
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		//score += QueenTable[SQ64(sq)];
		//int InCheck = SqAttacked(pos->KingSq[BLACK], WHITE, pos);
		KingSq = pos->KingSq[BLACK];
		
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += QueenOpenFile;
		}
		if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += QueenSemiOpenFile;
		}
		if((RankBBMask[RanksBrd[sq]] > RANK_1)) {
			score += 2;
		}
		if((RankBBMask[RanksBrd[sq]] == RankBBMask[RanksBrd[KingSq]])) {
			score += 2;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score += 4;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score += 3;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score += 3;
		}
		if(!(pos->pawns[WHITE]) && (pos->pawns[BLACK]) & FileBBMask[FilesBrd[sq]]) {
			score += 4;
		}
		if(!(pos->pawns[WHITE]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score += 6;
		}
		if(!(pos->pawns[WHITE]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score += 5;
		}
		if(!(pos->pawns[WHITE]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score += 5;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score += 8;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score += 7;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score += 7;
		}
		if((pos->pieces[sq-9] == wP) || (pos->pieces[sq-11] == wP) && RankBBMask[RanksBrd[sq]] >= RANK_5) {
			score += 4;
		}//else if(KingSqC == TRUE) {
			//score += 2;
		//} //else if(KingSqC2 == TRUE) {
			//score += 2;
		//}/*else if(InCheck == TRUE) {
			//score += 3;
		//}
	}	

	pce = bQ;	
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		//int KingSqC = KingSqAttackedbyBishopQueenB(pos);
		//int KingSqC2 = KingSqAttackedbyRookQueenB(pos);
		//int InCheck = SqAttacked(pos->KingSq[WHITE],BLACK,pos);
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(FileRankValid(FilesBrd[sq]));
		//score -= QueenTable[MIRROR64(SQ64(sq))];
		KingSq = pos->KingSq[WHITE];

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenOpenFile;
		}
		if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenSemiOpenFile;
		}
		if((RankBBMask[RanksBrd[sq]] < RANK_8)) {
			score -= 2;
		}
		if((RankBBMask[RanksBrd[sq]] == RankBBMask[RanksBrd[KingSq]])) {
			score -= 2;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score -= 4;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score -= 3;
		}
		if((FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score -= 3;
		}
		if(!(pos->pawns[BLACK]) && (pos->pawns[WHITE]) & FileBBMask[FilesBrd[sq]]) {
			score -= 4;
		}
		if(!(pos->pawns[BLACK]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score -= 6;
		}
		if(!(pos->pawns[BLACK]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score -= 5;
		}
		if(!(pos->pawns[BLACK]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score -= 5;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq]])) {
			score -= 8;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq-1]])) {
			score -= 7;
		}
		if(!(pos->pawns[BOTH]) && (FileBBMask[FilesBrd[sq]] == FileBBMask[FilesBrd[KingSq+1]])) {
			score -= 7;
		}
		if((pos->pieces[sq+9] == bP) || (pos->pieces[sq+11] == bP) && RankBBMask[RanksBrd[sq]] <= RANK_4) {
			score -= 4;
		}
		 //else if(KingSqC == TRUE) {
			//score -= 2;
		//} //else if(KingSqC2 == TRUE) {
			//score -= 2;
		//} /*else if(InCheck == TRUE) {
			//score -= 3;
		//}*/
	}	
	//8/p6k/6p1/5p2/P4K2/8/5pB1/8 b - - 2 62 
	pce = wK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
	
	if( (pos->material[BLACK] <= ENDGAME_MAT) ) {
		score += KingE[SQ64(sq)];
		
		if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= KingSemiOpenFile;
		}
		if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq+1]])) {
			score -= KingSemiOpenFile;
		}
		if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq-1]])) {
			score -= KingSemiOpenFile;
		}
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= KingOpenFile;
		}
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq+1]])) {
			score -= KingOpenFile;
		}
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq-1]])) {
			score -= KingOpenFile;
		}
		if(pos->pieces[sq+9] == wP && pos->pieces[sq+10] == wP && pos->pieces[sq+11] == wP) { //safe castle
			score += 8;
		}
		if(pos->pieces[sq+10] == wP && pos->pieces[sq+20] == wP && pos->pieces[sq+11] == wP) { //safe castle
			score += 8;
		}
		if(pos->pieces[sq+10] == wP && pos->pieces[sq+20] == wP && pos->pieces[sq+9] == wP) { //safe castle
			score += 8;
		}
		if(pos->pieces[sq+9] == wP && pos->pieces[sq+10] == wB && pos->pieces[sq+11] == wP) { //safe castle
			score += 8;
		}

	} else {
		score += KingO[SQ64(sq)];
		if((pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += KingOpenFile;
		}
		if((pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += KingSemiOpenFile;
		}
		if((pos->pawns[WHITE] & FileBBMask[FilesBrd[sq-1]])) {
			score += KingSemiOpenFile;
		}
		if((pos->pawns[WHITE] & FileBBMask[FilesBrd[sq+1]])) {
			score += KingSemiOpenFile;
		}
	}
	
	pce = bK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
	
	if( (pos->material[WHITE] <= ENDGAME_MAT) ) {
		score -= KingE[MIRROR64(SQ64(sq))];
		
		if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += KingSemiOpenFile;
		}
		if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq+1]])) {
			score += KingSemiOpenFile;
		}
		if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq-1]])) {
			score += KingSemiOpenFile;
		}
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += KingOpenFile;
		}
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq+1]])) {
			score += KingOpenFile;
		}
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq-1]])) {
			score += KingOpenFile;
		}
		if(pos->pieces[sq-9] == bP && pos->pieces[sq-10] == bP && pos->pieces[sq-11] == bP) { //safe castle
			score -= 8;
		}
		if(pos->pieces[sq-10] == bP && pos->pieces[sq-20] == bP && pos->pieces[sq-11] == bP) { //safe castle
			score -= 8;
		}
		if(pos->pieces[sq-10] == bP && pos->pieces[sq-20] == bP && pos->pieces[sq-9] == bP) { //safe castle
			score -= 8;
		}
		if(pos->pieces[sq-9] == bP && pos->pieces[sq-10] == bB && pos->pieces[sq-11] == bP) { //safe castle
			score -= 8;
		}

	} else {
		score -= KingO[MIRROR64(SQ64(sq))];

		if((pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= KingOpenFile;
		}
		if((pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= KingSemiOpenFile;
		}
		if((pos->pawns[BLACK] & FileBBMask[FilesBrd[sq-1]])) {
			score -= KingSemiOpenFile;
		}
		if((pos->pawns[BLACK] & FileBBMask[FilesBrd[sq+1]])) {
			score -= KingSemiOpenFile;
		}
	}
	
	if(pos->pceNum[wB] >= 2) score += BishopPair;
	if(pos->pceNum[bB] >= 2) score -= BishopPair;
	
	if(pos->side == WHITE) {
		return score;
	} else {
		return -score;
	}	
}