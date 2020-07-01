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

// ttable.c

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ttable.h"
#include "defs.h"

TTable TT; // The Global Transposition Table

void clearTTable() {

    // Wipe the Transposition Table in preperation for a new game.
    // The Hash Mask is known to be one less than the size

    memset(TT.clusters, 0, sizeof(TT_Cluster) * (TT.hashMask + 1u));
}

void initTTable(uint64_t MB) {

    uint64_t keySize = 16ull;

    // Cleanup memory when resizing the table
    if (TT.hashMask) free(TT.clusters);

    // The smallest TT size we allow is 1MB, which matches up with
    // a TT using a 15 bit lookup key. We start the key at 16, because
    // while adjusting the given size to the nearest power of two less
    // than or equal to the size, we end with a decrement to the key
    // size. The formula works under the assumption that a TTCluster is
    // exactly 32 bytes. We assure this in order to get good caching

    for (;1ull << (keySize + 5) <= MB << 20 ; keySize++);
    assert(sizeof(TT_Cluster) == 32);
    keySize = keySize - 1;

    // Allocate the TTClusters and save the lookup mask
    TT.hashMask = (1ull << keySize) - 1u;
    TT.clusters = malloc(sizeof(TT_Cluster) * (1ull << keySize));

    clearTTable(); // Clear the table and load everything into the cache
}

void prefetchTTable(uint64_t key) {

    TT_Cluster *cluster = &TT.clusters[key & TT.hashMask];
    __builtin_prefetch(cluster);
}

int hashSizeTTable() {
    return ((TT.hashMask + 1) * sizeof(TT_Cluster)) / (1ull << 20);
}

void updateTTable() {

    // The two LSBs are used for storing the entry bound
    // types, and the six MSBs are for storing the entry
    // age. Therefore add TT_MASK_BOUND + 1 to increment.

    TT.generation += TT_MASK_BOUND + 1;
    assert(!(TT.generation & TT_MASK_BOUND));
}

int hashfullTTable() {

    // Take a sample of the first thousand clusters in the table
    // in order to estimate the permill of the table that is in
    // use for the most recent search.

    int cnt = 0;

    for (int i = 0; i < 1000; i++)
        for (int j = 0; j < TT_CLUSTER_NB; j++)
            cnt += (TT.clusters[i].entry[j].generation & TT_MASK_BOUND) != BOUND_NONE
                && (TT.clusters[i].entry[j].generation & TT_MASK_AGE) == TT.generation;

    return cnt / TT_CLUSTER_NB;
}

int probeTTEntry(uint64_t key, int *move, int *value, int *eval, int *depth, int *bound) {

    const uint16_t key16 = key >> 48;
    TT_Entry *entry = TT.clusters[key & TT.hashMask].entry;

    // Search for a matching key16 signature
    for (int i = 0; i < TT_CLUSTER_NB; i++) {
        if (entry[i].key16 == key16) {

            // Update age but retain bound type
            entry[i].generation = TT.generation | (entry[i].generation & TT_MASK_BOUND);

            // Copy over the TTEntry and signal success
            *move  = entry[i].move;
            *value = entry[i].value;
            *eval  = entry[i].eval;
            *depth = entry[i].depth;
            *bound = entry[i].generation & TT_MASK_BOUND;
            return 1;
        }
    }

    return 0;
}

void storeTTEntry(uint64_t key, int move, int value, int eval, int depth, int bound) {

    int i;
    const uint16_t key16 = key >> 48;
    TT_Entry *entry = TT.clusters[key & TT.hashMask].entry;
    TT_Entry *replace = entry;

    // Find a matching key16, or replace using MAX(x1, x2, x3),
    // where xN equals the depth minus 4 times the age difference
    for (i = 0; i < TT_CLUSTER_NB && entry[i].key16 != key16; i++)
        if (   replace->depth - ((259 + TT.generation - replace->generation) & TT_MASK_AGE)
            >= entry[i].depth - ((259 + TT.generation - entry[i].generation) & TT_MASK_AGE))
            replace = &entry[i];

    // Prefer a matching keys, otherwise score a replacement
    replace = (i != TT_CLUSTER_NB) ? &entry[i] : replace;

    // Don't overwrite an entry from the same position, unless we have
    // an exact bound or depth that is nearly as good as the old one
    if (   bound != BOUND_EXACT
        && key16 == replace->key16
        && depth < replace->depth - 3)
        return;

    // Finally, copy the new data into the replaced slot
    replace->depth      = (int8_t)depth;
    replace->generation = (uint8_t)bound | TT.generation;
    replace->value      = (int16_t)value;
    replace->eval       = (int16_t)eval;
    replace->key16      = (uint16_t)key16;
    replace->move       = (int)move;
}

int valueFromTT(int value, int ply) {

    // When probing MATE scores into the table
    // we must factor in the search ply

    return value >=  MATE_IN_MAX ? value - ply
         : value <= MATED_IN_MAX ? value + ply : value;
}

int valueToTT(int value, int ply) {

    // When storing MATE scores into the table
    // we must factor in the search ply

    return value >=  MATE_IN_MAX ? value + ply
         : value <= MATED_IN_MAX ? value - ply : value;
}