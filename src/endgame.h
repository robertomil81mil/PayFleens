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

#include "defs.h"

extern Material_Table Table;

enum {
    KPK,
    KBNK,
    KQKR,
    KQKP,
    KRKP,
    KRKB,
    KRKN,
    KNNK,
    KNNKP,
    KPKP,
    KBPKB,
    KBPPKB,
    KRPKB,
    KRPKR,
    KRPPKRP,
    ENDGAME_NB,
};

enum {
    MT_KEY_SIZE   = 16,
    MT_SIZE       = 1 << MT_KEY_SIZE,
    MT_HASH_SHIFT = 64 - MT_KEY_SIZE,
};

struct Material_Entry {
    uint64_t key;
    int factor;
    int imbalance;
    int gamePhase;
    int eval, evalExists;
};

struct Material_Table {
    Material_Entry entry[MT_SIZE];
};

int EndgameKXK(const Board *pos, int strongSide);
int EndgameKPK(const Board *pos, int strongSide);
int EndgameKBNK(const Board *pos, int strongSide);
int EndgameKQKR(const Board *pos, int strongSide);
int EndgameKQKP(const Board *pos, int strongSide);
int EndgameKRKP(const Board *pos, int strongSide);
int EndgameKRKB(const Board *pos, int strongSide);
int EndgameKRKN(const Board *pos, int strongSide);
int EndgameKNNK(const Board *pos, int strongSide);
int EndgameKNNKP(const Board *pos, int strongSide);

int EndgameKPsK(const Board *pos, int strongSide);
int EndgameKPKP(const Board *pos, int strongSide);
int EndgameKBPsK(const Board *pos, int strongSide);
int EndgameKBPKB(const Board *pos, int strongSide);
int EndgameKBPPKB(const Board *pos, int strongSide);
int EndgameKRPKB(const Board *pos, int strongSide);
int EndgameKRPKR(const Board *pos, int strongSide);
int EndgameKRPPKRP(const Board *pos, int strongSide);

bool is_KXK(const Board *pos, int strongSide);
bool is_KPsK(const Board *pos, int strongSide);
bool is_KBPsK(const Board *pos, int strongSide);

void endgameInit(Board *pos);
void endgameAdd(Board *pos, int eg, char *fen);
int fixSquare(const Board *pos, int strongSide, int sq);

Material_Entry* Material_probe(const Board *pos, Material_Table *materialTable);
int EndgameValue_probe(const Board *pos, uint64_t key);
int EndgameFactor_probe(const Board *pos, uint64_t key);

static const int PushToEdges[64] = {
   55, 49, 44, 38, 38, 44, 49, 55,
   49, 38, 33, 27, 27, 33, 38, 49,
   44, 33, 22, 16, 16, 22, 33, 44,
   38, 27, 16, 11, 11, 16, 27, 38,
   38, 27, 16, 11, 11, 16, 27, 38,
   44, 33, 22, 16, 16, 22, 33, 44,
   49, 38, 33, 27, 27, 33, 38, 49,
   55, 49, 44, 38, 38, 44, 49, 55,
};

static const int PushToCorners[64] = {
    3545, 3368, 3190, 3013, 2836, 2659, 2481, 2304,
    3368, 3190, 3013, 2836, 2659, 2481, 2304, 2481,
    3190, 3013, 2747, 2481, 2481, 2215, 2481, 2659,
    3013, 2836, 2481, 2127, 1950, 2481, 2659, 2836,
    2836, 2659, 2481, 1950, 2127, 2481, 2836, 3013,
    2659, 2481, 2215, 2481, 2481, 2747, 3013, 3190,
    2481, 2304, 2481, 2659, 2836, 3013, 3190, 3368,
    2304, 2481, 2659, 2836, 3013, 3190, 3368, 3545,
};

static const int PushClose[8] = { 0, 0, 55, 44, 33, 22, 11, 5  };
static const int PushAway [8] = { 0, 2, 11, 22, 33, 44, 49, 55 };
static const int KRPPKRPScaleFactors[RANK_NB] = { 0, 18, 20, 28, 42, 88, 0, 0 };