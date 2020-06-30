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

#include <string.h>

#include "board.h"
#include "data.h"
#include "defs.h"
#include "history.h"
#include "makemove.h"
#include "movegen.h"
#include "movepicker.h"
#include "validate.h"

static int popMove(Move *moves, int *size, int index) {
    int popped = moves[index].move;
    moves[index].move = moves[--*size].move;
    moves[index].score = moves[*size].score;
    return popped;
}

static int getBestMoveIndex(MoveList *list, int start, int end) {

    int best = start;

    for (int i = start + 1; i < end; ++i)
        if (list->moves[i].score > list->moves[best].score)
            best = i;

    return best;
}

static void scoreNoisyMoves(MoveList *list, Board *pos, int noisySize) {

    static const int MVVLVAValues[PIECE_TYPE_NB] = {
        0, 100, 450, 475, 700, 1300, 0
    };

    // Use modified MVV-LVA to evaluate moves
    for (int i = 0; i < noisySize; ++i) {

        int fromType = pieceType(pos->pieces[FROMSQ(list->moves[i].move)]);
        int toType   = pieceType(pos->pieces[TOSQ(list->moves[i].move)]);

        // Start with the standard MVV-LVA method
        list->moves[i].score = MVVLVAValues[toType] - (fromType - 1);

        // Enpass is a special case of MVV-LVA
        if (list->moves[i].move & MFLAGEP)
            list->moves[i].score = MVVLVAValues[PAWN];

        // A bonus is in order for only queen promotions
        else if (PieceQueen[PROMOTED(list->moves[i].move)])
            list->moves[i].score += MVVLVAValues[QUEEN];

        ASSERT(list->moves[i].score >= 0);
    }
}

void initMovePicker(MovePicker *mp, Board *pos, SearchInfo *info, MoveList *list, int ttMove, int height) {

    // Start with the ttMove, but first generate moves
    // to probe if ttMove is pseudo legal.
    mp->stage = GENERATE_MOVES;
    mp->ttMove = ttMove;

    // Lookup our refutations (killers and counter moves)
    getRefutationMoves(pos, info, height, &mp->killer1, &mp->killer2, &mp->counter);

    mp->info = info;
    mp->list = list;
    mp->height = height;
    mp->type = NORMAL_PICKER;
}

void initNoisyMovePicker(MovePicker *mp, SearchInfo *info, MoveList *list) {

    // Start generating noisy moves
    mp->stage = GENERATE_MOVES;

    // Skip all of the special (refutation and table) moves
    mp->ttMove = mp->killer1 = mp->killer2 = mp->counter = NONE_MOVE;
    mp->info = info;
    mp->list = list;
    mp->height = 0;
    mp->type = NOISY_PICKER;
}

int selectNextMove(MovePicker *mp, Board *pos, int skipQuiets) {

    int best, bestMove;

    switch (mp->stage) {

        case GENERATE_MOVES:

            // Generate the moves to be played.
            mp->stage = TTABLE;
            if (mp->type == NORMAL_PICKER)
                GenerateAllMoves(pos, mp->list);

            // Skip ttMove here.
            if (mp->type == NOISY_PICKER)
                genNoisyMoves(pos, mp->list), mp->stage = SCORE_NOISY;

            /* fallthrough */

        case TTABLE:

            // Play ttMove if it is pseudo legal
            mp->stage = SCORE_NOISY;
            if (MoveExists(mp->list, mp->ttMove))
                return mp->ttMove;

            /* fallthrough */

        case SCORE_NOISY:

            // Score noisy moves, also set mp->split as a break point
            // to seperate the noisy from the quiet moves.
            mp->noisySize = mp->split = mp->list->count - mp->list->quiets;
            scoreNoisyMoves(mp->list, pos, mp->noisySize);
            mp->stage = NOISY;

            ASSERT(MoveListOk(mp->list, pos));

            /* fallthrough */

        case NOISY:

            // Check to see if there are still more noisy moves
            if (mp->noisySize) {

                // Select next best noisy and reduce the effective move list size
                best = getBestMoveIndex(mp->list, 0, mp->noisySize);
                bestMove = popMove(mp->list->moves, &mp->noisySize, best);

                // Don't play the ttMove twice
                if (bestMove == mp->ttMove)
                    return selectNextMove(mp, pos, skipQuiets);

                // Don't play the refutation moves twice
                if (bestMove == mp->killer1) mp->killer1 = NONE_MOVE;
                if (bestMove == mp->killer2) mp->killer2 = NONE_MOVE;
                if (bestMove == mp->counter) mp->counter = NONE_MOVE;

                return bestMove;
            }

            // Terminate when skipping quiets
            if (skipQuiets) {
                mp->stage = DONE;
                return NONE_MOVE;
            }

            mp->stage = KILLER_1;

            /* fallthrough */

        case KILLER_1:

            // Play killer move if not yet played, and pseudo legal
            mp->stage = KILLER_2;
            if (   !skipQuiets
                &&  mp->killer1 != mp->ttMove
                &&  MoveExists(mp->list, mp->killer1))
                return mp->killer1;

            /* fallthrough */

        case KILLER_2:

            // Play killer move if not yet played, and pseudo legal
            mp->stage = COUNTER_MOVE;
            if (   !skipQuiets
                &&  mp->killer2 != mp->ttMove
                &&  MoveExists(mp->list, mp->killer2))
                return mp->killer2;

            /* fallthrough */

        case COUNTER_MOVE:

            // Play counter move if not yet played, and pseudo legal
            mp->stage = SCORE_QUIET;
            if (   !skipQuiets
                &&  mp->counter != mp->ttMove
                &&  mp->counter != mp->killer1
                &&  mp->counter != mp->killer2
                &&  MoveExists(mp->list, mp->counter))
                return mp->counter;

            /* fallthrough */

        case SCORE_QUIET:

            // Score quiet moves when not skipping them
            mp->stage = QUIET;
            if (!skipQuiets) {

                mp->quietSize = mp->list->quiets;        
                scoreQuietMoves(pos, mp->info, mp->list, mp->split, mp->quietSize, mp->height);

                ASSERT(MoveListOk(mp->list, pos));
            }

            /* fallthrough */

        case QUIET:

            // Check to see if there are still more quiet moves
            if (!skipQuiets && mp->quietSize) {

                // Select next best quiet and reduce the effective move list size
                best = getBestMoveIndex(mp->list, mp->split, mp->split + mp->quietSize) - mp->split;
                bestMove = popMove(mp->list->moves + mp->split, &mp->quietSize, best);

                // Don't play a move more than once
                if (   bestMove == mp->ttMove
                    || bestMove == mp->killer1
                    || bestMove == mp->killer2
                    || bestMove == mp->counter)
                    return selectNextMove(mp, pos, skipQuiets);

                return bestMove;
            }

            mp->stage = DONE;

            /* fallthrough */

        case DONE:
            return NONE_MOVE;

        default:
            ASSERT(0);
            return NONE_MOVE;
    }
}