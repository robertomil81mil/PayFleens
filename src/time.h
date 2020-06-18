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

enum { OptimumTime, MaxTime };

double getTimeMs();
double elapsedTime(S_SEARCHINFO *info);
double move_importance(int ply);
double remaining(int T, double myTime, double slowMover, int movesToGo, int ply);

void TimeManagementInit(S_SEARCHINFO *info, Limits *limits, int ply);
int TerminateTimeManagement(S_BOARD *pos, S_SEARCHINFO *info, double *timeReduction);

int InputWaiting();
void ReadInput(S_SEARCHINFO *info);
void CheckTime(S_SEARCHINFO *info);

static const int MoveHorizon   = 50;
static const double MaxRatio   = 7.3;
static const double StealRatio = 0.34;