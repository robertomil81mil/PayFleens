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

#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "defs.h"
#include "endgame.h"
#include "evaluate.h"
#include "init.h"
#include "validate.h"

EvalTrace T, EmptyTrace;
evalInfo ei;
evalData e;

int getTropism(const int s1, const int s2) {
    return 7 - (distanceByFile(s1, s2) + distanceByRank(s1, s2));
}

int king_proximity(const int c, const int s, const S_BOARD *pos) {
    return MIN(distanceBetween(pos->KingSq[c], s), 5);
};

bool opposite_bishops(const S_BOARD *pos) {
    return  (   pos->pceNum[wB] == 1
             && pos->pceNum[bB] == 1
             && opposite_colors(SQ64(pos->pList[wB][0]), SQ64(pos->pList[bB][0])));
}

int pawns_on_same_color_squares(const S_BOARD *pos, const int colour, const int sq) {
    ASSERT(0 <= sq && sq < 120);
    return popcount(pos->pawns[colour] & (testBit(WHITE_SQUARES, SQ64(sq))) ? WHITE_SQUARES : BLACK_SQUARES);
}

int isPiece(const int piece, const int sq, const S_BOARD *pos) {
    return (pos->pieces[sq] == piece);
}

int NonSlideMob(const S_BOARD *pos, int side, int pce, int sq) {

    int index, t_sq, ksq, mobility = 0, att = 0;

    ksq = FR2SQ(clamp(FilesBrd[pos->KingSq[!side]], FILE_B, FILE_G),
                clamp(RanksBrd[pos->KingSq[!side]], RANK_2, RANK_7));

    for (index = 0; index < NumDir[pce]; ++index) {
        t_sq = sq + PceDir[pce][index];

        if (  !SQOFFBOARD(t_sq)
            && PieceCol[pos->pieces[t_sq]] != side) {

            if (!pos->pawn_ctrl[!side][t_sq])
                mobility++;

            if (e.sqNearK[ksq][t_sq])
                att++;
        }
    }

    if (att) {
        ei.attCnt[side] += att;
        ei.attckersCnt[side] += 1;
        ei.attWeight[side] += Weight[pce];
    }

    return mobility;
}

int SlideMob(const S_BOARD *pos, int side, int pce, int sq) {

    int index, t_sq, ksq, mobility = 0, att = 0;

    ksq = FR2SQ(clamp(FilesBrd[pos->KingSq[!side]], FILE_B, FILE_G),
                clamp(RanksBrd[pos->KingSq[!side]], RANK_2, RANK_7));

    for (index = 0; index < NumDir[pce]; ++index) {
        t_sq = sq + PceDir[pce][index];

        while (!SQOFFBOARD(t_sq)) {

            if (pos->pieces[t_sq] == EMPTY) {

                if (!pos->pawn_ctrl[!side][t_sq])
                    mobility++;

                if (e.sqNearK[ksq][t_sq])
                    att++;

            } else { // non empty sq

                if (PieceCol[pos->pieces[t_sq]] != side) { // opponent's piece

                    if (!pos->pawn_ctrl[!side][t_sq])
                        mobility++;

                    if (e.sqNearK[ksq][t_sq])
                        att++;
                }
                break;
            }
            t_sq += PceDir[pce][index];   
        }
    }

    if (att) {
        ei.attCnt[side] += att;
        ei.attckersCnt[side] += 1;
        ei.attWeight[side] += Weight[pce];
    }

    return mobility;
}

#define S(mg, eg) (makeScore((mg), (eg)))

/* Material Value Evaluation Terms */

const int PawnValue   = S(  68, 117);
const int KnightValue = S( 421, 443);
const int BishopValue = S( 448, 492);
const int RookValue   = S( 707, 746);
const int QueenValue  = S(1391,1465);
const int KingValue   = S(   0,   0);

int PieceValPhases[13] = { S( 0, 0), S( 68, 117), S( 421, 443), S( 448, 492), S( 707, 746), S(1391,1465), 
                           S( 0, 0), S( 68, 117), S( 421, 443), S( 448, 492), S( 707, 746), S(1391,1465), S( 0, 0) };

/* Piece Square Evaluation Terms */

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(  -8,   1), S(   1,   3), S(  -3,   9), S(  11,   3),
    S( -14,  -1), S( -10,   2), S(   2,  -3), S(  11,   0),
    S(  -5,   5), S(   1,   4), S(  19, -11), S(  30, -17),
    S(  13,  11), S(   4,   7), S(   4,  -7), S(  18, -24),
    S(   1,  26), S( -12,  20), S(  -5,  10), S(  -2,   5),
    S(  -9,   2), S(   6,  -3), S( -11,   7), S(  -5,  17),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -84, -47), S( -35, -44), S( -41, -26), S( -34, -11),
    S( -36, -37), S( -24, -28), S( -18, -17), S(  -4,  -4),
    S( -33, -21), S(  -6, -16), S(   1,  -6), S(  10,  14),
    S( -22, -15), S(   5,  -2), S(  16,  10), S(  20,  18),
    S( -16, -19), S(   3,  -5), S(  22,   8), S(  30,  23),
    S(  -6, -27), S(  15, -22), S(  33,  -2), S(  25,  12),
    S( -33, -34), S( -14, -22), S(   3, -23), S(  17,  10),
    S( -99, -51), S( -38, -42), S( -26, -27), S( -13,  -9),
};

const int BishopPSQT32[32] = {
    S( -29, -29), S(  -1, -11), S(  -9, -18), S( -13,  -3),
    S(  -7, -20), S(  16, -16), S(   6, -11), S(   0,   1),
    S(  -7, -10), S(   7,  -5), S(   3,  -1), S(   4,   8),
    S(  -5, -14), S(   3,  -6), S(   8,  -2), S(  22,   7),
    S(  -1,  -5), S(   7,  -1), S(  10,  -5), S(  20,  10),
    S(  -5,  -9), S(   3,   2), S(   1,   4), S(   3,   1),
    S(  -7, -11), S(  -5,  -4), S(   0,   1), S(   1,   1),
    S( -24, -23), S(   1, -18), S(  -8, -21), S( -12, -11),
};

const int RookPSQT32[32] = {
    S( -19,  -1), S(  -4,  -4), S(  12, -10), S(  16, -13),
    S( -31,  -5), S(  -7,  -9), S(  -7,  -2), S(   5,  -5),
    S( -22,  -1), S(  -9,  -6), S(   0,  -5), S(  -2,  -8),
    S( -18,  -4), S(  -2,   0), S(   0,  -4), S(  -4,   4),
    S( -15,  -1), S(  -9,   0), S(   2,   9), S(   9,   2),
    S(  -9,   2), S(   4,   2), S(   1,  -2), S(   6,   8),
    S(  -3,  -1), S(   3,   2), S(   8,   9), S(  11,  -3),
    S(  -7,   7), S(  -6,   2), S(  -1,  14), S(   6,  11),
};

const int QueenPSQT32[32] = {
    S(  -1, -33), S(   3, -27), S(   2, -25), S(  11, -13),
    S(  -4, -26), S(   1, -14), S(  12, -12), S(  14,  -4),
    S(   3, -17), S(   9, -13), S(  10,  -2), S(   8,   3),
    S(   4, -11), S(   0,   0), S(   4,   7), S(   1,  13),
    S(  -2, -13), S(  -3,  -1), S(  -1,   5), S(   1,  12),
    S(   6, -18), S(   5,  -9), S(   1,  -5), S(   0,   0),
    S(  -8, -26), S( -21, -14), S(  -1, -14), S(   0,  -4),
    S(  -5, -39), S(  -2, -26), S(   0, -20), S(  -2, -21),
};

const int KingPSQT32[32] = {
    S( 137, -14), S( 163,  12), S( 114,  36), S(  71,  32),
    S( 139,  15), S( 143,  46), S(  96,  63), S(  78,  61),
    S(  87,  40), S( 120,  66), S(  78,  80), S(  57,  85),
    S(  74,  48), S(  90,  78), S(  68,  84), S(  47,  82),
    S(  72,  48), S(  88,  90), S(  52, 101), S(  34,  95),
    S(  58,  43), S(  71,  89), S(  41,  93), S(  16,  87),
    S(  41,  19), S(  55,  52), S(  30,  53), S(  16,  59),
    S(  26,   1), S(  41,  24), S(  21,  33), S(  -1,  32),
};

/* Imbalance Evaluation Terms */

const int QuadraticOurs[6][6] = {
   {S( 798, 800), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(  18,  21), S(  21,  24), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(  15,  13), S( 148, 147), S( -36, -40), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(   4,   9), S(  72,  61), S(   3,   5), S(  -7,  -7), S(   0,   0), S(   0,   0)},
   {S(  -8, -12), S(  -2,  22), S(  23,   2), S(  62,  37), S(-110,-149), S(   0,   0)},
   {S(-103,-103), S(  13,  35), S(  43,  56), S(  58,  68), S( -89, -95), S( -15, -28)},
};

const int QuadraticTheirs[6][6] = {
   {S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(  15,  13), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(   2,   3), S(  42,  45), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(  37,  32), S(  36,  51), S(  23,   9), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(  25,  21), S(  19,  49), S(  14,  32), S( -10,   8), S(   0,   0), S(   0,   0)},
   {S(  51,  53), S(  69,  75), S(  -3, -18), S(  89,  82), S( 144, 146), S(   0,   0)},
};

/* Pawn Evaluation Terms */

const int PawnDoubled = S(   6, -51);

const int PawnIsolated[2] = { S(  -5,  -7), S( -12, -31) };

const int PawnBackward[2] = { S(   1,  -4), S( -17, -20) };

const int PawnPassed[RANK_NB] = {
    S(   0,   0), S(   5,  23), S(  11,  25), S(   4,  31),
    S(  31,  71), S( 137, 169), S( 268, 277), S(   0,   0),
};

const int PawnPassedConnected[RANK_NB] = {
    S(   0,   0), S(   6,  -1), S(  -1,   0), S(  -3,   9),
    S(   7,  21), S(  40,  43), S(  85,  84), S(   0,   0),
};

const int PawnConnected[RANK_NB] = {
    S(   0,   0), S(  10,   7), S(  15,   8), S(  17,  13),
    S(  35,  31), S(  45,  53), S(  84,  83), S(   0,   0),
};

const int PawnSupport = S(  30,  19);

const int KP_1 = S(   0,   5);
const int KP_2 = S(   0,  -3);
const int KP_3 = S(   0,  -1);

const int Connected[8] = { 0, 10, 15, 17, 35, 45, 84, 0};

/* Knight Evaluation Terms */

const int KnightOutpost[2] = { S(  17, -21), S(  37,   4) };

const int KnightTropism = S(   4,   2);

const int KnightDefender = S( -10,  -7);

const int KnightMobility[9] = {
    S( -35, -24), S( -24, -24), S( -14, -20), S(  -9,  -8),
    S(   1,   0), S(   7,   8), S(  16,   9), S(  23,  11),
    S(  23,  10),
};

/* Bishop Evaluation Terms */

const int BishopOutpost[2] = { S(  14,  -8), S(  34,   0) };

const int BishopTropism = S(   1,   1);

const int BishopRammedPawns = S(  -3,  -7);

const int BishopDefender = S( -10,  -7);

const int BishopMobility[14] = {
    S( -30, -21), S( -17, -16), S(  -5, -11), S(  -2,  -6),
    S( -16, -12), S(  -8,  -7), S(  -1,  -3), S(   2,   2),
    S(   3,   7), S(   7,  10), S(  18,   9), S(  20,  16),
    S(  18,  17), S(  20,  20),
};

/* Rook Evaluation Terms */

const int RookOpen = S(  30,  11);

const int RookSemi = S(  13,   5);

const int RookOnSeventh = S(   9,  27);

const int RookTropism = S(   3,   0);

const int RookMobility[15] = {
    S( -33, -41), S( -22, -38), S( -19, -20), S( -12, -24),
    S( -10, -13), S(  -6,  -3), S(  -2,   2), S(   4,   3),
    S(   9,  10), S(  12,  15), S(  14,  21), S(  19,  26),
    S(  24,  30), S(  20,  35), S(  25,  38),
};

/* Queen Evaluation Terms */

const int QueenPreDeveloped = S(  -9,  -6);

const int QueenTropism = S(   5,   2);

const int QueenMobility[28] = {
    S( -28, -24), S( -27, -23), S( -22, -20), S( -19, -19),
    S( -12, -21), S(  -8, -20), S(  -8, -15), S(  -5, -14),
    S(  -2, -15), S(   1, -10), S(   2,  -6), S(   6,  -3),
    S(   2,   1), S(   7,   2), S(   7,   7), S(   8,   9),
    S(   8,  12), S(  10,  10), S(  12,  16), S(  11,  17),
    S(   9,  16), S(  10,  16), S(   8,  17), S(  10,  21),
    S(  11,  22), S(  13,  27), S(  14,  28), S(  15,  30),
};

/* King Evaluation Terms */

const int KingPawnLessFlank = S( -16, -48);

const int BlockedStorm = S( -42, -26);

const int ShelterStrength[FILE_NB/2][RANK_NB] = {
   {S( -11,   5), S(  24, -15), S(  28,  -6), S(   9,   0),
    S(   8,  -2), S(   4,  -2), S(   5,   0), S(   0,   0)},
   {S( -23,   2), S(  36,  -7), S(  21,  -5), S( -15,   2),
    S( -14,   4), S(  -3,   0), S( -16,   0), S(   0,   0)},
   {S( -12,  -3), S(  35,  -5), S(   1,   4), S(  -7,   1),
    S(   1,   0), S(   4,  -3), S( -10,  -3), S(   0,   0)},
   {S( -22,   0), S(   0,  -2), S( -11,   2), S( -16,   5),
    S(  -8,   1), S( -18,  -2), S( -40,   2), S(   0,   0)},
};

const int UnblockedStorm[FILE_NB/2][RANK_NB] = {
   {S( -32,   0), S(  72,   1), S(  52,   2), S( -18,   3),
    S( -13,   2), S(  -5,  -7), S( -12,  -3), S(   0,   0)},
   {S(  -5,  -1), S(   4,   1), S( -28,  -2), S( -12,  10),
    S(  -5,   8), S(   9,   0), S(  -1,  -2), S(   0,   0)},
   {S(   5,   1), S( -13,   1), S( -40,   6), S(  -9,   7),
    S(   0,   6), S(   4,   3), S(   0,   3), S(   0,   0)},
   {S(   2,   8), S(   1,   0), S( -22,  -2), S(  -6,   4),
    S(  -2,   4), S(   4,   6), S(   3,   1), S(   0,   0)},
};

/* Complexity Evaluation Terms */

const int ComplexityPassedPawns = S(   0,   6);
const int ComplexityTotalPawns  = S(   0,  10);
const int ComplexityOutflanking = S(   0,   7);
const int ComplexityPawnFlanks  = S(   0,  35);
const int ComplexityPawnEndgame = S(   0,  48);
const int ComplexityUnwinnable  = S(   0, -14);
const int ComplexityAdjustment  = S(   0,-117);

#undef S

int Pawns(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, bonus, support, pawnbrothers;
    int sq, t_sq, ksq, blockSq, w, R, Su, Up;
    U64 opposed;

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    if (TRACE) T.PawnValue[side]++;
    if (TRACE) T.PawnPSQT32[relativeSquare32(side, SQ64(sq))][side]++;

    ksq = FR2SQ(clamp(FilesBrd[pos->KingSq[!side]], FILE_B, FILE_G),
                clamp(RanksBrd[pos->KingSq[!side]], RANK_2, RANK_7));

    for (int index = 0; index < NumDir[pce]; ++index) {
        t_sq = sq + PceDir[pce][index];

        if (  !SQOFFBOARD(t_sq)
            && e.sqNearK[ksq][t_sq])
            ei.attCnt[side]++;
    }

    R  = relativeRank(side, SQ64(sq));
    Su = side == WHITE ? -10 :  10;
    Up = side == WHITE ?  10 : -10;

    pawnbrothers =  pos->pieces[sq-1] == pce
                 || pos->pieces[sq+1] == pce;

    opposed = FilesBB[FilesBrd[sq]] & pos->pawns[side^1];
    support = pos->pawn_ctrl[side][sq];

    //printf("%c Pawn:%s\n",PceChar[pce], PrSq(sq));
    //printf("opposed %d\n", (bool)(opposed));
    //printf("support %d %d\n", (bool)(support), support);
    //printf("pawnbrothers %d\n",(bool)(pawnbrothers));

    if (pos->pieces[sq+Su] == pce) {
        //printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
        score += PawnDoubled;
        if (TRACE) T.PawnDoubled[side]++;
    }
    if (pos->pieces[sq+Su*2] == pce) {
        //printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
        score += PawnDoubled;
        if (TRACE) T.PawnDoubled[side]++;
    }
    if (pos->pieces[sq+Su*3] == pce) {
        //printf("%c Dou:%s\n",PceChar[pce], PrSq(sq));
        score += PawnDoubled;
        if (TRACE) T.PawnDoubled[side]++;
    }
    
    if (!(IsolatedMask[SQ64(sq)] & pos->pawns[side])) {
        //printf("%c Iso:%s\n",PceChar[pce], PrSq(sq));
        score += PawnIsolated[!opposed];
        if (TRACE) T.PawnIsolated[!opposed][side]++;
    }

    if ( !(PassedPawnMasks[side^1][SQ64(sq)] & pos->pawns[side])
        && pos->pawn_ctrl[side^1][sq + Up]) {
        //printf("%c BackW:%s\n",PceChar[pce], PrSq(sq));
        score += PawnBackward[!opposed];
        if (TRACE) T.PawnBackward[!opposed][side]++;
    }

    if (!(PassedPawnMasks[side][SQ64(sq)] & pos->pawns[side^1])) {
        //printf("%c Passed:%s\n",PceChar[pce], PrSq(sq));
        ei.passedCnt++;
        score += PawnPassed[R];
        if (TRACE) T.PawnPassed[R][side]++;

        if (support || pawnbrothers) {
            //printf("%c PassedConnected:%s\n",PceChar[pce], PrSq(sq));
            score += PawnPassedConnected[R];
            if (TRACE) T.PawnPassedConnected[R][side]++;
        }

        if (R > RANK_3) {

            blockSq = sq + Up;
            w = 5 * R - 13;

            bonus = ((  king_proximity(side^1, blockSq, pos) * 5
                      - king_proximity(side,   blockSq, pos) * 3) * w);

            if (TRACE) T.KP_1[side] += (w * king_proximity(side^1, blockSq, pos));
            if (TRACE) T.KP_2[side] += (w * king_proximity(side,   blockSq, pos));
            
            if (R != RANK_7) {
                bonus -= ( king_proximity(side, blockSq + Up, pos) * w);
                if (TRACE) T.KP_3[side] += (w * king_proximity(side, blockSq + Up, pos));
            }

            bonus = (bonus * 100) / 410;
            score += makeScore(0, bonus);
        }
    }

    if (support || pawnbrothers) {
        int i =  Connected[R] * (2 + (bool)(pawnbrothers) - (bool)(opposed))
                + 30 * support;

        i = (i * 100) / 1220;

        if (TRACE) T.PawnConnected[R][side] += (2 + (bool)(pawnbrothers) - (bool)(opposed));
        if (TRACE) T.PawnSupport[side] += support;
        
        //printf("supportCount %d v %d v eg %d R %d\n",support, i, i * (R - 2) / 4, R);
        score += makeScore(i, i * (R - 2) / 4);
    }
    //ei.pawns[side] += score;

    return score;
}

int Knights(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, mobility, tropism;
    int defended, Count, sq, R, P1, P2;

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    R = relativeRank(side, SQ64(sq));
    if (TRACE) T.KnightValue[side]++;
    if (TRACE) T.KnightPSQT32[relativeSquare32(side, SQ64(sq))][side]++;

    if ((   R == RANK_4 
         || R == RANK_5
         || R == RANK_6)
        && !(outpostSquareMasks(side, SQ64(sq)) & pos->pawns[side^1])) {
        //printf("%c KnightOutpost:%s\n",PceChar[pce], PrSq(sq));

        defended = (pos->pawn_ctrl[side][sq]);
        score += KnightOutpost[defended];
        if (TRACE) T.KnightOutpost[defended][side]++;
    }

    tropism = getTropism(sq, pos->KingSq[side^1]);
    score += tropism * KnightTropism;
    if (TRACE) T.KnightTropism[side] += tropism;

    mobility = NonSlideMob(pos, side, pce, sq);
    ei.Mob[side] += KnightMobility[mobility];
    if (TRACE) T.KnightMobility[mobility][side]++;

    Count = distanceBetween(sq, pos->KingSq[side]);
    P1 = (10 * Count) * 100 / 410;
    P2 = ( 7 * Count) * 100 / 410;
    score += makeScore(-P1, -P2);
    if (TRACE) T.KnightDefender[side] += Count;

    //ei.knights[side] += score;

    return score;   
}

int Bishops(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, mobility, tropism;
    int defended, Count, sq, R, P1, P2;

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    R = relativeRank(side, SQ64(sq));
    if (TRACE) T.BishopValue[side]++;
    if (TRACE) T.BishopPSQT32[relativeSquare32(side, SQ64(sq))][side]++;

    if ((   R == RANK_4 
         || R == RANK_5
         || R == RANK_6)
        && !(outpostSquareMasks(side, SQ64(sq)) & pos->pawns[side^1])) {
        //printf("%c BishopOutpost:%s\n",PceChar[pce], PrSq(sq));

        defended = (pos->pawn_ctrl[side][sq]);
        score += BishopOutpost[defended];
        if (TRACE) T.BishopOutpost[defended][side]++;
    }

    tropism = getTropism(sq, pos->KingSq[side^1]);
    score += tropism * BishopTropism;
    if (TRACE) T.BishopTropism[side] += tropism;

    mobility = SlideMob(pos, side, pce, sq);
    ei.Mob[side] += BishopMobility[mobility];
    if (TRACE) T.BishopMobility[mobility][side]++;

    if (mobility <= 3) {

        Count = pawns_on_same_color_squares(pos,side,sq);
        P1 = (3 * Count) * 100 / 310;
        P2 = (7 * Count) * 100 / 310;
        score += makeScore(-P1, -P2);
        if (TRACE) T.BishopRammedPawns[side] += Count;
    } 

    Count = distanceBetween(sq, pos->KingSq[side]);
    P1 = (10 * Count) * 100 / 410;
    P2 = ( 7 * Count) * 100 / 410;
    score += makeScore(-P1, -P2);
    if (TRACE) T.BishopDefender[side] += Count;
  
    //ei.bishops[side] += score;

    return score;
}

int Rooks(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, mobility, tropism, sq, R, KR;

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    if (TRACE) T.RookValue[side]++;
    if (TRACE) T.RookPSQT32[relativeSquare32(side, SQ64(sq))][side]++;

    R  = relativeRank(side, SQ64(sq));
    KR = relativeRank(side, SQ64(pos->KingSq[side^1]));

    if (!(pos->pawns[COLOUR_NB] & FilesBB[FilesBrd[sq]])) {
        score += RookOpen;
        if (TRACE) T.RookOpen[side]++;
    }
    if (!(pos->pawns[side] & FilesBB[FilesBrd[sq]])) {
        score += RookSemi;
        if (TRACE) T.RookSemi[side]++;
    }
    if ((R == RANK_7 && KR == RANK_8)) {
        score += RookOnSeventh;
        if (TRACE) T.RookOnSeventh[side]++;
    }

    tropism = getTropism(sq, pos->KingSq[side^1]);
    score += tropism * RookTropism;
    if (TRACE) T.RookTropism[side] += tropism;

    mobility = SlideMob(pos, side, pce, sq);
    ei.Mob[side] += RookMobility[mobility];
    if (TRACE) T.RookMobility[mobility][side]++;

    //ei.rooks[side] += score;

    return score;
}

int Queens(const S_BOARD *pos, int side, int pce, int pceNum) {

    int score = 0, mobility, tropism, sq, Knight, Bishop;

    Knight = side == WHITE ? wN : bN;
    Bishop = side == WHITE ? wB : bB;

    sq = pos->pList[pce][pceNum];
    ASSERT(SqOnBoard(sq));
    ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);

    if (TRACE) T.QueenValue[side]++;
    if (TRACE) T.QueenPSQT32[relativeSquare32(side, SQ64(sq))][side]++;

    if (relativeRank(side, SQ64(sq)) > RANK_2) {
        if (isPiece(Knight, relativeSquare(side, B1), pos)) {
            //printf("%c QueenPreDeveloped:%s\n",PceChar[pce], PrSq(sq));
            score += QueenPreDeveloped;
            if (TRACE) T.QueenPreDeveloped[side]++;
        }
        if (isPiece(Bishop, relativeSquare(side, C1), pos)) {
            //printf("%c QueenPreDeveloped:%s\n",PceChar[pce], PrSq(sq));
            score += QueenPreDeveloped;
            if (TRACE) T.QueenPreDeveloped[side]++;
        }
        if (isPiece(Bishop, relativeSquare(side, F1), pos)) {
            //printf("%c QueenPreDeveloped:%s\n",PceChar[pce], PrSq(sq));
            score += QueenPreDeveloped;
            if (TRACE) T.QueenPreDeveloped[side]++;
        }
        if (isPiece(Knight, relativeSquare(side, G1), pos)) {
            //printf("%c QueenPreDeveloped:%s\n",PceChar[pce], PrSq(sq));
            score += QueenPreDeveloped;
            if (TRACE) T.QueenPreDeveloped[side]++;
        }
    }

    tropism = getTropism(sq, pos->KingSq[side^1]);
    score += tropism * QueenTropism;
    if (TRACE) T.QueenTropism[side] += tropism;

    mobility = SlideMob(pos, side, pce, sq);
    ei.Mob[side] += QueenMobility[mobility];
    if (TRACE) T.QueenMobility[mobility][side]++;

    //ei.queens[side] += score;

    return score;
}

int hypotheticalShelter(const S_BOARD *pos, int side, int KingSq) {

    int score = 0, d, center, file, ourRank, theirRank;
    U64 ours, theirs;

    center = clamp(FilesBrd[KingSq], FILE_B, FILE_G);
    for (file = center - 1; file <= center + 1; ++file) {

        ours = (pos->pawns[side] & FilesBB[file]);
        ourRank = ours ? relativeRank(side, frontmost(side^1, ours)) : 0;

        theirs = (pos->pawns[side^1] & FilesBB[file]);
        theirRank = theirs ? relativeRank(side, frontmost(side^1, theirs)) : 0;

        d = map_to_queenside(file);
        score  = makeScore(1, 1);
        score += ShelterStrength[d][ourRank];
        if (TRACE) T.ShelterStrength[d][ourRank][side]++;

        if (ourRank && (ourRank == theirRank - 1)) {
            score += BlockedStorm * (int)(theirRank == RANK_3);
            if (TRACE) T.BlockedStorm[side] += (int)(theirRank == RANK_3);
        } else {
            score += UnblockedStorm[d][theirRank];
            if (TRACE) T.UnblockedStorm[d][theirRank][side]++;
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

    if (TRACE) T.KingPSQT32[relativeSquare32(side, SQ64(pos->KingSq[side]))][side]++;

    enemyQueen = side == WHITE ? bQ : wQ;

    if (!(pos->pawns[COLOUR_NB] & KingFlank[FilesBrd[pos->KingSq[side]]])) {
        score += KingPawnLessFlank;
        if (TRACE) T.KingPawnLessFlank[side]++;
    }

    score += evaluateShelter(pos, side);

    if (ei.attckersCnt[side^1] > 1 - pos->pceNum[enemyQueen]) {

        float scaledAttackCounts = 9.0 * ei.attCnt[side^1] / popcount(ei.kingAreas[side]);

        count =  32 * scaledAttackCounts
               +      ei.attckersCnt[side^1] * ei.attWeight[side^1]
               +      mgScore(ei.Mob[side^1] - ei.Mob[side]) / 4
               -  6 * mgScore(ei.pkeval[side]) / 8
               - 17 ;

        if (count > 26) {
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

    int complexity, outflanking, pawnsOnBothFlanks, pawnEndgame, almostUnwinnable, sign, eg, v;

    eg = egScore(score);
    sign = (eg > 0) - (eg < 0);

    outflanking =  distanceByFile(pos->KingSq[WHITE], pos->KingSq[BLACK])
                 - distanceByRank(pos->KingSq[WHITE], pos->KingSq[BLACK]);

    pawnsOnBothFlanks =   (pos->pawns[COLOUR_NB] & KING_FLANK )
                       && (pos->pawns[COLOUR_NB] & QUEEN_FLANK);

    pawnEndgame =   (!pos->pceNum[wN] && !pos->pceNum[wB] && !pos->pceNum[wR] && !pos->pceNum[wQ])
                 && (!pos->pceNum[bN] && !pos->pceNum[bB] && !pos->pceNum[bR] && !pos->pceNum[bQ]);

    almostUnwinnable =   !ei.passedCnt
                      && !pawnsOnBothFlanks
                      &&  outflanking < 0;

    complexity =  ComplexityPassedPawns * ei.passedCnt
                + ComplexityTotalPawns  * popcount(pos->pawns[COLOUR_NB])
                + ComplexityOutflanking * outflanking
                + ComplexityPawnFlanks  * pawnsOnBothFlanks
                + ComplexityPawnEndgame * pawnEndgame
                + ComplexityUnwinnable  * almostUnwinnable
                + ComplexityAdjustment  ;

    if (TRACE) T.ComplexityPassedPawns[WHITE] += sign * ei.passedCnt;
    if (TRACE) T.ComplexityTotalPawns[WHITE]  += sign * popcount(pos->pawns[COLOUR_NB]);
    if (TRACE) T.ComplexityOutflanking[WHITE] += sign * outflanking;
    if (TRACE) T.ComplexityPawnFlanks[WHITE]  += sign * pawnsOnBothFlanks;
    if (TRACE) T.ComplexityPawnEndgame[WHITE] += sign * pawnEndgame;
    if (TRACE) T.ComplexityUnwinnable[WHITE]  += sign * almostUnwinnable;
    if (TRACE) T.ComplexityAdjustment[WHITE]  += sign;

    v = sign * MAX(egScore(complexity), -abs(eg));

    //ei.Complexity = makeScore(0, v);

    return makeScore(0, v);
}

int imbalance(const int pieceCount[2][6], int side) {

    int u = 0, v = 0, pt1, pt2;

    // Adaptation of polynomial material imbalance, by Tord Romstad
    for (pt1 = NO_PIECE_TYPE; pt1 <= QUEEN; ++pt1) {

        if (!pieceCount[side][pt1])
            continue;

        int w = 0;

        for (pt2 = NO_PIECE_TYPE; pt2 <= pt1; ++pt2) {
            w +=  QuadraticOurs[pt1][pt2] * pieceCount[side][pt2]
              + QuadraticTheirs[pt1][pt2] * pieceCount[side^1][pt2];

            if (TRACE) T.QuadraticOurs[pt1][pt2][side] += pieceCount[side][pt2] * pieceCount[side][pt1];
            if (TRACE) T.QuadraticTheirs[pt1][pt2][side] += pieceCount[side^1][pt2] * pieceCount[side][pt1];
        }
        
        u += pieceCount[side][pt1] * mgScore(w);
        v += pieceCount[side][pt1] * egScore(w);
    }
    //ei.imbalance[side] = makeScore(u / 16, v / 16);

    return makeScore(u / 16, v / 16);
}

int evaluateScaleFactor(const S_BOARD *pos, int egScore) {

    const int strongSide = egScore > 0 ? WHITE : BLACK;
    const int pawnStrong = egScore > 0 ? wP : bP;

    if (   !pos->pceNum[pawnStrong]
        && (pos->material[strongSide] - pos->material[!strongSide]) <= PieceValue[EG][wB])
        return pos->material[ strongSide] <  PieceValue[EG][wR] ? SCALE_FACTOR_DRAW    :
               pos->material[!strongSide] <= PieceValue[EG][wB] ? SCALE_DRAWISH_BISHOP :
                                                                  SCALE_DRAWISH_ROOK   ;

    /*if (opposite_bishops(pos)) {

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
    }*/

    if (opposite_bishops(pos)) {

        if (   !pos->pceNum[wN] && !pos->pceNum[bN]
            && !pos->pceNum[wR] && !pos->pceNum[bR] 
            && !pos->pceNum[wQ] && !pos->pceNum[bQ]) {
            return SCALE_OCB_BISHOPS_ONLY;
        }

        if (   !pos->pceNum[wR] && !pos->pceNum[bR]
            && !pos->pceNum[wQ] && !pos->pceNum[bQ]
            && (pos->pceNum[wN] == 1 && pos->pceNum[bN] == 1)) {
            return SCALE_OCB_ONE_KNIGHT;
        }

        if (   !pos->pceNum[wN] && !pos->pceNum[bN]
            && !pos->pceNum[wQ] && !pos->pceNum[bQ]
            && (pos->pceNum[wR] == 1 && pos->pceNum[bR] == 1)) {
            return SCALE_OCB_ONE_ROOK;
        }
    }

    return SCALE_NORMAL;
}

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

int EvalPosition(const S_BOARD *pos, Material_Table *materialTable) {
    // setboard 8/3k3p/6p1/3nK1P1/8/8/7P/8 b - - 3 64
    // setboard r2q1rk1/p2b1p1p/1p1b2pQ/2p1pP2/1nPp4/1P1BP3/PB1P2PP/RN3RK1 w - - 1 16
    // setboard 8/6R1/2k5/6P1/8/8/4nP2/6K1 w - - 1 41 

    int score, factor, s1, s2;
    Material_Entry* me = Material_probe(pos, materialTable);

    if (me->evalExists)
        return me->eval;

    memset(&ei, 0, sizeof(evalInfo));

    s1 = makeSq(clamp(FilesBrd[pos->KingSq[WHITE]], FILE_B, FILE_G),
                clamp(RanksBrd[pos->KingSq[WHITE]], RANK_2, RANK_7));

    s2 = makeSq(clamp(FilesBrd[pos->KingSq[BLACK]], FILE_B, FILE_G),
                clamp(RanksBrd[pos->KingSq[BLACK]], RANK_2, RANK_7));

    ei.kingAreas[WHITE] = kingAreaMasks(s1);
    ei.kingAreas[BLACK] = kingAreaMasks(s2);

    score  = pos->mPhases[WHITE] - pos->mPhases[BLACK];
    score += pos->PSQT[WHITE] - pos->PSQT[BLACK];
    score += ei.Mob[WHITE] - ei.Mob[BLACK];
    score += me->imbalance;
    score += evaluatePieces(pos);
    score += evaluateComplexity(pos, score);

    blockedPiecesW(pos);
    blockedPiecesB(pos);

    factor = me->factor != SCALE_NORMAL
           ? me->factor : evaluateScaleFactor(pos, egScore(score));

    score = (mgScore(score) * (256 - me->gamePhase)
          +  egScore(score) * me->gamePhase * factor / SCALE_NORMAL) / 256;

    score += ei.blockages[WHITE] - ei.blockages[BLACK];

    score += pos->side == WHITE ? TEMPO : -TEMPO;
    
    return pos->side == WHITE ? score : -score;     
}

void printEvalFactor( int WMG, int WEG, int BMG, int BEG ) {
    printf("| %4d  %4d  | %4d  %4d  | %4d  %4d \n",WMG, WEG, BMG, BEG, WMG - BMG, WEG - BEG );
}

void printEval(const S_BOARD *pos) {

    int v = EvalPosition(pos, &Table);
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
    printf("        Total "); printf("| ----  ----  | ----  ----  | %4d cp: %d\n", v, (v * 100) / 118);
    printf("\n");
}

void setPcsq32() {

    for (int i = 0; i < 120; ++i) {

        if (!SQOFFBOARD(i)) {
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