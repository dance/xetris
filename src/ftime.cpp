// VC++ Win32 C specific code
// compile: cl ftimevc.cpp user32.lib gdi32.lib
// or use or IDE

#include <windows.h> // has qpf and qpc
#include "ftime.h"

__int64 startclock;  // we want a start time
double rate_inv;

BOOL initftime() {
  __int64 rate;

  // we need the accuracy
  if(!QueryPerformanceFrequency((LARGE_INTEGER*)&rate)) {
    return FALSE; // win errors
  }

  // usually the rate will be 1193180
  if(!rate) {
    return FALSE;
  }

  rate_inv=1.0/(double)rate;

  if(!QueryPerformanceCounter((LARGE_INTEGER*)&startclock)) {
    return FALSE; // win errors
  }

  return TRUE; // there is a clock
}
// you would have to start up with initftime() at the beginning
// of the game. And check for errors
 
double ftime() {
  // by dividing by its rate you get accurate seconds

  __int64 endclock;

  QueryPerformanceCounter((LARGE_INTEGER*)&endclock);

  return (double)(endclock-startclock)*rate_inv;

  // note: I recommend that you multiply but the inverse of a constant.
  // (speed reasons)
}
