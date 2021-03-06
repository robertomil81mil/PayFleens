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

#if defined(TUNE)

#pragma once

#include "defs.h"

#define NPARTITIONS  (     64) // Total thread partitions
#define KPRECISION   (     10) // Iterations for computing K
#define REPORTING    (    100) // How often to report progress
#define NTERMS       (    457) // Total terms in the Tuner (457)

#define LEARNING     (    5.0) // Learning rate
#define LRDROPRATE   (   1.25) // Cut LR by this each failure
#define BATCHSIZE    (  16384) // FENs per mini-batch
#define NPOSITIONS   ( 724167) // Total FENS in the book

#define STACKSIZE ((int)((double) NPOSITIONS * NTERMS / 32))

#define TunePawnValue                   (1)
#define TuneKnightValue                 (1)
#define TuneBishopValue                 (1)
#define TuneRookValue                   (1)
#define TuneQueenValue                  (1)
#define TuneKingValue                   (1)
#define TunePawnPSQT32                  (1)
#define TuneKnightPSQT32                (1)
#define TuneBishopPSQT32                (1)
#define TuneRookPSQT32                  (1)
#define TuneQueenPSQT32                 (1)
#define TuneKingPSQT32                  (1)
#define TunePawnDoubled                 (1)
#define TunePawnIsolated                (1)
#define TunePawnBackward                (1)
#define TunePawnPassed                  (1)
#define TunePawnPassedConnected         (1)
#define TunePawnConnected               (1)
#define TunePawnSupport                 (1)
#define TuneKP_1                        (1)
#define TuneKP_2                        (1)
#define TuneKP_3                        (1)
#define TuneKnightOutpost               (1)
#define TuneKnightTropism               (1)
#define TuneKnightDefender              (1)
#define TuneKnightMobility              (1)
#define TuneBishopOutpost               (1)
#define TuneBishopTropism               (1)
#define TuneBishopRammedPawns           (1)
#define TuneBishopDefender              (1)
#define TuneBishopMobility              (1)
#define TuneRookOpen                    (1)
#define TuneRookSemi                    (1)
#define TuneRookOnSeventh               (1)
#define TuneRookTropism                 (1)
#define TuneRookMobility                (1)
#define TuneQueenPreDeveloped           (1)
#define TuneQueenTropism                (1)
#define TuneQueenMobility               (1)
#define TuneBlockedStorm                (1)
#define TuneShelterStrength             (1)
#define TuneUnblockedStorm              (1)
#define TuneKingPawnLessFlank           (1)
#define TuneComplexityPassedPawns       (1)
#define TuneComplexityTotalPawns        (1)
#define TuneComplexityOutflanking       (1)
#define TuneComplexityPawnFlanks        (1)
#define TuneComplexityPawnEndgame       (1)
#define TuneComplexityUnwinnable        (1)
#define TuneComplexityAdjustment        (1)
#define TuneQuadraticOurs               (1)
#define TuneQuadraticTheirs             (1)

enum { NORMAL, MGONLY, EGONLY };

typedef struct TexelTuple {
    int index;
    int coeff;
} TexelTuple;

typedef struct TexelEntry {
    int ntuples;
    double result;
    double eval, phase;
    double factors[PHASE_NB];
    TexelTuple *tuples;
} TexelEntry;

typedef double TexelVector[NTERMS][PHASE_NB];

void runTexelTuning();
void initTexelEntries(TexelEntry *tes, Board *pos);
void initCoefficients(int coeffs[NTERMS]);
void initCurrentParameters(TexelVector cparams);
void initPhaseManager(TexelVector phases);

void updateMemory(TexelEntry *te, int size);
void updateGradient(TexelEntry *tes, TexelVector gradient, TexelVector params, TexelVector phases, double K, int batch);
void shuffleTexelEntries(TexelEntry *tes);

double computeOptimalK(TexelEntry *tes);
double completeEvaluationError(TexelEntry *tes, double K);
double completeLinearError(TexelEntry *tes, TexelVector params, double K);
double singleLinearError(TexelEntry *te, TexelVector params, double K);
double linearEvaluation(TexelEntry *te, TexelVector params);
double sigmoid(double K, double S);

void printParameters(TexelVector params, TexelVector cparams);
void print_0(char *name, int params[NTERMS][PHASE_NB], int i, char *S);
void print_1(char *name, int params[NTERMS][PHASE_NB], int i, int A, char *S);
void print_2(char *name, int params[NTERMS][PHASE_NB], int i, int A, int B, char *S);
void print_3(char *name, int params[NTERMS][PHASE_NB], int i, int A, int B, int C, char *S);

uint64_t rand64();

// Initalize the Phase Manger for an N dimensional array

#define INIT_PHASE_0(term, P, S) do {                           \
    phases[i  ][MG] = (P == NORMAL || P == MGONLY);             \
    phases[i++][EG] = (P == NORMAL || P == EGONLY);             \
} while (0)

#define INIT_PHASE_1(term, A, P, S) do {                        \
    for (int _a = 0; _a < A; _a++)                              \
       {phases[i  ][MG] = (P == NORMAL || P == MGONLY);         \
        phases[i++][EG] = (P == NORMAL || P == EGONLY);}        \
} while (0)

#define INIT_PHASE_2(term, A, B, P, S) do {                     \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_PHASE_1(term[_b], B, P, S);                        \
} while (0)

#define INIT_PHASE_3(term, A, B, C, P, S) do {                  \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_PHASE_2(term[_c], B, C, P, S);                     \
} while (0)

// Initalize Parameters of an N dimensional array

#define INIT_PARAM_0(term, P, S) do {                           \
     cparams[i  ][MG] = mgScore(term);                          \
     cparams[i++][EG] = egScore(term);                          \
} while (0)

#define INIT_PARAM_1(term, A, P, S) do {                        \
    for (int _a = 0; _a < A; _a++)                              \
       {cparams[i  ][MG] = mgScore(term[_a]);                   \
        cparams[i++][EG] = egScore(term[_a]);}                  \
} while (0)

#define INIT_PARAM_2(term, A, B, P, S) do {                     \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_PARAM_1(term[_b], B, P, S);                        \
} while (0)

#define INIT_PARAM_3(term, A, B, C, P, S) do {                  \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_PARAM_2(term[_c], B, C, P, S);                     \
} while (0)

// Initalize Coefficients from an N dimensional array

#define INIT_COEFF_0(term, P, S) do {                           \
    coeffs[i++] = T.term[WHITE] - T.term[BLACK];                \
} while (0)

#define INIT_COEFF_1(term, A, P, S) do {                        \
    for (int _a = 0; _a < A; _a++)                              \
        coeffs[i++] = T.term[_a][WHITE] - T.term[_a][BLACK];    \
} while (0)

#define INIT_COEFF_2(term, A, B, P, S) do {                     \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_COEFF_1(term[_b], B, P, S);                        \
} while (0)

#define INIT_COEFF_3(term, A, B, C, P, S) do {                  \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_COEFF_2(term[_c], B, C, P, S);                     \
} while (0)

// Print Parameters of an N dimensional array

#define PRINT_0(term, P, S) (print_0(#term, tparams, i, S), i+=1)

#define PRINT_1(term, A, P, S) (print_1(#term, tparams, i, A, S), i+=A)

#define PRINT_2(term, A, B, P, S) (print_2(#term, tparams, i, A, B, S), i+=A*B)

#define PRINT_3(term, A, B, C, P, S) (print_3(#term, tparams, i, A, B, C, S), i+=A*B*C)

// Generic wrapper for all of the above functions

#define ENABLE_0(F, term, P, S) do {                            \
    if (Tune##term) F##_0(term, P, S);                          \
} while (0)

#define ENABLE_1(F, term, A, P, S) do {                         \
    if (Tune##term) F##_1(term, A, P, S);                       \
} while (0)

#define ENABLE_2(F, term, A, B, P, S) do {                      \
    if (Tune##term) F##_2(term, A, B, P, S);                    \
} while (0)

#define ENABLE_3(F, term, A, B, C, P, S) do {                   \
    if (Tune##term) F##_3(term, A, B, C, P, S);                 \
} while (0)

// Configuration for each aspect of the evaluation terms

#define EXECUTE_ON_TERMS(F) do {                                            \
    ENABLE_0(F, PawnValue, NORMAL, "");                                     \
    ENABLE_0(F, KnightValue, NORMAL, "");                                   \
    ENABLE_0(F, BishopValue, NORMAL, "");                                   \
    ENABLE_0(F, RookValue, NORMAL, "");                                     \
    ENABLE_0(F, QueenValue, NORMAL, "");                                    \
    ENABLE_0(F, KingValue, NORMAL, "");                                     \
    ENABLE_1(F, PawnPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_1(F, KnightPSQT32, 32, NORMAL, "[32]");                          \
    ENABLE_1(F, BishopPSQT32, 32, NORMAL, "[32]");                          \
    ENABLE_1(F, RookPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_1(F, QueenPSQT32, 32, NORMAL, "[32]");                           \
    ENABLE_1(F, KingPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_2(F, QuadraticOurs, 6, 6, NORMAL, "[6][6]");                     \
    ENABLE_2(F, QuadraticTheirs, 6, 6, NORMAL, "[6][6]");                   \
    ENABLE_0(F, PawnDoubled, NORMAL, "");                                   \
    ENABLE_1(F, PawnIsolated, 2, NORMAL, "[2]");                            \
    ENABLE_1(F, PawnBackward, 2, NORMAL, "[2]");                            \
    ENABLE_1(F, PawnPassed, 8, NORMAL, "[RANK_NB]");                        \
    ENABLE_1(F, PawnPassedConnected, 8, NORMAL, "[RANK_NB]");               \
    ENABLE_1(F, PawnConnected, 8, NORMAL, "[RANK_NB]");                     \
    ENABLE_0(F, PawnSupport, NORMAL, "");                                   \
    ENABLE_0(F, KP_1, EGONLY, "");                                          \
    ENABLE_0(F, KP_2, EGONLY, "");                                          \
    ENABLE_0(F, KP_3, EGONLY, "");                                          \
    ENABLE_1(F, KnightOutpost, 2, NORMAL, "[2]");                           \
    ENABLE_0(F, KnightTropism, NORMAL, "");                                 \
    ENABLE_0(F, KnightDefender, NORMAL, "");                                \
    ENABLE_1(F, KnightMobility, 9, NORMAL, "[9]");                          \
    ENABLE_1(F, BishopOutpost, 2, NORMAL, "[2]");                           \
    ENABLE_0(F, BishopTropism, NORMAL, "");                                 \
    ENABLE_0(F, BishopRammedPawns, NORMAL, "");                             \
    ENABLE_0(F, BishopDefender, NORMAL, "");                                \
    ENABLE_1(F, BishopMobility, 14, NORMAL, "[14]");                        \
    ENABLE_0(F, RookOpen, NORMAL, "");                                      \
    ENABLE_0(F, RookSemi, NORMAL, "");                                      \
    ENABLE_0(F, RookOnSeventh, NORMAL, "");                                 \
    ENABLE_0(F, RookTropism, NORMAL, "");                                   \
    ENABLE_1(F, RookMobility, 15, NORMAL, "[15]");                          \
    ENABLE_0(F, QueenPreDeveloped, NORMAL, "");                             \
    ENABLE_0(F, QueenTropism, NORMAL, "");                                  \
    ENABLE_1(F, QueenMobility, 28, NORMAL, "[28]");                         \
    ENABLE_0(F, KingPawnLessFlank, NORMAL, "");                             \
    ENABLE_0(F, BlockedStorm, NORMAL, "");                                  \
    ENABLE_2(F, ShelterStrength, 4, 8, NORMAL, "[FILE_NB/2][RANK_NB]");     \
    ENABLE_2(F, UnblockedStorm, 4, 8, NORMAL, "[FILE_NB/2][RANK_NB]");      \
    ENABLE_0(F, ComplexityPassedPawns, EGONLY, "");                         \
    ENABLE_0(F, ComplexityTotalPawns, EGONLY, "");                          \
    ENABLE_0(F, ComplexityOutflanking, EGONLY, "");                         \
    ENABLE_0(F, ComplexityPawnFlanks, EGONLY, "");                          \
    ENABLE_0(F, ComplexityPawnEndgame, EGONLY, "");                         \
    ENABLE_0(F, ComplexityUnwinnable, EGONLY, "");                          \
    ENABLE_0(F, ComplexityAdjustment, EGONLY, "");                          \
} while (0)

#endif