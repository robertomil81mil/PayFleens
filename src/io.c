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

// io.c

#include <stdio.h>

#include "attack.h"
#include "board.h"
#include "defs.h"
#include "io.h"
#include "makemove.h"
#include "movegen.h"
#include "validate.h"

char *PrSq(const int sq) {

	static char SqStr[3];

	int file = FilesBrd[sq];
	int rank = RanksBrd[sq];

	sprintf(SqStr, "%c%c", ('a'+file), ('1'+rank));

	return SqStr;
}

char *PrMove(const int move) {

	static char MvStr[6];

	int ff = FilesBrd[FROMSQ(move)];
	int rf = RanksBrd[FROMSQ(move)];
	int ft = FilesBrd[TOSQ(move)];
	int rt = RanksBrd[TOSQ(move)];

	int promoted = PROMOTED(move);

	if (promoted) {
		char pchar = 'q';

		if (IsKn(promoted))
			pchar = 'n';

		else if (IsRQ(promoted) && !IsBQ(promoted))
			pchar = 'r';

		else if (!IsRQ(promoted) && IsBQ(promoted))
			pchar = 'b';

		sprintf(MvStr, "%c%c%c%c%c", ('a'+ff), ('1'+rf), ('a'+ft), ('1'+rt), pchar);

	}

	else
		sprintf(MvStr, "%c%c%c%c", ('a'+ff), ('1'+rf), ('a'+ft), ('1'+rt));

	return MvStr;
}

int ParseMove(char *ptrChar, Board *pos) {

	ASSERT(CheckBoard(pos));

	if (ptrChar[1] > '8' || ptrChar[1] < '1') return NONE_MOVE;
    if (ptrChar[3] > '8' || ptrChar[3] < '1') return NONE_MOVE;
    if (ptrChar[0] > 'h' || ptrChar[0] < 'a') return NONE_MOVE;
    if (ptrChar[2] > 'h' || ptrChar[2] < 'a') return NONE_MOVE;

    int from = FR2SQ(ptrChar[0] - 'a', ptrChar[1] - '1');
    int to = FR2SQ(ptrChar[2] - 'a', ptrChar[3] - '1');

	ASSERT(SqOnBoard(from) && SqOnBoard(to));

	MoveList list = {0};
    GenerateAllMoves(pos, &list);
    int move = 0, PromPce = EMPTY;

	for (int MoveNum = 0; MoveNum < list.count; ++MoveNum) {
		move = list.moves[MoveNum].move;
		if (FROMSQ(move) == from && TOSQ(move) == to) {
			PromPce = PROMOTED(move);

			if (PromPce != EMPTY) {

				if (IsRQ(PromPce) && !IsBQ(PromPce) && ptrChar[4] == 'r')
					return move;

				else if (!IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4]== 'b')
					return move;

				else if (IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4]== 'q')
					return move;

				else if (IsKn(PromPce)&& ptrChar[4] == 'n')
					return move;

				continue;
			}
			return move;
		}
    }

    return NONE_MOVE;
}

void PrintMoveList(const MoveList *list) {

	int score = 0, move = 0;
	printf("MoveList:\n");

	for (int index = 0; index < list->count; ++index) {

		move = list->moves[index].move;
		score = list->moves[index].score;

		printf("Move:%d > %s (score:%d)\n",index+1,PrMove(move),score);
	}

	printf("MoveList Total %d Moves:\n\n",list->count);
}