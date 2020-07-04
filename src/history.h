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

#include <stdint.h>

#include "defs.h"

static const int HistoryMax = 400;
static const int HistoryMultiplier = 32;
static const int HistoryDivisor = 512;
static const int HistexLimit = 10000;

void updateHistoryStats(Board *pos, SearchInfo *info, int *quiets, int quietsPlayed, int height, int bonus);
void updateKillerMoves(Board *pos, int height, int move);

void getHistoryScore(Board *pos, SearchInfo *info, int move, int height, int *hist, int *cmhist, int *fmhist, int *gcmhist, int *gchmhist);
void scoreQuietMoves(Board *pos, SearchInfo *info, MoveList *list, int start, int length, int height);
void getRefutationMoves(Board *pos, SearchInfo *info, int height, int *killer1, int *killer2, int *counter);