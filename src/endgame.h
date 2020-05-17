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

struct MaterialEntry {
    int gamePhase, imbalance;
    int endgameEvalExists, endgameEval;
};

void MaterialProbe(const S_BOARD *pos, MaterialEntry *me);
int Endgame_eval(const S_BOARD *pos, MaterialEntry *me);

int EndgameKXK(const S_BOARD *pos, int strongSide);
int EndgameKBNK(const S_BOARD *pos, int strongSide);
int EndgameKQKR(const S_BOARD *pos, int strongSide);
int EndgameKQKP(const S_BOARD *pos, int strongSide);
int EndgameKRKP(const S_BOARD *pos, int strongSide);
int EndgameKRKB(const S_BOARD *pos, int strongSide);
int EndgameKRKN(const S_BOARD *pos, int strongSide);
int EndgameKNNKP(const S_BOARD *pos, int strongSide);

bool is_KXK(const S_BOARD *pos, int strongSide);
bool is_KBNK(const S_BOARD *pos, int strongSide);
bool is_KQKR(const S_BOARD *pos, int strongSide);
bool is_KQKP(const S_BOARD *pos, int strongSide);
bool is_KRKP(const S_BOARD *pos, int strongSide);
bool is_KRKB(const S_BOARD *pos, int strongSide);
bool is_KRKN(const S_BOARD *pos, int strongSide);
bool is_KNNK(const S_BOARD *pos, int strongSide);
bool is_KNNKP(const S_BOARD *pos, int strongSide);

#define Endgame(term, S) do {                            \
    return Endgame##term(pos, S);                        \
} while (0)


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