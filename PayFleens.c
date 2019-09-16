// PayFleens.c

#include "stdio.h"
#include "defs.h"
#include "stdlib.h"
#include <cstring>

#define FEN1 "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
#define FEN2 "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2"
#define FEN3 "4rrk1/1p3qbp/p2n1p2/2NP2p1/1P1B4/3Q1R2/P5PP/5RK1 b - - 7 30"
#define FEN4 "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
#define FEN5 "3q4/4p3/8/5P2/4Q3/8/8/8 b - - 7 30"

#define PAWNMOVESW "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define PAWNMOVESB "rnbqkbnr/p1p1p3/3p3p/1p1p4/2P1Pp2/8/PP1P1PpP/RNBQKB1R b KQkq e3 0 1"
#define KNIGHTSKINGS "5k2/1n6/4n3/6N1/8/3N4/8/5K2 b - - 0 1"
#define ROOKS "6k1/8/5r2/8/1nR5/5N2/8/6K1 w - - 0 1"
#define QUEENS "6k1/8/4nq2/8/1nQ5/5N2/1N6/6K1 w - - 0 1 "
#define BISHOPS "6k1/1b6/4n3/8/1n4B1/1B3N2/1N6/2b3K1 w - - 0 1 "

#define PERFTFEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

void PrintBin(int move) {

	int index = 0;
	printf("As binary:\n");
	for(index = 27; index >= 0; index--) {
		if( (1<<index) & move) printf("1");
		else printf("0");
		if(index!=20 && index%4==0) printf(" ");
	}
	printf("\n");
}

int main() {

	AllInit();

	S_BOARD pos[1];
    S_SEARCHINFO info[1];
    info->quit = FALSE;
	pos->HashTable->pTable = NULL;
    InitHashTable(pos->HashTable, 64);
	setbuf(stdin, NULL);
    setbuf(stdout, NULL);
	
	printf("Welcome to PayFleens! Type 'pay' for console mode...\n");

	char line[256];
	while (TRUE) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!fgets(line, 256, stdin))
			continue;
		if (line[0] == '\n')
			continue;
		if (!strncmp(line, "uci",3)) {
			Uci_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "xboard",6))	{
			XBoard_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if (!strncmp(line, "pay",3))	{
			Console_Loop(pos, info);
			if(info->quit == TRUE) break;
			continue;
		} else if(!strncmp(line, "quit",4))	{
			break;
		}
	}

	free(pos->HashTable->pTable);
	
	return 0;
}

/*int main() {	

	AllInit();	
	S_BOARD board[1];
	S_MOVELIST list[1];
	ParseFen(START_FEN,board);
	PrintBoard(board);

	
	//GenerateAllMoves(board,list);

	//PrintMoveList(list);
	
	//ParseFen(PERFTFEN,board);
	char input[6];
	int Move = NOMOVE;
	int PvNum = 0;
	int Max = 0;

	while(TRUE) {
		PrintBoard(board);
		printf("Please enter a move > ");
		fgets(input, 6, stdin);
		int count = 0;
		if(input[0]=='q') {
			break;
		} else if(input[0]=='t') {
			TakeMove(board);
		} else if(input[0]=='p') {
			//TakeMove(board);
			Max = GetPvLine(4, board);
			printf("PvLine of %d Moves: ",Max);
			for(PvNum = 0; PvNum < Max; ++PvNum) {
				Move = board->PvArray[PvNum];
				printf(" %s",PrMove(Move));
			}
			printf("\n");
		} else {
			Move = ParseMove(input, board);
			if(Move != NOMOVE) {
				StorePvMove(board, Move);
				MakeMove(board,Move);
				if(IsRepetition(board)) {
					printf("3 REPETITIONs\n");
				}
				
			} else {
				printf("Move not parsed:%s\n",input);
			}
		}
		fflush(stdin);
	}
	//PerftTest(6,board);
	
	
	return 0;
}*/