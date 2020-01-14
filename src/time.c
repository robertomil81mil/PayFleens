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

// time.c

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "defs.h"
#include "time.h"
#include "uci.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/select.h>
#include <string.h>
#endif

double getTimeMs() {
#if defined(_WIN32) || defined(_WIN64)
    return (double)(GetTickCount());
#else
    struct timeval tv;
    double milliSecs, milliUsecs;

    gettimeofday(&tv, NULL);
    milliSecs = ((double)tv.tv_sec) * 1000;
    milliUsecs = tv.tv_usec / 1000;

    return milliSecs + milliUsecs;
#endif
}

double elapsedTime(S_SEARCHINFO *info) {
    return getTimeMs() - info->startTime;
}

// http://home.arcor.de/dreamlike/chess/
int InputWaiting() {
#ifndef WIN32
    fd_set readfds;
    struct timeval tv;
    FD_ZERO (&readfds);
    FD_SET (fileno(stdin), &readfds);
    tv.tv_sec = 0; tv.tv_usec = 0;
    select(16, &readfds, 0, 0, &tv);

    return (FD_ISSET(fileno(stdin), &readfds));
#else
    static int init = 0, pipe;
    static HANDLE inh;
    DWORD dw;

    if (!init) {
        init = 1;
        inh = GetStdHandle(STD_INPUT_HANDLE);
        pipe = !GetConsoleMode(inh, &dw);
        if (!pipe) {
            SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(inh);
        }
    }
    if (pipe) {
        if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
        return dw;
    } else {
        GetNumberOfConsoleInputEvents(inh, &dw);
        return dw <= 1 ? 0 : dw;
    }
#endif
}

void ReadInput(S_SEARCHINFO *info) {
    int  bytes;
    char input[256] = "", *endc;

    if (InputWaiting()) {
        info->stop = TRUE;
        do {
            bytes=read(fileno(stdin),input,256);
        } while (bytes<0);
        endc = strchr(input,'\n');
        if (endc) *endc=0;

        if (strlen(input) > 0) {
            if (!strncmp(input, "quit", 4)) {
                info->quit = TRUE;
            }
        }
        return;
    }
}

double center(double v, double lo, double hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

double move_importance(int ply) {

    const double XScale = 6.85;
    const double XShift = 64.5;
    const double Skew   = 0.171;

    return pow((1 + exp((ply - XShift) / XScale)), -Skew) + DBL_MIN;
}

double remaining(int T, double myTime, double slowMover, int movesToGo, int ply) {

    const double TMaxRatio   = (T == OptimumTime ? 1.0 : MaxRatio);
    const double TStealRatio = (T == OptimumTime ? 0.0 : StealRatio);

    double moveImportance = (move_importance(ply) * slowMover) / 100.0;
    double otherMovesImportance = 0.0;

    for (int i = 1; i < movesToGo; ++i)
        otherMovesImportance += move_importance(ply + 2 * i);

    double ratio1 = (TMaxRatio * moveImportance) / (TMaxRatio * moveImportance + otherMovesImportance);
    double ratio2 = (moveImportance + TStealRatio * otherMovesImportance) / (moveImportance + otherMovesImportance);

    return myTime * MIN(ratio1, ratio2);
}

void TimeManagementInit(S_SEARCHINFO *info, Limits *limits, int ply) {

    double minThinkingTime = Options.MinThinkingTime;
    double moveOverhead    = Options.MoveOverHead;
    double slowMover       = Options.SlowMover;
    double hypMyTime;

    info->startTime = limits->start; // Save off the start time of the search

    // Allocate time if we are handling the clock
    if (limits->limitedBySelf) {

        info->optimumTime = info->maximumTime = MAX(limits->time, minThinkingTime);

        const int maxMTG = limits->mtg ? MIN(limits->mtg, MoveHorizon) : MoveHorizon;

        for (int hypMTG = 1; hypMTG <= maxMTG; ++hypMTG) {

            hypMyTime =  limits->time
                       + limits->inc  * (hypMTG - 1)
                       - moveOverhead * (2 + MIN(hypMTG, 40));

            hypMyTime = MAX(hypMyTime, 0);

            double t1 = minThinkingTime + remaining(OptimumTime, hypMyTime, slowMover, hypMTG, ply);
            double t2 = minThinkingTime + remaining(MaxTime    , hypMyTime, slowMover, hypMTG, ply);
            
            info->optimumTime = MIN(t1, info->optimumTime);
            info->maximumTime = MIN(t2, info->maximumTime);
        }
    }

    // Interface told us to search for a predefined duration
    if (limits->limitedByTime) {
        info->optimumTime = limits->timeLimit;
        info->maximumTime = limits->timeLimit;
    }
}

int TerminateTimeManagement(S_BOARD *pos, S_SEARCHINFO *info, double *timeReduction) {

    int completedDepth, lastBestMoveDepth, lastBestMove = NOMOVE;
    double TimeRdction = 1, totBestMoveChanges = 0;

    if (info->stop) 
        return 1;

    if (!info->stop)
        completedDepth = info->depth;

    if (pos->pv.line[0] != lastBestMove) {
        lastBestMove = pos->pv.line[0];
        lastBestMoveDepth = info->depth;
    }

    totBestMoveChanges /= 2;

    int previousScore = info->depth == 1 ? 32000 : info->values[info->depth-1];
    double fallingEval = (383 + 10 * (previousScore - info->values[info->depth])) / 692.0;
    fallingEval = center(fallingEval, 0.5, 1.5);

    TimeRdction = lastBestMoveDepth + 9 < completedDepth ? 1.97 : 0.98;
    double reduction = (1.36 + info->previousTimeReduction) / (2.29 * TimeRdction);

    *timeReduction = TimeRdction;
    totBestMoveChanges += info->bestMoveChanges;
    info->bestMoveChanges = 0;

    double bestMoveInstability = 1 + totBestMoveChanges;
    double cutoff = info->optimumTime * fallingEval * reduction * bestMoveInstability;

    /*printf("optimumTime %f fallingEval %f reduction %f bestMoveInstability %f cutoff %f elapsedTime %f\n",
     info->optimumTime,fallingEval,reduction,bestMoveInstability,cutoff,elapsedTime(info));*/

    return elapsedTime(info) > cutoff;
}