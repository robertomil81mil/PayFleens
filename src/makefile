#  PayFleens is a UCI chess engine by Roberto Martinez.
# 
#  Copyright (C) 2019 Roberto Martinez
#
#  PayFleens is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  PayFleens is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

CC     = gcc
SRC    = uci.c evaluate.c ttable.c init.c bitboards.c hashkeys.c board.c data.c attack.c io.c movegen.c validate.c makemove.c perft.c search.c time.c polybook.c polykeys.c
LIBS   = -lm
EXE    = delta2

WFLAGS = -std=gnu11 -Wall -Wextra -Wshadow
CFLAGS = -DNDEBUG -O3 $(WFLAGS) -march=native -flto

all:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)
