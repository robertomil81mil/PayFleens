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

struct SearchInfo {

	double startTime, optimumTime, maximumTime;
	double previousTimeReduction, bestMoveChanges;

	uint64_t nodes;
	float fh, fhf;

	int depth, seldepth;
	int quit, stop, timeset;

	int values[MAX_PLY];
	int staticEval[MAX_PLY];
	int currentMove[MAX_PLY];
	int currentPiece[MAX_PLY];
	int historyScore[MAX_PLY];

	int nullColor, nullMinPly;

	int nullCut, probCut, TTCut;
};

struct PVariation {
    int line[MAX_PLY];
    int length;
};

void getBestMove(SearchInfo *info, Board *pos, Limits *limits, int *best);
void iterativeDeepening(Board *pos, SearchInfo *info, Limits *limits, int *best);

int aspirationWindow(Board *pos, SearchInfo *info, int *best);
int search(int alpha, int beta, int depth, Board *pos, SearchInfo *info, PVariation *pv, int height);
int qsearch(int alpha, int beta, int depth, Board *pos, SearchInfo *info, PVariation *pv, int height);

void initLMRTable();
int valueDraw(SearchInfo *info);
void ClearForSearch(Board *pos, SearchInfo *info);

static const int BetaPruningDepth = 5;
static const int BetaMargin = 85;

static const int IterativeDepth = 7;

static const int ProbCutDepth = 5;
static const int ProbCutMargin[] = { 100, 72 };

static const int QFutilityMargin = 100;

static const int RazorDepth = 1;
static const int RazorMargin = 326;

static const int WindowDepth = 4;
static const int WindowSize = 11;
static const int WindowTimerMS = 2500;
