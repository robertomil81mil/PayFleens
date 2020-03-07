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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "evaluate.h"

evalInfo ei;
evalData e;

int popcount(U64 bb) {
    return __builtin_popcountll(bb);
}

int pawns_on_same_color_squares(const S_BOARD *pos, int c, int s) {
    return popcount(pos->pawns[c] & (WHITE_SQUARES & SQ64(s)) ? WHITE_SQUARES : BLACK_SQUARES);
}

bool opposite_colors(int s1, int s2) {
    return (bool)((WHITE_SQUARES & SQ64(s1)) != (WHITE_SQUARES & SQ64(s2)));
}

bool opposite_bishops(const S_BOARD *pos) {
    return  (   pos->pceNum[wB] == 1
             && pos->pceNum[bB] == 1
             && opposite_colors(pos->pList[wB][0], pos->pList[bB][0]));
}

int getTropism(const int sq1, const int sq2) {
    return 7 - (abs(FilesBrd[(sq1)] - FilesBrd[(sq2)]) + abs(RanksBrd[(sq1)] - RanksBrd[(sq2)]));
}

int king_proximity(const int c, const int s, const S_BOARD *pos) {
    return MIN(SquareDistance[pos->KingSq[c]][s], 5);
};

int distanceBetween(int s1, int s2) {
    return SquareDistance[s1][s2];
}

int distanceByFile(int s1, int s2) {
    return FileDistance[s1][s2];
}

int distanceByRank(int s1, int s2) {
    return RankDistance[s1][s2];
}

int map_to_queenside(const int f) {
    return MIN(f, FILE_H - f);
}

int clamp(const int v, const int lo, const int hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

int isPiece(const int color, const int piece, const int sq, const S_BOARD *pos) {
    return ( (pos->pieces[sq] == piece) && (PieceCol[pos->pieces[sq]] == color) );
}

int REL_SQ(const int sq120, const int side) {
    return side == WHITE ? sq120 : Mirror120[SQ64(sq120)];
}

int file_of(int s) {
    ASSERT(0 <= sq && sq < 64);
    return s % 8;
}

int rank_of(int s) {
    ASSERT(0 <= sq && sq < 64);
    return s / 8;
}

int relativeRank(int colour, int sq) {
    ASSERT(0 <= colour && colour < BOTH);
    ASSERT(0 <= sq && sq < 64);
    return colour == WHITE ? rank_of(sq) : 7 - rank_of(sq);
}

int relativeSquare32(int c, int sq) {
    ASSERT(0 <= c && c < BOTH);
    ASSERT(0 <= sq && sq < 64);
    return 4 * relativeRank(c, sq) + map_to_queenside(file_of(sq));
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

    if (opposite_bishops(pos)) {

        if (   (!pos->pceNum[wN] && !pos->pceNum[bN])
            || (!pos->pceNum[wR] && !pos->pceNum[bR]) 
            || (!pos->pceNum[wQ] && !pos->pceNum[bQ])) {
            return SCALE_OCB_BISHOPS_ONLY;
        }

        if ((   (!pos->pceNum[wR] && !pos->pceNum[bR])
             || (!pos->pceNum[wQ] && !pos->pceNum[bQ]))
            && (pos->pceNum[wN] == 1 && pos->pceNum[bN] == 1)) {
            return SCALE_OCB_ONE_KNIGHT;
        }

        if ((   (!pos->pceNum[wN] && !pos->pceNum[bN])
             || (!pos->pceNum[wQ] && !pos->pceNum[bQ]))
            && (pos->pceNum[wR] == 1 && pos->pceNum[bR] == 1)) {
            return SCALE_OCB_ONE_ROOK;
        }
    }

    return SCALE_NORMAL;
}

#define S(mg, eg) (makeScore((mg), (eg)))

int PieceValPhases[13] = { S( 0, 0), S( 70, 118), S( 433, 479), S( 459, 508), S( 714, 763), S(1401,1488), 
                           S( 0, 0), S( 70, 118), S( 433, 479), S( 459, 508), S( 714, 763), S(1401,1488), S( 0, 0)  };

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

const int RookOpen = S(  22,  11);
const int RookSemi = S(  10,   3);
const int RookSeventh = S(   8,  16);

const int QueenPreDeveloped = S(  -2,  -2);
const int PawnLessFlank = S(  -8, -44);

#undef S

int Pawns(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, att = 0, w, R, Su, Up, bonus, support, pawnbrothers;
    int sq, blockSq, t_sq, dir, index;
    U64 opposed, flag;

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    for(index = 0; index < NumDir[pce]; ++index) {
        dir = PceDir[pce][index];
        t_sq = sq + dir;
        if(!SQOFFBOARD(t_sq)) {
            if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
                att++;
            }    
        }
    }
    if(att) {
        ei.attCnt[side] += att; 
    }

    R  = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);
    Su = (side == WHITE ? -10 :  10);
    Up = (side == WHITE ?  10 : -10);

    opposed = (FileBBMask[FilesBrd[sq]] & pos->pawns[side^1]);
    support = (pos->pawn_ctrl[side][sq]);
    pawnbrothers = (pos->pieces[sq-1] == pce || pos->pieces[sq+1] == pce);

    //printf("opposed %d\n", (bool)(opposed) );
    //printf("support %d\n", (bool)(support) );
    //printf("pawnbrothers %d\n",(bool)(pawnbrothers) );

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
        ei.passedCnt++;
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

            bonus = (bonus * 100) / 410;
            score += makeScore(0, bonus);
        }
        /*if (!PassedPawnMasks[side][SQ64(sq + Up)
        || (pos->pawns[BOTH] & (SQ64(sq + Up)))) {
            bonus = bonus / 2;
        }*/  
    }

    if(support || pawnbrothers) {
        int i =  Connected[R] * (2 + (bool)(pawnbrothers) - (bool)(opposed))
                + 21 * pos->pawn_ctrl[side][sq];
        i = (i * 100) / 1220;
        //printf("supportCount %d v %d v eg %d R %d\n",pos->pawn_ctrl[side][sq], i, i * (R - 2) / 4, R);
        score += makeScore(i, i * (R - 2) / 4);
    }
    //ei.pawns[side] += score;

    return score;
}

int Knights(const S_BOARD *pos, int side, int pce, int pceNum) {
    
    int score = 0, att = 0, mobility = 0, tropism;
    int sq, t_sq, dir, index, R, defended, Count, P1, P2;

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
                
            if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
                att++;
            }    
        }
    }

    tropism = getTropism(sq, pos->KingSq[side^1]);
    score += makeScore(3 * tropism, 3 * tropism);
    ei.Mob[side] += KnightMobility[mobility];

    Count = distanceBetween(sq, pos->KingSq[side]);
    P1 = (7 * Count) * 100 / 410;
    P2 = (8 * Count) * 100 / 410;
    score += makeScore(-P1, -P2);

    if(att) {
        ei.attCnt[side] += att;
        ei.attckersCnt[side] += 1;
        ei.attWeight[side] += Weight[pce];
    }
    //ei.knights[side] += score;

    return score;   
}

int Bishops(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, att = 0, mobility = 0, tropism;
    int sq, t_sq, dir, index, R, defended, Count, P1, P2;

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
                if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
                    att++;
                }
            } else { // non empty sq
                if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
                    if(!pos->pawn_ctrl[side^1][t_sq]) {
                        mobility++;
                    }
                    if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
                        att++;
                    }
                }
                break;
            } 
            t_sq += dir;   
        }
    }

    tropism = getTropism(sq, pos->KingSq[side^1]);
    score += makeScore(2 * tropism, 1 * tropism);
    ei.Mob[side] += BishopMobility[mobility];

    if(mobility <= 3) {

        Count = pawns_on_same_color_squares(pos,side,sq);
        P1 = (3 * Count) * 100 / 310;
        P2 = (7 * Count) * 100 / 310;

        score += makeScore(-P1, -P2);
    } 

    Count = distanceBetween(sq, pos->KingSq[side]);
    P1 = (7 * Count) * 100 / 410;
    P2 = (8 * Count) * 100 / 410;

    score += makeScore(-P1, -P2);
  
    if(att) {
        ei.attCnt[side] += att;
        ei.attckersCnt[side] += 1;
        ei.attWeight[side] += Weight[pce];
    }
    //ei.bishops[side] += score;

    return score;
}

int Rooks(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, att = 0, mobility = 0, tropism;
    int sq, t_sq, dir, index, R, KR;

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    R  = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);
    KR = (side == WHITE ? RanksBrd[pos->KingSq[side^1]] : 7 - RanksBrd[pos->KingSq[side^1]]);

    for(index = 0; index < NumDir[pce]; ++index) {
        dir = PceDir[pce][index];
        t_sq = sq + dir;
        while(!SQOFFBOARD(t_sq)) {

            if(pos->pieces[t_sq] == EMPTY) {
                if(!pos->pawn_ctrl[side^1][t_sq]) {
                    mobility++;
                }
                if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
                    att++;
                }
            } else { // non empty sq
                if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
                    if(!pos->pawn_ctrl[side^1][t_sq]) {
                        mobility++;
                    }
                    if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
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
    score += makeScore(2 * tropism, 1 * tropism);
    ei.Mob[side] += RookMobility[mobility];

    if (att) {
        ei.attCnt[side] += att;
        ei.attckersCnt[side] += 1;
        ei.attWeight[side] += Weight[pce];
    }
    //ei.rooks[side] += score;

    return score;
}

int Queens(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, att = 0, mobility = 0, tropism;
    int sq, t_sq, dir, index, R, Knight, Bishop;

    Knight = (side == WHITE ? wN : bN);
    Bishop = (side == WHITE ? wB : bB);

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    R = (side == WHITE ? RanksBrd[sq] : 7 - RanksBrd[sq]);

    for(index = 0; index < NumDir[pce]; ++index) {
        dir = PceDir[pce][index];
        t_sq = sq + dir;
        while(!SQOFFBOARD(t_sq)) {

            if(pos->pieces[t_sq] == EMPTY) {
                if(!pos->pawn_ctrl[side^1][t_sq]) {
                    mobility++;
                }
                if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
                    att++;
                }
            } else { // non empty sq
                if(PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece
                    if(!pos->pawn_ctrl[side^1][t_sq]) {
                        mobility++;
                    }
                    if ( e.sqNearK[side^1] [pos->KingSq[side^1] ] [t_sq] ) {
                        att++;
                    }
                }
                break;
            } 
            t_sq += dir;   
        }
    }

    if (R > RANK_2) {
        if(isPiece(side, Knight, REL_SQ(side,B1), pos)) {
            score += QueenPreDeveloped;
        }
        if(isPiece(side, Bishop, REL_SQ(side,C1), pos)) {
            score += QueenPreDeveloped;
        }
        if(isPiece(side, Bishop, REL_SQ(side,F1), pos)) {
            score += QueenPreDeveloped;
        }
        if(isPiece(side, Knight, REL_SQ(side,G1), pos)) {
            score += QueenPreDeveloped;
        }
    }

    tropism = getTropism(sq, pos->KingSq[side^1]);
    score += makeScore(2 * tropism, 4 * tropism);
    ei.Mob[side] += QueenMobility[mobility];

    if (att) {
        ei.attCnt[side] += att;
        ei.attckersCnt[side] += 1;
        ei.attWeight[side] += Weight[pce];
    }
    //ei.queens[side] += score;

    return score;
}

int hypotheticalShelter(const S_BOARD *pos, int side, int KingSq) {

    int score = 0, d, center, file, ourRank, theirRank;
    U64 ours, theirs;

    center = clamp(FilesBrd[KingSq], FILE_B, FILE_G);
    for (file = center - 1; file <= center + 1; ++file) {

        ours = (pos->pawns[side] & FileBBMask[file]);
        ourRank = ours ? relativeRank(side, frontmost(side^1, ours)) : 0;

        theirs = (pos->pawns[side^1] & FileBBMask[file]);
        theirRank = theirs ? relativeRank(side, frontmost(side^1, theirs)) : 0;

        d = map_to_queenside(file);
        score  = makeScore(1, 1);
        score += makeScore((ShelterStrength[d][ourRank] * 100) / 410, 0);

        if (ourRank && (ourRank == theirRank - 1)) {
            score -= BlockedStorm * (int)(theirRank == RANK_3);
        } else {
            score -= makeScore((UnblockedStorm[d][theirRank] * 100) / 410, 0);
        }
    }

    return score;
}

int evaluateShelter(const S_BOARD *pos, int side) {

    int shelter = hypotheticalShelter(pos, side, pos->KingSq[side]);

    if (side == WHITE) {

        if (pos->castlePerm & WKCA)
            shelter = MAX(shelter, hypotheticalShelter(pos, side, G1));

        if (pos->castlePerm & WQCA) 
            shelter = MAX(shelter, hypotheticalShelter(pos, side, C1));
    } else {

        if (pos->castlePerm & BKCA)
            shelter = MAX(shelter, hypotheticalShelter(pos, side, G8));

        if (pos->castlePerm & BQCA) 
            shelter = MAX(shelter, hypotheticalShelter(pos, side, C8));
    }
    ei.pkeval[side] = shelter;

    return shelter;
}

int evaluateKings(const S_BOARD *pos, int side) {

    int score = 0, count, enemyQueen;

    enemyQueen = (side == WHITE ? bQ : wQ);

    if (!(pos->pawns[BOTH] & KingFlank[FilesBrd[pos->KingSq[side]]])) {
        score += PawnLessFlank;
    }

    score += evaluateShelter(pos, side);

    if (ei.attckersCnt[side^1] > 1 - pos->pceNum[enemyQueen]) {
    
        float scaledAttackCounts = 9.0 * ei.attCnt[side^1] / popcount(ei.kingAreas[side]);

        count =       ei.attckersCnt[side^1] * ei.attWeight[side^1]
               + 32 * scaledAttackCounts
               +      mgScore(ei.Mob[side^1] - ei.Mob[side]) / 4
               -  6 * mgScore(ei.pkeval[side]) / 8
               - 17 ;

        if(count > 0) {
            score -= makeScore(count * count / 720, count / 18);
            //ei.KingDanger[side^1] = makeScore(count * count / 720, count / 18);
        }
    }

    return score;
}

int evaluatePieces(const S_BOARD *pos) {
    int pce, pceNum, score = 0;
    pce = wP;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score += Pawns(pos, WHITE, pce, pceNum);
    }
    pce = bP;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score -= Pawns(pos, BLACK, pce, pceNum);
    }
    pce = wN;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score += Knights(pos, WHITE, pce, pceNum);
    }
    pce = bN;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score -= Knights(pos, BLACK, pce, pceNum);
    }
    pce = wB;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score += Bishops(pos, WHITE, pce, pceNum);
    }
    pce = bB;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score -= Bishops(pos, BLACK, pce, pceNum);
    }
    pce = wR;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score += Rooks(pos, WHITE, pce, pceNum);
    }
    pce = bR;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score -= Rooks(pos, BLACK, pce, pceNum);
    }
    pce = wQ;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score += Queens(pos, WHITE, pce, pceNum);
    }
    pce = bQ;
    for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
        score -= Queens(pos, BLACK, pce, pceNum);
    }
    score += evaluateKings(pos, WHITE);
    score -= evaluateKings(pos, BLACK);

    return score;
}

int evaluateComplexity(const S_BOARD *pos, int score) {

    int complexity, outflanking, pawnsOnBothFlanks, pawnEndgame, almostUnwinnable, mg, eg, u, v;

    mg = mgScore(score);
    eg = egScore(score);

    outflanking =  distanceByFile(pos->KingSq[WHITE], pos->KingSq[BLACK])
                 - distanceByRank(pos->KingSq[WHITE], pos->KingSq[BLACK]);

    pawnsOnBothFlanks =    (pos->pawns[BOTH] & KING_FLANK )
                        && (pos->pawns[BOTH] & QUEEN_FLANK);

    pawnEndgame = !(pos->pceNum[wN] && pos->pceNum[wB] && pos->pceNum[wR] && pos->pceNum[wQ]
                 && pos->pceNum[bN] && pos->pceNum[bB] && pos->pceNum[bR] && pos->pceNum[bQ]);

    almostUnwinnable =   !ei.passedCnt
                      &&  outflanking < 0
                      && !pawnsOnBothFlanks;

    complexity =   5 * ei.passedCnt
                +  7 * popcount(pos->pawns[BOTH])
                +  5 * outflanking
                + 13 * pawnsOnBothFlanks
                + 32 * pawnEndgame
                - 27 * almostUnwinnable
                - 60 ;

    u = ((mg > 0) - (mg < 0)) * MAX(MIN(complexity + 50, 0), -abs(mg));
    v = ((eg > 0) - (eg < 0)) * MAX(complexity, -abs(eg));

    //ei.Complexity = makeScore(u, v);

    return makeScore(u, v);
}

int imbalance(const int pieceCount[2][6], int side) {

    int bonus = 0, pt1, pt2;

    // Adaptation of polynomial material imbalance, by Tord Romstad
    for (pt1 = NO_PIECE_TYPE; pt1 <= QUEEN; ++pt1)
    {
        if (!pieceCount[side][pt1])
            continue;

        int v = 0;

        for (pt2 = NO_PIECE_TYPE; pt2 <= pt1; ++pt2) 
            v +=  QuadraticOurs[pt1][pt2] * pieceCount[side][pt2]
                + QuadraticTheirs[pt1][pt2] * pieceCount[side^1][pt2];
        
        bonus += pieceCount[side][pt1] * v;
    }
    //ei.imbalance[side] = makeScore(bonus / 16, bonus / 16);

    return makeScore(bonus / 16, bonus / 16);
}

int EndgameKXK(const S_BOARD *pos, int weakSide, int strongSide) {

    ASSERT(pos->material[weakSide] == PieceVal[wK]);

    int winnerKSq, loserKSq, result, Queen, Rook, Bishop, Knight;

    winnerKSq = pos->KingSq[strongSide];
    loserKSq  = pos->KingSq[weakSide];

    result =  pos->material[strongSide]
            + PushToEdges[SQ64(loserKSq)]
            + PushClose[distanceBetween(winnerKSq, loserKSq)];

    Queen  = (strongSide == WHITE ? wQ : bQ);
    Rook   = (strongSide == WHITE ? wR : bR);
    Bishop = (strongSide == WHITE ? wB : bB);
    Knight = (strongSide == WHITE ? wN : bN);

    if (pos->pceNum[Queen ]
    ||  pos->pceNum[Rook  ]
    || (pos->pceNum[Bishop] && pos->pceNum[Knight])
    || (   (SQ64(pos->pList[Bishop][0]) & ~BLACK_SQUARES)
        && (SQ64(pos->pList[Bishop][1]) &  BLACK_SQUARES)) )
        result = MIN(result + KNOWN_WIN, MATE_IN_MAX - 1);

    return strongSide == pos->side ? result : -result;
};

void blockedPiecesW(const S_BOARD *pos) {

    int side = WHITE;

    // central pawn blocked, bishop hard to develop
    if (pos->pieces[C1] == wB
    &&  pos->pieces[D2] == wP
    && pos->pieces[D3] != EMPTY) {
       ei.blockages[side] -= P_BLOCK_CENTRAL_PAWN;
    }

    if (pos->pieces[F1] == wB
    && pos->pieces[E2] == wP
    && pos->pieces[E3] != EMPTY) {
       ei.blockages[side] -= P_BLOCK_CENTRAL_PAWN;
    }

    // trapped knight
    if ( pos->pieces[A8] == wN
    &&  ( pos->pieces[A7] == bP || pos->pieces[C7] == bP) ) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A8;
    }

    if ( pos->pieces[H8] == wN
    && ( pos->pieces[H7] == bP || pos->pieces[F7] == bP)) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A8;
    }

    if (pos->pieces[A7] == wN
    &&  pos->pieces[A6] == bP
    &&  pos->pieces[B7] == bP) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A7;
    }

    if (pos->pieces[H7] == wN
    &&  pos->pieces[H6] == bP
    &&  pos->pieces[G7] == bP) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A7;
    }

     // knight blocking queenside pawns
    if (pos->pieces[C3] == wN
    &&  pos->pieces[C2] == wP
    && (pos->pieces[D4] == wP || pos->pieces[D2] == wP)
    &&  pos->pieces[E4] != wP) {
        ei.blockages[side] -= P_C3_KNIGHT;
    }

     // trapped bishop
    if (pos->pieces[A7] == wB
    &&  pos->pieces[B6] == bP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[H7] == wB
    &&  pos->pieces[G6] == bP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[B8] == wB
    &&  pos->pieces[C7] == bP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[G8] == wB
    &&  pos->pieces[F7] == bP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[A6] == wB
    &&  pos->pieces[B5] == bP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A6;
    }

    if (pos->pieces[H6] == wB
    &&  pos->pieces[G5] == bP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A6;
    }

     // bishop on initial square supporting castled king
    if (pos->pieces[F1] == wB
    &&  pos->pieces[G1] == wK) {
        ei.blockages[side] += RETURNING_BISHOP;
    }

    if (pos->pieces[C1] == wB
    &&  pos->pieces[B1] == wK) {
        ei.blockages[side] += RETURNING_BISHOP;
    }

    // uncastled king blocking own rook
    if ( (pos->pieces[F1] == wK || pos->pieces[G1] == wK)
    &&  (pos->pieces[H1] == wR || pos->pieces[G1] == wR)) {
    //&& (pos->pieces[F2] == wP || pos->pieces[G2] == wP || pos->pieces[H2] == wP)) {
        ei.blockages[side] -= P_KING_BLOCKS_ROOK;
    }

    if ( (pos->pieces[C1] == wK || pos->pieces[B1] == wK)
    &&  (pos->pieces[A1] == wR || pos->pieces[B1] == wR)) {
    //&& (pos->pieces[F2] == wP || pos->pieces[G2] == wP || pos->pieces[H2] == wP)) {
        ei.blockages[side] -= P_KING_BLOCKS_ROOK;
    }
}

void blockedPiecesB(const S_BOARD *pos) {

    int side = BLACK;

    // central pawn blocked, bishop hard to develop
    if (pos->pieces[C8] == bB
    &&  pos->pieces[D7] == bP
    && pos->pieces[D6] != EMPTY) {
       ei.blockages[side] -= P_BLOCK_CENTRAL_PAWN;
    }

    if (pos->pieces[F8] == bB
    && pos->pieces[E7] == bP
    && pos->pieces[E6] != EMPTY) {
       ei.blockages[side] -= P_BLOCK_CENTRAL_PAWN;
    }

    // trapped knight
    if ( pos->pieces[A1] == bN
    && ( pos->pieces[A2] == wP || pos->pieces[C2] == wP)) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A8;
    }

    if ( pos->pieces[H1] == bN
    && ( pos->pieces[H2] == wP || pos->pieces[F2] == wP)) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A8;
    }

    if (pos->pieces[A2] == bN
    &&  pos->pieces[A3] == wP
    &&  pos->pieces[B2] == wP) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A7;
    }

    if (pos->pieces[H2] == bN
    &&  pos->pieces[H3] == wP
    &&  pos->pieces[G2] == wP) {
        ei.blockages[side] -= P_KNIGHT_TRAPPED_A7;
    }

     // knight blocking queenside pawns
    if (pos->pieces[C6] == bN
    &&  pos->pieces[C7] == bP
    && (pos->pieces[D5] == bP || pos->pieces[D7] == bP)
    &&  pos->pieces[E5] != bP) {
        ei.blockages[side] -= P_C3_KNIGHT;
    }

     // trapped bishop
    if (pos->pieces[A2] == bB
    &&  pos->pieces[B3] == wP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[H2] == bB
    &&  pos->pieces[G3] == wP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[B1] == bB
    &&  pos->pieces[C2] == wP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[G1] == bB
    &&  pos->pieces[F2] == wP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A7;
    }

    if (pos->pieces[A3] == bB
    &&  pos->pieces[B4] == wP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A6;
    }

    if (pos->pieces[H3] == bB
    &&  pos->pieces[G4] == wP) {
        ei.blockages[side] -= P_BISHOP_TRAPPED_A6;
    }

     // bishop on initial square supporting castled king
    if (pos->pieces[F8] == bB
    &&  pos->pieces[G8] == bK) {
        ei.blockages[side] += RETURNING_BISHOP;
    }

    if (pos->pieces[C8] == bB
    &&  pos->pieces[B8] == bK) {
        ei.blockages[side] += RETURNING_BISHOP;
    }

    // uncastled king blocking own rook
    if ((pos->pieces[F8] == bK || pos->pieces[G8] == bK)
    && (pos->pieces[H8] == bR || pos->pieces[G8] == bR)) {
    //&& (pos->pieces[F7] == bP || pos->pieces[G7] == bP || pos->pieces[H7] == bP)) {
        ei.blockages[side] -= P_KING_BLOCKS_ROOK;
    }

    if ((pos->pieces[C8] == bK || pos->pieces[B8] == bK)
    && (pos->pieces[A8] == bR || pos->pieces[B8] == bR)) { 
    //&& (pos->pieces[F7] == bP || pos->pieces[G7] == bP || pos->pieces[H7] == bP)) {
        ei.blockages[side] -= P_KING_BLOCKS_ROOK;
    }
}

int EvalPosition(const S_BOARD *pos) {
    // setboard 8/3k3p/6p1/3nK1P1/8/8/7P/8 b - - 3 64
    // setboard r2q1rk1/p2b1p1p/1p1b2pQ/2p1pP2/1nPp4/1P1BP3/PB1P2PP/RN3RK1 w - - 1 16
    // setboard 8/6R1/2k5/6P1/8/8/4nP2/6K1 w - - 1 41 
    //int startTime = GetTimeMs();

    int score, phase, factor, stronger, weaker, PAWNST, PAWNWK;

    memset(&ei, 0, sizeof(evalInfo));

    ei.kingAreas[WHITE] = kingAreaMasks(WHITE, SQ64(pos->KingSq[WHITE]));
    ei.kingAreas[BLACK] = kingAreaMasks(BLACK, SQ64(pos->KingSq[BLACK]));

    const int pieceCount[2][6] = {
        { pos->pceNum[wB] > 1, pos->pceNum[wP], pos->pceNum[wN],
          pos->pceNum[wB]    , pos->pceNum[wR], pos->pceNum[wQ] },
        { pos->pceNum[bB] > 1, pos->pceNum[bP], pos->pceNum[bN],
          pos->pceNum[bB]    , pos->pceNum[bR], pos->pceNum[bQ] } 
    };

    score  = (pos->mPhases[WHITE] - pos->mPhases[BLACK]);
    score += (pos->PSQT[WHITE] - pos->PSQT[BLACK]);
    score += (ei.Mob[WHITE] - ei.Mob[BLACK]);
    score += (imbalance(pieceCount, WHITE) - imbalance(pieceCount, BLACK));
    score +=  evaluatePieces(pos);
    score +=  evaluateComplexity(pos, score);

    blockedPiecesW(pos);
    blockedPiecesB(pos);

    phase = 24 - 4 * (pos->pceNum[wQ] + pos->pceNum[bQ])
               - 2 * (pos->pceNum[wR] + pos->pceNum[bR])
               - 1 * (pos->pceNum[wN] + pos->pceNum[bN] + pos->pceNum[wB] + pos->pceNum[bB]);

    phase = (phase * 256 + 12) / 24;

    factor = evaluateScaleFactor(pos);

    score = (mgScore(score) * (256 - phase)
          +  egScore(score) * phase * factor / SCALE_NORMAL) / 256;

    score += (ei.blockages[WHITE] - ei.blockages[BLACK]);

    score += pos->side == WHITE ? TEMPO : -TEMPO;

    stronger = (score > 0 ? WHITE : BLACK);
    weaker   = (score > 0 ? BLACK : WHITE);

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
    
    if (pos->material[weaker] == PieceVal[wK]) {
        return EndgameKXK(pos, weaker, stronger);
    }
    
    //printf("Elapsed %d\n",GetTimeMs() - startTime);
    return pos->side == WHITE ? score : -score;     
}

void printEvalFactor( int WMG, int WEG, int BMG, int BEG ) {
    printf("| %4d  %4d  | %4d  %4d  | %4d  %4d \n",WMG, WEG, BMG, BEG, WMG - BMG, WEG - BEG );
}

void printEval(const S_BOARD *pos) {

    int v = EvalPosition(pos);
    v = pos->side == WHITE ? v : -v;

    printf("\n");
    printf("      Term    |    White    |    Black    |    Total   \n");
    printf("              |   MG    EG  |   MG    EG  |   MG    EG \n");
    printf("--------------+-------------+-------------+------------\n");
    printf("     Material "); printEvalFactor( mgScore(pos->mPhases[WHITE]),egScore(pos->mPhases[WHITE]),mgScore(pos->mPhases[BLACK]),egScore(pos->mPhases[BLACK]));
    printf("    Imbalance "); printEvalFactor( mgScore(ei.imbalance[WHITE]),egScore(ei.imbalance[WHITE]),mgScore(ei.imbalance[BLACK]),egScore(ei.imbalance[BLACK]));
    printf("         PSQT "); printEvalFactor( mgScore(pos->PSQT[WHITE]),egScore(pos->PSQT[WHITE]),mgScore(pos->PSQT[BLACK]),egScore(pos->PSQT[BLACK]));
    printf("        Pawns "); printEvalFactor( mgScore(ei.pawns[WHITE]),egScore(ei.pawns[WHITE]),mgScore(ei.pawns[BLACK]),egScore(ei.pawns[BLACK]));
    printf("      Knights "); printEvalFactor( mgScore(ei.knights[WHITE]),egScore(ei.knights[WHITE]),mgScore(ei.knights[BLACK]),egScore(ei.knights[BLACK]));
    printf("      Bishops "); printEvalFactor( mgScore(ei.bishops[WHITE]),egScore(ei.bishops[WHITE]),mgScore(ei.bishops[BLACK]),egScore(ei.bishops[BLACK]));
    printf("        Rooks "); printEvalFactor( mgScore(ei.rooks[WHITE]),egScore(ei.rooks[WHITE]),mgScore(ei.rooks[BLACK]),egScore(ei.rooks[BLACK]));
    printf("       Queens "); printEvalFactor( mgScore(ei.queens[WHITE]),egScore(ei.queens[WHITE]),mgScore(ei.queens[BLACK]),egScore(ei.queens[BLACK]));
    printf("     Mobility "); printEvalFactor( mgScore(ei.Mob[WHITE]),egScore(ei.Mob[WHITE]),mgScore(ei.Mob[BLACK]),egScore(ei.Mob[BLACK]));
    printf("  King safety "); printEvalFactor( mgScore(ei.KingDanger[WHITE]),egScore(ei.KingDanger[WHITE]),mgScore(ei.KingDanger[BLACK]),egScore(ei.KingDanger[BLACK]));
    printf("  King shield "); printEvalFactor( mgScore(ei.pkeval[WHITE]),egScore(ei.pkeval[WHITE]),mgScore(ei.pkeval[BLACK]),egScore(ei.pkeval[BLACK]));
    printf("   Initiative "); printf("| ----  ----  | ----  ----  | %4d  %4d \n", mgScore(ei.Complexity), egScore(ei.Complexity));
    printf("--------------+-------------+-------------+------------\n");
    printf("        Total "); printf("| ----  ----  | ----  ----  |    %4d     \n", v);
    printf("\n");
}

void setPcsq32() {

    for (int i = 0; i < 120; ++i) {

        if(!SQOFFBOARD(i)) {
            /* set the piece/square tables for each piece type */

            const int SQ64 = SQ64(i);
            const int w32 = relativeSquare32(WHITE, SQ64);
            const int b32 = relativeSquare32(BLACK, SQ64);

            e.PSQT[wP][i] =    PawnPSQT32[w32]; 
            e.PSQT[bP][i] =    PawnPSQT32[b32];
            e.PSQT[wN][i] =  KnightPSQT32[w32];
            e.PSQT[bN][i] =  KnightPSQT32[b32];
            e.PSQT[wB][i] =  BishopPSQT32[w32];
            e.PSQT[bB][i] =  BishopPSQT32[b32];
            e.PSQT[wR][i] =    RookPSQT32[w32];
            e.PSQT[bR][i] =    RookPSQT32[b32];
            e.PSQT[wQ][i] =   QueenPSQT32[w32];
            e.PSQT[bQ][i] =   QueenPSQT32[b32];
            e.PSQT[wK][i] =    KingPSQT32[w32];
            e.PSQT[bK][i] =    KingPSQT32[b32];
        }        
    }
}