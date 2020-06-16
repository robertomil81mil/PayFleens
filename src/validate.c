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

// validate.c

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "defs.h"
#include "endgame.h"
#include "evaluate.h"
#include "io.h"
#include "makemove.h"
#include "movegen.h"
#include "search.h"
#include "time.h"
#include "ttable.h"
#include "validate.h"

uint64_t leafNodes;

#ifdef DEBUG

int MoveListOk(const S_MOVELIST *list, const S_BOARD *pos) {

	if (list->count < 0 || list->count >= MAXPOSITIONMOVES)
		return 0;

	int from, to;

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		to = TOSQ(list->moves[MoveNum].move);
		from = FROMSQ(list->moves[MoveNum].move);
		if (!SqOnBoard(to) || !SqOnBoard(from))
			return 0;
		
		if (!PieceValid(pos->pieces[from])) {
			PrintBoard(pos);
			return 0;
        }
	}

	return 1;
}

int SqIs120(const int sq) {
	return (sq >= 0 && sq < 120);
}

int PceValidEmptyOffbrd(const int pce) {
	return (PieceValidEmpty(pce) || pce == OFFBOARD);
}

int SqOnBoard(const int sq) {
	return !(FilesBrd[sq] == OFFBOARD);
}

int SideValid(const int side) {
	return (side == WHITE || side == BLACK);
}

int FileRankValid(const int fr) {
	return (fr >= 0 && fr <= 7);
}

int PieceValidEmpty(const int pce) {
	return (pce >= EMPTY && pce <= bK);
}

int PieceValid(const int pce) {
	return (pce >= wP && pce <= bK);
}

void Perft(int depth, S_BOARD *pos) {

    ASSERT(CheckBoard(pos));

    if (depth == 0) {
        leafNodes++;
        return;
    }   

    S_MOVELIST list[1];
    GenerateAllMoves(pos,list);

    for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {   
       
        if (!MakeMove(pos, list->moves[MoveNum].move))
            continue;

        Perft(depth - 1, pos);
        TakeMove(pos);
    }

    return;
}


void PerftTest(int depth, S_BOARD *pos) {

    ASSERT(CheckBoard(pos));

    PrintBoard(pos);
    printf("\nStarting Test To Depth:%d\n",depth);  
    leafNodes = 0;
    int move;
    
    S_MOVELIST list[1];
    GenerateAllMoves(pos,list);         

    for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        move = list->moves[MoveNum].move;
        if (!MakeMove(pos, move))
            continue;
        
        uint64_t cumnodes = leafNodes;
        Perft(depth - 1, pos);
        TakeMove(pos);        
        uint64_t oldnodes = leafNodes - cumnodes;
        printf("move %d : %s : %I64d\n",MoveNum+1, PrMove(move), oldnodes);
    }
    
    printf("\nTest Complete : %I64d nodes visited\n",leafNodes);

    return;
}

#endif

void MirrorEvalTest(S_BOARD *pos) {
    FILE *file;
    file = fopen("../perftsuite.epd","r");
    char lineIn[1024];
    int ev1, ev2, positions = 0;

    if (file == NULL) {
        printf("File Not Found\n");
        return;
    }  else {
        while(fgets(lineIn , 1024 ,file) != NULL) {
            ParseFen(lineIn, pos);
            positions++;
            ev1 = EvalPosition(pos, &Table);
            MirrorBoard(pos);
            ev2 = EvalPosition(pos, &Table);

            if (ev1 != ev2) {
                printf("\n\n\n");
                ParseFen(lineIn, pos);
                PrintBoard(pos);
                printEval(pos);
                MirrorBoard(pos);
                PrintBoard(pos);
                printEval(pos);
                printf("\n\nMirror Fail:\n%s\n",lineIn);
                getchar();
                return;
            }

            if ((positions % 1000) == 0)
                printf("position %d\n",positions);

            memset(&lineIn[0], 0, sizeof(lineIn));
        }
    }

    printf("MirrorEvalTest() finished with succes!\n");
}