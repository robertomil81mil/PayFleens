// misc.c

#include "stdio.h"
#include "defs.h"
#include "unistd.h"
#include <math.h>
#include "float.h"

#ifdef WIN32
#include "windows.h"
#else
#include "sys/time.h"
#include "sys/select.h"
#include "unistd.h"
#include "string.h"
#endif

int GetTimeMs() {
#ifdef WIN32
  return GetTickCount();
#else
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec*1000 + t.tv_usec/1000;
#endif
}

// http://home.arcor.de/dreamlike/chess/
int InputWaiting()
{
#ifndef WIN32
  fd_set readfds;
  struct timeval tv;
  FD_ZERO (&readfds);
  FD_SET (fileno(stdin), &readfds);
  tv.tv_sec=0; tv.tv_usec=0;
  select(16, &readfds, 0, 0, &tv);

  return (FD_ISSET(fileno(stdin), &readfds));
#else
   static int init = 0, pipe;
   static HANDLE inh;
   DWORD dw;

   if (!init) {
     init = 1;
     inh = GetStdHandle(STD_INPUT_HANDLE);
     pipe = !GetConsoleMode(inh, &dw);
     if (!pipe) {
        SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
        FlushConsoleInputBuffer(inh);
      }
    }
    if (pipe) {
      if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
      return dw;
    } else {
      GetNumberOfConsoleInputEvents(inh, &dw);
      return dw <= 1 ? 0 : dw;
	}
#endif
}

void ReadInput(S_SEARCHINFO *info) {
  int             bytes;
  char            input[256] = "", *endc;

    if (InputWaiting()) {
		info->stopped = TRUE;
		do {
		  bytes=read(fileno(stdin),input,256);
		} while (bytes<0);
		endc = strchr(input,'\n');
		if (endc) *endc=0;

		if (strlen(input) > 0) {
			if (!strncmp(input, "quit", 4))    {
			  info->quit = TRUE;
			}
		}
		return;
    }
}

const int MoveHorizon   = 50;
const double MaxRatio   = 7.3;
const double StealRatio = 0.34;

double move_importance(int ply) {

    const double XScale = 6.85;
    const double XShift = 64.5;
    const double Skew   = 0.171;

    return pow((1 + exp((ply - XShift) / XScale)), -Skew) + DBL_MIN;
}

int remaining(int T, int myTime, int movesToGo, int ply, int slowMover) {

    const double TMaxRatio   = (T == OptimumTime ? 1.0 : MaxRatio);
    const double TStealRatio = (T == OptimumTime ? 0.0 : StealRatio);

    double moveImportance = (move_importance(ply) * slowMover) / 100.0;
    double otherMovesImportance = 0.0;

    for (int i = 1; i < movesToGo; ++i)
        otherMovesImportance += move_importance(ply + 2 * i);

    double ratio1 = (TMaxRatio * moveImportance) / (TMaxRatio * moveImportance + otherMovesImportance);
    double ratio2 = (moveImportance + TStealRatio * otherMovesImportance) / (moveImportance + otherMovesImportance);

    return myTime * MIN(ratio1, ratio2);
}

void TimeManagementInit(S_SEARCHINFO *info, int myTime, int increment, int ply, int movestogo) {

    int minThinkingTime = 20;
    int moveOverhead    = 1000;
    int slowMover       = 64;
    int hypMyTime;

    info->optimumTime = info->maximumTime = MAX(myTime, minThinkingTime);

    const int maxMTG = movestogo ? MIN(movestogo, MoveHorizon) : MoveHorizon;
    //printf("maxMTG %d\n",maxMTG );

    for (int hypMTG = 1; hypMTG <= maxMTG; ++hypMTG) {
        hypMyTime =  myTime
        + increment * (hypMTG - 1)
        - moveOverhead * (2 + MIN(hypMTG, 40));

        hypMyTime = MAX(hypMyTime, 0);

        int t1 = minThinkingTime + remaining(OptimumTime, hypMyTime, hypMTG, ply, slowMover);
        int t2 = minThinkingTime + remaining(MaxTime    , hypMyTime, hypMTG, ply, slowMover);
        
        info->optimumTime = MIN(t1, info->optimumTime);
        info->maximumTime = MIN(t2, info->maximumTime);
    }
}
