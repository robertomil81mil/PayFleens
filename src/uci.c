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

// uci.c

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "evaluate.h"
#include "search.h"
#include "texel.h"
#include "time.h"
#include "ttable.h"
#include "uci.h"

EngineOptions Options; // Our global engine options

int main(int argc, char **argv) {

	S_BOARD pos[1];
    S_SEARCHINFO info[1];
    info->quit = 0;

    // Set default options
    Options.MinThinkingTime = 20;
    Options.MoveOverHead    = 30;
    Options.SlowMover       = 84;
    Options.PolyBook        = 0;

    char str[8192];

    // Initialize components of PayFleens
    AllInit(); ParseFen(StartPosition, pos);
    printf("Payfleens %s by Roberto M. & Andrew Grant\n", VERSION_ID);

    handleCommandLine(argc, argv);

    while (getInput(str)) {

        if (strEquals(str, "uci")) {

        	info->GAME_MODE = UCIMODE;
			printf("id name Payfleens %s\n", VERSION_ID);
			printf("id author Roberto M. & Andrew Grant\n");
            printf("option name Hash type spin default 16 min 1 max 65536\n");
            printf("option name Minimum Thinking Time type spin default 20 min 0 max 5000\n");
            printf("option name Move Overhead type spin default 30 min 0 max 5000\n");
            printf("option name Slow Mover type spin default 84 min 10 max 1000\n");
            printf("option name PolyBook type check default false\n");
            printf("uciok\n"), fflush(stdout);
        }

        else if (strEquals(str, "isready"))
            printf("readyok\n"), fflush(stdout);

        else if (strEquals(str, "ucinewgame"))
            clearTTable();

        else if (strStartsWith(str, "setoption"))
            uciSetOption(str);

        else if (strStartsWith(str, "position"))
        	uciPosition(str, pos);

        else if (strStartsWith(str, "go"))
            uciGo(str, info, pos);

        else if (strEquals(str, "quit"))
            break;

        else if (strStartsWith(str, "print"))
            PrintBoard(pos), fflush(stdout);

        else if (strStartsWith(str, "eval"))
            printEval(pos), fflush(stdout);

        else if (strStartsWith(str, "stats"))
            printStats(info), fflush(stdout);

        else if (strStartsWith(str, "mirror"))
            MirrorEvalTest(pos), fflush(stdout);
    }

    return 0;
}

void uciGo(char *str, S_SEARCHINFO *info, S_BOARD *pos) {

    // Get our starting time as soon as possible
    double start = getTimeMs();

    Limits limits;

    int bestMove = NOMOVE;
    int depth = 0, infinite = 0, mtg = 0;
    double wtime = 0, btime = 0, movetime = 0;
    double winc = 0, binc = 0;

    // Init the tokenizer with spaces
    char* ptr = strtok(str, " ");

    // Parse any time control and search method information that was sent
    for (ptr = strtok(NULL, " "); ptr != NULL; ptr = strtok(NULL, " ")) {
        if (strEquals(ptr, "wtime")) wtime = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "btime")) btime = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "winc")) winc = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "binc")) binc = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "movestogo")) mtg = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "depth")) depth = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "movetime")) movetime = atoi(strtok(NULL, " "));
        if (strEquals(ptr, "infinite")) infinite = 1;
    }

    // Initialize limits for the search
    limits.limitedByNone  = infinite != 0;
    limits.limitedByTime  = movetime != 0;
    limits.limitedByDepth = depth    != 0;
    limits.limitedBySelf  = !depth && !movetime && !infinite;
    limits.timeLimit      = movetime;
    limits.depthLimit     = depth;

    info->timeset = (limits.limitedBySelf || limits.limitedByTime) ? 1 : 0;

    // Pick the time values for the colour we are playing as
    limits.start = (pos->side == WHITE) ? start : start;
    limits.time  = (pos->side == WHITE) ? wtime : btime;
    limits.inc   = (pos->side == WHITE) ?  winc :  binc;
    limits.mtg   = (pos->side == WHITE) ?   mtg :   mtg;

    // Execute search, return best and ponder moves
    getBestMove(info, pos, &limits, &bestMove);

    // Report best move ( we should always have one )
    printf("bestmove %s", PrMove(bestMove));

    // Make sure this all gets reported
    printf("\n"); fflush(stdout);
}

void uciSetOption(char *str) {

    // Handle setting UCI options in PayFleens. Options include:
    //  Hash                  : Size of the Transposition Table in Megabyes
    //  Minimum Thinking Time : Think for at least this ms per move
    //  Move OverHead         : Overhead on time allocation to avoid time losses
    //  PolyBook              : Precalculated opening moves
    //  Slow Mover            : Lower values will make PayFleens think less time

    if (strStartsWith(str, "setoption name Hash value ")) {
        int megabytes = atoi(str + strlen("setoption name Hash value "));
        initTTable(megabytes); printf("info string set Hash to %dMB\n", megabytes);
    }

    if (strStartsWith(str, "setoption name Minimum Thinking Time value ")) {
        int minThinkingTime = atoi(str + strlen("setoption name Minimum Thinking Time value "));
        printf("info string set Minimum Thinking Time to %d\n", minThinkingTime);
        Options.MinThinkingTime = minThinkingTime;
    }

    if (strStartsWith(str, "setoption name Move Overhead value ")) {
        int moveOverhead = atoi(str + strlen("setoption name Move Overhead value "));
        printf("info string set Move Overhead to %d\n", moveOverhead);
        Options.MoveOverHead = moveOverhead;
    }

    if (strStartsWith(str, "setoption name Slow Mover value ")) {
        int slowMover = atoi(str + strlen("setoption name Slow Mover value "));
        printf("info string set Slow Mover to %d\n", slowMover);
        Options.SlowMover = slowMover;
    }

    if (strStartsWith(str, "setoption name PolyBook value ")) {
        if (strStartsWith(str, "setoption name PolyBook value true")) {
            printf("info string set PolyBook to true\n"), Options.PolyBook = 1;
            InitPolyBook();
        }
        if (strStartsWith(str, "setoption name PolyBook value false"))
            printf("info string set PolyBook to false\n"), Options.PolyBook = 0;
    }

    fflush(stdout);
}

void uciPosition(char* str, S_BOARD *pos) {

    str += 9;
    char *ptr = str;

    if (strncmp(str, "startpos", 8) == 0) {
        ParseFen(StartPosition, pos);
    } else {
        ptr = strstr(str, "fen");
        if (ptr == NULL) {
            ParseFen(StartPosition, pos);
        } else {
            ptr+=4;
            ParseFen(ptr, pos);
        }
    }

    ptr = strstr(str, "moves");
    int move;

    if (ptr != NULL) {
        ptr += 6;
        while (*ptr) {
              move = ParseMove(ptr,pos);
              if (move == NOMOVE) break;
              MakeMove(pos, move);
              pos->ply=0;
              while (*ptr && *ptr!= ' ') ptr++;
              ptr++;
        }
    }
}

void uciReport(S_SEARCHINFO *info, S_BOARD *pos, int alpha, int beta, int value) {

    // Gather all of the statistics that the UCI protocol would be
    // interested in. Also, bound the value passed by alpha and
    // beta, since we are using a mix of fail-hard and fail-soft

    int hashfull    = hashfullTTable();
    int depth       = info->depth;
    int seldepth    = info->seldepth;
    int elapsed     = elapsedTime(info);
    int bounded     = MAX(alpha, MIN(value, beta));
    uint64_t nodes  = info->nodes;
    int nps         = (int)(1000 * (nodes / (1 + elapsed)));

    // If the score is MATE or MATED in X, convert to X
    int score   = bounded >=  MATE_IN_MAX ?  (INFINITE - bounded + 1) / 2
                : bounded <= MATED_IN_MAX ? -(bounded + INFINITE)     / 2 : bounded;

    char *type  = bounded >=  MATE_IN_MAX ? "mate"
                : bounded <= MATED_IN_MAX ? "mate" : "cp";

    // Partial results from a windowed search have bounds
    char *bound = bounded >=  beta ? " lowerbound "
                : bounded <= alpha ? " upperbound " : " ";

    printf("info depth %d seldepth %d score %s %d%stime %d "
           "nodes %"PRIu64" nps %d hashfull %d pv ",
           depth, seldepth, type, score, bound, elapsed, nodes, nps, hashfull);

    // Iterate over the PV and print each move
    for (int i = 0; i < pos->pv.length; i++)
        printf("%s ",PrMove(pos->pv.line[i]));

    // Send out a newline and flush
    puts(""); fflush(stdout);
}

void uciReportCurrentMove(int move, int currmove, int depth) {

    printf("info depth %d currmove %s currmovenumber %d\n", depth, PrMove(move), currmove);
    fflush(stdout);
}

void printStats(S_SEARCHINFO *info) {
    printf("TTCut:%d Ordering:%.2f NullCut:%d\n",info->TTCut,(info->fhf/info->fh)*100,info->nullCut);
}

void handleCommandLine(int argc, char **argv) {

    // USAGE: ./Payfleens evalbook <book> <depth> <hash>
    if (argc > 1 && strEquals(argv[1], "evalbook")) {
        runEvalBook(argc, argv);
        exit(EXIT_SUCCESS);
    }

    if (argc > 1 && strEquals(argv[1], "testbook")) {
        runTestBook(argv);
        exit(EXIT_SUCCESS);
    }

    // Tuner is being run from the command line
    #ifdef TUNE
        runTexelTuning();
        exit(EXIT_SUCCESS);
    #endif
}

void runEvalBook(int argc, char **argv) {

    printf("STARTING RUN\n");

    S_BOARD pos[1];
    S_SEARCHINFO info[1];
    char line[256];
    Limits limits = {0};
    int i, best;
    double start = getTimeMs();

    FILE *newbook = fopen("colab45k.epd", "w");
    FILE *book    = fopen(argv[2], "r");
    int positions = 502131;
    int depth     = argc > 3 ? atoi(argv[3]) : 12;
    int megabytes = argc > 4 ? atoi(argv[5]) :  1;

    limits.limitedByDepth = 1;
    limits.depthLimit = depth;
    initTTable(megabytes);

    for (i = 0; i < positions; i++) {

        // Read next position from the FEN file
        if (fgets(line, 256, book) == NULL) {
            printf("Unable to read line #%d\n", i);
            exit(EXIT_FAILURE);
        }

        ParseFen(line, pos);

        if (!LegalMoveExist(pos))
            continue;

        getBestMove(info, pos, &limits, &best);
        clearTTable();

        printf("\rINITIALIZING SCORES FROM FENS...  [%7d OF %7d]", i + 1, positions);
        fprintf(newbook, "FEN [#   %6d] %5d %s", i+1, info->values[depth], line);
    }

    printf("Time %dms\n", (int)(getTimeMs() - start));
}

void runTestBook(char **argv) {

    printf("STARTING RUN\n");

    S_BOARD pos[1];
    char line[256];
    int i,eval;
    double start = getTimeMs();

    FILE *newbook = fopen("26870.epd", "w");
    FILE *book    = fopen(argv[2], "r");
    int positions = 26870;//609034;

    for (i = 0; i < positions; i++) {

        // Read next position from the FEN file
        if (fgets(line, 256, book) == NULL) {
            printf("Unable to read line #%d\n", i);
            exit(EXIT_FAILURE);
        }

        eval = atoi(strstr(line, "] ") + 2);
        /*float result;
        if      (strstr(line, "[1.0]")) result = 1.0;
        else if (strstr(line, "[0.0]")) result = 0.0;
        else if (strstr(line, "[0.5]")) result = 0.5;
        else    {printf("Cannot Parse %s\n", line); exit(EXIT_FAILURE);}*/

        //printf("#%d %f\n",i+1,result );

        //printf("#%d eval %d %s",i+1,eval,strstr(line, "] ")+((eval<=-10000) ? 9 : 8 ));

        ParseFen(strstr(line, "] ") + ((eval<=-10000) ? 9 : 8), pos);
        //ParseFen(line, pos);

        if (!LegalMoveExist(pos))
            continue;

        printf("\rINITIALIZING SCORES FROM FENS...  [%7d OF %7d]", i + 1, positions);
        //fprintf(newbook, "%s %d;\n",strstr(line, "] ")+((eval<=-10000) ? 9 : 8 ),eval );
        fprintf(newbook, "%s", line);
    }

    printf("Time %dms\n", (int)(getTimeMs() - start));
}

int getInput(char *str) {

    char *ptr;

    if (fgets(str, 8192, stdin) == NULL)
        return 0;

    ptr = strchr(str, '\n');
    if (ptr != NULL) *ptr = '\0';

    ptr = strchr(str, '\r');
    if (ptr != NULL) *ptr = '\0';

    return 1;
}

int strEquals(char *str1, char *str2) {
    return strcmp(str1, str2) == 0;
}

int strStartsWith(char *str, char *key) {
    return strstr(str, key) == str;
}

int strContains(char *str, char *key) {
    return strstr(str, key) != NULL;
}