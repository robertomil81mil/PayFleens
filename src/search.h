/*
 *  PayFleens is a UCI chess engine by Roberto Martinez.
 * 
 *  Copyright (C) 2019 Roberto Martinez
 *  Copyright (C) 2019 Andrew Grant
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

enum {
 	DEPTH_QS_CHECKS     =  0,
 	DEPTH_QS_NO_CHECKS  = -1,
 	DEPTH_NONE          = -6,
};

void getBestMove(S_SEARCHINFO *info, S_BOARD *pos, Limits *limits, int *best);
void iterativeDeepening(S_BOARD *pos, S_SEARCHINFO *info, Limits *limits, int *best);

int aspirationWindow(S_BOARD *pos, S_SEARCHINFO *info, int *best);
int search(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, PVariation *pv, int height);
int qsearch(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, PVariation *pv, int height);

void initLMRTable();
int valueDraw(S_SEARCHINFO *info);
void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info);

static const int BetaPruningDepth     = 8;
static const int BetaMargin           = 85;
static const int FutilityMargin       = 9;
static const int FutilityPruningDepth = 8;
static const int NullMovePruningDepth = 2;
static const int ProbCutDepth         = 5;
static const int ProbCutMargin        = 100;
static const int QFutilityMargin      = 100;
static const int RazorDepth           = 1;
static const int RazorMargin          = 325;
static const int WindowDepth          = 4;
static const int WindowSize           = 11;
static const int WindowTimerMS        = 2500;
