/*
 *  PayFleens is a UCI chess engine by Roberto Martinez.
 * 
 *  Copyright (C) 2019 Roberto Martinez
 *  Copyright (C) 2019 Andrew Grant
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

#pragma once

#include <stdint.h>
#include "defs.h"

enum {
    BOUND_NONE  = 0,
    BOUND_LOWER = 1,
    BOUND_UPPER = 2,
    BOUND_EXACT = 3,
};

enum {
    TT_MASK_BOUND = 0x03,
    TT_MASK_AGE   = 0xFC,
    TT_CLUSTER_NB = 3,
};

struct TT_Entry {
    int8_t depth;
    uint8_t generation;
    int16_t eval, value;
    uint16_t key16;
    int move;
};

struct TT_Cluster {
    TT_Entry entry[TT_CLUSTER_NB];
    uint16_t padding;
};

struct TTable {
    TT_Cluster *clusters;
    uint64_t hashMask;
    uint8_t generation;
};

void clearTTable();
void initTTable(uint64_t MB);
void updateTTable();
int hashfullTTable();
int probeTTEntry(uint64_t key, int *move, int *value, int *eval, int *depth, int *bound);
void storeTTEntry(uint64_t key, int move, int value, int eval, int depth, int bound);
int valueFromTT(int value, int ply);
int valueToTT(int value, int ply);