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

enum { NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

enum {
    RANKBB_1 = 0x00000000000000FFull,
    RANKBB_2 = 0x000000000000FF00ull,
    RANKBB_3 = 0x0000000000FF0000ull,
    RANKBB_4 = 0x00000000FF000000ull,
    RANKBB_5 = 0x000000FF00000000ull,
    RANKBB_6 = 0x0000FF0000000000ull,
    RANKBB_7 = 0x00FF000000000000ull,
    RANKBB_8 = 0xFF00000000000000ull,

    FILEBB_A = 0x0101010101010101ull,
    FILEBB_B = 0x0202020202020202ull,
    FILEBB_C = 0x0404040404040404ull,
    FILEBB_D = 0x0808080808080808ull,
    FILEBB_E = 0x1010101010101010ull,
    FILEBB_F = 0x2020202020202020ull,
    FILEBB_G = 0x4040404040404040ull,
    FILEBB_H = 0x8080808080808080ull,

    WHITE_SQUARES = 0x55AA55AA55AA55AAull,
    BLACK_SQUARES = 0xAA55AA55AA55AA55ull,

    QUEEN_FLANK  = FILEBB_A | FILEBB_B | FILEBB_C | FILEBB_D,
    CENTER_FLANK = FILEBB_C | FILEBB_D | FILEBB_E | FILEBB_F,
    KING_FLANK   = FILEBB_E | FILEBB_F | FILEBB_G | FILEBB_H,
};

enum {
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
bool opposite_colors(int s1, int s2);
bool opposite_bishops(const S_BOARD *pos);
int popcount(U64 bb);
int getlsb(U64 bb);
int getmsb(U64 bb);
int frontmost(int colour, U64 b);
int backmost(int colour, U64 b);
int pawns_on_same_color_squares(const S_BOARD *pos, int c, int s);
int getTropism(const int sq1, const int sq2);
int king_proximity(const int c, const int s, const S_BOARD *pos);
int distanceBetween(int s1, int s2);
int distanceByFile(int s1, int s2);
int distanceByRank(int s1, int s2);
int map_to_queenside(const int f);
int clamp(const int v, const int lo, const int hi);
int isPiece(const int color, const int piece, const int sq, const S_BOARD *pos);
int REL_SQ(const int sq120, const int side);
int rank_of(int s);
int relativeRank(int colour, int sq);
int relativeSquare32(int c, int sq);
int evaluateScaleFactor(const S_BOARD *pos);
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
int EndgameKXK(const S_BOARD *pos, int weakSide, int strongSide);
int EvalPosition(const S_BOARD *pos);

#define ENDGAME_MAT (1 * PieceVal[wR] + 2 * PieceVal[wN] + 2 * PieceVal[wP] + PieceVal[wK])
#define makeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define mgScore(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define egScore(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

extern evalData e;
extern const uint64_t KingFlank[8];

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
static const int Tempo = 14;

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

static const int PushToEdges[64] = {
  100, 90, 80, 70, 70, 80, 90, 100,
   90, 70, 60, 50, 50, 60, 70,  90,
   80, 60, 40, 30, 30, 40, 60,  80,
   70, 50, 30, 20, 20, 30, 50,  70,
   70, 50, 30, 20, 20, 30, 50,  70,
   80, 60, 40, 30, 30, 40, 60,  80,
   90, 70, 60, 50, 50, 60, 70,  90,
  100, 90, 80, 70, 70, 80, 90, 100
};

static const int PushClose[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

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
    {   4,   34,   0                    }, // Knight      OUR PIECES
    {  32,   36,  23,     0             }, // Bishop
    {  25,   21,  13,   -13,    0       }, // Rook
    {  53,   55, -23,    75,  148,    0 }  // Queen
};