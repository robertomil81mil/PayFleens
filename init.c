// init.c

#include "defs.h"
#include "stdio.h"
#include "stdlib.h"
#include "evaluate.h"

#define RAND_64 	((U64)rand() | \
					(U64)rand() << 15 | \
					(U64)rand() << 30 | \
					(U64)rand() << 45 | \
					((U64)rand() & 0xf) << 60 )

int index_black[64] = {
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1
};

int index_white[64] = {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

int Sq120ToSq64[BRD_SQ_NUM];
int Sq64ToSq120[64];

U64 SetMask[64];
U64 ClearMask[64];

U64 PieceKeys[13][120];
U64 SideKey;
U64 CastleKeys[16];

int FilesBrd[BRD_SQ_NUM];
int RanksBrd[BRD_SQ_NUM];

U64 FileBBMask[8];
U64 RankBBMask[8];

U64 BlackPassedMask[64];
U64 WhitePassedMask[64];
U64 IsolatedMask[64];
U64 SqBlocker[2][64];

const U64 QueenSide   =  FileBBMask[FILE_A] | FileBBMask[FILE_B] | FileBBMask[FILE_C] | FileBBMask[FILE_D];
const U64 CenterFiles =  FileBBMask[FILE_C] | FileBBMask[FILE_D] | FileBBMask[FILE_E] | FileBBMask[FILE_F];
const U64 KingSide    =  FileBBMask[FILE_E] | FileBBMask[FILE_F] | FileBBMask[FILE_G] | FileBBMask[FILE_H];
const U64 Center      = (FileBBMask[FILE_D] | FileBBMask[FILE_E]) & (RankBBMask[RANK_4] | RankBBMask[RANK_5]);

const U64 KingFlank[FILE_NONE] = {
	QueenSide ^ FileBBMask[FILE_D], QueenSide, QueenSide,
	CenterFiles, CenterFiles,
	KingSide, KingSide, KingSide ^ FileBBMask[FILE_E]
};

int SquareDistance[120][120];
S_OPTIONS EngineOptions[1];
//EVAL_DATA e[1];

void InitEvalMasks() {

	int sq, tsq, r, f;

	for(sq = 0; sq < 8; ++sq) {
        FileBBMask[sq] = 0ULL;
		RankBBMask[sq] = 0ULL;
	}

	for(r = RANK_8; r >= RANK_1; r--) {
        for (f = FILE_A; f <= FILE_H; f++) {
            sq = r * 8 + f;
            FileBBMask[f] |= (1ULL << sq);
            RankBBMask[r] |= (1ULL << sq);
        }
	}

	for(sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = 0ULL;
		WhitePassedMask[sq] = 0ULL;
		BlackPassedMask[sq] = 0ULL;
		SqBlocker[WHITE][sq] = 0ULL;
		SqBlocker[BLACK][sq] = 0ULL;
    }

	for(sq = 0; sq < 64; ++sq) {
		tsq = sq + 8;
		SqBlocker[WHITE][sq] |= (1ULL << tsq);
        while(tsq < 64) {
            WhitePassedMask[sq] |= (1ULL << tsq);
            tsq += 8;        
        }

        tsq = sq - 8;
        SqBlocker[BLACK][sq] |= (1ULL << tsq);
        while(tsq >= 0) {
            BlackPassedMask[sq] |= (1ULL << tsq);
            tsq -= 8;
        }

        if(FilesBrd[SQ120(sq)] > FILE_A) {
            IsolatedMask[sq] |= FileBBMask[FilesBrd[SQ120(sq)] - 1];

            tsq = sq + 7;
            while(tsq < 64) {
                WhitePassedMask[sq] |= (1ULL << tsq);
                tsq += 8;
            }

            tsq = sq - 9;
            while(tsq >= 0) {
                BlackPassedMask[sq] |= (1ULL << tsq);
                tsq -= 8;
            }
        }

        if(FilesBrd[SQ120(sq)] < FILE_H) {
            IsolatedMask[sq] |= FileBBMask[FilesBrd[SQ120(sq)] + 1];

            tsq = sq + 9;
            while(tsq < 64) {
                WhitePassedMask[sq] |= (1ULL << tsq);
                tsq += 8;
            }

            tsq = sq - 7;
            while(tsq >= 0) {
                BlackPassedMask[sq] |= (1ULL << tsq);
                tsq -= 8;
            }
        }
	}
}

void InitFilesRanksBrd() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;
	int s1,s2;

	for(index = 0; index < BRD_SQ_NUM; ++index) {
		FilesBrd[index] = OFFBOARD;
		RanksBrd[index] = OFFBOARD;
	}

	for(rank = RANK_1; rank <= RANK_8; ++rank) {
		for(file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file,rank);
			FilesBrd[sq] = file;
			RanksBrd[sq] = rank;
		}
	}

	for (s1 = 0; s1 < 120; ++s1) {
      	for (s2 = 0; s2 < 120; ++s2) { 
            SquareDistance[s1][s2] = MAX(abs(FilesBrd[s1] - FilesBrd[s2]), abs(RanksBrd[s1] - RanksBrd[s2]));
      	}
	}
}

void InitHashKeys() {

	int index = 0;
	int index2 = 0;
	for(index = 0; index < 13; ++index) {
		for(index2 = 0; index2 < 120; ++index2) {
			PieceKeys[index][index2] = RAND_64;
		}
	}
	SideKey = RAND_64;
	for(index = 0; index < 16; ++index) {
		CastleKeys[index] = RAND_64;
	}

}

void InitBitMasks() {
	int index = 0;

	for(index = 0; index < 64; index++) {
		SetMask[index] = 0ULL;
		ClearMask[index] = 0ULL;
	}

	for(index = 0; index < 64; index++) {
		SetMask[index] |= (1ULL << index);
		ClearMask[index] = ~SetMask[index];
	}
}

void InitSq120To64() {

	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;
	int sq64 = 0;
	for(index = 0; index < BRD_SQ_NUM; ++index) {
		Sq120ToSq64[index] = 65;
	}

	for(index = 0; index < 64; ++index) {
		Sq64ToSq120[index] = 120;
	}

	for(rank = RANK_1; rank <= RANK_8; ++rank) {
		for(file = FILE_A; file <= FILE_H; ++file) {
			sq = FR2SQ(file,rank);
			ASSERT(SqOnBoard(sq));
			Sq64ToSq120[sq64] = sq;
			Sq120ToSq64[sq] = sq64;
			sq64++;
		}
	}
}

U64 kingAreaMasks(int colour, int sq) {
    ASSERT(0 <= colour && colour < BOTH);
    ASSERT(0 <= sq && sq < 64);
    return KingAreaMasks[colour][sq];
}

U64 pawnAttacks(int colour, int sq) {
    ASSERT(0 <= colour && colour < BOTH);
    ASSERT(0 <= sq && sq < 64);
    return PawnAttacks[colour][sq];
}

void setPcsq() {

    for (int i = 0; i < 120; ++i) {

    	if(!SQOFFBOARD(i)) {
	    	/* set the piece/square tables for each piece type */
	    	for(int l = 0; l < 13; l++) {
	    		e->mgPst[l][i] = 0;
	    	}
	    	e->mgPst[wP][i] = PawnTable[SQ64(i)];
	        e->mgPst[bP][i] = PawnTable[MIRROR64(SQ64(i))];
	        e->mgPst[wN][i] = KnightTable[SQ64(i)];
	        e->mgPst[bN][i] = KnightTable[MIRROR64(SQ64(i))];
	        e->mgPst[wB][i] = BishopTable[SQ64(i)];
	        e->mgPst[bB][i] = BishopTable[MIRROR64(SQ64(i))];
	        e->mgPst[wR][i] = RookTable[SQ64(i)];
	        e->mgPst[bR][i] = RookTable[MIRROR64(SQ64(i))];
	        e->mgPst[wQ][i] = QueenTableMG[SQ64(i)];
	        e->mgPst[bQ][i] = QueenTableMG[MIRROR64(SQ64(i))];
	        e->mgPst[wK][i] = KingMG[SQ64(i)];
	        e->mgPst[bK][i] = KingMG[MIRROR64(SQ64(i))];

	        e->egPst[wP][i] = PawnTable[SQ64(i)];
	        e->egPst[bP][i] = PawnTable[MIRROR64(SQ64(i))];
	        e->egPst[wN][i] = KnightTable[SQ64(i)];
	        e->egPst[bN][i] = KnightTable[MIRROR64(SQ64(i))];
	        e->egPst[wB][i] = BishopTable[SQ64(i)];
	        e->egPst[bB][i] = BishopTable[MIRROR64(SQ64(i))];
	        e->egPst[wR][i] = RookTable[SQ64(i)];
	        e->egPst[bR][i] = RookTable[MIRROR64(SQ64(i))];
	        e->egPst[wQ][i] = QueenTableEG[SQ64(i)];
	        e->egPst[bQ][i] = QueenTableEG[MIRROR64(SQ64(i))];
	        e->egPst[wK][i] = KingEG[SQ64(i)];
	        e->egPst[bK][i] = KingEG[MIRROR64(SQ64(i))];

	        /*e->mgPst[wP][i] = PawnTableMG[SQ64(i)];
	        e->mgPst[bP][i] = PawnTableMG[MIRROR64(SQ64(i))];
	        e->mgPst[wN][i] = KnightTableMG[SQ64(i)];
	        e->mgPst[bN][i] = KnightTableMG[MIRROR64(SQ64(i))];
	        e->mgPst[wB][i] = BishopTableMG[SQ64(i)];
	        e->mgPst[bB][i] = BishopTableMG[MIRROR64(SQ64(i))];
	        e->mgPst[wR][i] = RookTableMG[SQ64(i)];
	        e->mgPst[bR][i] = RookTableMG[MIRROR64(SQ64(i))];
	        e->mgPst[wQ][i] = QueenTableMG[SQ64(i)];
	        e->mgPst[bQ][i] = QueenTableMG[MIRROR64(SQ64(i))];
	        e->mgPst[wK][i] = KingMG[SQ64(i)];
	        e->mgPst[bK][i] = KingMG[MIRROR64(SQ64(i))];

	        e->egPst[wP][i] = PawnTableEG[SQ64(i)];
	        e->egPst[bP][i] = PawnTableEG[MIRROR64(SQ64(i))];
	        e->egPst[wN][i] = KnightTableEG[SQ64(i)];
	        e->egPst[bN][i] = KnightTableEG[MIRROR64(SQ64(i))];
	        e->egPst[wB][i] = BishopTableEG[SQ64(i)];
	        e->egPst[bB][i] = BishopTableEG[MIRROR64(SQ64(i))];
	        e->egPst[wR][i] = RookTableEG[SQ64(i)];
	        e->egPst[bR][i] = RookTableEG[MIRROR64(SQ64(i))];
	        e->egPst[wQ][i] = QueenTableEG[SQ64(i)];
	        e->egPst[bQ][i] = QueenTableEG[MIRROR64(SQ64(i))];
	        e->egPst[wK][i] = KingEG[SQ64(i)];
	        e->egPst[bK][i] = KingEG[MIRROR64(SQ64(i))];*/
    	}
        
    }
}

void AllInit() {
	InitSq120To64();
	InitBitMasks();
	InitHashKeys();
	InitFilesRanksBrd();
	InitEvalMasks();
	InitMvvLva();
	initLMRTable();
	//InitPolyBook();
	setSquaresNearKing();
	KingAreaMask();
	PawnAttacksMasks();
	setPcsq();
}