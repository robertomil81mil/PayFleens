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

// init.c

#include <stdio.h>
#include <stdlib.h>

#include "bitboards.h"
#include "board.h"
#include "defs.h"
#include "evaluate.h"
#include "hashkeys.h"
#include "init.h"
#include "movegen.h"
#include "search.h"
#include "ttable.h"

uint64_t PassedPawnMasks[COLOUR_NB][64];
uint64_t OutpostSquareMasks[COLOUR_NB][64];
uint64_t KingAreaMasks[COLOUR_NB][64];
uint64_t PawnAttacks[COLOUR_NB][64];
uint64_t IsolatedMask[64];

uint64_t kingAreaMasks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return KingAreaMasks[colour][sq];
}

uint64_t outpostSquareMasks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return OutpostSquareMasks[colour][sq];
}

uint64_t pawnAttacks(int colour, int sq) {
    ASSERT(0 <= colour && colour < COLOUR_NB);
    ASSERT(0 <= sq && sq < 64);
    return PawnAttacks[colour][sq];
}

void KingAreaMask() {

	int sq, t_sq, pce, dir, index;

	for(sq = 0; sq < 64; ++sq) {
		KingAreaMasks[WHITE][sq] = 0ULL;
		KingAreaMasks[BLACK][sq] = 0ULL;
	}
	pce = wK;
	for(sq = 0; sq < BRD_SQ_NUM; ++sq) {
		if(!SQOFFBOARD(sq)) {
			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;
				KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq));
				KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq));
				if(!SQOFFBOARD(t_sq)) {
					KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(t_sq));
					KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(t_sq));
				}
				if(FilesBrd[sq] == FILE_A) {
					if (!SQOFFBOARD(sq+12)) { 
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+12));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+12));
					}
					if (!SQOFFBOARD(sq+2)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+2));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+2));
					}
					if (!SQOFFBOARD(sq-8)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-8));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-8));
					}
				}
				if(FilesBrd[sq] == FILE_H) {
					if (!SQOFFBOARD(sq-12)) { 
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-12));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-12));
					}
					if (!SQOFFBOARD(sq-2)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-2));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-2));
					}
					if (!SQOFFBOARD(sq+8)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+8));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+8));
					}
				}
				if(RanksBrd[sq] == RANK_1) {
					if (!SQOFFBOARD(sq+19)) { 
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+19));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+19));
					} 
					if (!SQOFFBOARD(sq+20)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+20));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+20));
					} 
					if (!SQOFFBOARD(sq+21)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+21));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+21));
					}
					if(FilesBrd[sq] == FILE_A) {
						if (!SQOFFBOARD(sq+22)) {
							KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+22));
							KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+22));
						}
					}
					if(FilesBrd[sq] == FILE_H) {
						if (!SQOFFBOARD(sq+18)) {
							KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq+18));
							KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq+18));
						}
					}
				}
				if(RanksBrd[sq] == RANK_8) {
					if (!SQOFFBOARD(sq-19)) { 
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-19));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-19));
					} 
					if (!SQOFFBOARD(sq-20)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-20));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-20));
					} 
					if (!SQOFFBOARD(sq-21)) {
						KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-21));
						KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-21));
					}
					if(FilesBrd[sq] == FILE_A) {
						if (!SQOFFBOARD(sq-18)) {
							KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-18));
							KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-18));
						}
					}
					if(FilesBrd[sq] == FILE_H) {
						if (!SQOFFBOARD(sq-22)) {
							KingAreaMasks[WHITE][SQ64(sq)] |= (1ULL << SQ64(sq-22));
							KingAreaMasks[BLACK][SQ64(sq)] |= (1ULL << SQ64(sq-22));
						}
					}
				}
			}
		}	
	}
}

void PawnAttacksMasks() {

	int sq, t_sq, pce, index;

	for(sq = 0; sq < 64; ++sq) {
		PawnAttacks[WHITE][sq] = 0ULL;
		PawnAttacks[BLACK][sq] = 0ULL;
	}
	pce = wP;
	for(sq = 0; sq < BRD_SQ_NUM; ++sq) {
		if(!SQOFFBOARD(sq)) {
			for(index = 0; index < NumDir[pce]; ++index) {
				t_sq = sq + PceDir[pce][index];
				if(!SQOFFBOARD(t_sq)) {
					PawnAttacks[WHITE][SQ64(sq)] |= (1ULL << SQ64(t_sq));
				}
			}
		}	
	}
	pce = bP;
	for(sq = 0; sq < BRD_SQ_NUM; ++sq) {
		if(!SQOFFBOARD(sq)) {
			for(index = 0; index < NumDir[pce]; ++index) {
				t_sq = sq + PceDir[pce][index];
				if(!SQOFFBOARD(t_sq)) {
					PawnAttacks[BLACK][SQ64(sq)] |= (1ULL << SQ64(t_sq));
				}
			}
		}	
	}
}

void setSquaresNearKing() {

    for (int i = 0; i < 120; ++i)
        for (int j = 0; j < 120; ++j)
        {
            e.sqNearK[WHITE][i][j] = 0;
            e.sqNearK[BLACK][i][j] = 0;
            // e.sqNearK[side^1] [ KingSq[side^1] ] [t_sq] 

            if ( !SQOFFBOARD(i) && !SQOFFBOARD(j) ) {

            	if (j == i) {
            		e.sqNearK[WHITE][i][j] = 1;
                    e.sqNearK[BLACK][i][j] = 1;
            	}

                /* squares constituting the ring around both kings */
                if (j == i + 10 || j == i - 10 
				||  j == i + 1  || j == i - 1 
				||  j == i + 9  || j == i - 9 
				||  j == i + 11 || j == i - 11) {
                	e.sqNearK[WHITE][i][j] = 1;
                    e.sqNearK[BLACK][i][j] = 1;
                }

                if (FilesBrd[i] == FILE_A && FilesBrd[j] == FILE_C) {

                	if (j == i + 12
                	||  j == i + 2 
                	||  j == i - 8) {
                		e.sqNearK[WHITE][i][j] = 1;
                    	e.sqNearK[BLACK][i][j] = 1;
                	}
                }
                if (FilesBrd[i] == FILE_H && FilesBrd[j] == FILE_F) {

                	if (j == i - 12
                	||  j == i - 2 
                	||  j == i + 8) {
                		e.sqNearK[WHITE][i][j] = 1;
                    	e.sqNearK[BLACK][i][j] = 1;
                	}
                }
                if (RanksBrd[i] == RANK_1) {

                	if (j == i + 19
                	||  j == i + 20 
                	||  j == i + 21) {
                		e.sqNearK[WHITE][i][j] = 1;
                    	e.sqNearK[BLACK][i][j] = 1;
                    }
                    if (FilesBrd[i] == FILE_A) {
                    	if (j == i + 22) {
                    		e.sqNearK[WHITE][i][j] = 1;
                    		e.sqNearK[BLACK][i][j] = 1;
                    	}
                    }
                    if (FilesBrd[i] == FILE_H) {
                    	if (j == i + 18) {
                    		e.sqNearK[WHITE][i][j] = 1;
                    		e.sqNearK[BLACK][i][j] = 1;
                    	}
                    }
                }
                if (RanksBrd[i] == RANK_8) {

                	if (j == i - 19
                	||  j == i - 20 
                	||  j == i - 21) {
                		e.sqNearK[WHITE][i][j] = 1;
                    	e.sqNearK[BLACK][i][j] = 1;
                    }
                    if (FilesBrd[i] == FILE_A) {
                    	if (j == i - 18) {
                    		e.sqNearK[WHITE][i][j] = 1;
                    		e.sqNearK[BLACK][i][j] = 1;
                    	}
                    }
                    if (FilesBrd[i] == FILE_H) {
                    	if (j == i - 22) {
                    		e.sqNearK[WHITE][i][j] = 1;
                    		e.sqNearK[BLACK][i][j] = 1;
                    	}
                    }
                }
            }
        }
}

void InitEvalMasks() {

	for (int sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = adjacentFiles(sq);
		PassedPawnMasks[WHITE][sq] = passedPawnSpan(WHITE, sq);
		PassedPawnMasks[BLACK][sq] = passedPawnSpan(BLACK, sq);
		OutpostSquareMasks[WHITE][sq] = PassedPawnMasks[WHITE][sq] & ~FilesBB[file_of(sq)];
		OutpostSquareMasks[BLACK][sq] = PassedPawnMasks[BLACK][sq] & ~FilesBB[file_of(sq)];
    }
}

void AllInit() {
	InitSq120To64();
	InitHashKeys();
	InitFilesRanksBrd();
	InitEvalMasks();
	InitMvvLva();
	initLMRTable();
	setSquaresNearKing();
	KingAreaMask();
	PawnAttacksMasks();
	setPcsq32();
	initTTable(16);
}