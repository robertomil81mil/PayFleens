// search.c

#include "stdio.h"
#include <math.h>
#include "defs.h"
#include "evaluate.h"

int rootDepth;
int LMRTable[64][64]; // Init LMR Table 

const int RazorDepth = 1;
const int RazorMargin = 325;

const int BetaPruningDepth = 8;
const int BetaMargin = 85;
const int NullMovePruningDepth = 2;
const int ProbCutDepth = 5;
const int ProbCutMargin = 100;
const int FutilityMargin = 9;
const int FutilityPruningDepth = 8;
const int WindowDepth   = 5;
const int WindowSize    = 14;
const int WindowTimerMS = 2500;

static int MATE_IN(int ply) {
  return INFINITE - ply;
}
               
static int MATED_IN(int ply) {
  return -INFINITE + ply;
}

static void CheckUp(S_SEARCHINFO *info) {
	// .. check if time up, or interrupt from GUI
	if(info->timeset == TRUE && GetTimeMs() > info->stoptime) {
		info->stopped = TRUE;
	}

	ReadInput(info);
}

static void PickNextMove(int moveNum, S_MOVELIST *list) {

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

int isPiece(const int color, const int piece, const int sq, const S_BOARD *pos) {
    return ( (pos->pieces[sq] == piece) && (PieceCol[pos->pieces[sq]] == color) );
}

int leaperAttack( int side, int sq, const S_BOARD *pos ) {
	int t_sq, dir;

	for(int index = 0; index < 8; ++index) {
		dir = PceDir[wN][index];
		t_sq = sq + dir;
		if(!SQOFFBOARD(t_sq) && IsKn(pos->pieces[t_sq]) && PieceCol[pos->pieces[t_sq]] == side ) { 
			return 1;   
		}
	}
    return 0;
}

int bishAttack(int side, int sq, int dir, const S_BOARD *pos) {
	int t_sq = sq + dir;

	while (!SQOFFBOARD(t_sq)) {
		if (pos->pieces[t_sq] != EMPTY ) {
			if (PieceCol[pos->pieces[t_sq]] == side 
			&& (pos->pieces[t_sq] == wB || pos->pieces[t_sq] == bB))
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

    /* captures by pawn do not lose material */
    if (pos->pieces[from] == wP || pos->pieces[from] == bP) return 0;

    /* Captures "lower takes higher" (as well as BxN) are good by definition. */
    if ( PieceVal[captured] >= PieceVal[pos->pieces[from]] - 50) return 0;

	if (pos->pawn_ctrl[pos->side ^ 1][to]
	&& PieceVal[captured] + 200 < PieceVal[pos->pieces[from]])
        return 1;

    if (PieceVal[captured] + 500 < PieceVal[pos->pieces[from]]) {
		if (leaperAttack(pos->side ^ 1, to, pos)) return 1;
		if (bishAttack(pos->side ^ 1, to, NE, pos)) return 1;
		if (bishAttack(pos->side ^ 1, to, NW, pos)) return 1;
		if (bishAttack(pos->side ^ 1, to, SE, pos)) return 1;
		if (bishAttack(pos->side ^ 1, to, SW, pos)) return 1;
	}

    /* if a capture is not processed, it cannot be considered bad */
    return 0;
}

int move_canSimplify(int move, const S_BOARD *pos) {

    int captured = CAPTURED(move);

    if ( (captured == wP || captured == bP)
    ||    pos->material[pos->side^1] - PieceVal[captured] > ENDGAME_MAT )
    	return 0;
    else
    	return 1;
}

static int IsRepetition(const S_BOARD *pos) {

	int index = 0;

	for(index = pos->hisPly - pos->fiftyMove; index < pos->hisPly-1; ++index) {
		ASSERT(index >= 0 && index < MAXGAMEMOVES);
		if(pos->posKey == pos->history[index].posKey) {
			return TRUE;
		}
	}
	return FALSE;
}

int boardDrawnByRepetition(S_BOARD *board, int height) {

    int reps = 0;

    // Look through hash histories for our moves
    for (int i = board->hisPly - 2; i >= 0; i -= 2) {

        // No draw can occur before a zeroing move
        if (i < board->hisPly - board->fiftyMove)
            break;

        // Check for matching hash with a fold after the root,
        // or a three fold which occurs in part before the root move
        if ( board->posKey == board->history[i].posKey
            && (i > board->hisPly - height || ++reps == 2))
            return 1;
    }

    return 0;
}

static void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {

	int index = 0;
	int index2 = 0;
	int index3 = 0;

	for(index = 0; index < 13; ++index) {
		for(index2 = 0; index2 < BRD_SQ_NUM; ++index2) {
			pos->searchHistory[index][index2] = 0;
		}
	}

	for(index = 0; index < 2; ++index) {
		for(index2 = 0; index2 < MAXDEPTH; ++index2) {
			pos->searchKillers[index][index2] = 0;
		}
	}

	for(index3 = 0; index3 < MAXDEPTH; ++index3) {
		info->Value[index3] = 0;
		info->currentMove[index3] = 0;
		info->staticEval[index3] = 0;
	}
	
	pos->HashTable->overWrite=0;
	pos->HashTable->hit=0;
	pos->HashTable->cut=0;
	pos->ply = 0;

	info->stopped = 0;
	info->nodes = 0;
	//info->seldepth = 0;
	info->fh = 0;
	info->fhf = 0;
}

void initLMRTable() {

    // Init Late Move Reductions Table
    for (int depth = 1; depth < 64; depth++)
        for (int played = 1; played < 64; played++)
            LMRTable[depth][played] = 0.75 + log(depth) * log(played) / 2.25;
}
void printLMR() {
	for (int depth = 1; depth < 64; depth++)
        for (int played = 1; played < 64; played++)
        	printf("LMRTable[%d][%d] FINAL CALCULATION = %d\n",depth,played,LMRTable[depth][played] );
}

static int lmr( int move, bool InCheck, int depth, S_BOARD *pos) {

	int from = FROMSQ(move);
    int to = TOSQ(move);
    int captured = CAPTURED(move);
    int prPce = PROMOTED(move);
    int side = pos->side;

	int interesting = (InCheck || captured != EMPTY || prPce != EMPTY || 
						pos->pieces[to] == wK ||
						pos->pieces[to] == bK ||
						side == WHITE && pos->pieces[from] == wP && RanksBrd[to] >= RANK_5 ||
						side == BLACK && pos->pieces[from] == bP && RanksBrd[to] <= RANK_4);
	int red = 0;
	if(!interesting && depth >= 3) {
		red = 1;
		if(depth >= 5) {
			red = depth / 3;
		}
	}
	return red;
}

static int Quiescence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info, int height) {

	ASSERT(CheckBoard(pos));
	ASSERT(beta>alpha);

	int BestMove, BestScore, value, captured;

	if(( info->nodes & 1023 ) == 0) {
		CheckUp(info);
	}

	info->nodes++;
	info->seldepth = MAX(info->seldepth, height);

	//printf("pos-ply in QS %d\n",pos->ply );
	int eval; // Score

	if (info->currentMove[height-1] != NOMOVE) {
        info->staticEval[height] = eval = EvalPosition(pos);
    } else {
    	info->staticEval[height] = eval = -info->staticEval[height-1] + 2 * TEMPO;
    }

	int InCheck = SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);

	if(IsRepetition(pos) || pos->fiftyMove > 99 && !InCheck) {
		return 0;
	}

	if(pos->ply > MAXDEPTH - 1) {
		return eval;
	}

	int PvMove = NOMOVE;

	if( ProbeHashEntry(pos, &PvMove, &value, alpha, beta, height) == TRUE ) {
		pos->HashTable->cut++;
		return value;
	}

	int best = eval;
	alpha = MAX(alpha, eval);
    if (alpha >= beta){
    	return eval;
    } 

	S_MOVELIST list[1];
    GenerateAllCaps(pos,list);

    int MoveNum = 0;
	int Legal = 0;
	int OldAlpha = alpha; // best = eval  alpha != oldalpha eval != best
	BestMove = NOMOVE;
	BestScore = -INFINITE;

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

		captured = CAPTURED(list->moves[MoveNum].move);

		if ( ( eval + PieceVal[ captured  ] + 200 < alpha ) 
		&&   ( pos->material[pos->side^1] - PieceVal[captured] > ENDGAME_MAT ) 
		&&   ( PROMOTED(list->moves[MoveNum].move) == EMPTY ) )
            continue;

        if ( badCapture( list->moves[MoveNum].move, pos )
        &&  !move_canSimplify( list->moves[MoveNum].move, pos )
        &&  PROMOTED(list->moves[MoveNum].move) == EMPTY )
            continue;

        if ( !MakeMove(pos,list->moves[MoveNum].move))  {
            continue;
        }

		Legal++;
		info->currentMove[height] = list->moves[MoveNum].move;

		value = -Quiescence( -beta, -alpha, pos, info, height+1);
        TakeMove(pos);

		if(info->stopped == TRUE) {
			return 0;
		}
		// Improved current value
        if (value > best) {
            best = value;

            // Improved current lower bound
            if (value > alpha) {
                alpha = value;

                // Update the Principle Variation
                BestScore = value;
				BestMove = list->moves[MoveNum].move;
				//StoreHashEntry(pos, BestMove, alpha, HFALPHA, height);
				//StoreHashEntry(pos, BestMove, alpha, HFALPHA, height);
            }
        }

        // Search has failed high
        if (alpha >= beta) {
        	if(Legal==1) {
				info->fhf++;
			}
			info->fh++;
			//StoreHashEntry(pos, BestMove, beta, HFBETA, height);
            return best;
        }
    }

	ASSERT(alpha >= OldAlpha);

	/*if(eval != best) {
		StoreHashEntry(pos, BestMove, BestScore, HFEXACT, height);
	} else {
		StoreHashEntry(pos, BestMove, alpha, HFALPHA, height);
	}*/

	return best;
}

static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int DoNull, int height) {

	ASSERT(CheckBoard(pos));
	ASSERT(beta>alpha);
	ASSERT(depth>=0);
	int rAlpha, rBeta, improving, isQuiet, R, extension, newDepth, played,elapsed;
	int quiets = 0;
	const int PvNode = (alpha != beta - 1);
	const int RootNode = (pos->ply == 0);
	//int starttime = GetTimeMs();
	int InCheck = SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
	int eval;

	if (info->currentMove[height-1] != NOMOVE) {
        info->staticEval[height] = eval = EvalPosition(pos);
    } else {
    	info->staticEval[height] = eval = -info->staticEval[height-1] + 2 * TEMPO;
    }

	//printf("pos-ply %d\n",pos->ply );

	if(depth <= 0) {
		return Quiescence(alpha, beta, pos, info, height);
	}

	if(( info->nodes & 1023 ) == 0) {
		CheckUp(info);
	}

	info->nodes++;
	info->seldepth = RootNode ? 0 : MAX(info->seldepth, height);

	MAX(0, depth);

	if (!RootNode) {
		if((IsRepetition(pos) || pos->fiftyMove > 99 && !InCheck) && pos->ply) {
			return depth < 4 ? 0 : 0 + (2 * (info->nodes & 1) - 1);
		}
		if(pos->ply > MAXDEPTH - 1) {
			return EvalPosition(pos);
		}
		rAlpha = alpha > -INFINITE + pos->ply     ? alpha : -INFINITE + pos->ply;
        rBeta  =  beta <  INFINITE - pos->ply - 1 ?  beta :  INFINITE - pos->ply - 1;
        if (rAlpha >= rBeta) return rAlpha;
	}

	if(InCheck == TRUE) {
		depth++;
	}

	int Score = -INFINITE;
	int PvMove = NOMOVE;

	if( ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth) == TRUE ) {
		pos->HashTable->cut++;
		return Score;
	}

	improving = height >= 2 && eval > info->staticEval[height-2];

	if( !PvNode && !InCheck && depth <= RazorDepth && eval + RazorDepth < alpha) {
		return Quiescence(alpha, beta, pos, info, height);
	}
	if (   !PvNode
        && !InCheck
        &&  depth <= BetaPruningDepth
        &&  eval - BetaMargin * depth > beta) {
		return eval;
	}

	if( DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 0) && depth >= 4) {
		//int R = 4 + depth / 6 + MIN(3, (eval - beta) / 200);
		MakeNullMove(pos);

		info->currentMove[height] = NOMOVE;

		Score = -AlphaBeta( -beta, -beta + 1, depth-4, pos, info, FALSE, height+1);
		TakeNullMove(pos);
		if(info->stopped == TRUE) {
			return 0;
		}

		if (Score >= beta && abs(Score) < MATE_IN_MAX) {
			info->nullCut++;
			return beta;
		}
	}	     

	S_MOVELIST list[1];
    GenerateAllMoves(pos,list);

    int MoveNum = 0;
	int Legal = 0;
	int OldAlpha = alpha;
	int BestMove = NOMOVE;
	int BestScore = -INFINITE;
	Score = -INFINITE;
	int cntMoves = 0;

	if( PvMove != NOMOVE) {
		for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
			if( list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
				//printf("Pv move found \n");
				break;
			}
		}
	}

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		PickNextMove(MoveNum, list);

        if ( !MakeMove(pos,list->moves[MoveNum].move))  {
            continue;
        }

		Legal++;
		played += 1;
		info->currentMove[height] = list->moves[MoveNum].move;

		//elapsed = GetTimeMs()-info->starttime;

		/*if(RootNode && elapsed >= 2500) {
			printf("info depth %d currmove %s currmovenumber %d\n", depth, PrMove(list->moves[MoveNum].move), MoveNum);
		}*/

		int from = FROMSQ(list->moves[MoveNum].move);
    	int to = TOSQ(list->moves[MoveNum].move);
    	int captured = CAPTURED(list->moves[MoveNum].move);
    	int prPce = PROMOTED(list->moves[MoveNum].move);

    	isQuiet = (captured == EMPTY || prPce == EMPTY);

		int lmrRed = 0;
		int ext = 0;
		/*int ext = (InCheck)
               	|| (isQuiet && list->quiets <= 4 );
		ext = MIN(depth -1, MAX(ext, 1));*/
		
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

		if (R != 1) {
			Score = -AlphaBeta( -alpha-1, -alpha, depth-R, pos, info, TRUE, height+1);
		}
		if ((R != 1 && Score > alpha) || (R == 1 && !(PvNode && played == 1))) {
            Score = -AlphaBeta( -alpha-1, -alpha, depth-1+ext, pos, info, TRUE, height+1);
		}
		if(PvNode && (played == 1 || Score > alpha)) {
			Score = -AlphaBeta( -beta, -alpha, depth-1+ext, pos, info, TRUE, height+1);
		}
		
		TakeMove(pos);
		cntMoves++;

		if(info->stopped == TRUE) {
			return 0;
		}
		if(Score > BestScore) {
			BestScore = Score;
			BestMove = list->moves[MoveNum].move;
			if(Score > alpha) {
				if(Score >= beta) {
					if(Legal==1) {
						info->fhf++;
					}
					info->fh++;

					if(!(list->moves[MoveNum].move & MFLAGCAP)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
					}

					StoreHashEntry(pos, BestMove, beta, HFBETA, depth);

					return beta;
				}
				alpha = Score;

				if(!(list->moves[MoveNum].move & MFLAGCAP)) {
					pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
				}
			}
		}
    }

	if(Legal == 0) {
		if(InCheck) {
			return -INFINITE + pos->ply;
		} else {
			return 0;
		}
	}

	ASSERT(alpha>=OldAlpha);

	if(alpha != OldAlpha) {
		StoreHashEntry(pos, BestMove, BestScore, HFEXACT, depth);
	} else {
		StoreHashEntry(pos, BestMove, alpha, HFALPHA, depth);
	}

	return alpha;
}

static int aspirationWindow( int depth, int lastValue, S_BOARD *pos, S_SEARCHINFO *info) {

    ASSERT(CheckBoard(pos));

    int alpha, beta, value, delta = WindowSize;

    //ASSERT(beta>alpha);

    // Create an aspiration window, unless still below the starting depth
    alpha = depth >= WindowDepth ? MAX(-INFINITE, lastValue - delta) : -INFINITE;
    beta  = depth >= WindowDepth ? MIN( INFINITE, lastValue + delta) :  INFINITE;

    // Keep trying larger windows until one works
    while (1) {

        // Perform a search on the window, return if inside the window
        value = AlphaBeta(alpha, beta, depth, pos, info, TRUE, 0);
        if (value > alpha && value < beta)
            return value;


        // Search failed low
        if (value <= alpha) {
            beta  = (alpha + beta) / 2;
            alpha = MAX(-INFINITE, alpha - delta);
        }

        // Search failed high
        if (value >= beta) { 
            beta = MIN(INFINITE, beta + delta);
        }

        // Expand the search window
        delta = delta + delta / 2;
    }
}

void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {

	int bestMove = NOMOVE;
	int bestScore = -INFINITE;
	int currentDepth = 0;
	int pvMoves = 0;
	int pvNum = 0;
	int lastValue;

	S_MOVELIST list[1];
    GenerateAllMoves(pos,list);
	ClearForSearch(pos,info);

	/*if(EngineOptions->UseBook == TRUE) {
		bestMove = GetBookMove(pos);
	}*/

	//printf("Search depth:%d\n",info->depth);

	// iterative deepening
	if(bestMove == NOMOVE) { 
		for( currentDepth = 1; currentDepth <= info->depth; ++currentDepth ) {
								// alpha	 beta
			rootDepth = currentDepth;

			//bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE, 0);
			info->Value[currentDepth] = aspirationWindow(currentDepth, info->Value[currentDepth], pos, info);
			bestScore = info->Value[currentDepth];
			int thisValue = info->Value[currentDepth];
	    	int stopValue = info->Value[currentDepth-4];
	    	int stopSearchingINFINITE= info->Value[currentDepth-2];

	    	/*if(currentDepth >= 24) {
	    		if(stopValue == thisValue) {
	    			info->stopped = TRUE;
	    		}
	    	}*/
	    	if(currentDepth >= 12 && abs(bestScore) >= MATE_IN_MAX ) {
	    		if(stopSearchingINFINITE== thisValue) {
	    			info->stopped = TRUE;
	    		}	
	    	}
	    	
	    	if(list->count == 1 && currentDepth > 5) {
	    		info->stopped = TRUE;
	    	}
	    	if(currentDepth == MAXDEPTH - 1) {
	    		info->stopped = TRUE;
	    	}
			if(info->stopped == TRUE) {
				break;
			}
			// nt nps         = (int)(1000 * (nodes / (1 + elapsed)));
			pvMoves = GetPvLine(currentDepth, pos);
			int elapsed = GetTimeMs()-info->starttime;
			int nps = (1000*(info->nodes / (1 + elapsed)));
			bestMove = pos->PvArray[0];
			if(info->GAME_MODE == UCIMODE) {
				printf("info score cp %d depth %d seldepth %d nodes %ld nps %d time %d ",
					bestScore,currentDepth,info->seldepth,info->nodes,nps,GetTimeMs()-info->starttime);
			} else if(info->GAME_MODE == XBOARDMODE && info->POST_THINKING == TRUE) {
				printf("%d %d %d %ld ",
					currentDepth,bestScore,(GetTimeMs()-info->starttime)/10,info->nodes);
			} else if(info->POST_THINKING == TRUE) {
				printf("score:%d depth:%d seldepth %d nodes:%ld nps %d time:%d(ms) ",
					bestScore,currentDepth,info->seldepth,info->nodes,nps,GetTimeMs()-info->starttime);
			}
			if(info->GAME_MODE == UCIMODE || info->POST_THINKING == TRUE) {
				pvMoves = GetPvLine(currentDepth, pos);
				if(!info->GAME_MODE == XBOARDMODE) {
					printf("pv");
				}
				for(pvNum = 0; pvNum < pvMoves; ++pvNum) {
					printf(" %s",PrMove(pos->PvArray[pvNum]));
				}
				printf("\n");
			}

			//printf("Hits:%d Overwrite:%d NewWrite:%d Cut:%d\nOrdering %.2f NullCut:%d\n",pos->HashTable->hit,pos->HashTable->overWrite,pos->HashTable->newWrite,pos->HashTable->cut,
			//(info->fhf/info->fh)*100,info->nullCut);
		}
	}

	if(info->GAME_MODE == UCIMODE) {
		printf("bestmove %s\n",PrMove(bestMove));
	} else if(info->GAME_MODE == XBOARDMODE) {
		printf("move %s\n",PrMove(bestMove));
		MakeMove(pos, bestMove);
	} else {
		printf("\n\n***!! PayFleens makes move %s !!***\n\n",PrMove(bestMove));
		MakeMove(pos, bestMove);
		//printEval(pos);
		PrintBoard(pos);
	}
}