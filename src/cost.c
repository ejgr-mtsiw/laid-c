/*
 ============================================================================
 Name        : cost.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage costs
 ============================================================================
 */

#include "cost.h"
#include "dataset.h"

/**
 * Prints the cost array to the stream
 */
void print_cost(FILE *stream, const char *title, const unsigned long *cost) {

	fprintf(stream, "%s: ", title);

	for (unsigned long i = 0; i < g_n_attributes; i++) {
		fprintf(stream, "%lu ", cost[i]);

		if (g_n_attributes > MAX_COST_COLUMNS_TO_SHOW
				&& i == MAX_COST_COLUMNS_TO_SHOW / 2) {
			//skip
			fprintf(stream, "... ");
			i = g_n_attributes - MAX_COST_COLUMNS_TO_SHOW / 2;
		}
	}
	fprintf(stream, "\n");
}

/**
 * Calculates the cost of the full (virtual) disjoint matrix
 */
int calculate_cost(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const unsigned long n_lines,
		const unsigned char *line_blacklist,
		const unsigned char *attribute_blacklist, unsigned long *cost) {

	// Alocate buffer
	unsigned long *buffer = (unsigned long*) malloc(
			sizeof(unsigned long) * g_n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, g_n_longs };

	// Calculate cost
	for (unsigned long i = 0; i < n_lines; i++) {

		// If this line is blacklisted: skip!
		if (line_blacklist[i]) {
			continue;
		}

		// Update offset
		offset[0] = i;

		// Select hyperslab on file dataset
		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
		NULL, count, NULL);

		// Read line to dataset
		H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id,
		H5P_DEFAULT, buffer);

		// Current attribute
		unsigned long c = 0;

		for (unsigned int n = 0; n < g_n_longs && c < g_n_attributes; n++) {

			for (int bit = LONG_BITS - 1; c < g_n_attributes && bit >= 0;
					bit--) {

				if (attribute_blacklist[c] == 0
						&& (buffer[n] & AND_MASK_TABLE[bit])) {
					// Disagreement
					// Add to cost
					cost[c]++;
				}
				c++;
			}
		}

		if ((i % (n_lines / 10)) == 0) {
			fprintf(stdout, "... %07lu\n", i);
		}
	}

	free(buffer);

	return OK;
}
