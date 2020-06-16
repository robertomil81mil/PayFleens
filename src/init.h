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

#pragma once

void AllInit();

uint64_t kingAreaMasks(int colour, int sq);
uint64_t outpostSquareMasks(int colour, int sq);
uint64_t pawnAttacks(int colour, int sq);

extern uint64_t PassedPawnMasks[COLOUR_NB][64];
extern uint64_t OutpostSquareMasks[COLOUR_NB][64];
extern uint64_t KingAreaMasks[COLOUR_NB][64];
extern uint64_t PawnAttacks[COLOUR_NB][64];
extern uint64_t IsolatedMask[64];