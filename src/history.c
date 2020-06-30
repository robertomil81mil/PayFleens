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

#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "defs.h"
#include "history.h"
#include "makemove.h"

void updateHistoryStats(Board *pos, SearchInfo *info, int *quiets, int quietsPlayed, int height, int bonus) {

    int entry, colour = pos->side;
    int bestMove = quiets[quietsPlayed-1];

    // Extract information from one move ago.
    int counter = info->currentMove[height-1];
    int cmPiece = info->currentPiece[height-1];
    int cmTo = SQ64(TOSQ(counter));

    // Extract information from two moves ago.
    int follow = info->currentMove[height-2];
    int fmPiece = info->currentPiece[height-2];
    int fmTo = SQ64(TOSQ(follow));

    // Avoid saturate history table
    bonus = MIN(bonus, HistoryMax);

    for (int i = 0; i < quietsPlayed; ++i) {

        // Apply a malus if this is not the bestMove
        int delta = (quiets[i] == bestMove) ? bonus : -bonus;

        // Extract information from this move
        int to = SQ64(TOSQ(quiets[i]));
        int from = SQ64(FROMSQ(quiets[i]));
        int piece = pieceType(pos->pieces[SQ120(from)]);

        // Update Butterfly History
        entry = pos->mainHistory[colour][from][to];
        entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
        pos->mainHistory[colour][from][to] = entry;

        // Update Counter Move History
        if (counter != NONE_MOVE && counter != NULL_MOVE) {
            entry = pos->continuation[0][cmPiece][cmTo][piece][to];
            entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
            pos->continuation[0][cmPiece][cmTo][piece][to] = entry;
        }

        // Update Followup Move History
        if (follow != NONE_MOVE && follow != NULL_MOVE) {
            entry = pos->continuation[1][fmPiece][fmTo][piece][to];
            entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
            pos->continuation[1][fmPiece][fmTo][piece][to] = entry;
        }
    }

    // Update Counter Moves (BestMove refutes the previous move)
    if (counter != NONE_MOVE && counter != NULL_MOVE)
        pos->cmtable[!colour][cmPiece][cmTo] = bestMove;

    // Update Killer Moves (Avoid duplicates)
    updateKillerMoves(pos, height, bestMove);
}

void updateKillerMoves(Board *pos, int height, int move) {

    // Update Killer Moves (Avoid duplicates)
    if (pos->killers[height][0] != move) {
        pos->killers[height][1] = pos->killers[height][0];
        pos->killers[height][0] = move;
    }
}

void getHistoryScore(Board *pos, SearchInfo *info, int move, int height, int *hist, int *cmhist, int *fmhist) {

    // Extract information from this move
    int to = SQ64(TOSQ(move));
    int from = SQ64(FROMSQ(move));
    int piece = pieceType(pos->pieces[SQ120(from)]);

    // Extract information from one move ago.
    int counter = info->currentMove[height-1];
    int cmPiece = info->currentPiece[height-1];
    int cmTo = SQ64(TOSQ(counter));

    // Extract information from two moves ago.
    int follow = info->currentMove[height-2];
    int fmPiece = info->currentPiece[height-2];
    int fmTo = SQ64(TOSQ(follow));

    // Set basic Butterfly history
    *hist = pos->mainHistory[pos->side][from][to];

    // Set Counter Move History if it exists
    if (counter == NONE_MOVE || counter == NULL_MOVE) *cmhist = 0;
    else *cmhist = pos->continuation[0][cmPiece][cmTo][piece][to];

    // Set Followup Move History if it exists
    if (follow == NONE_MOVE || follow == NULL_MOVE) *fmhist = 0;
    else *fmhist = pos->continuation[1][fmPiece][fmTo][piece][to];
}

void scoreQuietMoves(Board *pos, SearchInfo *info, MoveList *list, int start, int length, int height) {

    // Extract information from one move ago.
    int counter = info->currentMove[height-1];
    int cmPiece = info->currentPiece[height-1];
    int cmTo = SQ64(TOSQ(counter));

    // Extract information from two moves ago.
    int follow = info->currentMove[height-2];
    int fmPiece = info->currentPiece[height-2];
    int fmTo = SQ64(TOSQ(follow));

    for (int i = start; i < start + length; ++i) {

        // Extract information from this move
        int to = SQ64(TOSQ(list->moves[i].move));
        int from = SQ64(FROMSQ(list->moves[i].move));
        int piece = pieceType(pos->pieces[SQ120(from)]);

        // Start with the basic Butterfly history
        list->moves[i].score = pos->mainHistory[pos->side][from][to];

        // Add Counter Move History if it exists
        if (counter != NONE_MOVE && counter != NULL_MOVE)
            list->moves[i].score += pos->continuation[0][cmPiece][cmTo][piece][to];

        // Add Followup Move History if it exists
        if (follow != NONE_MOVE && follow != NULL_MOVE)
            list->moves[i].score += pos->continuation[1][fmPiece][fmTo][piece][to];
    }
}

void getRefutationMoves(Board *pos, SearchInfo *info, int height, int *killer1, int *killer2, int *counter) {

    // Extract information from one move ago.
    int previous = info->currentMove[height-1];
    int cmPiece = info->currentPiece[height-1];
    int cmTo = SQ64(TOSQ(previous));

    // Set Killer Moves by height
    *killer1 = pos->killers[height][0];
    *killer2 = pos->killers[height][1];

    // Set Counter Move if one exists
    if (previous == NONE_MOVE || previous == NULL_MOVE) *counter = NONE_MOVE;
    else *counter = pos->cmtable[!pos->side][cmPiece][cmTo];
}
