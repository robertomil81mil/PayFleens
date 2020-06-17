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

#include <stdbool.h>
#include <stdint.h>

#include "bitbase.h"
#include "bitboards.h"
#include "board.h"
#include "defs.h"
#include "init.h"

// There are 24 possible pawn squares: files A to D and ranks from 2 to 7.
// Positions with the pawn on files E to H will be mirrored before probing.
// There are 128 possible king squares: 64 for White and 64 for Black.
// We calculate MAX_INDEX as follow: stm * psq * wksq * bksq = 196608
#define MAX_INDEX 196608

// Each position contains: stm, wksq, psq, bksq and result,
// stm and squares are obtained from an index,
// the result will be computed later.
KPKPos db[MAX_INDEX];

// Each uint32_t stores results of 32 positions, one per bit
uint32_t KPKBitbase[MAX_INDEX / 32];

// Information is mapped in a way that minimizes the number of iterations:
//
// bit  0- 5: white king square (from A1 to H8)
// bit  6-11: black king square (from A1 to H8)
// bit    12: side to move (WHITE or BLACK)
// bit 13-14: white pawn file (from FILE_A to FILE_D)
// bit 15-17: white pawn RANK_7 - rank (from RANK_7 - RANK_7 to RANK_7 - RANK_2)
unsigned idxpos(int us, int bksq, int wksq, int psq) {
    return ((int)((wksq) | (bksq << 6) | (us << 12) | (file_of(psq) << 13) | ((RANK_7 - rank_of(psq)) << 15)));
}

#define wksq(idx) ((idx >>  0) & 0x3F)
#define bksq(idx) ((idx >>  6) & 0x3F)
#define us(idx)   ((idx >> 12) & 0x01)
#define psq(idx)  (makeSq(((idx >> 13) & 0x3), (RANK_7 - ((idx >> 15) & 0x7))))

bool Bitbases_probe(int wksq, int wpsq, int bksq, int us) {

    ASSERT(file_of(wpsq) <= FILE_D);

    unsigned idx = idxpos(us, bksq, wksq, wpsq);
    return KPKBitbase[idx / 32] & (1ull << (idx & 0x1F));
}

void Bitbases_init() {

    unsigned idx, repeat = 1;

    // Initialize db with known win / draw positions
    for (idx = 0; idx < MAX_INDEX; ++idx)
        KPKPosition(idx);

    // Iterate through the positions until none of the unknown positions can be
    // changed to either wins or draws (15 cycles needed).
    while (repeat)
        for (repeat = idx = 0; idx < MAX_INDEX; ++idx)
            repeat |= (db[idx].result == UNKNOWN && (db[idx].result = classify(idx)) != UNKNOWN);

    // Map 32 results into one KPKBitbase[] entry
    for (idx = 0; idx < MAX_INDEX; ++idx)
        if (db[idx].result == WIN)
            KPKBitbase[idx / 32] |= (1ull << (idx & 0x1F));
}

void KPKPosition(unsigned idx) {

    KPKPos *edb = &db[idx];

    edb->ksq[WHITE] = bksq(idx);
    edb->ksq[BLACK] = wksq(idx);
    edb->psq        = psq(idx);
    edb->us         = us(idx);

    // Check if two pieces are on the same square or if a king can be captured
    if (   distanceBetween(SQ120(edb->ksq[WHITE]), SQ120(edb->ksq[BLACK])) <= 1
        || edb->ksq[WHITE] == edb->psq
        || edb->ksq[BLACK] == edb->psq
        || (edb->us == WHITE && (PawnAttacks[WHITE][edb->psq] & edb->ksq[BLACK])))
        edb->result = INVALID;

    // Immediate win if a pawn can be promoted without getting captured
    else if (   edb->us == WHITE
             && rank_of(edb->psq) == RANK_7
             && edb->ksq[WHITE] != edb->psq + NORTH
             && (    distanceBetween(SQ120(edb->ksq[BLACK]), SQ120(edb->psq + NORTH)) > 1
                 || (KingAreaMasks[edb->ksq[WHITE]] & (edb->psq + NORTH))))
        edb->result = WIN;

    // Immediate draw if it is a stalemate or a king captures undefended pawn
    else if (   edb->us == BLACK
             && (  !(KingAreaMasks[edb->ksq[BLACK]] & ~(KingAreaMasks[edb->ksq[WHITE]] | PawnAttacks[WHITE][edb->psq]))
                 || (KingAreaMasks[edb->ksq[BLACK]] & edb->psq & ~KingAreaMasks[edb->ksq[WHITE]])))
        edb->result = DRAW;

    // Position will be classified later
    else
        edb->result = UNKNOWN;
}

int classify(unsigned idx) {

    // White to move: If one move leads to a position classified as WIN, the result
    // of the current position is WIN. If all moves lead to positions classified
    // as DRAW, the current position is classified as DRAW, otherwise the current
    // position is classified as UNKNOWN.
    //
    // Black to move: If one move leads to a position classified as DRAW, the result
    // of the current position is DRAW. If all moves lead to positions classified
    // as WIN, the position is classified as WIN, otherwise the current position is
    // classified as UNKNOWN.

    const int Us   =  db[idx].us;
    const int Them = (Us == WHITE ? BLACK : WHITE);
    const int Good = (Us == WHITE ? WIN   : DRAW);
    const int Bad  = (Us == WHITE ? DRAW  : WIN);

    int r = INVALID, result;
    U64 b = KingAreaMasks[db[idx].ksq[Us]];

    while (b)
        r |= Us == WHITE ? db[idxpos(Them, db[idx].ksq[Them]  , poplsb(&b), db[idx].psq)].result
                         : db[idxpos(Them, poplsb(&b), db[idx].ksq[Them]  , db[idx].psq)].result;

    if (Us == WHITE) {

        if (rank_of(db[idx].psq) < RANK_7)      // Single push
            r |= db[idxpos(Them, db[idx].ksq[Them], db[idx].ksq[Us], db[idx].psq + NORTH)].result;

        if (   rank_of(db[idx].psq) == RANK_2   // Double push
            && db[idx].psq + NORTH != db[idx].ksq[Us]
            && db[idx].psq + NORTH != db[idx].ksq[Them])
            r |= db[idxpos(Them, db[idx].ksq[Them], db[idx].ksq[Us], db[idx].psq + NORTH + NORTH)].result;
    }

    return result = r & Good  ? Good  : r & UNKNOWN ? UNKNOWN : Bad;
}