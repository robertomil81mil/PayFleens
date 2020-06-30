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

// board.c

#include <stdio.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "data.h"
#include "defs.h"
#include "evaluate.h"
#include "hashkeys.h"
#include "validate.h"

const char PceChar[] = ".PNBRQKpnbrqk";
const char SideChar[] = "wb-";
const char RankChar[] = "12345678";
const char FileChar[] = "abcdefgh";

int Sq120ToSq64[BRD_SQ_NUM];
int Sq64ToSq120[64];

int FilesBrd[BRD_SQ_NUM];
int RanksBrd[BRD_SQ_NUM];

int SquareDistance[120][120];
int FileDistance[120][120];
int RankDistance[120][120];

#ifdef DEBUG

int PceListOk(const Board *pos) {

	int pce, num, sq;

	for (pce = wP; pce <= bK; ++pce) {
		if (   pos->pceNum[pce] < 0 
			|| pos->pceNum[pce] >= 10) return 0;
	}

	if (pos->pceNum[wK] != 1 || pos->pceNum[bK] != 1) return 0;

	for (pce = wP; pce <= bK; ++pce) {
		for (num = 0; num < pos->pceNum[pce]; ++num) {
			sq = pos->pList[pce][num];
			if (!SqOnBoard(sq)) return 0;
		}
	}

    return 1;
}

int CheckBoard(const Board *pos) {

	int t_pceNum[13]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int t_bigPce[2]   = { 0, 0 };
	int t_material[2] = { 0, 0 };

	int sq64, t_piece, t_pce_num, sq120, pcount, colour;

	uint64_t t_pawns[3] = {0ULL, 0ULL, 0ULL};

	t_pawns[WHITE] = pos->pawns[WHITE];
	t_pawns[BLACK] = pos->pawns[BLACK];
	t_pawns[COLOUR_NB] = pos->pawns[COLOUR_NB];

	// check piece lists
	for (t_piece = wP; t_piece <= bK; ++t_piece) {
		for (t_pce_num = 0; t_pce_num < pos->pceNum[t_piece]; ++t_pce_num) {
			sq120 = pos->pList[t_piece][t_pce_num];
			ASSERT(pos->pieces[sq120] == t_piece);
		}
	}

	// check piece count and other counters
	for (sq64 = 0; sq64 < 64; ++sq64) {
		sq120 = SQ120(sq64);
		t_piece = pos->pieces[sq120];
		t_pceNum[t_piece]++;
		colour = PieceCol[t_piece];
		t_material[colour] += PieceValue[EG][t_piece];

		if (PieceBig[t_piece]) t_bigPce[colour]++;
	}

	for (t_piece = wP; t_piece <= bK; ++t_piece) {
		ASSERT(t_pceNum[t_piece] == pos->pceNum[t_piece]);
	}

	// check bitboards count
	pcount = popcount(t_pawns[WHITE]);
	ASSERT(pcount == pos->pceNum[wP]);
	pcount = popcount(t_pawns[BLACK]);
	ASSERT(pcount == pos->pceNum[bP]);
	pcount = popcount(t_pawns[COLOUR_NB]);
	ASSERT(pcount == (pos->pceNum[bP] + pos->pceNum[wP]));

	// check bitboards squares
	while (t_pawns[WHITE]) {
		sq64 = poplsb(&t_pawns[WHITE]);
		ASSERT(pos->pieces[SQ120(sq64)] == wP);
	}

	while (t_pawns[BLACK]) {
		sq64 = poplsb(&t_pawns[BLACK]);
		ASSERT(pos->pieces[SQ120(sq64)] == bP);
	}

	while (t_pawns[COLOUR_NB]) {
		sq64 = poplsb(&t_pawns[COLOUR_NB]);
		ASSERT(pos->pieces[SQ120(sq64)] == bP || pos->pieces[SQ120(sq64)] == wP);
	}

	ASSERT(t_material[WHITE] == pos->material[WHITE] && t_material[BLACK] == pos->material[BLACK]);
	ASSERT(t_bigPce[WHITE] == pos->bigPce[WHITE] && t_bigPce[BLACK] == pos->bigPce[BLACK]);

	ASSERT(pos->side==WHITE || pos->side==BLACK);
	ASSERT(GeneratePosKey(pos) == pos->posKey);

	ASSERT(    pos->enPas == NO_SQ
		   || (RanksBrd[pos->enPas] == RANK_6 && pos->side == WHITE)
		   || (RanksBrd[pos->enPas] == RANK_3 && pos->side == BLACK));

	ASSERT(pos->pieces[pos->KingSq[WHITE]] == wK);
	ASSERT(pos->pieces[pos->KingSq[BLACK]] == bK);

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	ASSERT(PceListOk(pos));

	return 1;
}

#endif

void UpdateListsMaterial(Board *pos) {

	int piece, sq, index, colour;

	for (index = 0; index < BRD_SQ_NUM; ++index) {
		sq = index;
		piece = pos->pieces[index];
		ASSERT(PceValidEmptyOffbrd(piece));
		if (piece != OFFBOARD && piece != EMPTY) {
			colour = PieceCol[piece];
			ASSERT(SideValid(colour));

		    if (PieceBig[piece]) pos->bigPce[colour]++;

		    pos->PSQT[colour] += e.PSQT[piece][sq];

			pos->material[colour] += PieceValue[EG][piece];
			pos->mPhases[colour] += PieceValPhases[piece];

			ASSERT(pos->pceNum[piece] < 10 && pos->pceNum[piece] >= 0);

			pos->pList[piece][pos->pceNum[piece]] = sq;
			pos->pceNum[piece]++;

			if (piece == wP || piece == bP) {
				int ne = (colour == WHITE ?  9 :  -9);
				int nw = (colour == WHITE ? 11 : -11);
				pos->pawn_ctrl[colour][sq + ne]++;
				pos->pawn_ctrl[colour][sq + nw]++;	
			}
			
			if (piece == wK) pos->KingSq[WHITE] = sq;
			if (piece == bK) pos->KingSq[BLACK] = sq;

			if (piece == wP) {
				setBit(&pos->pawns[WHITE], SQ64(sq));
				setBit(&pos->pawns[COLOUR_NB], SQ64(sq));
			} else if(piece == bP) {
				setBit(&pos->pawns[BLACK], SQ64(sq));
				setBit(&pos->pawns[COLOUR_NB], SQ64(sq));
			}
		}
	}
}

int ParseFen(char *fen, Board *pos) {

	ASSERT(fen != NULL);
	ASSERT(pos != NULL);

	int rank = RANK_8, file = FILE_A;
    int piece = 0, count = 0, i = 0, sq64 = 0, sq120 = 0;

	ResetBoard(pos);

	while ((rank >= RANK_1) && *fen) {
	    count = 1;
		switch (*fen) {
            case 'p': piece = bP; break;
            case 'r': piece = bR; break;
            case 'n': piece = bN; break;
            case 'b': piece = bB; break;
            case 'k': piece = bK; break;
            case 'q': piece = bQ; break;
            case 'P': piece = wP; break;
            case 'R': piece = wR; break;
            case 'N': piece = wN; break;
            case 'B': piece = wB; break;
            case 'K': piece = wK; break;
            case 'Q': piece = wQ; break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                piece = EMPTY;
                count = *fen - '0';
                break;

            case '/':
            case ' ':
                rank--;
                file = FILE_A;
                fen++;
                continue;

            default:
                printf("FEN error\n");
                return -1;
        }

		for (i = 0; i < count; ++i) {
            sq64 = rank * 8 + file;
			sq120 = SQ120(sq64);

            if (piece != EMPTY)
            	pos->pieces[sq120] = piece;

			file++;
        }
		fen++;
	}

	ASSERT(*fen == 'w' || *fen == 'b');

	pos->side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	for (i = 0; i < 4; i++) {
        if (*fen == ' ') {
            break;
        }
		switch(*fen) {
			case 'K': pos->castlePerm |= WKCA; break;
			case 'Q': pos->castlePerm |= WQCA; break;
			case 'k': pos->castlePerm |= BKCA; break;
			case 'q': pos->castlePerm |= BQCA; break;
			default:	     break;
        }
		fen++;
	}
	fen++;

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	if (*fen != '-') {
		file = fen[0] - 'a';
		rank = fen[1] - '1';

		ASSERT(file >= FILE_A && file <= FILE_H);
		ASSERT(rank >= RANK_1 && rank <= RANK_8);

		pos->enPas = FR2SQ(file, rank);
    }

    fen += 2;
    if (*fen != ' ') {
		sscanf(fen, "%d", &pos->fiftyMove);
		ASSERT(pos->fiftyMove >= 0 && pos->fiftyMove <= 100);
    }

    fen += 2;
    if (*fen != ' ') {
    	int ply = 0;
    	sscanf(fen, "%d", &ply);
		pos->gamePly = ply;
		pos->hisPly = ply;
		ASSERT(pos->hisPly >= 0 && pos->hisPly <= MAXGAMEMOVES);
    }

	pos->posKey = GeneratePosKey(pos);
	pos->gamePly = MAX(2 * (pos->gamePly - 1), 0) + (pos->side == BLACK);

	UpdateListsMaterial(pos);
	pos->materialKey = GenerateMaterialKey(pos);

	return 0;
}

void ResetBoard(Board *pos) {

	int index = 0;

	for (index = 0; index < BRD_SQ_NUM; ++index) {
		pos->pieces[index] = OFFBOARD;
		pos->pawn_ctrl[WHITE][index] = 0;
	    pos->pawn_ctrl[BLACK][index] = 0;
	}

	for (index = 0; index < 2; ++index) {
		pos->bigPce[index]   = 0;
		pos->material[index] = 0;
		pos->mPhases[index]  = 0;
		pos->PSQT[index]     = 0;
	}

	for (index = 0; index < 64; ++index)
		pos->pieces[SQ120(index)] = EMPTY;

	for (index = 0; index < 3; ++index)
		pos->pawns[index] = 0ull;

	for (index = 0; index < 13; ++index)
		pos->pceNum[index] = 0;

	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	pos->side = COLOUR_NB;
	pos->enPas = NO_SQ;
	pos->fiftyMove = 0;

	pos->ply = 0;
	pos->hisPly = 0;

	pos->castlePerm = 0;

	pos->posKey = pos->materialKey = 0ULL;
}

void PrintBoard(const Board *pos) {

	int sq,file,rank,piece;

	printf("\nGame Board:\n\n");

	for (rank = RANK_8; rank >= RANK_1; --rank) {
		printf("%d  ", rank+1);
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file, rank);
			piece = pos->pieces[sq];
			printf("%3c",PceChar[piece]);
		}
		printf("\n");
	}

	printf("\n   ");
	for (file = FILE_A; file <= FILE_H; ++file) {
		printf("%3c",'a'+file);
	}
	printf("\n");
	printf("side:%c\n",SideChar[pos->side]);
	printf("enPas:%d\n",pos->enPas);
	printf("castle:%c%c%c%c\n",
			pos->castlePerm & WKCA ? 'K' : '-',
			pos->castlePerm & WQCA ? 'Q' : '-',
			pos->castlePerm & BKCA ? 'k' : '-',
			pos->castlePerm & BQCA ? 'q' : '-'
			);
	printf("PosKey:%"PRIu64"\n",pos->posKey);
}

void MirrorBoard(Board *pos) {

    int tempPiecesArray[64];
    int tempSide = !pos->side;
	int SwapPiece[13] = { EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK };
    int tempCastlePerm = 0;
    int tempEnPas = NO_SQ;
	int sq, tp;

    if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
    if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;

    if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
    if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

	if (pos->enPas != NO_SQ)
        tempEnPas = SQ120(Mirror64[SQ64(pos->enPas)]);

    for (sq = 0; sq < 64; ++sq)
        tempPiecesArray[sq] = pos->pieces[SQ120(Mirror64[sq])];

    ResetBoard(pos);

	for (sq = 0; sq < 64; ++sq) {
        tp = SwapPiece[tempPiecesArray[sq]];
        pos->pieces[SQ120(sq)] = tp;
    }

	pos->side = tempSide;
    pos->castlePerm = tempCastlePerm;
    pos->enPas = tempEnPas;

    pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);
	pos->materialKey = GenerateMaterialKey(pos);

    ASSERT(CheckBoard(pos));
}

int pieceType(int piece) {
    ASSERT(0 <= piece && piece < PIECE_NB);
    ASSERT(0 <= PieceCol[piece] && PieceCol[piece] <= COLOUR_NB);
    return PieceType[piece];
}

int IsRepetition(const Board *pos) {

    int index = 0;

    for (index = pos->hisPly - pos->fiftyMove; index < pos->hisPly-1; ++index) {
        ASSERT(index >= 0 && index < MAXGAMEMOVES);
        if (pos->posKey == pos->history[index].posKey) {
            return 1;
        }
    }
    return 0;
}

int posIsDrawn(const Board *pos, int ply) {

    if (    pos->fiftyMove > 99 
    	&& !SqAttacked(pos->KingSq[pos->side],!pos->side, pos))
        return 1;

    // Return a draw score if a position has a two fold after the root,
    // or a three fold which occurs in part before the root move.
    int end = MIN(pos->fiftyMove, pos->plyFromNull);

    if (end >= 4) {

        int reps = 0;
        
        for (int i = pos->hisPly - 4; i >= pos->hisPly - end; i -= 2) {

            if (    pos->posKey == pos->history[i].posKey
                && (i > pos->hisPly - ply || ++reps == 2))
                return 1;
        }
    }

    return 0;
}

void InitFilesRanksBrd() {

	int sq;

	for (int index = 0; index < BRD_SQ_NUM; ++index) {
		FilesBrd[index] = OFFBOARD;
		RanksBrd[index] = OFFBOARD;
	}

	for (int rank = RANK_1; rank <= RANK_8; ++rank) {
		for (int file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file,rank);
			FilesBrd[sq] = file;
			RanksBrd[sq] = rank;
		}
	}

	for (int s1 = 0; s1 < 120; ++s1) {
      	for (int s2 = 0; s2 < 120; ++s2) { 
      		FileDistance[s1][s2]   = abs(FilesBrd[s1] - FilesBrd[s2]);
            RankDistance[s1][s2]   = abs(RanksBrd[s1] - RanksBrd[s2]);
            SquareDistance[s1][s2] = MAX(FileDistance[s1][s2], RankDistance[s1][s2]);
      	}
	}
}

void InitSq120To64() {

	int file, rank, sq, index, sq64 = 0;

	for (index = 0; index < BRD_SQ_NUM; ++index)
		Sq120ToSq64[index] = 65;

	for (index = 0; index < 64; ++index)
		Sq64ToSq120[index] = 120;

	for (rank = RANK_1; rank <= RANK_8; ++rank) {
		for (file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file, rank);
			ASSERT(SqOnBoard(sq));
			Sq64ToSq120[sq64] = sq;
			Sq120ToSq64[sq] = sq64;
			sq64++;
		}
	}
}