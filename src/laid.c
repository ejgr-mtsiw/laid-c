/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

#include "clargs.h"
#include "dataset.h"
#include "dataset_lines.h"
#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"

/**
 *
 */
int main(int argc, char **argv) {

	/**
	 * Command line arguments set by the user
	 */
	clargs args;

	/**
	 * File identifier
	 */
	hid_t file_id = 0;

	/**
	 * Dataset identifier
	 */
	hid_t dataset_id = 0;

	/**
	 * The dataset
	 */
	dataset dataset;

	/**
	 * Structure to store dataset lines info
	 */
	dataset_lines dataset_lines;

	// Parse command line arguments
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK) {
		return EXIT_FAILURE;
	}

	//Open the data file
	file_id = H5Fopen(args.filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", args.filename);
		return -1;
	}
	fprintf(stdout, " - Dataset file opened.\n");

	dataset_id = H5Dopen2(file_id, args.datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", args.datasetname);
		return -1;
	}
	fprintf(stdout, " - Dataset opened.\n");

	setup_dataset(dataset_id, &dataset);

	setup_dataset_lines(&dataset_lines, dataset.n_observations);

	build_dataset(dataset_id, &dataset, &dataset_lines);

	// Build classes lists

	// Do LAD

	// Profit!

	free(dataset.data);
	free(dataset_lines.lines);

	H5Dclose(dataset_id);
	H5Fclose(file_id);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
