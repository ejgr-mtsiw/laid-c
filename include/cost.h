/*
 ============================================================================
 Name        : cost.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage costs
 ============================================================================
 */

#ifndef COST_H
#define COST_H

#include "bit_utils.h"
#include "globals.h"
#include "hdf5.h"
#include "malloc.h"

/**
 * Max number of columns to print
 * Will choose half from the start and half from the end
 */
#define MAX_COST_COLUMNS_TO_SHOW 20

#ifdef DEBUG
#define DEBUG_PRINT_COST(stream, title, cost) print_cost(stream, title, cost);
#else
#define DEBUG_PRINT_COST(stream, title, cost)
#endif

/**
 * Prints the cost array to the stream
 */
void print_cost(FILE *strem, const char *title, const unsigned long *cost);

/**
 * Calculates the cost of the full (virtual) disjoint matrix
 */
int calculate_cost(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const unsigned long n_lines,
		const unsigned char *line_blacklist,
		const unsigned char *attribute_blacklist, unsigned long *cost);

#endif
