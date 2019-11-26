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

// pvtable.c

#include "stdio.h"
#include "defs.h"

int GetPvLine(const int depth, S_BOARD *pos) {

	ASSERT(depth < MAXDEPTH && depth >= 1);

	int move = ProbePvMove(pos);
	int count = 0;
	
	while(move != NOMOVE && count < depth) {
	
		ASSERT(count < MAXDEPTH);
	
		if( MoveExists(pos, move) ) {
			MakeMove(pos, move);
			pos->PvArray[count++] = move;
		} else {
			break;
		}		
		move = ProbePvMove(pos);	
	}
	
	while(pos->ply > 0) {
		TakeMove(pos);
	}
	
	return count;
	
}

void ClearHashTable(S_HASHTABLE *table) {

  S_HASHENTRY *tableEntry;
  
  for (tableEntry = table->pTable; tableEntry < table->pTable + table->numEntries; tableEntry++) {
    tableEntry->posKey = 0ULL;
    tableEntry->move = NOMOVE;
    tableEntry->depth = 0;
    tableEntry->score = 0;
    tableEntry->eval  = 0;
    tableEntry->flags = 0;
  }
  table->newWrite=0;
}

void InitHashTable(S_HASHTABLE *table, const int MB) {  
	
	int HashSize = 0x100000 * MB;
    table->numEntries = HashSize / sizeof(S_HASHENTRY);
    table->numEntries -= 2;
	
	if(table->pTable!=NULL) {
		free(table->pTable);
	}
		
    table->pTable = (S_HASHENTRY *) malloc(table->numEntries * sizeof(S_HASHENTRY));
	if(table->pTable == NULL) {
		printf("Hash Allocation Failed, trying %dMB...\n",MB/2);
		InitHashTable(table,MB/2);
	} else {
		ClearHashTable(table);
		printf("HashTable init complete with %d entries\n",table->numEntries);
	}
	
}

int ProbeHashEntry(S_BOARD *pos, int *move, int *score, int *eval, int *depth, int *flag) {

	int index = pos->posKey % pos->HashTable->numEntries;
	
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);
    ASSERT(pos->ply>=0&&pos->ply<MAXPLY);

    if( pos->HashTable->pTable[index].posKey == pos->posKey ) {
		ASSERT(pos->HashTable->pTable[index].depth>=0&&pos->HashTable->pTable[index].depth<MAXDEPTH);
        ASSERT(pos->HashTable->pTable[index].flags>=HFALPHA&&pos->HashTable->pTable[index].flags<=HFEXACT);
        pos->HashTable->hit++;

		*move  = pos->HashTable->pTable[index].move;
		*score = pos->HashTable->pTable[index].score;
		*eval  = pos->HashTable->pTable[index].eval;
		*depth = pos->HashTable->pTable[index].depth;
		*flag  = pos->HashTable->pTable[index].flags;

		if(*score > ISMATE) *score -= pos->ply;
        else if(*score < -ISMATE) *score += pos->ply;

        return 1;
	}
	
	return 0;
}

void StoreHashEntry(S_BOARD *pos, const int move, int score, const int eval, const int flags, const int depth) {

	int index = pos->posKey % pos->HashTable->numEntries;
	
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);
	ASSERT(depth>=0&&depth<MAXDEPTH);
    ASSERT(flags>=HFALPHA&&flags<=HFEXACT);
    ASSERT(score>=-INFINITE&&score<=INFINITE);
    ASSERT(pos->ply>=0&&pos->ply<MAXPLY);
	
	if( pos->HashTable->pTable[index].posKey == 0) {
		pos->HashTable->newWrite++;
	} else {
		pos->HashTable->overWrite++;
	}
	
	if(score > ISMATE) score += pos->ply;
    else if(score < -ISMATE) score -= pos->ply;
	
	pos->HashTable->pTable[index].move = move;
    pos->HashTable->pTable[index].posKey = pos->posKey;
	pos->HashTable->pTable[index].flags = flags;
	pos->HashTable->pTable[index].score = score;
	pos->HashTable->pTable[index].eval  = eval;
	pos->HashTable->pTable[index].depth = depth;
}

int ProbePvMove(const S_BOARD *pos) {

	int index = pos->posKey % pos->HashTable->numEntries;
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);
	
	if( pos->HashTable->pTable[index].posKey == pos->posKey ) {
		return pos->HashTable->pTable[index].move;
	}
	
	return NOMOVE;
}