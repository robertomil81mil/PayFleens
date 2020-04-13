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

enum {
    SCALE_FACTOR_DRAW      =   0,
    SCALE_DRAWISH_BISHOP   =   8,
    SCALE_DRAWISH_ROOK     =  28,
    SCALE_OCB_BISHOPS_ONLY =  64,
    SCALE_OCB_ONE_KNIGHT   = 106,
    SCALE_OCB_ONE_ROOK     =  96,
    SCALE_NORMAL           = 128,
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
int EvalPosition(const S_BOARD *pos);

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

static const int ShelterStrength[4][8] = {
    {  -6,  81,  93,  58,  39,  18,   25 },
    { -43,  61,  35, -49, -29, -11,  -63 },
    { -10,  75,  23,  -2,  32,   3,  -45 },
    { -39, -13, -29, -52, -48, -67, -166 }
};

static const int UnblockedStorm[4][8] = {
    {  89, -285, -185, 93, 57,  45,  51 },
    {  44,  -18,  123, 46, 39,  -7,  23 },
    {   4,   52,  162, 37,  7, -14,  -2 },
    { -10,  -14,   90, 15,  2,  -7, -16 }
};

static const int QuadraticOurs[][6] = {
    //            OUR PIECES
    // pair pawn knight bishop rook queen
    { 796                               }, // Bishop pair
    {  22,   21                         }, // Pawn
    {  17,  141,  -34                   }, // Knight      OUR PIECES
    {   0,   57,    2,    0             }, // Bishop
    { -14,   -1,   26,   58,  -115      }, // Rook
    {-104,   13,   64,   73,   -74, -3  }  // Queen
};

static const int QuadraticTheirs[][6] = {
    //           THEIR PIECES
    // pair pawn knight bishop rook queen
    {   0                               }, // Bishop pair
    {  19,    0                         }, // Pawn
    {   4,   34,    0                   }, // Knight      OUR PIECES
    {  32,   36,   23,    0             }, // Bishop
    {  25,   21,   13,  -13,    0       }, // Rook
    {  53,   55,  -23,   75,  148,   0  }  // Queen
};