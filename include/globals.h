/*
 ============================================================================
 Name        : globals.h
 Author      : Eduardo Ribeiro
 Description : Global variables definitions
 ============================================================================
 */

#ifndef GLOBALS_H__
#define GLOBALS_H__

//#define DEBUG 1

#define NOK 0
#define OK 1

//#ifdef DEBUG
#include <time.h>
#define SETUP_TIMING struct timespec tick, tock;
#define TICK clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
#define TOCK(stream) clock_gettime(CLOCK_MONOTONIC_RAW, &tock); fprintf(stream, "[%0.3fs]\n", (tock.tv_nsec - tick.tv_nsec) / 1000000000.0F + (tock.tv_sec  - tick.tv_sec));
//#else
//#define SETUP_TIMING
//#define TICK
//#define TOCK
//#endif

#endif
