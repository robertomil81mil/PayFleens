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

#include "defs.h"
#include "makemove.h"
#include "search.h"

enum { NORMAL_PICKER, NOISY_PICKER };

enum {
    GENERATE_MOVES, TTABLE,
    SCORE_NOISY, NOISY,
    KILLER_1, KILLER_2, COUNTER_MOVE,
    SCORE_QUIET, QUIET, DONE,
};

struct MovePicker {
    int noisySize, split, quietSize;
    int stage, height, type;
    int ttMove, killer1, killer2, counter;
    MoveList *list;
    SearchInfo *info;
};

void initMovePicker(MovePicker *mp, Board *pos, SearchInfo *info, MoveList *list, int ttMove, int height);
void initNoisyMovePicker(MovePicker *mp, SearchInfo *info, MoveList *list);
int selectNextMove(MovePicker *mp, Board *pos, int skipQuiets);
