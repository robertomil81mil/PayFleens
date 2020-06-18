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
#include "makemove.h"
#include "movegen.h"
#include "polybook.h"
#include "search.h"
#include "time.h"
#include "ttable.h"
#include "uci.h"

int LMRTable[64][64]; // Late Move Reductions Table 

void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {

    for (int index = 0; index < 13; ++index)
        for (int index2 = 0; index2 < BRD_SQ_NUM; ++index2)
            pos->searchHistory[index][index2] = 0;
    
    pos->ply = 0;

    info->stop = 0;
    info->nodes = 0;
    info->fh = 0;
    info->fhf = 0;
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

void getBestMove(S_SEARCHINFO *info, S_BOARD *pos, Limits *limits, int *best) {

    // Minor house keeping for starting a search
    updateTTable(); // Table has an age component
    TimeManagementInit(info, limits, pos->gamePly);
    iterativeDeepening(pos, info, limits, best);
}

void iterativeDeepening(S_BOARD *pos, S_SEARCHINFO *info, Limits *limits, int *best) {

    double timeReduction = 1;

    // Return a book move if we have one
    if (Options.PolyBook) {
        int bookMove = GetBookMove(pos);
        if (bookMove != NOMOVE) {
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

int aspirationWindow(S_BOARD *pos, S_SEARCHINFO *info, int *best) {

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

        int adjustedDepth = MAX(1, info->depth - (failedHighCnt / 2));

        // Perform a search on the window, return if inside the window
        value = search(alpha, beta, adjustedDepth, pos, info, pv, 0);

        if (info->stop)
            return value;

        if (   (value > alpha && value < beta)
            || (elapsedTime(info) >= WindowTimerMS && info->GAME_MODE == UCIMODE))
            uciReport(info, pos, alpha, beta, value);

        if (value > alpha && value < beta) {
            *best = pos->pv.line[0];
            return value;
        }

        // Search failed low
        if (value <= alpha) {
            beta  = (alpha + beta) / 2;
            alpha = MAX(-INFINITE, value - delta);
            failedHighCnt = 0;
        }

        // Search failed high
        if (value >= beta) { 
            beta = MIN(INFINITE, value + delta);
            failedHighCnt++;
        }

        // Expand the search window
        delta += delta / 4 + 5;
    }
}

int search(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, PVariation *pv, int height) {

    ASSERT(CheckBoard(pos));
    ASSERT(beta>alpha);

    PVariation lpv;
    pv->length = 0;

    int InCheck = SqAttacked(pos->KingSq[pos->side],!pos->side,pos);

    if (depth <= 0 && !InCheck)
        return qsearch(alpha, beta, depth, pos, info, pv, height);

    depth = MAX(0, depth);

    info->nodes++;
    info->seldepth = (height == 0) ? 0 : MAX(info->seldepth, height);

    if ((info->nodes & 1023) == 1023)
        CheckTime(info);

    const int PvNode   = (alpha != beta - 1);
    const int RootNode = (pos->ply == 0);
    
    int ttHit, ttValue = 0, ttEval = 0, ttDepth = 0, ttBound = 0;
    int MoveNum = 0, played = 0, singularExt = 0, LMRflag = 0;
    int R, newDepth, rAlpha, rBeta;
    int isQuiet, improving, extension;
    int eval, value = -INFINITE, best = -INFINITE;
    uint16_t ttMove = NOMOVE; int move = NOMOVE, bestMove = NOMOVE, excludedMove = NOMOVE;

    if (!RootNode) {
        if (info->stop || posIsDrawn(pos, pos->ply))
            return depth < 4 ? VALUE_DRAW : VALUE_DRAW + (2 * (info->nodes & 1) - 1);

        if (pos->ply >= MAX_PLY)
            return EvalPosition(pos, &Table);

        rAlpha = alpha > -INFINITE + pos->ply     ? alpha : -INFINITE + pos->ply;
        rBeta  =  beta <  INFINITE - pos->ply - 1 ?  beta :  INFINITE - pos->ply - 1;
        if (rAlpha >= rBeta) return rAlpha;
    }

    if ((ttHit = probeTTEntry(pos->posKey, &ttMove, &ttValue, &ttEval, &ttDepth, &ttBound))) {

        ttValue = valueFromTT(ttValue, pos->ply);

        if (ttDepth >= depth && (depth == 0 || !PvNode)) {
       
            if (    ttBound == BOUND_EXACT
                || (ttBound == BOUND_LOWER && ttValue >= beta)
                || (ttBound == BOUND_UPPER && ttValue <= alpha)) {
                info->TTCut++;
                return ttValue;
            }      
        }
    }

    eval = info->staticEval[height] =
           ttHit && ttEval != VALUE_NONE            ?  ttEval
         : info->currentMove[height-1] != NULL_MOVE ?  EvalPosition(pos, &Table)
                                                    : -info->staticEval[height-1] + 2 * TEMPO;

    improving = height >= 2 && eval > info->staticEval[height-2];

    pos->searchKillers[0][pos->ply+1] = NOMOVE;
    pos->searchKillers[1][pos->ply+1] = NOMOVE;

    if (ttHit) {

        if (    ttValue != VALUE_NONE
            && (ttBound & (ttValue > eval ? BOUND_LOWER : BOUND_UPPER)))
            eval = ttValue;
    }

    if (InCheck)
        improving = 0;

    if (   !PvNode 
        && !InCheck 
        &&  depth <= RazorDepth 
        &&  eval + RazorMargin < alpha)
        return qsearch(alpha, beta, depth, pos, info, pv, height);

    if (   !PvNode
        && !InCheck
        &&  depth <= BetaPruningDepth
        &&  eval - BetaMargin * depth > beta
        &&  eval < KNOWN_WIN)
        return eval;

    if (   !PvNode
        && !InCheck
        &&  eval >= beta
        &&  depth >= NullMovePruningDepth
        &&  info->currentMove[height-1] != NULL_MOVE
        &&  info->currentMove[height-2] != NULL_MOVE
        && !excludedMove
        && (pos->bigPce[pos->side] > 0)
        && (!ttHit || !(ttBound & BOUND_UPPER) || ttValue >= beta)) {

        R = 4 + depth / 6 + MIN(3, (eval - beta) / 200);

        MakeNullMove(pos);
        info->currentMove[height] = NULL_MOVE;
        value = -search( -beta, -beta + 1, depth-R, pos, info, &lpv, height+1);
        TakeNullMove(pos);

        if (value >= beta && abs(beta) < KNOWN_WIN) {
            info->nullCut++;
            return beta;
        }
    }

    if (   !PvNode
        &&  depth >= ProbCutDepth
        &&  abs(beta) < MATE_IN_MAX  
        &&  eval + moveBestCaseValue(pos) >= beta + ProbCutMargin) {

        // Try tactical moves which maintain rBeta
        rBeta = MIN(beta + ProbCutMargin, INFINITE - MAX_PLY - 1);

        S_MOVELIST list[1];
        GenerateAllCaps(pos,list);

        for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

            PickNextMove(MoveNum, list);
            move = list->moves[MoveNum].move;

            if (move == excludedMove)
                continue;

            else if (!MakeMove(pos, move)) 
                continue;

            info->currentMove[height] = move;
            value = -search( -rBeta, -rBeta + 1, depth-4, pos, info, &lpv, height+1);

            TakeMove(pos);

            // Probcut failed high
            if (value >= rBeta) {
                //info->probCut++;
                return value;
            } 
        }
    }   

    S_MOVELIST list[1];
    GenerateAllMoves(pos,list);

    if (ttMove != NOMOVE) {
        for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
            if (list->moves[MoveNum].move == ttMove) {
                list->moves[MoveNum].score = 2000000;
                //printf("TT move found \n");
                break;
            }
        }
    }

    for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        PickNextMove(MoveNum, list);
        move = list->moves[MoveNum].move;

        if (move == excludedMove)
            continue;

        if (    depth >= 6
            &&  move == ttMove
            && !RootNode
            && !excludedMove // Avoid recursive singular search
            &&  abs(ttValue) < KNOWN_WIN
            && (ttBound & BOUND_LOWER)
            &&  ttDepth >= depth - 3) {

            rBeta = ttValue - 2 * depth;
            excludedMove = move;
            value = search( rBeta - 1, rBeta, depth / 2, pos, info, &lpv, height+1);
            excludedMove = NOMOVE;

            if (value < rBeta) {
                singularExt = 1;

                if (value < rBeta - MIN(4 * depth, 36))
                    LMRflag = 1;
            }

            else if (   eval >= beta 
                     && rBeta >= beta)
                return rBeta;
        }

        if (!MakeMove(pos, move))
            continue;  

        played += 1;
        info->currentMove[height] = move;

        isQuiet =  !(move &  MFLAGCAP)
                || !(move & (MFLAGPROM | MFLAGEP));

        if (RootNode && elapsedTime(info) > WindowTimerMS)
            uciReportCurrentMove(move, played, depth);
        
        if (isQuiet && list->quiets > 4 && depth > 2 && played > 1) {

            // Use the LMR Formula as a starting point
            R  = LMRTable[MIN(depth, 63)][MIN(played-1, 63)];

            // Increase for non PV and non improving nodes
            R += !PvNode + !improving;

            // Increase for King moves that evade checks
            R += InCheck && IsKi(pos->pieces[TOSQ(move)]);

            // Reduce if ttMove has been singularly extended
            R -= singularExt - LMRflag;

            // Don't extend or drop into QS
            R  = MIN(depth - 1, MAX(R, 1));

        } else R = 1;

        extension =  (InCheck)
                  || (singularExt);

        newDepth = depth + (extension && !RootNode);

        if (R != 1)
            value = -search( -alpha-1, -alpha, newDepth-R, pos, info, &lpv, height+1);
        
        if ((R != 1 && value > alpha) || (R == 1 && !(PvNode && played == 1)))
            value = -search( -alpha-1, -alpha, newDepth-1, pos, info, &lpv, height+1);
        
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
                    memcpy(pv->line + 1, lpv.line, sizeof(bestMove) * lpv.length);
                }

                if (PvNode && value < beta)
                    alpha = value;
                else {
                    if (played == 1)
                        info->fhf++;
                    info->fh++;

                    if (!(move & MFLAGCAP)) {
                        pos->searchHistory[pos->pieces[FROMSQ(bestMove)]][TOSQ(bestMove)] += depth;

                        if (pos->searchKillers[0][pos->ply] != move) {
                            pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
                            pos->searchKillers[0][pos->ply] = move;
                        }
                    }
                    break;
                }
            }
        }
    }

    if (played == 0)
        best =  excludedMove ?  alpha
              :      InCheck ? -INFINITE + pos->ply : VALUE_DRAW;

    if (!excludedMove) {
        ttBound =  best >= beta       ? BOUND_LOWER
                 : PvNode && bestMove ? BOUND_EXACT : BOUND_UPPER;
        storeTTEntry(pos->posKey, (uint16_t)(bestMove), valueToTT(best, pos->ply), eval, depth, ttBound);
    }

    return best;
}

int qsearch(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, PVariation *pv, int height) {

    ASSERT(CheckBoard(pos));
    ASSERT(beta>alpha);

    PVariation lpv;
    pv->length = 0;

    info->nodes++;
    info->seldepth = MAX(info->seldepth, height);

    if ((info->nodes & 1023) == 1023)
        CheckTime(info);

    int InCheck = SqAttacked(pos->KingSq[pos->side],!pos->side,pos);

    if (posIsDrawn(pos, pos->ply))
        return VALUE_DRAW;

    if (pos->ply >= MAX_PLY)
        return EvalPosition(pos, &Table);

    const int PvNode = (alpha != beta - 1);

    int ttHit, ttValue = 0, ttEval = 0, ttDepth = 0, ttBound = 0;
    int MoveNum = 0, played = 0, moveIsBadCapture, moveIsPruneable, PieceOnTo;
    int eval, value, best, futilityValue, futilityBase, oldAlpha = 0;
    uint16_t ttMove = NOMOVE; int move = NOMOVE, bestMove = NOMOVE;

    int QSDepth = (InCheck || depth >= DEPTH_QS_CHECKS) ? DEPTH_QS_CHECKS : DEPTH_QS_NO_CHECKS;

    if ((ttHit = probeTTEntry(pos->posKey, &ttMove, &ttValue, &ttEval, &ttDepth, &ttBound))) {

        ttValue = valueFromTT(ttValue, pos->ply);

        if (ttDepth >= QSDepth && !PvNode) {

            if (    ttBound == BOUND_EXACT
                || (ttBound == BOUND_LOWER && ttValue >= beta)
                || (ttBound == BOUND_UPPER && ttValue <= alpha)) {
                info->TTCut++;
                return ttValue;
            }     
        }
    }

    eval = info->staticEval[height] =
           ttHit && ttEval != VALUE_NONE            ?  ttEval
         : info->currentMove[height-1] != NULL_MOVE ?  EvalPosition(pos, &Table)
                                                    : -info->staticEval[height-1] + 2 * TEMPO;

    if (ttHit) {
        if (   ttValue != VALUE_NONE
            && (ttBound & (ttValue > eval ? BOUND_LOWER : BOUND_UPPER)))
            eval = ttValue;
    }

    if (PvNode)
        oldAlpha = alpha;

    best = eval;
    alpha = MAX(alpha, best);
    if (alpha >= beta) return eval;
    futilityBase = best + QFutilityMargin;

    S_MOVELIST list[1];
    GenerateAllCaps(pos,list);

    for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

        PickNextMove(MoveNum, list);
        move = list->moves[MoveNum].move;

        moveIsBadCapture = (!see(pos, move, 1)
                         || badCapture(move, pos));

        moveIsPruneable = (    moveIsBadCapture
                           && !move_canSimplify(move, pos)
                           && !advancedPawnPush(move, pos));

        PieceOnTo = pos->pieces[TOSQ(move)];

        if (!MakeMove(pos, move))
            continue;

        if (   !InCheck
            && !SqAttacked(pos->KingSq[pos->side], !pos->side, pos)
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
                    memcpy(pv->line + 1, lpv.line, sizeof(bestMove) * lpv.length);
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

    ttBound = best >= beta              ? BOUND_LOWER
            : PvNode && best > oldAlpha ? BOUND_EXACT : BOUND_UPPER;
    storeTTEntry(pos->posKey, (uint16_t)(bestMove), valueToTT(best, pos->ply), eval, QSDepth, ttBound);

    return best;
}