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

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"

enum Result {
    INVALID = 0,
    UNKNOWN = 1,
    DRAW    = 2,
    WIN     = 4,
};

struct KPKPos {
	int us;
	int ksq[COLOUR_NB], psq;
    int result;
};

unsigned idxpos(int us, int bksq, int wksq, int psq);

void Bitbases_init();
bool Bitbases_probe(int wksq, int wpsq, int bksq, int us);

void KPKPosition(unsigned idx);
int classify(unsigned idx);