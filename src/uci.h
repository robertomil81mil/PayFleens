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

#define StartPosition "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define VERSION_ID "1.49"

struct Limits {
    double start, time, inc, timeLimit;
    int limitedByNone, limitedByTime, limitedBySelf;
    int limitedByDepth, depthLimit, mtg;
};

struct EngineOptions {
	int PolyBook;
	double MinThinkingTime, MoveOverHead, SlowMover; 
};

void uciGo(char *str, S_SEARCHINFO *info, S_BOARD *pos);
void uciSetOption(char *str);
void uciPosition(char* str, S_BOARD *pos);

void uciReport(S_SEARCHINFO *info, S_BOARD *pos, int alpha, int beta, int value);
void uciReportCurrentMove(int move, int currmove, int depth);
void printStats(S_SEARCHINFO *info);

int getInput(char *str);
int strEquals(char *str1, char *str2);
int strStartsWith(char *str, char *key);
int strContains(char *str, char *key);

extern EngineOptions Options;