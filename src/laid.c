/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

//
#include "clargs.h"
#include "dataset.h"
#include "disjoint_matrix.h"
#include "globals.h"
#include "jnsq.h"
#include "set_cover.h"
#include "sort_r.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Reads dataset attributes from hdf5 file
 * Read dataset
 * Sort dataset
 * Remove duplicates
 * Add jnsqs
 * Write disjoint matrix
 * Apply set covering algorithm
 * Show solution
 */
int main(int argc, char **argv) {

	/**
	 * Command line arguments set by the user
	 */
	clargs_t args;

	// Parse command line arguments
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK) {
		return EXIT_FAILURE;
	}

	// We can jump straight to the set covering algorithm
	// if we already have the matrix in the hdf5 dataset
	if (!is_matrix_created(args.filename)) {

		/**
		 * The dataset
		 */
		dataset_t dataset;

		/**
		 * READ DATASET
		 */
		if (hdf5_read_dataset(args.filename, args.datasetname,
				&dataset) != DATASET_OK) {
			// Error reading attributes
			return EXIT_FAILURE;
		}

		fprintf(stdout, "Dataset %s\n", args.datasetname);
		fprintf(stdout, " - classes = %d ", dataset.n_classes);
		fprintf(stdout, "[%d bits]\n", dataset.n_bits_for_class);
		fprintf(stdout, " - attributes = %d \n", dataset.n_attributes);
		fprintf(stdout, " - observations = %d \n", dataset.n_observations);

		// Sort dataset
		// We need to know the number of longs in each line of the dataset so
		// we can't use the standard qsort implementation
		sort_r(dataset.data, dataset.n_observations,
				dataset.n_longs * sizeof(unsigned long), compare_lines_extra,
				&dataset.n_longs);

		// remove duplicates
		fprintf(stdout, "Removing duplicates:\n");
		unsigned int duplicates = remove_duplicates(&dataset);

		fprintf(stdout, " - %d duplicate(s) removed\n", duplicates);

		// Fill class arrays
		fprintf(stdout, "Checking classes:\n");

		/**
		 * Array that stores the number of observations for each class
		 */
		dataset.n_observations_per_class = (unsigned int*) calloc(
				dataset.n_classes, sizeof(unsigned int));
		if (dataset.n_observations_per_class == NULL) {
			fprintf(stderr, "Error allocating n_observations_per_class\n");
			return EXIT_FAILURE;
		}

		/**
		 * Matrix that stores the list of observations per class
		 */
		// WHATIF: should we replace the static matrix by pointers so each
		// class has n items? Right now we waste at least half the matrix space
		// WHATIF: If we reduce the number of possible columns and lines to
		// 2^32-1 we could use half the memory by storing the line indexes
		dataset.observations_per_class = (unsigned long**) calloc(
				dataset.n_classes * dataset.n_observations,
				sizeof(unsigned long*));
		if (dataset.observations_per_class == NULL) {
			fprintf(stderr, "Error allocating observations_per_class\n");
			return EXIT_FAILURE;
		}

		fill_class_arrays(&dataset);

		for (unsigned int i = 0; i < dataset.n_classes; i++) {
			fprintf(stdout, " - class %d: %d item(s)\n", i,
					dataset.n_observations_per_class[i]);
		}

		// Set JNSQ
		fprintf(stdout, "Setting up JNSQ attributes:\n");

		unsigned int max_jnsq = add_jnsqs(&dataset);
		fprintf(stdout, " - Max JNSQ: %d [%d bits]\n", max_jnsq,
				dataset.n_bits_for_jnsqs);

		unsigned long matrix_lines =
				calculate_number_of_lines_of_disjoint_matrix(&dataset);

		double matrixsize = (matrix_lines
				* (dataset.n_attributes + dataset.n_bits_for_class))
				/ (1024 * 1024 * 8);

		fprintf(stdout, "Estimated disjoint matrix size: %lu, [%0.2fMB]\n",
				matrix_lines, matrixsize);

		fflush(stdout);

		// Do LAD

		// Build disjoint matrix and store it in the hdf5 file
		if (create_disjoint_matrix(args.filename, &dataset) != OK) {
			return EXIT_FAILURE;
		}

		/**
		 * From this point forward we no longer need the dataset
		 */
		free(dataset.data);
		free(dataset.n_observations_per_class);
		free(dataset.observations_per_class);
	}

	calculate_solution(args.filename, DISJOINT_MATRIX_DATASET_NAME);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
