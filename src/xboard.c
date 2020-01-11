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

// xboard.c

#include "stdio.h"
#include "string.h"
#include "defs.h"
#include "search.h"
#include "evaluate.h"
#include "time.h"
#include "ttable.h"

void PrintNonBits2(int side, int sq) {

	int tsq,file,rank;

	printf("\n%c:\n\n",SideChar[side]);

	for(rank = RANK_8; rank >= RANK_1; rank--) {
		printf("%d  ",rank+1);
		for(file = FILE_A; file <= FILE_H; file++) {
			tsq = FR2SQ(file,rank);	
			printf("%3d",e.sqNearK[side][sq][tsq]);
		}
		printf("\n");
	}

	printf("\n   ");
	for(file = FILE_A; file <= FILE_H; file++) {
		printf("%3c",'a'+file);
	}
}

int ThreeFoldRep(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	int i = 0, r = 0;
	for (i = 0; i < pos->hisPly; ++i)	{
	    if (pos->history[i].posKey == pos->posKey) {
		    r++;
		}
	}
	return r;
}

int DrawMaterial(const S_BOARD *pos) {
	ASSERT(CheckBoard(pos));

    if (pos->pceNum[wP] || pos->pceNum[bP]) return FALSE;
    if (pos->pceNum[wQ] || pos->pceNum[bQ] || pos->pceNum[wR] || pos->pceNum[bR]) return FALSE;
    if (pos->pceNum[wB] > 1 || pos->pceNum[bB] > 1) {return FALSE;}
    if (pos->pceNum[wN] > 1 || pos->pceNum[bN] > 1) {return FALSE;}
    if (pos->pceNum[wN] && pos->pceNum[wB]) {return FALSE;}
    if (pos->pceNum[bN] && pos->pceNum[bB]) {return FALSE;}

    return TRUE;
}

int checkresult(S_BOARD *pos) {
	ASSERT(CheckBoard(pos));

    if (pos->fiftyMove > 100) {
     printf("1/2-1/2 {fifty move rule (claimed by Vice)}\n"); return TRUE;
    }

    if (ThreeFoldRep(pos) >= 2) {
     printf("1/2-1/2 {3-fold repetition (claimed by Vice)}\n"); return TRUE;
    }

	if (DrawMaterial(pos) == TRUE) {
     printf("1/2-1/2 {insufficient material (claimed by Vice)}\n"); return TRUE;
    }

	S_MOVELIST list[1];
    GenerateAllMoves(pos,list);

    int MoveNum = 0;
	int found = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        if ( !MakeMove(pos,list->moves[MoveNum].move))  {
            continue;
        }
        found++;
		TakeMove(pos);
		break;
    }

	if(found != 0) return FALSE;

	int InCheck = SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);

	if(InCheck == TRUE)	{
	    if(pos->side == WHITE) {
	      printf("0-1 {black mates (claimed by Vice)}\n");return TRUE;
        } else {
	      printf("1-0 {white mates (claimed by Vice)}\n");return TRUE;
        }
    } else {
      printf("\n1/2-1/2 {stalemate (claimed by Vice)}\n");return TRUE;
    }
	return FALSE;
}

void PrintOptions() {
	printf("feature ping=1 setboard=1 colors=0 usermove=1 memory=1\n");
	printf("feature done=1\n");
}

void XBoard_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	info->GAME_MODE = XBOARDMODE;
	info->POST_THINKING = TRUE;
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);
	PrintOptions(); // HACK

	int depth = -1, movestogo[2] = {0,0 };
	int movetime = -1, time = -1, inc = 0;
	int engineSide = BOTH;
	int timeLeft;
	int sec;
	int mps;
	int move = NOMOVE;
	char inBuf[80], command[80];
	int MB;

	engineSide = BLACK;
	ParseFen(START_FEN, pos);
	depth = -1;
	time = -1;

	while(TRUE) {

		fflush(stdout);

		if(pos->side == engineSide && checkresult(pos) == FALSE) {
			info->startTime = getTimeMs();
			info->depth = depth;
			updateTTable();

			if(time != -1) {
				info->timeset = TRUE;

				//TimeManagementInit(info, time, inc, pos->gamePly, movestogo[pos->side]);
				TimeManagementInit(info, time, inc, pos->gamePly, 30);
				info->stoptime = info->startTime + info->optimumTime;
			}

			if(depth == -1 || depth > MAXDEPTH) {
				info->depth = MAXDEPTH;
			}

			printf("time:%d start:%f stop:%f depth:%d timeset:%d movestogo:%d mps:%d\n",
				time,info->startTime,info->stoptime,info->depth,info->timeset, movestogo[pos->side], mps);
				SearchPosition(pos, info);

			if(mps != 0) {
				movestogo[pos->side^1]--;
				if(movestogo[pos->side^1] < 1) {
					movestogo[pos->side^1] = mps;
				}
			}
		}

		fflush(stdout);

		memset(&inBuf[0], 0, sizeof(inBuf));
		fflush(stdout);
		if (!fgets(inBuf, 80, stdin))
		continue;

		sscanf(inBuf, "%s", command);

		printf("command seen:%s\n",inBuf);

		if(!strcmp(command, "quit")) {
			info->quit = TRUE;
			break;
		}

		if(!strcmp(command, "force")) {
			engineSide = BOTH;
			continue;
		}

		if(!strcmp(command, "protover")){
			PrintOptions();
		    continue;
		}

		if(!strcmp(command, "sd")) {
			sscanf(inBuf, "sd %d", &depth);
		    printf("DEBUG depth:%d\n",depth);
			continue;
		}

		if(!strcmp(command, "st")) {
			sscanf(inBuf, "st %d", &movetime);
		    printf("DEBUG movetime:%d\n",movetime);
			continue;
		}

		if(!strcmp(command, "time")) {
			sscanf(inBuf, "time %d", &time);
			time *= 10;
		    printf("DEBUG time:%d\n",time);
			continue;
		}
		
		if(!strcmp(command, "memory")) {			
			sscanf(inBuf, "memory %d", &MB);		
			initTTable(MB);
			printf("Set Hash to %d MB\n",MB);
			continue;
		}

		if(!strcmp(command, "level")) {
			sec = 0;
			movetime = -1;
			if( sscanf(inBuf, "level %d %d %d", &mps, &timeLeft, &inc) != 3) {
			  sscanf(inBuf, "level %d %d:%d %d", &mps, &timeLeft, &sec, &inc);
		      printf("DEBUG level with :\n");
			}	else {
		      printf("DEBUG level without :\n");
			}
			timeLeft *= 60000;
			timeLeft += sec * 1000;
			if(mps != 0) {
				movestogo[0] = movestogo[1] = mps;
			}
			time = -1;
		    printf("DEBUG level timeLeft:%d movesToGo:%d inc:%d mps%d\n",timeLeft,movestogo[0],inc,mps);
			continue;
		}

		if(!strcmp(command, "ping")) {
			printf("pong%s\n", inBuf+4);
			continue;
		}

		if(!strcmp(command, "new")) {
			clearTTable();
			engineSide = BLACK;
			ParseFen(START_FEN, pos);
			depth = -1;
			time = -1;
			continue;
		}

		if(!strcmp(command, "setboard")){
			engineSide = BOTH;
			ParseFen(inBuf+9, pos);
			continue;
		}

		/*if (!strcmp(command, "Book")) {
			
			EngineOptions->UseBook = TRUE;
		}*/

		if(!strcmp(command, "go")) {
			engineSide = pos->side;
			continue;
		}

		if(!strcmp(command, "usermove")){

			move = ParseMove(inBuf+9, pos);
			if(move == NOMOVE) continue;
			MakeMove(pos, move);
            pos->ply=0;
		}
    }
}


void Console_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	printf("Welcome to PayFleens In Console Mode!\n");
	printf("Type help for commands\n\n");

	info->GAME_MODE = CONSOLEMODE;
	info->POST_THINKING = TRUE;
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);

	int depth = MAXDEPTH, movetime = 3000;
	int engineSide = BOTH;
	int move = NOMOVE;
	char inBuf[80], command[80];

	engineSide = BLACK;
	ParseFen(START_FEN, pos);

	while(TRUE) {

		fflush(stdout);

		if(pos->side == engineSide && checkresult(pos) == FALSE) {
			info->startTime = getTimeMs();
			info->depth = depth;
			updateTTable();

			if(movetime != 0) {
				info->timeset = TRUE;
				info->stoptime = info->startTime + movetime;
			}

			SearchPosition(pos, info);
		}

		printf("\nPayFleens > ");

		fflush(stdout);

		memset(&inBuf[0], 0, sizeof(inBuf));
		fflush(stdout);
		if (!fgets(inBuf, 80, stdin))
		continue;

		sscanf(inBuf, "%s", command);

		if(!strcmp(command, "help")) {
			printf("Commands:\n");
			printf("quit - quit game\n");
			printf("force - computer will not think\n");
			printf("print - show board\n");
			printf("post - show thinking\n");
			printf("nopost - do not show thinking\n");
			printf("new - start new game\n");
			printf("go - set computer thinking\n");
			printf("depth x - set depth to x\n");
			printf("time x - set thinking time to x seconds (depth still applies if set)\n");
			printf("view - show current depth and movetime settings\n");
			printf("setboard x - set position to fen x\n");
			printf("** note ** - to reset time and depth, set to 0\n");
			printf("enter moves using b7b8q notation\n\n\n");
			continue;
		}

		if(!strcmp(command, "mirror")) {
			engineSide = BOTH;
			MirrorEvalTest(pos);
			continue;
		}

		if(!strcmp(command, "eval")) {
			PrintBoard(pos);
			printEval(pos);
			//printf("moveBestCaseValue %d\n", moveBestCaseValue(pos));
			
			/*for(int sq = 0; sq < 120; ++sq) {
				if(!SQOFFBOARD(sq)) {
					PrintNonBits2(WHITE,sq);
    				//PrintNonBits2(BLACK,sq);
				}
			}*/
			
			//MirrorBoard(pos);
			//PrintBoard(pos);
			//printEval(pos);
			continue;
		}

		if(!strcmp(command, "take")) {
			engineSide = BOTH;
			TakeMove(pos);
			continue;
		}

		if(!strcmp(command, "perft")) {
			PerftTest(2, pos);
			continue;
		}

		if(!strcmp(command, "setboard")){
			engineSide = BOTH;
			ParseFen(inBuf+9, pos);
			continue;
		}

		if(!strcmp(command, "fen3")){
			engineSide = BOTH;
			ParseFen(FEN3, pos);
			continue;
		}

		if(!strcmp(command, "fen4")){
			engineSide = BOTH;
			ParseFen(FEN4, pos);
			continue;
		}

		if(!strcmp(command, "quit")) {
			info->quit = TRUE;
			break;
		}

		if(!strcmp(command, "post")) {
			info->POST_THINKING = TRUE;
			continue;
		}

		if(!strcmp(command, "print")) {
			PrintBoard(pos);
			continue;
		}

		if(!strcmp(command, "nopost")) {
			info->POST_THINKING = FALSE;
			continue;
		}

		if(!strcmp(command, "force")) {
			engineSide = BOTH;
			continue;
		}

		if(!strcmp(command, "view")) {
			if(depth == MAXDEPTH) printf("depth not set ");
			else printf("depth %d",depth);

			if(movetime != 0) printf(" movetime %ds\n",movetime/1000);
			else printf(" movetime not set\n");

			continue;
		}

		if(!strcmp(command, "depth")) {
			sscanf(inBuf, "depth %d", &depth);
		    if(depth==0) depth = MAXDEPTH;
			continue;
		}

		if(!strcmp(command, "time")) {
			sscanf(inBuf, "time %d", &movetime);
			movetime *= 1000;
			continue;
		}

		if(!strcmp(command, "new")) {
			clearTTable();
			engineSide = BLACK;
			ParseFen(START_FEN, pos);
			continue;
		}

		if(!strcmp(command, "go")) {
			engineSide = pos->side;
			continue;
		}

		move = ParseMove(inBuf, pos);
		if(move == NOMOVE) {
			printf("Command unknown:%s\n",inBuf);
			continue;
		}
		MakeMove(pos, move);
		pos->ply=0;
    }
}