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

int pawns_on_same_color_squares(const S_BOARD *pos, int c, int s) {
	return popcount(pos->pawns[c] & ((BLACK_SQUARES & s) ? BLACK_SQUARES : ~BLACK_SQUARES));
}

bool opposite_colors(int s1, int s2) {
	return bool(BLACK_SQUARES & s1) != bool(BLACK_SQUARES & s2);
}

bool opposite_bishops(const S_BOARD *pos) {
	return  ( pos->pceNum[wB] == 1
           && pos->pceNum[bB] == 1
           && opposite_colors(SQ64(pos->pList[wB][0]) , SQ64(pos->pList[bB][0])) );
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

int relativeSquare32(int c, int sq) {
    ASSERT(0 <= c && c < BOTH);
    ASSERT(0 <= sq && sq < 64);
    return 4 * relative_rank(c, sq) + map_to_queenside(FilesBrd[SQ120(sq)]);
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

int evaluateScaleFactor(const S_BOARD *pos) {

    if ( opposite_bishops(pos) ) {

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

#define S(mg, eg) (MakeScore((mg), (eg)))

int PieceValPhases[13] = { S( 0, 0), S( 105, 118), S( 450, 405), S( 473, 423), S( 669, 695), S(1295,1380), S( 0, 0), S( 105, 118), S( 450, 405), S( 473, 423), S( 669, 695), S(1295,1380), S( 0, 0)  };

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0, -13), S(   4,  -5), S(  13,   7), S(  16,   6),
    S( -14,  -6), S(  -4,  -7), S(  15,  -3), S(  22,   3),
    S(  -9,  -1), S(  -8,  -5), S(  10,  -9), S(  28,  -7),
    S(   8,   7), S(  -6,   7), S(  -7,  -1), S(   5, -11),
    S( -10,  19), S( -12,  12), S(  -5,  13), S(   6,  27),
    S(  -7,   3), S(   7,  -3), S(  -8,  14), S(  -3,  21),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -82, -45), S( -43, -30), S( -34, -23), S( -34,  -9),
    S( -36, -31), S( -19, -25), S( -12,  -8), S(  -7,   3),
    S( -28, -18), S(  -7, -12), S(   2,  -3), S(   5,  13),
    S( -16, -16), S(   3,   0), S(  18,   6), S(  23,  13),
    S( -15, -21), S(   6,  -7), S(  20,   4), S(  23,  18),
    S(  -4, -23), S(  10, -20), S(  27,  -7), S(  24,   7),
    S( -31, -32), S( -12, -23), S(   1, -23), S(  17,   5),
    S( -94, -46), S( -38, -41), S( -26, -26), S( -12,  -7),
};

const int BishopPSQT32[32] = {
    S( -24, -26), S(  -2, -14), S(  -3, -17), S( -10,  -5),
    S(  -7, -17), S(   3,  -6), S(   8,  -7), S(   1,   0),
    S(  -3,  -7), S(   9,   0), S(  -2,   0), S(   7,   4),
    S(  -2,  -9), S(   5,  -2), S(  11,   0), S(  18,   7),
    S(  -5,  -7), S(  13,   0), S(  10,  -6), S(  14,   7),
    S(  -7, -14), S(   2,   2), S(   0,   1), S(   5,   2),
    S(  -7, -14), S(  -6,  -9), S(   2,   0), S(   0,   0),
    S( -22, -21), S(   0, -19), S(  -6, -17), S( -10, -11),
};

const int RookPSQT32[32] = {
    S( -14,  -4), S(  -9,  -6), S(  -6,  -4), S(  -2,  -4),
    S(  -9,  -5), S(  -6,  -4), S(  -3,   0), S(   2,   0),
    S( -11,   2), S(  -5,  -3), S(   0,   0), S(   1,  -2),
    S(  -6,  -2), S(  -2,   0), S(  -1,  -4), S(  -2,   3),
    S( -12,  -2), S(  -7,   3), S(  -1,   3), S(   1,  -2),
    S( -10,   2), S(   0,   0), S(   2,  -3), S(   5,   4),
    S(   0,   1), S(   5,   2), S(   7,   9), S(   8,  -2),
    S(  -7,   8), S(  -8,   0), S(   0,   8), S(   4,   6),
};

const int QueenPSQT32[32] = {
    S(   1, -32), S(  -2, -26), S(  -2, -22), S(   1, -12),
    S(  -1, -25), S(   2, -14), S(   3, -10), S(   5,  -1),
    S(  -1, -18), S(   2,  -8), S(   6,  -4), S(   3,   1),
    S(   1, -10), S(   2,  -1), S(   4,   6), S(   3,  11),
    S(   0, -13), S(   6,  -2), S(   5,   4), S(   2,   9),
    S(  -1, -17), S(   4,  -8), S(   2,  -5), S(   3,   0),
    S(  -2, -23), S(   2, -12), S(   4, -11), S(   3,  -3),
    S(   0, -35), S(   0, -24), S(   0, -20), S(   0, -16),
};

const int KingPSQT32[32] = {
    S( 127,   0), S( 153,  21), S( 126,  39), S(  90,  35),
    S( 130,  24), S( 142,  46), S( 107,  62), S(  81,  63),
    S(  91,  41), S( 121,  61), S(  79,  79), S(  56,  82),
    S(  76,  48), S(  89,  73), S(  64,  80), S(  46,  80),
    S(  72,  45), S(  84,  77), S(  49,  93), S(  32,  93),
    S(  57,  43), S(  68,  80), S(  38,  86), S(  14,  89),
    S(  41,  22), S(  56,  56), S(  30,  54), S(  15,  61),
    S(  27,   5), S(  41,  27), S(  21,  34), S(   0,  36),
};

const int KnightMobility[9] = {
    S( -16, -16), S( -12, -12), S(  -8,  -8), S(  -4,  -4),
    S(   0,   0), S(   4,   4), S(   8,   8), S(  12,  12),
    S(  16,  16),
};

const int BishopMobility[14] = {
    S( -18, -18), S( -15, -15), S( -12, -12), S(  -9,  -9),
    S(  -6,  -6), S(  -3,  -3), S(   0,   0), S(   3,   3),
    S(   6,   6), S(   9,   9), S(  12,  12), S(  15,  15),
    S(  18,  18), S(  21,  21),
};

const int RookMobility[15] = {
    S( -12, -24), S( -10, -20), S(  -8, -16), S(  -6, -12),
    S(  -4,  -8), S(  -2,  -4), S(   0,   0), S(   2,   4),
    S(   4,   8), S(   6,  12), S(   8,  16), S(  10,  20),
    S(  12,  24), S(  14,  28), S(  16,  32),
};

const int QueenMobility[28] = {
    S( -12, -24), S( -11, -22), S( -10, -20), S(  -9, -18),
    S(  -8, -16), S(  -7, -14), S(  -6, -12), S(  -5, -10),
    S(  -4,  -8), S(  -3,  -6), S(  -2,  -4), S(  -1,  -2),
    S(   0,   0), S(   1,   2), S(   2,   4), S(   3,   6),
    S(   4,   8), S(   5,  10), S(   6,  12), S(   7,  14),
    S(   8,  16), S(   9,  18), S(  10,  20), S(  11,  22),
    S(  12,  24), S(  13,  26), S(  14,  28), S(  15,  30),
};

const int PawnBackward[2] = { S(   7,   0), S(  -7, -19) };
const int PawnDoubled = S(  11,  56);
const int PawnIsolated = S(   5,  15);
const int WeakUnopposed = S(  13,  27);
const int BlockedStorm  = S(  20,  20);
const int PawnPassed[8] = { 
	S(   0,   0), S(  10,  28), S(  17,  33), S(  15,  41),
    S(  62,  82), S( 168, 177), S( 276, 290), S(   0,   0),
};
const int PawnPassedConnected[8] = { 
	S(   0,   0), S(   7,   7), S(   8,   8), S(  12,  12),
    S(  29,  29), S(  48,  48), S(  86,  86), S(   0,   0),
};
const int Connected[8] = { 0, 7, 8, 12, 29, 48, 86, 0};

const int KnightOutpost[2] = { S(   4, -16), S(  19,  -2) };
const int BishopOutpost[2] = { S(   6,  -7), S(  25,   0) };

const int PairOfBishops = S(  30,  42);
const int BadBishop = S(  -1,  -2);

const int KingDefender = S(  -1,  -1);

const int RookOpen = S(  22,  11);
const int RookSemi = S(  10,   3);
const int RookSeventh = S(   8,  16);

const int QueenPreDeveloped = S(  -2,  -2);
const int PawnLessFlank = S(  -8, -44);

#undef S

int Pawns(const S_BOARD *pos, int side, int pce, int pceNum) {

	int score = 0;
	int sq;
	int support, pawnbrothers;
	U64 opposed;
	U64 flag;
	int blockSq, w, R;
	int bonus;
	int dir, t_sq, index;
	int Su, Up;
	int att = 0;

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

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
    	ei->attCnt[side] += att;	
    }

	R  = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);
	Su = (side == WHITE ? -10 :  10);
	Up = (side == WHITE ?  10 : -10);

	opposed = (FileBBMask[FilesBrd[sq]] & pos->pawns[side^1]);
	support = (pos->pawn_ctrl[side][sq]);
	pawnbrothers = (pos->pieces[sq-1] == pce || pos->pieces[sq+1] == pce);

	//printf("opposed %d\n",bool(opposed) );
	//printf("support %d\n",bool(support) );
	//printf("pawnbrothers %d\n",bool(pawnbrothers) );

	if(pos->pieces[sq+Su] == pce) {
		//printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
		score -= PawnDoubled;
	}
	if(pos->pieces[sq+Su*2] == pce) {
		//printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
		score -= PawnDoubled;
	}
	if(pos->pieces[sq+Su*3] == pce) {
		//printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
		score -= PawnDoubled;
	}

	if( (IsolatedMask[SQ64(sq)] & pos->pawns[side]) == 0) {
		//printf("%c Iso:%s\n",PceChar[pce], PrSq(sq));
		//printf("Iso P %d\n",PawnIsolated + WeakUnopposed * !opposed );
		score -= PawnIsolated + WeakUnopposed * !opposed;
	}

	if ( !(PassedPawnMasks[side^1][SQ64(sq)] & pos->pawns[side])
    	&& pos->pawn_ctrl[side^1][sq + Up]) {
		//printf("%c BackW:%s\n",PceChar[pce], PrSq(sq));
        flag = !(opposed);
    	score += PawnBackward[flag];
    }

    if( (PassedPawnMasks[side][SQ64(sq)] & pos->pawns[side^1]) == 0) {
		//printf("%c Passed:%s\n",PceChar[pce], PrSq(sq));
		score += PawnPassed[R];

		if(support || pawnbrothers) {
			//printf("%c PassedConnected:%s\n",PceChar[pce], PrSq(sq));
			score += PawnPassedConnected[R];
		}

		if(R > RANK_3) {

			blockSq = sq + Up;
			w = 5 * R - 13;

			bonus = ((  king_proximity(side^1, blockSq, pos) * 5
                      - king_proximity(side,   blockSq, pos) * 2) * w);
			
            if (R != RANK_7) {
                bonus -= ( king_proximity(side, blockSq + Up, pos) * w);
            }
            bonus *= 100;
            bonus /= 410;

            score += MakeScore(0, bonus);
		}
		/*if (!PassedPawnMasks[side][SQ64(sq + Up)
        || (pos->pawns[BOTH] & (SQ64(sq + Up)))) {
			bonus = bonus / 2;
		}*/  
	}

	if(support || pawnbrothers) {
		int i =  Connected[R] * (2 + bool(pawnbrothers) - bool(opposed))
				+ 21 * pos->pawn_ctrl[side][sq];
        i *= 100;
        i /= 1220;
        //printf("supportCount %d v %d v eg %d \n",pos->pawn_ctrl[side][sq], i, i * (R - 2) / 4);
		score += MakeScore(i, i * (R - 2) / 4);
	}

	//ei->pawns[side] += score;

	return score;
}

int Knights(const S_BOARD *pos, int side, int pce, int pceNum) {
	
	int score = 0;
	int sq;
	int index;
	int t_sq;
	int dir;
	int R, defended;

	int att = 0;
	int mobility = 0;
	int tropism;

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	R = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);

    if ((R == RANK_4
    ||   R == RANK_5
    ||   R == RANK_6)
    && !(outpostSquareMasks(side, SQ64(sq)) & pos->pawns[side^1])) {
    	//printf("%c KnightOutpost:%s\n",PceChar[pce], PrSq(sq));

    	defended = (pos->pawn_ctrl[side][sq]);
    	score += KnightOutpost[defended];
    }

	for(index = 0; index < NumDir[pce]; ++index) {
		dir = PceDir[pce][index];
		t_sq = sq + dir;
		if(!SQOFFBOARD(t_sq) && PieceCol[pos->pieces[t_sq]] != side) {

			if(!pos->pawn_ctrl[side^1][t_sq]) {
				mobility++;
			}
				
			if ( e->sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
				att++;
			}    
		}
	}

    tropism = getTropism(sq, pos->KingSq[side^1]);
	score += MakeScore(3 * tropism, 3 * tropism);
	score += KnightMobility[mobility];
	ei->Mob[side] += KnightMobility[mobility];

    int Count = distanceBetween(sq, pos->KingSq[side]);
    int P1 = (7 * Count) * 100 / 410;
    int P2 = (8 * Count) * 100 / 410;
   	score += MakeScore(-P1, -P2);
    //score += KingDefender * Count;

	if(att) {
    	ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }
    //ei->knights[side] += score;

    return score;	
}

int Bishops(const S_BOARD *pos, int side, int pce, int pceNum) {

	int score = 0;
	int sq;
	int index;
	int t_sq;
	int dir;
	int Count, P1, P2;
	int R, defended;

	int att = 0;
	int mobility = 0;
	int tropism;

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	R = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);

    if ((R == RANK_4
    ||   R == RANK_5
    ||   R == RANK_6)
    && !(outpostSquareMasks(side, SQ64(sq)) & pos->pawns[side^1])) {
    	//printf("%c BishopOutpost:%s\n",PceChar[pce], PrSq(sq));

    	defended = (pos->pawn_ctrl[side][sq]);
    	score += BishopOutpost[defended];
    }

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

	tropism = getTropism(sq, pos->KingSq[side^1]);
	score += MakeScore(2 * tropism, 1 * tropism);
	score += BishopMobility[mobility];
	ei->Mob[side] += BishopMobility[mobility];

    if(mobility <= 3) {

    	Count = pawns_on_same_color_squares(pos,side,SQ64(sq));
    	P1 = (3 * Count) * 100 / 210;
    	P2 = (7 * Count) * 100 / 210;

    	score += MakeScore(-P1, -P2);
    } 

    Count = distanceBetween(sq, pos->KingSq[side]);
    P1 = (7 * Count) * 100 / 410;
    P2 = (8 * Count) * 100 / 410;

    score += MakeScore(-P1, -P2);

    //if(pos->pceNum[pce] > 1) score += PairOfBishops;
  
    if(att) {
    	ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }
    //ei->bishops[side] += score;

    return score;
}

int Rooks(const S_BOARD *pos, int side, int pce, int pceNum ) {

	int score = 0;
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

	int R  = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);
	int KR = (side == WHITE ? RanksBrd[pos->KingSq[side^1]] : 7 - RanksBrd[pos->KingSq[side^1]]);

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

	if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
		score += RookOpen;
	}
	if(!(pos->pawns[side] & FileBBMask[FilesBrd[sq]])) {
		score += RookSemi;
	}

	if((R == RANK_7 && KR == RANK_8)) {
		score += RookSeventh;
	}

	tropism = getTropism(sq, pos->KingSq[side^1]);
	score += MakeScore(2 * tropism, 1 * tropism);
	score += RookMobility[mobility];
	ei->Mob[side] += RookMobility[mobility];

	if (att) {
		ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }
    //ei->rooks[side] += score;

    return score;
}

int Queens(const S_BOARD *pos, int side, int pce, int pceNum) {

	int score = 0;
	int sq;
	int index;
	int t_sq;
	int dir;

	int att = 0;
	int mobility = 0;
	int tropism;

	int KNIGHT, BISHOP;

	KNIGHT = (side == WHITE ? wN : bN);
	BISHOP = (side == WHITE ? wB : bB);

	sq = pos->pList[pce][pceNum];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

	int R  = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);

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

	if (R > RANK_2) {
		if(isPiece(side, KNIGHT, REL_SQ(side,B1), pos)) {
			score += QueenPreDeveloped;
		}
		if(isPiece(side, BISHOP, REL_SQ(side,C1), pos)) {
			score += QueenPreDeveloped;
		}
		if(isPiece(side, BISHOP, REL_SQ(side,F1), pos)) {
			score += QueenPreDeveloped;
		}
		if(isPiece(side, KNIGHT, REL_SQ(side,G1), pos)) {
			score += QueenPreDeveloped;
		}
	}

	tropism = getTropism(sq, pos->KingSq[side^1]);
	score += MakeScore(2 * tropism, 4 * tropism);
	score += QueenMobility[mobility];
	ei->Mob[side] += QueenMobility[mobility];

	if (att) {
		ei->attCnt[side] += att;
        ei->attckersCnt[side] += 1;
        ei->attWeight[side] += Weight[pce];
    }
    //ei->queens[side] += score;

    return score;
}

int EvaluateKings(const S_BOARD *pos) {

	int score = 0;
	U64 ours;
	U64 theirs;
	int side;
	int count;
	int center;

	side = WHITE;

	if (!(pos->pawns[BOTH] & KingFlank[FilesBrd[pos->KingSq[side]]])) { 
		score += PawnLessFlank;
	}

	center = clamp(FilesBrd[pos->KingSq[side]], FILE_B, FILE_G);
  	for (int file = center - 1; file <= center + 1; ++file) {

  		ours = (pos->pawns[side] & FileBBMask[file]);
  		int ourRank = ours ? relative_rank(side, frontmost(side^1, ours)) : 0;

  	 	theirs = (pos->pawns[side^1] & FileBBMask[file]);
      	int theirRank = theirs ? relative_rank(side, frontmost(side^1, theirs)) : 0;

      	int d = map_to_queenside(file);
      	int bonus = MakeScore(1, 1);
      	bonus += MakeScore((ShelterStrength[d][ourRank] * 100) / 410, 0);

      	if (ourRank && (ourRank == theirRank - 1)) {
      		bonus -= BlockedStorm * int(theirRank == RANK_3);
      	} else {
      		bonus -= MakeScore((UnblockedStorm[d][theirRank] * 100) / 410, 0);
      	}
      	ei->pkeval[side] += bonus;
        score += bonus;
  	}

	side = BLACK;

	if (!(pos->pawns[BOTH] & KingFlank[FilesBrd[pos->KingSq[side]]])) { 
		score -= PawnLessFlank;
	}

	center = clamp(FilesBrd[pos->KingSq[side]], FILE_B, FILE_G);
  	for (int file = center - 1; file <= center + 1; ++file) {

  		ours = (pos->pawns[side] & FileBBMask[file]);
  		int ourRank = ours ? relative_rank(side, frontmost(side^1, ours)) : 0;

  	 	theirs = (pos->pawns[side^1] & FileBBMask[file]);
      	int theirRank = theirs ? relative_rank(side, frontmost(side^1, theirs)) : 0;

      	int d = map_to_queenside(file);
      	int bonus = MakeScore(1, 1);
      	bonus += MakeScore((ShelterStrength[d][ourRank] * 100) / 410, 0);

      	if (ourRank && (ourRank == theirRank - 1)) {
      		bonus -= BlockedStorm * int(theirRank == RANK_3);
      	} else {
      		bonus -= MakeScore((UnblockedStorm[d][theirRank] * 100) / 410, 0);
      	}
      	ei->pkeval[side] += bonus;
        score -= bonus;
  	}

  	//PrintBitBoard(ei->kingAreas[WHITE]);
	//PrintBitBoard(ei->kingAreas[BLACK]);
	if (ei->attckersCnt[BLACK] > 1 - pos->pceNum[bQ]) {
	
		float scaledAttackCounts = 9.0 * ei->attCnt[BLACK] / popcount(ei->kingAreas[WHITE]) + 1;
		count = ei->attckersCnt[BLACK] * ei->attWeight[BLACK];
		count += 32  * scaledAttackCounts //29 //32
			  + -10  * popcount(pos->pawns[WHITE] & ei->kingAreas[WHITE])
			  + - 2  * ScoreMG(ei->pkeval[WHITE]) / 8
			  +        ScoreMG(ei->Mob[BLACK] - ei->Mob[WHITE]) / 4;
			  //+ -176 * !pos->pceNum[bQ];

		//printf("BLACK attacks %d attckersCnt %d attWeight %d \n", ei->attCnt[BLACK], ei->attckersCnt[BLACK], ei->attWeight[BLACK]);
		//printf("scaledAttackCounts %f Panws & KA %d\n",scaledAttackCounts, popcount(pos->pawns[WHITE] & ei->kingAreas[WHITE]) );
		if(count > 0) {
			//printf("count %d\n",count );
			//ei->KingDanger[BLACK] = MakeScore(count * count / 720, count / 18);
			score -= MakeScore(count * count / 720, count / 18);
		}
	}
    if (ei->attckersCnt[WHITE] > 1 - pos->pceNum[wQ]) {
    
    	float scaledAttackCounts = 9.0 * ei->attCnt[WHITE] / popcount(ei->kingAreas[BLACK]) + 1;
		count = ei->attckersCnt[WHITE] * ei->attWeight[WHITE];
		count += 32  * scaledAttackCounts
		      + -10  * popcount(pos->pawns[BLACK] & ei->kingAreas[BLACK])
		      + - 2  * ScoreMG(ei->pkeval[BLACK]) / 8
		      +        ScoreMG(ei->Mob[WHITE] - ei->Mob[BLACK]) / 4;
		      //+ -176 * !pos->pceNum[wQ];
		//printf(" WHITE attacks %d attckersCnt %d attWeight %d \n", ei->attCnt[WHITE], ei->attckersCnt[WHITE], ei->attWeight[WHITE]);
		//printf("scaledAttackCounts %f Panws & KA %d\n",scaledAttackCounts, popcount(pos->pawns[BLACK] & ei->kingAreas[BLACK]) );
		if(count > 0) {
			//printf("count %d\n",count );
			//ei->KingDanger[WHITE] = MakeScore(count * count / 720, count / 18);
			score += MakeScore(count * count / 720, count / 18);
		}
    }

    return score;
}


int EvaluatePieces(const S_BOARD *pos) {
	int pce, score = 0;
	pce = wP;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score += Pawns(pos, WHITE, pce, pceNum);
	}
	pce = bP;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score -= Pawns(pos, BLACK, pce, pceNum);
	}
	pce = wN;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score += Knights(pos, WHITE, pce, pceNum);
	}
	pce = bN;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score -= Knights(pos, BLACK, pce, pceNum);
	}
	pce = wB;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score += Bishops(pos, WHITE, pce, pceNum);
	}
	pce = bB;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score -= Bishops(pos, BLACK, pce, pceNum);
	}
	pce = wR;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score += Rooks(pos, WHITE, pce, pceNum);
	}
	pce = bR;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score -= Rooks(pos, BLACK, pce, pceNum);
	}
	pce = wQ;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score += Queens(pos, WHITE, pce, pceNum);
	}
	pce = bQ;
	for(int pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		score -= Queens(pos, BLACK, pce, pceNum);
	}
	score += EvaluateKings(pos);

	return score;
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
	    ei->blockages[side] += RETURNING_BISHOP;
	}

	if (pos->pieces[C1] == wB
	&&  pos->pieces[B1] == wK) {
	    ei->blockages[side] += RETURNING_BISHOP;
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
	    ei->blockages[side] += RETURNING_BISHOP;
	}

	if (pos->pieces[C8] == bB
	&&  pos->pieces[B8] == bK) {
	    ei->blockages[side] += RETURNING_BISHOP;
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

int EvalPosition(const S_BOARD *pos) {

	// setboard 8/3k3p/6p1/3nK1P1/8/8/7P/8 b - - 3 64
	// setboard r2q1rk1/p2b1p1p/1p1b2pQ/2p1pP2/1nPp4/1P1BP3/PB1P2PP/RN3RK1 w - - 1 16 

	//int startTime = GetTimeMs();

	int score, phase;

	phase = 24 - 4 * (pos->pceNum[wQ] + pos->pceNum[bQ])
               - 2 * (pos->pceNum[wR] + pos->pceNum[bR])
               - 1 * (pos->pceNum[wN] + pos->pceNum[bN] + pos->pceNum[wB] + pos->pceNum[bB]);

    phase = (phase * 256 + 12) / 24;
			
	for(int side = WHITE; side <= BLACK; side++) {
		ei->Mob[side] = 0;
		ei->attCnt[side] = 0;
		ei->attckersCnt[side] = 0;
		ei->attWeight[side] = 0;
		ei->adjustMaterial[side] = 0;
		ei->blockages[side] = 0;
		ei->pkeval[side] = 0;
		ei->pawns[side] = 0;
		ei->knights[side] = 0;
		ei->bishops[side] = 0;
		ei->rooks[side] = 0;
		ei->queens[side] = 0;
		ei->KingDanger[side] = 0;
		ei->kingAreas[side] = kingAreaMasks(side, SQ64(pos->KingSq[side]));
	} 

	/*if(!pos->pceNum[wP] && !pos->pceNum[bP] && MaterialDraw(pos) == TRUE) {
		return 0;
	}*/
	score   = EvaluatePieces(pos);
	score  += (pos->mPhases[WHITE] - pos->mPhases[BLACK]);
    score  += (pos->PSQT[WHITE] - pos->PSQT[BLACK]);

    blockedPiecesW(pos);
    blockedPiecesB(pos);

	if(pos->pceNum[wB] > 1) ei->adjustMaterial[WHITE] += BishopPair;
	if(pos->pceNum[bB] > 1) ei->adjustMaterial[BLACK] += BishopPair;
	if(pos->pceNum[wN] > 1) ei->adjustMaterial[WHITE] -= KnightPair;
	if(pos->pceNum[bN] > 1) ei->adjustMaterial[BLACK] -= KnightPair;
	ei->adjustMaterial[WHITE] += n_adj[pos->pceNum[wP]] * pos->pceNum[wN];
	ei->adjustMaterial[BLACK] += n_adj[pos->pceNum[bP]] * pos->pceNum[bN];
	ei->adjustMaterial[WHITE] += r_adj[pos->pceNum[wP]] * pos->pceNum[wR];
	ei->adjustMaterial[BLACK] += r_adj[pos->pceNum[bP]] * pos->pceNum[bR];

	int factor = evaluateScaleFactor(pos);

	score = (ScoreMG(score) * (256 - phase)
          +  ScoreEG(score) * phase * factor / SCALE_NORMAL) / 256;

	score += (ei->blockages[WHITE] - ei->blockages[BLACK]);
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

    PAWNST = (stronger == WHITE ? wP : bP);
    PAWNWK = (weaker   == WHITE ? wP : bP);

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

void printEvalFactor( int WMG, int WEG, int BMG, int BEG ) {
    printf("| %4d  %4d  | %4d  %4d  | %4d  %4d \n",WMG, WEG, BMG, BEG, WMG - BMG, WEG - BEG );
}

void printEval(const S_BOARD *pos) {
	printf("-------------------------------------------------------\n");
    printf("      Total       (for side to move): %d \n", EvalPosition(pos) );
	printf("      Term    |    White    |    Black    |    Total   \n");
	printf("              |   MG    EG  |   MG    EG  |   MG    EG \n");
	printf(" -------------+-------------+-------------+------------\n");
	printf("     Material "); printEvalFactor( ScoreMG(pos->mPhases[WHITE]),ScoreEG(pos->mPhases[WHITE]),ScoreMG(pos->mPhases[BLACK]),ScoreEG(pos->mPhases[BLACK]));
	printf("         PSQT "); printEvalFactor( ScoreMG(pos->PSQT[WHITE]),ScoreEG(pos->PSQT[WHITE]),ScoreMG(pos->PSQT[BLACK]),ScoreEG(pos->PSQT[BLACK]));
	printf("        Pawns "); printEvalFactor( ScoreMG(ei->pawns[WHITE]),ScoreEG(ei->pawns[WHITE]),ScoreMG(ei->pawns[BLACK]),ScoreEG(ei->pawns[BLACK]));
	printf("      Knights "); printEvalFactor( ScoreMG(ei->knights[WHITE]),ScoreEG(ei->knights[WHITE]),ScoreMG(ei->knights[BLACK]),ScoreEG(ei->knights[BLACK]));
	printf("      Bishops "); printEvalFactor( ScoreMG(ei->bishops[WHITE]),ScoreEG(ei->bishops[WHITE]),ScoreMG(ei->bishops[BLACK]),ScoreEG(ei->bishops[BLACK]));
	printf("        Rooks "); printEvalFactor( ScoreMG(ei->rooks[WHITE]),ScoreEG(ei->rooks[WHITE]),ScoreMG(ei->rooks[BLACK]),ScoreEG(ei->rooks[BLACK]));
	printf("       Queens "); printEvalFactor( ScoreMG(ei->queens[WHITE]),ScoreEG(ei->queens[WHITE]),ScoreMG(ei->queens[BLACK]),ScoreEG(ei->queens[BLACK]));
	printf("     Mobility "); printEvalFactor( ScoreMG(ei->Mob[WHITE]),ScoreEG(ei->Mob[WHITE]),ScoreMG(ei->Mob[BLACK]),ScoreEG(ei->Mob[BLACK]));
	printf("  King safety "); printEvalFactor( ScoreMG(ei->KingDanger[WHITE]),ScoreEG(ei->KingDanger[WHITE]),ScoreMG(ei->KingDanger[BLACK]),ScoreEG(ei->KingDanger[BLACK]));
	printf("  King shield "); printEvalFactor( ScoreMG(ei->pkeval[WHITE]),ScoreEG(ei->pkeval[WHITE]),ScoreMG(ei->pkeval[BLACK]),ScoreEG(ei->pkeval[BLACK]));
	printf("-------------------------------------------------------\n");
	printf("\n");
}

void setPcsq32() {

    for (int i = 0; i < 120; ++i) {

    	if(!SQOFFBOARD(i)) {
	    	/* set the piece/square tables for each piece type */

	    	const int SQ64 = SQ64(i);
	    	const int w32 = relativeSquare32(WHITE, SQ64);
        	const int b32 = relativeSquare32(BLACK, SQ64);

        	e->PSQT[wP][i] =    PawnPSQT32[w32]; 
	        e->PSQT[bP][i] =    PawnPSQT32[b32];
	        e->PSQT[wN][i] =  KnightPSQT32[w32];
	        e->PSQT[bN][i] =  KnightPSQT32[b32];
	        e->PSQT[wB][i] =  BishopPSQT32[w32];
	        e->PSQT[bB][i] =  BishopPSQT32[b32];
	        e->PSQT[wR][i] =    RookPSQT32[w32];
	        e->PSQT[bR][i] =    RookPSQT32[b32];
	        e->PSQT[wQ][i] =   QueenPSQT32[w32];
	        e->PSQT[bQ][i] =   QueenPSQT32[b32];
	        e->PSQT[wK][i] =    KingPSQT32[w32];
	        e->PSQT[bK][i] =    KingPSQT32[b32];
    	}        
    }
}