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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"

#ifdef TUNE
    #define TRACE (1)
#else
    #define TRACE (0)
#endif

enum {
    SCALE_FACTOR_DRAW      =   0,
    SCALE_DRAWISH_BISHOP   =   8,
    SCALE_DRAWISH_ROOK     =  28,
    SCALE_OCB_BISHOPS_ONLY =  64,
    SCALE_OCB_ONE_KNIGHT   = 106,
    SCALE_OCB_ONE_ROOK     =  96,
    SCALE_NORMAL           = 128,
};

struct EvalTrace {
    int PawnValue[COLOUR_NB];
    int KnightValue[COLOUR_NB];
    int BishopValue[COLOUR_NB];
    int RookValue[COLOUR_NB];
    int QueenValue[COLOUR_NB];
    int KingValue[COLOUR_NB];
    int PawnPSQT32[32][COLOUR_NB];
    int KnightPSQT32[32][COLOUR_NB];
    int BishopPSQT32[32][COLOUR_NB];
    int RookPSQT32[32][COLOUR_NB];
    int QueenPSQT32[32][COLOUR_NB];
    int KingPSQT32[32][COLOUR_NB];
    int PawnDoubled[COLOUR_NB];
    int PawnIsolated[2][COLOUR_NB];
    int PawnBackward[2][COLOUR_NB];
    int PawnPassed[8][COLOUR_NB];
    int PawnPassedConnected[8][COLOUR_NB];
    int PawnConnected[8][COLOUR_NB];
    int PawnSupport[COLOUR_NB];
    int KP_1[COLOUR_NB];
    int KP_2[COLOUR_NB];
    int KP_3[COLOUR_NB];
    int KnightOutpost[2][COLOUR_NB];
    int KnightTropism[COLOUR_NB];
    int KnightDefender[COLOUR_NB];
    int KnightMobility[9][COLOUR_NB];
    int BishopOutpost[2][COLOUR_NB];
    int BishopTropism[COLOUR_NB];
    int BishopRammedPawns[COLOUR_NB];
    int BishopDefender[COLOUR_NB];
    int BishopMobility[14][COLOUR_NB];
    int RookOpen[COLOUR_NB];
    int RookSemi[COLOUR_NB];
    int RookOnSeventh[COLOUR_NB];
    int RookTropism[COLOUR_NB];
    int RookMobility[15][COLOUR_NB];
    int QueenPreDeveloped[COLOUR_NB];
    int QueenTropism[COLOUR_NB];
    int QueenMobility[28][COLOUR_NB];
    int BlockedStorm[COLOUR_NB];
    int ShelterStrength[4][8][COLOUR_NB];
    int UnblockedStorm[4][8][COLOUR_NB];
    int KingPawnLessFlank[COLOUR_NB];
    int ComplexityPassedPawns[COLOUR_NB];
    int ComplexityTotalPawns[COLOUR_NB];
    int ComplexityOutflanking[COLOUR_NB];
    int ComplexityPawnFlanks[COLOUR_NB];
    int ComplexityPawnEndgame[COLOUR_NB];
    int ComplexityUnwinnable[COLOUR_NB];
    int ComplexityAdjustment[COLOUR_NB];
    int QuadraticOurs[6][6][COLOUR_NB];
    int QuadraticTheirs[6][6][COLOUR_NB];
};

struct evalInfo {
    int Mob[2];
    int attCnt[2];
    int attckersCnt[2];
    int attWeight[2];
    U64 kingAreas[2];
    int blockages[2];
    int pkeval[2];
    int passedCnt;
    int pawns[2];
    int knights[2];
    int bishops[2];
    int rooks[2];
    int queens[2];
    int Complexity;
    int imbalance[2];
    int KingDanger[2];
};

struct evalData {
    int sqNearK [2][120][120];
    int PSQT[13][120];
};

void blockedPiecesW(const S_BOARD *pos);
void blockedPiecesB(const S_BOARD *pos);
void printEvalFactor( int WMG, int WEG, int BMG, int BEG );
void printEval(const S_BOARD *pos);
void setPcsq32();
bool opposite_bishops(const S_BOARD *pos);
int pawns_on_same_color_squares(const S_BOARD *pos, const int colour, const int sq);
int getTropism(const int s1, const int s2);
int king_proximity(const int c, const int s, const S_BOARD *pos);
int isPiece(const int piece, const int sq, const S_BOARD *pos);
int NonSlideMob(const S_BOARD *pos, int side, int pce, int sq);
int SlideMob(const S_BOARD *pos, int side, int pce, int sq);
int evaluateScaleFactor(const S_BOARD *pos, int egScore);
int Pawns(const S_BOARD *pos, int side, int pce, int pceNum);
int Knights(const S_BOARD *pos, int side, int pce, int pceNum);
int Bishops(const S_BOARD *pos, int side, int pce, int pceNum);
int Rooks(const S_BOARD *pos, int side, int pce, int pceNum);
int Queens(const S_BOARD *pos, int side, int pce, int pceNum);
int hypotheticalShelter(const S_BOARD *pos, int side, int KingSq);
int evaluateShelter(const S_BOARD *pos, int side);
int evaluateKings(const S_BOARD *pos, int side);
int evaluatePieces(const S_BOARD *pos);
int evaluateComplexity(const S_BOARD *pos, int score);
int imbalance(const int pieceCount[2][6], int side);
int EvalPosition(const S_BOARD *pos, Material_Table *materialTable);

#define ENDGAME_MAT (1 * PieceValue[EG][wR] + 2 * PieceValue[EG][wN] + 2 * PieceValue[EG][wP])
#define makeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define mgScore(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define egScore(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

extern evalData e;

static const int PceDir[13][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    {  9, 11, 0, 0, 0, 0, 0, 0 },
    { -8, -19,  -21, -12, 8, 19, 21, 12 },
    { -9, -11, 11, 9, 0, 0, 0, 0 },
    { -1, -10,  1, 10, 0, 0, 0, 0 },
    { -1, -10,  1, 10, -9, -11, 11, 9 },
    { -1, -10,  1, 10, -9, -11, 11, 9 },
    { -9, -11, 0, 0, 0, 0, 0, 0 },
    { -8, -19,  -21, -12, 8, 19, 21, 12 },
    { -9, -11, 11, 9, 0, 0, 0, 0 },
    { -1, -10,  1, 10, 0, 0, 0, 0 },
    { -1, -10,  1, 10, -9, -11, 11, 9 },
    { -1, -10,  1, 10, -9, -11, 11, 9 }
};

static const int NumDir[13] = {
 0, 2, 8, 4, 4, 8, 8, 2, 8, 4, 4, 8, 8
};

static const int P_KING_BLOCKS_ROOK   = 24;
static const int P_BLOCK_CENTRAL_PAWN = 14;
static const int P_BISHOP_TRAPPED_A7  = 150;
static const int P_BISHOP_TRAPPED_A6  = 50;
static const int P_KNIGHT_TRAPPED_A8  = 150;
static const int P_KNIGHT_TRAPPED_A7  = 100;
static const int P_C3_KNIGHT = 30;
static const int RETURNING_BISHOP = 20;
static const int TEMPO = 10;

static const int Weight[13] = { 0, 4, 16, 10, 8, 2, 0, 4, 16, 10, 8, 2, 0};
