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

// search.c

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attack.h"
#include "bitboards.h"
#include "board.h"
#include "defs.h"
#include "endgame.h"
#include "evaluate.h"
#include "history.h"
#include "makemove.h"
#include "movepicker.h"
#include "movegen.h"
#include "polybook.h"
#include "search.h"
#include "time.h"
#include "ttable.h"
#include "uci.h"

int LMRTable[64][64]; // Late Move Reductions Table

// Add a small random variance to draw scores, to avoid 3fold-blindness
int valueDraw(SearchInfo *info) {
    return VALUE_DRAW + (2 * (info->nodes & 1)) - 1;
}

void ClearForSearch(Board *pos, SearchInfo *info) {
    // Reset the tables used for move ordering
    memset(pos->killers, 0, sizeof(KillerTable));
    memset(pos->cmtable, 0, sizeof(CounterMoveTable));
    memset(pos->mainHistory, 0, sizeof(HistoryTable));
    memset(pos->continuation, 0, sizeof(ContinuationTable));
    
    pos->ply      = 0;
    info->stop    = 0;
    info->nodes   = 0;
    info->fh      = 0;
    info->fhf     = 0;
    info->nullCut = 0;
    info->probCut = 0;
    info->previousTimeReduction = 1.0;
}

void initLMRTable() {

    // Init Late Move Reductions Table
    for (int depth = 1; depth < 64; depth++)
        for (int played = 1; played < 64; played++)
            LMRTable[depth][played] = 0.75 + log(depth) * log(played) / 2.25;
}

void getBestMove(SearchInfo *info, Board *pos, Limits *limits, int *best) {

    // Minor house keeping for starting a search
    updateTTable(); // Table has an age component
    TimeManagementInit(info, limits, pos->gamePly);
    iterativeDeepening(pos, info, limits, best);
}

void iterativeDeepening(Board *pos, SearchInfo *info, Limits *limits, int *best) {

    double timeReduction = 1;

    // Return a book move if we have one
    if (Options.PolyBook) {
        int bookMove = GetBookMove(pos);
        if (bookMove != NONE_MOVE) {
            *best = bookMove; return;
        }
    }

    ClearForSearch(pos, info);

    // Perform iterative deepening until exit conditions 
    for (info->depth = 1; info->depth <= MAX_PLY && !info->stop; info->depth++) {

        // Perform a search for the current depth
        info->values[info->depth] = aspirationWindow(pos, info, best);

        // Check for termination by any of the possible limits 
        if (   (limits->limitedBySelf  && TerminateTimeManagement(pos, info, &timeReduction))
            || (limits->limitedBySelf  && elapsedTime(info) > info->maximumTime - 10)
            || (limits->limitedByTime  && elapsedTime(info) > limits->timeLimit)
            || (limits->limitedByDepth && info->depth >= limits->depthLimit))
            break;
    }

    info->previousTimeReduction = timeReduction;
}

int aspirationWindow(Board *pos, SearchInfo *info, int *best) {

    ASSERT(CheckBoard(pos));

    int alpha, beta, value, lastValue, delta;
    PVariation *const pv = &pos->pv;

    // Create an aspiration window, unless still below the starting depth
    lastValue = info->depth >= WindowDepth ? info->values[info->depth-1]       : -INFINITE;
    delta     = info->depth >= WindowDepth ? WindowSize + abs(lastValue) / 141 : -INFINITE;
    alpha     = info->depth >= WindowDepth ? MAX(-INFINITE, lastValue - delta) : -INFINITE;
    beta      = info->depth >= WindowDepth ? MIN( INFINITE, lastValue + delta) :  INFINITE;

    // Keep trying larger windows until one works
    int failedHighCnt = 0;
    while (1) {

        int adjustedDepth = MAX(1, info->depth - failedHighCnt);

        // Perform a search on the window, return if inside the window
        value = search(alpha, beta, adjustedDepth, pos, info, pv, 0);

        if (info->stop)
            return value;

        if (   (value > alpha && value < beta)
            || (elapsedTime(info) >= WindowTimerMS))
            uciReport(info, pos, alpha, beta, value);

        // Search failed low
        if (value <= alpha) {
            beta  = (alpha + beta) / 2;
            alpha = MAX(-INFINITE, value - delta);
            failedHighCnt = 0;
        }

        // Search failed high
        else if (value >= beta) { 
            beta = MIN(INFINITE, value + delta);
            failedHighCnt++;
        }

        else {
            *best = pos->pv.line[0];
            return value;
        }

        // Expand the search window
        delta += delta / 4 + 5;
    }
}

int search(int alpha, int beta, int depth, Board *pos, SearchInfo *info, PVariation *pv, int height) {

    ASSERT(CheckBoard(pos));
    ASSERT(beta>alpha);

    int InCheck = KingSqAttacked(pos);

    if (depth <= 0 && !InCheck)
        return qsearch(alpha, beta, depth, pos, info, pv, height);

    const int PvNode   = (alpha != beta - 1);
    const int RootNode = (PvNode && pos->ply == 0);
    
    int ttHit, ttValue = 0, ttEval = 0, ttDepth = 0, ttBound = 0;
    int hist = 0, cmhist = 0, fmhist = 0, gcmhist = 0, gchmhist = 0;
    int played = 0, quietsTried = 0, skipQuiets = 0, bonus = 0;
    int improving, extension, singularExt = 0, LMRflag = 0;
    int R, newDepth, rAlpha, rBeta, isQuiet, ttNoisy;
    int eval, value = -INFINITE, best = -INFINITE;
    int move = NONE_MOVE, bestMove = NONE_MOVE, excludedMove = NONE_MOVE;
    int ttMove = NONE_MOVE, quiets[MAX_MOVES];
    MovePicker movePicker;
    PVariation lpv;

    // Ensure a new pv line
    pv->length = 0;

    // Prefetch TTable as early as possible
    prefetchTTable(pos->posKey);

    // Ensure positive depth
    depth = MAX(0, depth);

    // Updates for UCI reporting
    info->nodes++;
    info->seldepth = (height == 0) ? 0 : MAX(info->seldepth, height);

    // Do we have time left on the clock?
    if ((info->nodes & 1023) == 1023)
        CheckTime(info);

    if (!RootNode) {
        if (   info->stop 
            || posIsDrawn(pos, pos->ply))
            return valueDraw(info);

        if (height >= MAX_PLY)
            return EvalPosition(pos, &Table);

        rAlpha = alpha > -INFINITE + height     ? alpha : -INFINITE + height;
        rBeta  =  beta <  INFINITE - height - 1 ?  beta :  INFINITE - height - 1;
        if (rAlpha >= rBeta) return rAlpha;
    }

    if ((ttHit = probeTTEntry(pos->posKey, &ttMove, &ttValue, &ttEval, &ttDepth, &ttBound))) {

        ttValue = valueFromTT(ttValue, height);

        if (   !PvNode
            &&  ttDepth >= depth
            &&  ttValue != VALUE_NONE
            && (ttValue >= beta ? (ttBound & BOUND_LOWER)
                                : (ttBound & BOUND_UPPER))) {
            info->TTCut++;
            return ttValue;
        }
    }

    bonus = -info->historyScore[height-1] / HistoryDivisor;

    eval = info->staticEval[height] =
           ttHit && ttEval != VALUE_NONE            ?  ttEval
         : info->currentMove[height-1] != NULL_MOVE ?  EvalPosition(pos, &Table) + bonus
                                                    : -info->staticEval[height-1] + 2 * TEMPO;

    improving = height >= 2 && eval > info->staticEval[height-2];

    pos->killers[height+1][0] = NONE_MOVE;
    pos->killers[height+1][1] = NONE_MOVE;

    if (RootNode)
        info->historyScore[height+4] = 0;
    else
        info->historyScore[height+2] = 0;

    if (ttHit) {

        if (eval == VALUE_DRAW)
            eval = valueDraw(info);

        if (    ttValue != VALUE_NONE
            && (ttBound & (ttValue > eval ? BOUND_LOWER : BOUND_UPPER)))
            eval = ttValue;
    }
    else {
        storeTTEntry(pos->posKey, NONE_MOVE, VALUE_NONE, eval, DEPTH_NONE, BOUND_NONE);
    }

    if (InCheck)
        improving = 0;

    if (   !PvNode 
        && !InCheck
        && !RootNode
        &&  depth <= RazorDepth
        &&  eval + RazorMargin <= alpha)
        return qsearch(alpha, beta, depth, pos, info, pv, height);

    if (   !PvNode
        && !InCheck
        &&  depth <= BetaPruningDepth
        &&  eval - BetaMargin * (depth - improving) > beta
        &&  eval < KNOWN_WIN)
        return eval;

    if (   !PvNode
        && !InCheck
        && !excludedMove
        &&  eval >= beta
        &&  eval >= info->staticEval[height]
        &&  info->staticEval[height] >= beta - 18 * depth - 18 * improving + 121
        &&  info->currentMove[height-1] != NULL_MOVE
        &&  info->historyScore[height-1] < HistoryNMP
        && (pos->bigPce[pos->side] > 1)
        && (height >= info->nullMinPly || pos->side != info->nullColor)) {

        ASSERT(eval - beta >= 0);
        R = (408 + 42 * depth) / 136 + MIN(3, (eval - beta) / 106);

        MakeNullMove(pos);
        info->currentMove[height] = NULL_MOVE;
        value = -search( -beta, -beta + 1, depth-R, pos, info, &lpv, height+1);
        TakeNullMove(pos);

        if (value >= beta) {

            if (value >= MATE_IN_MAX)
                value = beta;

            if (info->nullMinPly || (abs(beta) < KNOWN_WIN && depth < 13))
                return value;

            info->nullMinPly = height + 3 * (depth-R) / 4;
            info->nullColor = pos->side;

            int value2 = search( beta - 1, beta, depth-R, pos, info, &lpv, height);

            info->nullMinPly = 0;

            if (value2 >= beta)
                return value;
        }
    }

    if (   !PvNode
        && !InCheck
        &&  depth >= ProbCutDepth
        &&  abs(beta) < MATE_IN_MAX) {

        // Try tactical moves which maintain rBeta
        rBeta = MIN(beta + ProbCutMargin[improving], INFINITE - MAX_PLY - 1);

        MoveList list = {0};
        initNoisyMovePicker(&movePicker, info, &list);
        int ProbCutTried = 0;

        while (  (move = selectNextMove(&movePicker, pos, 1)) != NONE_MOVE
               && ProbCutTried < 2 + 2 * !PvNode) {

            if (move == excludedMove)
                continue;

            else if (!MakeMove(pos, move))
                continue;

            ProbCutTried += 1;
            info->currentMove[height] = move;
            info->currentPiece[height] = pieceType(pos->pieces[TOSQ(move)]);

            // First perform the qsearch to verify that the move maintain rBeta, 
            // if the qsearch held rBeta, perform the regular search.
            value = -qsearch( -rBeta, -rBeta + 1, 0, pos, info, &lpv, height+1);

            if (value >= rBeta)
                value = -search( -rBeta, -rBeta + 1, depth-4, pos, info, &lpv, height+1);

            TakeMove(pos);

            // Probcut failed high
            if (value >= rBeta)
                return value;
        }
    }

    if (   !InCheck
        && !ttMove
        &&  depth >= IterativeDepth) {

        search(alpha, beta, depth-7, pos, info, &lpv, height);

        if ((ttHit = probeTTEntry(pos->posKey, &ttMove, &ttValue, &ttEval, &ttDepth, &ttBound)))
            ttValue = valueFromTT(ttValue, height);
    }

    ttNoisy = ttMove && !moveIsQuiet(ttMove);
    MoveList list = {0};
    initMovePicker(&movePicker, pos, info, &list, ttMove, height);
    while ((move = selectNextMove(&movePicker, pos, skipQuiets)) != NONE_MOVE) {

        if (move == excludedMove)
            continue;

        // Get history scores for quiet moves
        if ((isQuiet = moveIsQuiet(move)))
            getHistoryScore(pos, info, move, height, &hist, &cmhist, &fmhist, &gcmhist, &gchmhist);

        if (   !RootNode
            && !excludedMove // Avoid recursive singular search
            &&  depth >= 6
            &&  move == ttMove
            &&  abs(ttValue) < KNOWN_WIN
            && (ttBound & BOUND_LOWER)
            &&  ttDepth >= depth - 3) {

            rBeta = ttValue - 2 * depth;
            excludedMove = move;
            value = search( rBeta - 1, rBeta, depth / 2, pos, info, &lpv, height);
            excludedMove = NONE_MOVE;

            if (value < rBeta) {
                singularExt = 1;

                if (value < rBeta - MIN(4 * depth, 36))
                    LMRflag = 1;
            }

            else if (rBeta >= beta) {

                if (isQuiet)
                    updateKillerMoves(pos, height, move);

                return rBeta;
            }
        }

        if (!MakeMove(pos, move))
            continue;  

        played += 1;

        if (RootNode && elapsedTime(info) > WindowTimerMS)
            uciReportCurrentMove(move, played, info->depth);

        info->currentMove[height] = move;
        info->currentPiece[height] = pieceType(pos->pieces[TOSQ(move)]);

        extension =  (InCheck)
                  || (singularExt);

        newDepth = depth + (extension && !RootNode);

        if (isQuiet)
            quiets[quietsTried++] = move;
        
        if (    depth > 2 
            &&  played > 1
            && (   isQuiet
                || info->staticEval[height] + PieceValue[EG][CAPTURED(move)] <= alpha)) {

            // Use the LMR Formula as a starting point
            R  = LMRTable[MIN(depth, 63)][MIN(played-1, 63)];

            // Increase for non PV and non improving nodes
            R += !PvNode + !improving;

            // Increase for King moves that evade checks
            R += InCheck && IsKi(pos->pieces[TOSQ(move)]);

            // Reduce if ttMove has been singularly extended
            R -= (singularExt + LMRflag);

            if (isQuiet) {

                // Increase if ttMove is noisy
                R += ttNoisy;

                // Decrease or increase based on historyScore
                info->historyScore[height] = (hist   + cmhist 
                                            + fmhist + gcmhist)
                                            - 4608;

                if (   info->historyScore[height] < 0
                    && hist   >= 0
                    && cmhist >= 0
                    && fmhist >= 0)
                    info->historyScore[height] = 0;

                R -= info->historyScore[height] / 16384;
            }

        } else R = 1;

        // Late Move Reduction (LMR). Perform a reduced search on the null alpha window, 
        // if the move fails high it will be re-searched at full depth.
        int d = clamp(newDepth-R, 1, newDepth);
        if (R != 1)
            value = -search( -alpha-1, -alpha, d, pos, info, &lpv, height+1);

        // Do full depth search again on the null alpha window,
        // if LMR is skipped or fails high.
        if ((R != 1 && value > alpha && d != newDepth) || (R == 1 && (!PvNode || played > 1)))
            value = -search( -alpha-1, -alpha, newDepth-1, pos, info, &lpv, height+1);
        
        // For PvNodes only, do a full PV search on the normal window.
        if (PvNode && (played == 1 || (value > alpha && (RootNode || value < beta))))
            value = -search( -beta, -alpha, newDepth-1, pos, info, &lpv, height+1);
        
        TakeMove(pos);

        if (info->stop)
            return VALUE_DRAW;

        if (   RootNode
            && value > alpha
            && played > 1)
            info->bestMoveChanges++;

        if (value > best) {
            best = value;

            if (value > alpha) {
                bestMove = move;

                if (PvNode) {
                    pv->length = 1 + lpv.length;
                    pv->line[0] = bestMove;
                    memcpy(pv->line + 1, lpv.line, sizeof(int) * lpv.length);
                }

                if (PvNode && value < beta)
                    alpha = value;
                else {
                    if (played == 1)
                        info->fhf++;
                    info->fh++;
                    info->historyScore[height] = 0;
                    break;
                }
            }
        }
    }

    // Prefetch TTable for store
    prefetchTTable(pos->posKey);

    if (played == 0)
        best =  excludedMove ?  alpha
              :      InCheck ? -INFINITE + height : VALUE_DRAW;

    if (best >= beta && moveIsQuiet(bestMove))
        updateHistoryStats(pos, info, quiets, quietsTried, height, depth*depth);

    if (!excludedMove) {
        ttBound =  best >= beta       ? BOUND_LOWER
                 : PvNode && bestMove ? BOUND_EXACT : BOUND_UPPER;
        storeTTEntry(pos->posKey, bestMove, valueToTT(best, height), eval, depth, ttBound);
    }

    return best;
}

int qsearch(int alpha, int beta, int depth, Board *pos, SearchInfo *info, PVariation *pv, int height) {

    ASSERT(CheckBoard(pos));
    ASSERT(beta>alpha);

    MovePicker movePicker;
    PVariation lpv;
    pv->length = 0;

    // Prefetch TTable as early as possible
    prefetchTTable(pos->posKey);

    // Updates for UCI reporting
    info->nodes++;
    info->seldepth = MAX(info->seldepth, height);

    // Do we have time left on the clock?
    if ((info->nodes & 1023) == 1023)
        CheckTime(info);

    if (posIsDrawn(pos, pos->ply))
        return VALUE_DRAW;

    if (pos->ply >= MAX_PLY)
        return EvalPosition(pos, &Table);

    const int PvNode = (alpha != beta - 1);

    int InCheck = KingSqAttacked(pos);
    int ttHit, ttValue = 0, ttEval = 0, ttDepth = 0, ttBound = 0;
    int played = 0, moveIsBadCapture, moveIsPruneable, PieceOnTo;
    int value, best, futilityValue, futilityBase, oldAlpha = 0;
    int ttMove = NONE_MOVE, move = NONE_MOVE, bestMove = NONE_MOVE;

    int QSDepth = (InCheck || depth >= DEPTH_QS_CHECKS) ? DEPTH_QS_CHECKS : DEPTH_QS_NO_CHECKS;

    if ((ttHit = probeTTEntry(pos->posKey, &ttMove, &ttValue, &ttEval, &ttDepth, &ttBound))) {

        ttValue = valueFromTT(ttValue, height);

        if (   !PvNode
            &&  ttDepth >= QSDepth
            &&  ttValue != VALUE_NONE
            && (ttValue >= beta ? (ttBound & BOUND_LOWER)
                                : (ttBound & BOUND_UPPER))) {
            info->TTCut++;
            return ttValue;
        }
    }

    best = info->staticEval[height] =
           ttHit && ttEval != VALUE_NONE            ?  ttEval
         : info->currentMove[height-1] != NULL_MOVE ?  EvalPosition(pos, &Table)
                                                    : -info->staticEval[height-1] + 2 * TEMPO;

    if (ttHit) {
        if (   ttValue != VALUE_NONE
            && (ttBound & (ttValue > best ? BOUND_LOWER : BOUND_UPPER)))
            best = info->staticEval[height] = ttValue;
    }

    if (PvNode)
        oldAlpha = alpha;

    if (best >= beta)
        return best;

    alpha = MAX(alpha, best);
    futilityBase = best + QFutilityMargin;

    MoveList list = {0};
    initNoisyMovePicker(&movePicker, info, &list);
    while ((move = selectNextMove(&movePicker, pos, 1)) != NONE_MOVE) {

        moveIsBadCapture = (  !see(pos, move, 1)
                            || badCapture(move, pos));

        moveIsPruneable = (    moveIsBadCapture
                           && !move_canSimplify(move, pos)
                           && !advancedPawnPush(move, pos));

        PieceOnTo = pos->pieces[TOSQ(move)];

        if (!MakeMove(pos, move))
            continue;

        if (   !InCheck
            && !KingSqAttacked(pos)
            &&  futilityBase > -KNOWN_WIN
            &&  moveIsPruneable) {

            futilityValue = futilityBase + PieceValue[EG][PieceOnTo];

            if (futilityValue <= alpha) {
                best = MAX(best, futilityValue);
                TakeMove(pos);
                continue;
            }

            if (futilityBase <= alpha && moveIsBadCapture) {
                best = MAX(best, futilityBase);
                TakeMove(pos);
                continue;
            }
        }

        played += 1;
        info->currentMove[height] = move;
        info->currentPiece[height] = pieceType(pos->pieces[TOSQ(move)]);

        value = -qsearch( -beta, -alpha, depth-1, pos, info, &lpv, height+1);
        TakeMove(pos);

        if (info->stop)
            return VALUE_DRAW;

        // Improved current value
        if (value > best) {
            best = value;

            // Improved current lower bound
            if (value > alpha) {
                bestMove = move;

                if (PvNode) {
                    pv->length = 1 + lpv.length;
                    pv->line[0] = bestMove;
                    memcpy(pv->line + 1, lpv.line, sizeof(int) * lpv.length);
                }

                if (PvNode && value < beta)
                    alpha = value;
                else {
                    if (played == 1)
                        info->fhf++;
                    info->fh++;
                    break;
                }
            }
        }
    }

    // Prefetch TTable for store
    prefetchTTable(pos->posKey);

    ttBound = best >= beta              ? BOUND_LOWER
            : PvNode && best > oldAlpha ? BOUND_EXACT : BOUND_UPPER;
    storeTTEntry(pos->posKey, bestMove, valueToTT(best, height), info->staticEval[height], QSDepth, ttBound);

    return best;
}