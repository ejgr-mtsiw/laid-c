/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

//
//#define DEBUG
//
#include "bit_utils.h"
#include "clargs.h"
#include "cost.h"
#include "dataset.h"
#include "dataset_hdf5.h"
#include "disjoint_matrix.h"
#include "globals.h"
#include "hdf5.h"
#include "jnsq.h"
#include "set_cover.h"
#include <stdio.h>
#include <stdlib.h>

/**
 *
 */
int main(int argc, char **argv) {

	/**
	 * Command line arguments set by the user
	 */
	clargs args;

	// Parse command line arguments
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK) {
		return EXIT_FAILURE;
	}

	/**
	 * The dataset data
	 */
	unsigned long *dataset;

	// Read the dataset into memory
	herr_t status = read_dataset(args.filename, args.datasetname, &dataset);
	if (status != OK) {
		// TODO: Error
		return status;
	}

	fprintf(stdout, " - classes = %d \n", g_n_classes);
	fprintf(stdout, " - observations = %lu \n", g_n_observations);
	fprintf(stdout, " - attributes = %lu \n", g_n_attributes);

	DEBUG_PRINT_DATASET(stdout, "Initial data", dataset, WITH_EXTRA_BITS)

	// Sort dataset
	qsort(dataset, g_n_observations, g_n_longs * sizeof(unsigned long),
			compare_lines);

	DEBUG_PRINT_DATASET(stdout, "Sorted data", dataset, WITH_EXTRA_BITS)

	// remove duplicates
	unsigned long old_n = g_n_observations;
	fprintf(stdout, "Removing duplicates:\n");
	remove_duplicates(dataset);
	fprintf(stdout, " - %lu duplicate(s) removed\n", old_n - g_n_observations);

	DEBUG_PRINT_DATASET(stdout, "Unique observations", dataset, WITH_EXTRA_BITS)

	// FIll class arrays
	fprintf(stdout, "Checking classes:\n");

	/**
	 * Array that stores the number of observations for each class
	 */
	unsigned long *n_items_per_class = (unsigned long*) calloc(g_n_classes,
			sizeof(unsigned long));
	if (n_items_per_class == NULL) {
		fprintf(stderr, "Error allocating n_items_per_class\n");
		return EXIT_FAILURE;
	}

	/**
	 * Matrix that stores the list of observations per class
	 */
	// WHATIF: should we replace the static matrix by pointers so each
	// class has n items? Right now we waste at least half the matrix space
	// WHATIF: If we reduce the number of possible columns and lines to
	// 2^32-1 we could use half the memory by storing the line indexes
	unsigned long **observations_per_class = (unsigned long**) calloc(
			g_n_classes * g_n_observations, sizeof(unsigned long*));

	fill_class_arrays(dataset, n_items_per_class, observations_per_class);

	for (unsigned int i = 0; i < g_n_classes; i++) {
		fprintf(stdout, " - class %d: %lu item(s)\n", i, n_items_per_class[i]);
	}

	// Set JNSQ
	fprintf(stdout, "Setting up JNSQ attributes:\n");
	unsigned int max_jnsq = add_jnsqs(dataset);
	fprintf(stdout, " - Max JNSQ: %d\n", max_jnsq);

	// Update number of attributes to include the new JNSQs
	if (max_jnsq > 0) {
		// How many bits are needed for jnsq attributes
		unsigned int n_bits_for_jnsq = ceil(log2(max_jnsq + 1));

		g_n_attributes += n_bits_for_jnsq;

		fprintf(stdout, " - Added %d columns for JNSQ!\n", n_bits_for_jnsq);
	}

	DEBUG_PRINT_DATASET(stdout, "Data + jnsq\n", dataset, WITHOUT_EXTRA_BITS)

	// Do LAD

	// Build disjoint matrix and store it in the hdf5 file
	if (create_disjoint_matrix(args.filename, DISJOINT_MATRIX_DATASET_NAME,
			n_items_per_class, observations_per_class) != OK) {
		return EXIT_FAILURE;
	}

	free(dataset);

	calculate_solution(args.filename, DISJOINT_MATRIX_DATASET_NAME,
			n_items_per_class);

	fprintf(stdout, "All done!\n");

	free(n_items_per_class);
	free(observations_per_class);

	return EXIT_SUCCESS;
}
