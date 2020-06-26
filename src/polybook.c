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

#include "board.h"
#include "defs.h"
#include "io.h"
#include "polybook.h"
#include "polykeys.h"
#include "uci.h"

typedef struct {
	uint64_t key;
	unsigned short move;
	unsigned short weight;
	unsigned int learn;
	
} S_POLY_BOOK_ENTRY;

long NumEntries = 0;

S_POLY_BOOK_ENTRY *entries;

const int PolyKindOfPiece[13] = {
	-1, 1, 3, 5, 7, 9, 11, 0, 2, 4, 6, 8, 10
};

void InitPolyBook() {
	
	if (Options.PolyBook) {

		FILE *pFile = fopen("../book.bin","rb");

		if (pFile == NULL) { 
			printf("info string Could not open ../book.bin\n");
			Options.PolyBook = 0; return;
		} else {

			fseek(pFile,0,SEEK_END);
			long position = ftell(pFile);
			
			if ((unsigned int)(position) < sizeof(S_POLY_BOOK_ENTRY)) {
				printf("No Entries Found\n");
				Options.PolyBook = 0; return;
			}
			
			NumEntries = position / sizeof(S_POLY_BOOK_ENTRY);
			printf("%ld Entries Found In File\n", NumEntries);
			
			entries = (S_POLY_BOOK_ENTRY*)malloc(NumEntries * sizeof(S_POLY_BOOK_ENTRY));
			rewind(pFile);
			fread(entries, sizeof(S_POLY_BOOK_ENTRY), NumEntries, pFile);
			
			printf("info string Book loaded: ../book.bin\n");

			CleanPolyBook();
		}
	}
}

void CleanPolyBook() {
	free(entries);
}

int HasPawnForCapture(const Board *board) {
	int sqWithPawn = 0;
	int targetPce = board->side == WHITE ? wP : bP;

	if (board->enPas != NO_SQ) {

		if (board->side == WHITE)
			sqWithPawn = board->enPas - 10;
		else
			sqWithPawn = board->enPas + 10;
		
		if (board->pieces[sqWithPawn + 1] == targetPce)
			return 1;

		else if (board->pieces[sqWithPawn - 1] == targetPce)
			return 1;
	}

	return 0;
}

uint64_t PolyKeyFromBoard(const Board *board) {

	uint64_t finalKey = 0;
	int piece, polyPiece, rank, file, offset = 768;
	
	for (int sq = 0; sq < BRD_SQ_NUM; ++sq) {
		piece = board->pieces[sq];
		if (piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
			ASSERT(piece >= wP && piece <= bK);
			polyPiece = PolyKindOfPiece[piece];
			rank = RanksBrd[sq];
			file = FilesBrd[sq];
			finalKey ^= Random64Poly[(64 * polyPiece) + (8 * rank) + file];
		}
	}
	
	// castling
	if (board->castlePerm & WKCA) finalKey ^= Random64Poly[offset + 0];
	if (board->castlePerm & WQCA) finalKey ^= Random64Poly[offset + 1];
	if (board->castlePerm & BKCA) finalKey ^= Random64Poly[offset + 2];
	if (board->castlePerm & BQCA) finalKey ^= Random64Poly[offset + 3];
	
	// enpassant
	offset = 772;
	if (HasPawnForCapture(board)) {
		file = FilesBrd[board->enPas];
		finalKey ^= Random64Poly[offset + file];
	}
	
	if (board->side == WHITE)
		finalKey ^= Random64Poly[780];

	return finalKey;
}

uint16_t endian_swap_u16(uint16_t x) 
{ 
    x = (x>>8) | 
        (x<<8); 
    return x;
} 

uint32_t endian_swap_u32(uint32_t x) 
{ 
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) | 
        ((x>>8) & 0x0000FF00) | 
        (x<<24); 
    return x;
} 

uint64_t endian_swap_u64(uint64_t x) 
{ 
    x = (x>>56) | 
        ((x<<40) & 0x00FF000000000000) | 
        ((x<<24) & 0x0000FF0000000000) | 
        ((x<<8)  & 0x000000FF00000000) | 
        ((x>>8)  & 0x00000000FF000000) | 
        ((x>>24) & 0x0000000000FF0000) | 
        ((x>>40) & 0x000000000000FF00) | 
        (x<<56); 
    return x;
}

int ConvertPolyMoveToInternalMove(uint16_t polyMove, Board *board) {
	
	int ff = (polyMove >> 6) & 7;
	int fr = (polyMove >> 9) & 7;
	int tf = (polyMove >> 0) & 7;
	int tr = (polyMove >> 3) & 7;
	int pp = (polyMove >> 12) & 7;
	
	char moveString[6];
	if (pp == 0) {
		sprintf(moveString, "%c%c%c%c",
		FileChar[ff],
		RankChar[fr],
		FileChar[tf],
		RankChar[tr]);
	} else {
		char promChar = 'q';
		switch(pp) {
			case 1: promChar = 'n'; break;
			case 2: promChar = 'b'; break;
			case 3: promChar = 'r'; break;
		}
		sprintf(moveString, "%c%c%c%c%c",
		FileChar[ff],
		RankChar[fr],
		FileChar[tf],
		RankChar[tr],
		promChar);
	}
	
	return ParseMove(moveString, board);
}

int GetBookMove(Board *board) {

	const int MAXBOOKMOVES = 64;
	S_POLY_BOOK_ENTRY *entry;
	uint16_t move;
	int bookMoves[MAXBOOKMOVES];
	int tempMove, count = 0;

	uint64_t polyKey = PolyKeyFromBoard(board);
	
	for (entry = entries; entry < entries + NumEntries; ++entry) {
		if (polyKey == endian_swap_u64(entry->key)) {
			move = endian_swap_u16(entry->move);
			tempMove = ConvertPolyMoveToInternalMove(move, board);
			if (tempMove != NOMOVE) {
				bookMoves[count++] = tempMove;
				if (count > MAXBOOKMOVES) break;
			}
		}
	}
	
	if (count != 0) {
		for (int index = 0; index < count; ++index) {
			//printf("BookMove:%d : %s\n", index+1, PrMove(bookMoves[index]));
			return bookMoves[index];
		}
	} 
	return NOMOVE;
}