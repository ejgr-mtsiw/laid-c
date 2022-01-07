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
 * Calculates and adds the current lines cost to the cost array
 */
void increase_cost_line(const unsigned long *a, const unsigned long *b,
		unsigned long *cost) {

	// Current attribute
	unsigned long c = 0;

	unsigned long xored = 0;

	for (unsigned int n = 0; n < g_n_longs && c < g_n_attributes; n++) {
		xored = a[n] ^ b[n];
		for (int bit = LONG_BITS - 1; c < g_n_attributes && bit >= 0; bit--) {
			if (xored & AND_MASK_TABLE[bit]) {
				// Disagreement
				// Add to cost
				cost[c]++;
			}
			c++;
		}
	}
}

/**
 * Calculates the cost of the full (virtual) disjoint matrix
 */
void calculate_initial_cost(unsigned long **observations_per_class,
		const unsigned long *n_items_per_class, unsigned long *cost) {

	unsigned int i, j;
	unsigned long n_i, n_j;

	for (i = 0; i < g_n_classes - 1; i++) {
		for (j = i + 1; j < g_n_classes; j++) {

			for (n_i = i * g_n_observations;
					n_i < i * g_n_observations + n_items_per_class[i]; n_i++) {

				for (n_j = j * g_n_observations;
						n_j < j * g_n_observations + n_items_per_class[j];
						n_j++) {

					increase_cost_line(observations_per_class[n_i],
							observations_per_class[n_j], cost);
				}
			}
		}
	}
}

/**
 * Calculates the cost of the attributes that aren't blacklisted (removed)
 * and subtracts the cost from the cost matrix
 */
void decrease_cost_line(const unsigned long *a, const unsigned long *b,
		const unsigned char *blacklist, unsigned long *cost) {

	// Current attribute
	unsigned long c = 0;

	// Current bit
	int bit = 0;

	unsigned long xored = 0;

	for (unsigned int n = 0; n < g_n_longs && c < g_n_attributes; n++) {

		xored = a[n] ^ b[n];

		for (bit = LONG_BITS - 1; c < g_n_attributes && bit >= 0; bit--) {

			if ((blacklist[c] == 0) && (xored & AND_MASK_TABLE[bit])) {
				// Disagreement
				// Remove from cost
				cost[c]--;
			}
			c++;
		}
	}
}

/**
 *  We don't want to build the full disjoint matrix
 *  So we check again which line combinations contributed to the
 *  blacklisted attribute and remove their contribution from the cost array
 */
void decrease_cost_blacklisted_attribute(unsigned long **observations_per_class,
		const unsigned long *n_items_per_class,
		const unsigned long attribute_to_blacklist,
		const unsigned char *blacklist, unsigned long *cost) {

	unsigned int i, j;
	unsigned long n_i, n_j;

	unsigned int n = attribute_to_blacklist / LONG_BITS;
	unsigned int r = LONG_BITS - 1 - attribute_to_blacklist % LONG_BITS;
	unsigned long xored = 0;

	for (i = 0; i < g_n_classes - 1; i++) {
		for (j = i + 1; j < g_n_classes; j++) {

			for (n_i = i * g_n_observations;
					n_i < i * g_n_observations + n_items_per_class[i]; n_i++) {

				for (n_j = j * g_n_observations;
						n_j < j * g_n_observations + n_items_per_class[j];
						n_j++) {

					xored = observations_per_class[n_i][n]
							^ observations_per_class[n_j][n];

					if (xored & AND_MASK_TABLE[r]) {
						// These lines contributed to the blacklisted attribute
						// Remove them from the cost array
						// TODO: Bug here, we need to ignore all the
						// blacklisted attributes, not just the current one
						// because there is overlap

						/*
						 printf("%d[%lu]x%d[%lu] = ", i, n_i % n_observations,
						 j, n_j % n_observations);
						 print_line(stdout, &xored, WITHOUT_EXTRA_BITS);
						 printf("\n");
						 */

						decrease_cost_line(observations_per_class[n_i],
								observations_per_class[n_j], blacklist, cost);
					}
				}
			}
		}
	}
}
