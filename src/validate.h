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

void MirrorEvalTest(S_BOARD *pos);

#if defined(DEBUG)

int MoveListOk(const S_MOVELIST *list, const S_BOARD *pos);
int SqIs120(const int sq);
int PceValidEmptyOffbrd(const int pce);
int SqOnBoard(const int sq);
int SideValid(const int side);
int FileRankValid(const int fr);
int PieceValidEmpty(const int pce);
int PieceValid(const int pce);
void Perft(int depth, S_BOARD *pos);
void PerftTest(int depth, S_BOARD *pos);

#endif