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

#include "defs.h"
#include "evaluate.h"
#include "search.h"
#include "ttable.h"

int LMRTable[64][64]; // Init LMR Table 

void CheckUp(S_SEARCHINFO *info) {
	// .. check if time up, or interrupt from GUI
	if(info->timeset == TRUE && GetTimeMs() > info->stoptime) {
		info->stopped = TRUE;
	}

	ReadInput(info);
}

void PickNextMove(int moveNum, S_MOVELIST *list) {

	S_MOVE temp;
	int index = 0;
	int bestScore = 0;
	int bestNum = moveNum;

	for (index = moveNum; index < list->count; ++index) {
		if (list->moves[index].score > bestScore) {
			bestScore = list->moves[index].score;
			bestNum = index;
		}
	}

	ASSERT(moveNum>=0 && moveNum<list->count);
	ASSERT(bestNum>=0 && bestNum<list->count);
	ASSERT(bestNum>=moveNum);

	temp = list->moves[moveNum];
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}

int KnightAttack( int side, int sq, const S_BOARD *pos) {
	int t_sq, dir;
	int Knight = side == WHITE ? wN : bN;

	for(int index = 0; index < 8; ++index) {
		dir = PceDir[wN][index];
		t_sq = sq + dir;
		if(!SQOFFBOARD(t_sq) && pos->pieces[t_sq] == Knight)
			return 1;
	}
    return 0;
}

int BishopAttack(int side, int sq, int dir, const S_BOARD *pos) {
	int t_sq = sq + dir;
	int Bishop = side == WHITE ? wB : bB;

	while (!SQOFFBOARD(t_sq)) {
		if (pos->pieces[t_sq] != EMPTY ) {
			if (pos->pieces[t_sq] == Bishop)
				return 1;
			return 0;
		}
		t_sq += dir;
	}
	return 0;
}

int badCapture(int move, const S_BOARD *pos) {

	int from = FROMSQ(move);
    int to = TOSQ(move);
    int captured = CAPTURED(move);

    int Knight = (pos->side^1 == WHITE ? wN : bN);
    int Bishop = (pos->side^1 == WHITE ? wB : bB);

    // Captures by pawn do not lose material
    if (pos->pieces[from] == wP || pos->pieces[from] == bP) return 0;

    // Captures "lower takes higher" (as well as BxN) are good by definition
    if (PieceVal[captured] >= PieceVal[pos->pieces[from]] - 50) return 0;

	if (   pos->pawn_ctrl[pos->side ^ 1][to]
	    && PieceVal[captured] + 200 < PieceVal[pos->pieces[from]])
        return 1;

    if (PieceVal[captured] + 500 < PieceVal[pos->pieces[from]]) {
	
    	if (pos->pceNum[Knight])
			if (KnightAttack(pos->side ^ 1, to, pos)) return 1;

		if (pos->pceNum[Bishop]) {
			if (BishopAttack(pos->side ^ 1, to, NE, pos)) return 1;
			if (BishopAttack(pos->side ^ 1, to, NW, pos)) return 1;
			if (BishopAttack(pos->side ^ 1, to, SE, pos)) return 1;
			if (BishopAttack(pos->side ^ 1, to, SW, pos)) return 1;
		}
	}

    // If a capture is not processed, it cannot be considered bad
    return 0;
}

int move_canSimplify(int move, const S_BOARD *pos) {

    int captured = CAPTURED(move);

    if (  (captured == wP || captured == bP)
        || pos->material[pos->side^1] - PieceVal[captured] > ENDGAME_MAT )
    	return 0;
    else
    	return 1;
}

int IsRepetition(const S_BOARD *pos) {

	int index = 0;

	for(index = pos->hisPly - pos->fiftyMove; index < pos->hisPly-1; ++index) {
		ASSERT(index >= 0 && index < MAXGAMEMOVES);
		if(pos->posKey == pos->history[index].posKey) {
			return 1;
		}
	}
	return 0;
}

void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {

	int index = 0, index2 = 0;

	for(index = 0; index < 13; ++index) {
		for(index2 = 0; index2 < BRD_SQ_NUM; ++index2) {
			pos->searchHistory[index][index2] = 0;
		}
	}
	
	pos->ply = 0;

	info->stopped = 0;
	info->nodes = 0;
	info->fh = 0;
	info->fhf = 0;
	info->nullCut = 0;
	info->probCut = 0;
}

void initLMRTable() {

    // Init Late Move Reductions Table
    for (int depth = 1; depth < 64; depth++)
        for (int played = 1; played < 64; played++)
            LMRTable[depth][played] = 0.75 + log(depth) * log(played) / 2.25;
}

int Quiescence(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, PVariation *pv, int height) {

	ASSERT(CheckBoard(pos));
	ASSERT(beta>alpha);

	PVariation lpv;
	pv->length = 0;

	info->nodes++;
	info->seldepth = MAX(info->seldepth, height);

	if ((info->nodes & 1023) == 1023)
		CheckUp(info);

	int InCheck = SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);

	if (IsRepetition(pos) || pos->fiftyMove > 99 && !InCheck)
		return 0;

	if (pos->ply > MAXDEPTH - 1)
		return EvalPosition(pos);

	const int PvNode = (alpha != beta - 1);

	int ttHit, ttValue = 0, ttEval = 0, ttDepth = 0, ttBound = 0;
	int MoveNum = 0, played = 0;
	int eval, value, best, margin;
	uint16_t ttMove = NOMOVE;

	int TFDepth = ((InCheck || depth >= 0) ?  0  : -1);

    if ((ttHit = probeTTEntry(pos->posKey, &ttMove, &ttValue, &ttEval, &ttDepth, &ttBound))) {

    	ttValue = valueFromTT(ttValue, pos->ply);

        if (ttDepth >= TFDepth && !PvNode) {

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
         : info->currentMove[height-1] != NULL_MOVE ?  EvalPosition(pos)
                                                    : -info->staticEval[height-1] + 2 * TEMPO;        

    if (ttHit) {
        if (ttValue != VALUE_NONE
        && (ttBound & (ttValue > eval ? BOUND_LOWER : BOUND_UPPER)))
            eval = ttValue;
    }

	best = eval;
	alpha = MAX(alpha, eval);
    if (alpha >= beta)
    	return eval;

    /*margin = alpha - eval - QFutilityMargin;
    if (moveBestCaseValue(pos) < margin)
        return eval;*/

	S_MOVELIST list[1];
    GenerateAllCaps(pos,list);

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		//int captured = CAPTURED(list->moves[MoveNum].move);

		/*if ( ( eval + PieceVal[ captured  ] + 200 < alpha ) 
		&&   ( pos->material[pos->side^1] - PieceVal[captured] > ENDGAME_MAT ) 
		&&   ( PROMOTED(list->moves[MoveNum].move) == EMPTY ) )
            continue;*/

        if (   !move_canSimplify(list->moves[MoveNum].move, pos)
        	&& PROMOTED(list->moves[MoveNum].move) == EMPTY
            && badCapture(list->moves[MoveNum].move, pos))
            continue;

        if ( !MakeMove(pos,list->moves[MoveNum].move))
            continue;

		played +=1;
		info->currentMove[height] = list->moves[MoveNum].move;

		value = -Quiescence( -beta, -alpha, depth-1, pos, info, &lpv, height+1);
        TakeMove(pos);

		if (info->stopped)
			return 0;

		// Improved current value
        if (value > best) {
            best = value;

            // Improved current lower bound
            if (value > alpha) {
                alpha = value;

                pv->length = 1 + lpv.length;
                pv->line[0] = list->moves[MoveNum].move;
                memcpy(pv->line + 1, lpv.line, sizeof(list->moves[MoveNum].move) * lpv.length);
            }
        }

        // Search has failed high
        if (alpha >= beta) {
        	if(played==1)
				info->fhf++;
			info->fh++;
            return best;
        }
    }

	return best;
}

int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, PVariation *pv, int height) {

	ASSERT(CheckBoard(pos));
	ASSERT(beta>alpha);

	PVariation lpv;
	pv->length = 0;

	int InCheck = SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);

	if(depth <= 0 && !InCheck)
		return Quiescence(alpha, beta, depth, pos, info, pv, height);

	depth = MAX(0, depth);

	info->nodes++;
	info->seldepth = (height == 0) ? 0 : MAX(info->seldepth, height);

	if ((info->nodes & 1023) == 1023)
		CheckUp(info);

	const int PvNode   = (alpha != beta - 1);
	const int RootNode = (pos->ply == 0);
	
	int ttHit, ttValue = 0, ttEval = 0, ttDepth = 0, ttBound = 0;
	int MoveNum = 0, played = 0, cntMoves = 0, excludedMove = 0, singularExt = 0;
	int R, newDepth, rAlpha, rBeta, OldAlpha = alpha;
	int isQuiet, improving, extension, singular;
	int eval, Score = -INFINITE, BestScore = -INFINITE;
	uint16_t ttMove = NOMOVE; int BestMove = NOMOVE;

	if (!RootNode) {
		if (info->stopped || IsRepetition(pos) || pos->fiftyMove > 99 && !InCheck)
			return depth < 4 ? 0 : 0 + (2 * (info->nodes & 1) - 1);

		if (pos->ply > MAXDEPTH - 1)
			return EvalPosition(pos);

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
         : info->currentMove[height-1] != NULL_MOVE ?  EvalPosition(pos)
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
		return Quiescence(alpha, beta, depth, pos, info, pv, height);

	if (   !PvNode
        && !InCheck
        &&  depth <= BetaPruningDepth
        &&  eval - BetaMargin * depth > beta)
		return eval;

	if (   !PvNode
        && !InCheck
        &&  eval >= beta
        &&  depth >= NullMovePruningDepth
        &&  info->currentMove[height-1] != NULL_MOVE
        &&  info->currentMove[height-2] != NULL_MOVE
        && (pos->bigPce[pos->side] > 0)
        && (!ttHit || !(ttBound & BOUND_UPPER) || ttValue >= beta)) {

        R = 4 + depth / 6 + MIN(3, (eval - beta) / 200);

        MakeNullMove(pos);
        info->currentMove[height] = NULL_MOVE;
        Score = -AlphaBeta( -beta, -beta + 1, depth-R, pos, info, &lpv, height+1);
        TakeNullMove(pos);

        if (Score >= beta) {
        	info->nullCut++;
        	return beta;
        }
    }

    if (   !PvNode
        &&  depth >= ProbCutDepth
        &&  abs(beta) < MATE_IN_MAX  
        &&  eval + moveBestCaseValue(pos) >= beta + ProbCutMargin) {

        // Try tactical moves which maintain rBeta
        rBeta = MIN(beta + ProbCutMargin, INFINITE - MAXPLY - 1);

    	S_MOVELIST list[1];
    	GenerateAllCaps(pos,list);

		for(int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

			PickNextMove(MoveNum, list);

        	if ( !MakeMove(pos,list->moves[MoveNum].move)) continue;
        	info->currentMove[height] = list->moves[MoveNum].move;
        	Score = -AlphaBeta( -rBeta, -rBeta + 1, depth-4, pos, info, &lpv, height+1);
        	TakeMove(pos);

        	// Probcut failed high
            if (Score >= rBeta) {
            	//info->probCut++;
            	return Score;
            } 
        }
    }   

	S_MOVELIST list[1];
    GenerateAllMoves(pos,list);

    if (ttMove != NOMOVE) {
		for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
			if (list->moves[MoveNum].move == ttMove) {
				list->moves[MoveNum].score = 2000000;
				//printf("TT move found \n");
				break;
			}
		}
	}

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		int to = TOSQ(list->moves[MoveNum].move);
    	int captured = CAPTURED(list->moves[MoveNum].move);
    	int prPce = PROMOTED(list->moves[MoveNum].move);

    	isQuiet = (captured == EMPTY || prPce == EMPTY);
		
		if (isQuiet && list->quiets > 4 && depth > 2 && played > 1) {

            /// Use the LMR Formula as a starting point
            R  = LMRTable[MIN(depth, 63)][MIN(cntMoves, 63)];

            // Increase for non PV and non improving nodes
            R += !PvNode + !improving;

            // Increase for King moves that evade checks
            R += InCheck && pos->pieces[to] == (wK || bK);

            // Reduce for Killers and Counters
            //R -= movePicker.stage < STAGE_QUIET;

            // Adjust based on history scores
            //R -= MAX(-2, MIN(2, (hist + cmhist + fmhist) / 5000));

            // Don't extend or drop into QS
            R  = MIN(depth - 1, MAX(R, 1));
            //printf("R %d\n", R );

        } else R = 1;

        if (  depth >= 6
          &&  list->moves[MoveNum].move == ttMove
          && !RootNode
          && !excludedMove // Avoid recursive singular search
          &&  abs(ttValue) < 10000
          && (ttBound & BOUND_LOWER)
          &&  ttDepth >= depth - 3) {

          	rBeta = ttValue - 2 * depth;
          	excludedMove = 1;
          	Score = AlphaBeta( rBeta - 1, rBeta, depth / 2, pos, info, &lpv, height+1);
          	excludedMove = 0;

	        if (Score < rBeta) {
	            singularExt = 1;
	            R--;

	            if (Score < rBeta - MIN(4 * depth, 36)) R--;
	        } else if (eval >= beta && rBeta >= beta) return rBeta;
      	}

        if ( !MakeMove(pos,list->moves[MoveNum].move))  continue;  

		played += 1;
		info->currentMove[height] = list->moves[MoveNum].move;

		//elapsed = GetTimeMs()-info->starttime;

		/*if(RootNode && elapsed >= 2500) {
			printf("info depth %d currmove %s currmovenumber %d\n", depth, PrMove(list->moves[MoveNum].move), MoveNum);
		}*/

        extension =  (InCheck)
        		  || (singularExt);

        newDepth = depth + (extension && !RootNode);

		if (R != 1)
			Score = -AlphaBeta( -alpha-1, -alpha, newDepth-R, pos, info, &lpv, height+1);
		
		if ((R != 1 && Score > alpha) || (R == 1 && !(PvNode && played == 1)))
            Score = -AlphaBeta( -alpha-1, -alpha, newDepth-1, pos, info, &lpv, height+1);
		
		if (PvNode && (played == 1 || (Score > alpha && (RootNode || Score < beta))))
			Score = -AlphaBeta( -beta, -alpha, newDepth-1, pos, info, &lpv, height+1);
		
		
		TakeMove(pos);
		cntMoves++;

		if (info->stopped)
			return 0;

		if (Score > BestScore) {
			BestScore = Score;
			BestMove = list->moves[MoveNum].move;

			if (Score > alpha) {
				alpha = Score;

				pv->length = 1 + lpv.length;
                pv->line[0] = list->moves[MoveNum].move;
                memcpy(pv->line + 1, lpv.line, sizeof(list->moves[MoveNum].move) * lpv.length);

				if (alpha >= beta) break;
			}
		}
		if (Score > BestScore && Score >= beta) {
			if(played==1)
				info->fhf++;
			info->fh++;

			if (   !(list->moves[MoveNum].move & MFLAGCAP) 
			    &&  pos->searchKillers[0][pos->ply] != list->moves[MoveNum].move) {
				
				pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
				pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
			}
			if (!(list->moves[MoveNum].move & MFLAGCAP))
				pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
		}
    }

    if (played == 0) BestScore = (InCheck ? -INFINITE + pos->ply : 0);

	ASSERT(alpha>=OldAlpha);

	ttBound = (BestScore >= beta    ? BOUND_LOWER
			:  BestScore > OldAlpha ? BOUND_EXACT : BOUND_UPPER);
	storeTTEntry(pos->posKey, (uint16_t)(BestMove), valueToTT(BestScore, pos->ply), eval, depth, ttBound);

	return BestScore;
}

int aspirationWindow(int depth, int lastValue, S_BOARD *pos, S_SEARCHINFO *info) {

    ASSERT(CheckBoard(pos));

    int alpha, beta, value, delta = WindowSize;
    PVariation *const pv = &pos->pv;

    // Create an aspiration window, unless still below the starting depth
    alpha = depth >= WindowDepth ? MAX(-INFINITE, lastValue - delta) : -INFINITE;
    beta  = depth >= WindowDepth ? MIN( INFINITE, lastValue + delta) :  INFINITE;

    // Keep trying larger windows until one works
    int failedHighCnt = 0;
    while (1) {

    	int adjustedDepth = MAX(1, depth - failedHighCnt);

        // Perform a search on the window, return if inside the window
        value = AlphaBeta(alpha, beta, adjustedDepth, pos, info, pv, 0);

        if (info->stopped)
        	return value;

        if (value > alpha && value < beta)
            return value;

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

void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {

	int bestMove = NOMOVE;
	int bestScore = -INFINITE;
	int currentDepth = 0;

	ClearForSearch(pos,info);

	/*if(EngineOptions->UseBook == TRUE) {
		bestMove = GetBookMove(pos);
	}*/

	// iterative deepening
	if (bestMove == NOMOVE) { 
		for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth ) { 
			info->values[currentDepth] = aspirationWindow(currentDepth, info->values[currentDepth], pos, info);
			bestScore = info->values[currentDepth];

			if (   abs(info->values[info->depth]) >= MATE_IN_MAX 
        		&& currentDepth >= 12
        		&& info->values[currentDepth-2] == info->values[currentDepth])
	    		break;
	    	
	    	if (currentDepth == MAXDEPTH - 1)
	    		info->stopped = 1;

			if (info->stopped)
				break;

			int elapsed = GetTimeMs()-info->starttime;
			int nps = (1000 * (info->nodes / (1+elapsed)));
			bestMove = pos->pv.line[0];
			if(info->GAME_MODE == UCIMODE) {
				printf("info score cp %d depth %d seldepth %d nodes %ld nps %d time %d hashfull %d ",
					bestScore,currentDepth,info->seldepth,info->nodes,nps,GetTimeMs()-info->starttime,hashfullTTable() );
			} else if(info->GAME_MODE == XBOARDMODE && info->POST_THINKING == TRUE) {
				printf("%d %d %d %ld ",
					currentDepth,bestScore,(GetTimeMs()-info->starttime)/10,info->nodes);
			} else if(info->POST_THINKING == TRUE) {
				printf("score:%d depth:%d seldepth %d nodes:%ld nps %d time:%d(ms) hashfull %d ",
					bestScore,currentDepth,info->seldepth,info->nodes,nps,GetTimeMs()-info->starttime,hashfullTTable());
			}
			if(info->GAME_MODE == UCIMODE || info->POST_THINKING == TRUE) {
				if(!info->GAME_MODE == XBOARDMODE) {
					printf("pv");
				}
				for (int pvNum = 0; pvNum < pos->pv.length; pvNum++) {
        			printf(" %s",PrMove(pos->pv.line[pvNum]));
    			}
				printf("\n");
			}
		}
	}

	if(info->GAME_MODE == UCIMODE) {
		printf("bestmove %s\n",PrMove(bestMove));
	} else if(info->GAME_MODE == XBOARDMODE) {
		printf("move %s\n",PrMove(bestMove));
		MakeMove(pos, bestMove);
	} else {
		printf("\nHits:%d Overwrite:%d NewWrite:%d TTCut:%d\nOrdering %.2f NullCut:%d Probcut:%d\n",0,0,0,info->TTCut,
		(info->fhf/info->fh)*100,info->nullCut,info->probCut);
		printf("\n\n***!! PayFleens makes move %s !!***\n\n",PrMove(bestMove));
		MakeMove(pos, bestMove);
		PrintBoard(pos);
	}
}
