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

#include "stdio.h"
#include "string.h"
#include "defs.h"
#include "evaluate.h"
#include "search.h"
#include "time.h"
#include "ttable.h"

int MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos) {
	if(list->count < 0 || list->count >= MAXPOSITIONMOVES) {
		return FALSE;
	}

	int MoveNum;
	int from = 0;
	int to = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		to = TOSQ(list->moves[MoveNum].move);
		from = FROMSQ(list->moves[MoveNum].move);
		if(!SqOnBoard(to) || !SqOnBoard(from)) {
			return FALSE;
		}
		if(!PieceValid(pos->pieces[from])) {
			PrintBoard(pos);
			return FALSE;
		}
	}

	return TRUE;
}

int SqIs120(const int sq) {
	return (sq>=0 && sq<120);
}

int PceValidEmptyOffbrd(const int pce) {
	return (PieceValidEmpty(pce) || pce == OFFBOARD);
}
int SqOnBoard(const int sq) {
	return FilesBrd[sq]==OFFBOARD ? 0 : 1;
}

int SideValid(const int side) {
	return (side==WHITE || side == BLACK) ? 1 : 0;
}

int FileRankValid(const int fr) {
	return (fr >= 0 && fr <= 7) ? 1 : 0;
}

int PieceValidEmpty(const int pce) {
	return (pce >= EMPTY && pce <= bK) ? 1 : 0;
}

int PieceValid(const int pce) {
	return (pce >= wP && pce <= bK) ? 1 : 0;
}

void DebugAnalysisTest(S_BOARD *pos, S_SEARCHINFO *info) {

	FILE *file;
    file = fopen("lct2.epd","r");
    char lineIn [1024];

	info->depth = MAXDEPTH;
	info->timeset = TRUE;
	int time = 1140000;


    if(file == NULL) {
        printf("File Not Found\n");
        return;
    }  else {
        while(fgets (lineIn , 1024 , file) != NULL) {
			info->startTime = getTimeMs();
			info->stoptime = info->startTime + time;
			clearTTable();
            ParseFen(lineIn, pos);
            printf("\n%s\n",lineIn);
			printf("time:%d start:%f stop:%f depth:%d timeset:%d\n",
				time,info->startTime,info->stoptime,info->depth,info->timeset);
			SearchPosition(pos, info);
            memset(&lineIn[0], 0, sizeof(lineIn));
        }
    }
}



void MirrorEvalTest(S_BOARD *pos) {
    FILE *file;
    file = fopen("perftsuite.epd","r");
    char lineIn [1024];
    int ev1 = 0; int ev2 = 0;
    int positions = 0;
    if(file == NULL) {
        printf("File Not Found\n");
        return;
    }  else {
        while(fgets (lineIn , 1024 , file) != NULL) {
            ParseFen(lineIn, pos);
            positions++;
            ev1 = EvalPosition(pos);
            MirrorBoard(pos);
            ev2 = EvalPosition(pos);

            if(ev1 != ev2) {
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

            if( (positions % 1000) == 0)   {
                printf("position %d\n",positions);
            }

            memset(&lineIn[0], 0, sizeof(lineIn));
        }
    }
}