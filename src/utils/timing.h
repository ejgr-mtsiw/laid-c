/*
 ============================================================================
 Name        : utils/timing.h
 Author      : Eduardo Ribeiro
 Description : Timing helpers
 ============================================================================
 */

#ifndef UTILS_TIMING_H__
#define UTILS_TIMING_H__

#include <stdio.h>
#include <time.h>

#define SETUP_TIMING time_t tick = 0, tock = 0;

#define TICK tick = time(0);

#define TOCK                                                                   \
	tock = time(0);                                                            \
	fprintf(stdout, "[%lds]\n", tock - tick);

#endif // UTILS_TIMING_H__
