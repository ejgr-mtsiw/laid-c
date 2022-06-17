/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

//
#include "bit_utils.h"
#include "clargs.h"
#include "dataset.h"
#include "dataset_hdf5.h"
#include "disjoint_matrix.h"
#include "globals.h"
#include "hdf5.h"
#include "jnsq.h"
#include "set_cover.h"
#include "sort_r.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 *
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

	/**
	 * The dataset
	 */
	dataset_t dataset;

	/**
	 * READ AND SETUP DATASET
	 */

	//Open the data file
	hid_t file_id = H5Fopen(args.filename, H5F_ACC_RDWR, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", args.filename);
		return EXIT_FAILURE;
	}

	hid_t dataset_id = H5Dopen2(file_id, args.datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", args.datasetname);

		// Free resources
		H5Fclose(file_id);
		return EXIT_FAILURE;
	}

	// Fill dataset attributes
	if (read_attributes(dataset_id, &dataset) != DATASET_OK) {
		// Error reading attributes
		return EXIT_FAILURE;
	}

	fprintf(stdout, " - classes = %d \n", dataset.n_classes);
	fprintf(stdout, " - observations = %d \n", dataset.n_observations);
	fprintf(stdout, " - attributes = %d \n", dataset.n_attributes);

	// Allocate main buffer
	// https://vorpus.org/blog/why-does-calloc-exist/
	/**
	 * The dataset data
	 */
	dataset.data = (unsigned long*) calloc(
			dataset.n_observations * dataset.n_longs, sizeof(unsigned long));
	if (dataset.data == NULL) {
		fprintf(stderr, "Error allocating dataset\n");

		// Free resources
		H5Dclose(dataset_id);
		H5Fclose(file_id);
		return EXIT_FAILURE;
	}

	// Fill dataset from hdf5 file
	herr_t status = H5Dread(dataset_id, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL,
	H5P_DEFAULT, dataset.data);
	if (status < 0) {
		fprintf(stderr, "Error reading the dataset data\n");

		// Free resources
		free(dataset.data);
		H5Dclose(dataset_id);
		H5Fclose(file_id);
		return EXIT_FAILURE;
	}

	DEBUG_PRINT_DATASET(stdout, "Initial data", dataset.data,
			PRINT_WITH_EXTRA_BITS)

	// Sort dataset
	sort_r(dataset.data, dataset.n_observations,
			dataset.n_longs * sizeof(unsigned long), compare_lines_extra,
			&dataset.n_longs);

//	qsort(dataset.data, dataset.n_observations,
//			dataset.n_longs * sizeof(unsigned long), compare_lines);

	DEBUG_PRINT_DATASET(stdout, "Sorted data", dataset.data,
			PRINT_WITH_EXTRA_BITS)

	// remove duplicates
	fprintf(stdout, "Removing duplicates:\n");
	unsigned int duplicates = remove_duplicates(&dataset);

	fprintf(stdout, " - %d duplicate(s) removed\n", duplicates);

	DEBUG_PRINT_DATASET(stdout, "Unique observations", dataset.data,
			PRINT_WITH_EXTRA_BITS)

	// Fill class arrays
	fprintf(stdout, "Checking classes:\n");

	/**
	 * Array that stores the number of observations for each class
	 */
	dataset.n_observations_per_class = (unsigned int*) calloc(dataset.n_classes,
			sizeof(unsigned int));
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
			dataset.n_classes * dataset.n_observations, sizeof(unsigned long*));
	if (dataset.observations_per_class == NULL) {
		fprintf(stderr, "Error allocating observations_per_class\n");
		return EXIT_FAILURE;
	}

	fill_class_arrays(&dataset);

	for (unsigned int i = 0; i < dataset.n_classes; i++) {
		fprintf(stdout, " - class %du: %du item(s)\n", i,
				dataset.n_observations_per_class[i]);
	}

	// Set JNSQ
	fprintf(stdout, "Setting up JNSQ attributes:\n");

	unsigned int max_jnsq = add_jnsqs(&dataset);
	fprintf(stdout, " - Max JNSQ: %du\n", max_jnsq);

	unsigned long matrix_lines = calculate_number_of_lines_of_disjoint_matrix(
			&dataset);

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
	 * From this point forward we no longer need the data in the dataset
	 */
	free(dataset.data);
	free(dataset.n_observations_per_class);
	free(dataset.observations_per_class);

	cover_t cover;
	cover.dataset = &dataset;
	cover.matrix_n_lines = matrix_lines;

	calculate_solution(args.filename, DISJOINT_MATRIX_DATASET_NAME, &cover);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
