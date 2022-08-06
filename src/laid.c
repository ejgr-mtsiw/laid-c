/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

#include <dataset_hdf5.h>
#include "dataset.h"
#include "disjoint_matrix.h"
#include "jnsq.h"
#include "set_cover.h"
#include "utils/timing.h"
#include "utils/clargs.h"
#include "utils/sort_r.h"

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
int main(int argc, char** argv)
{
	SETUP_TIMING

	// Timing for the full operation
	struct timespec main_tick, main_tock;
	clock_gettime(CLOCK_MONOTONIC_RAW, &main_tick);

	/**
	 * Command line arguments set by the user
	 */
	clargs_t args;

	// Parse command line arguments
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK)
	{
		return EXIT_FAILURE;
	}

	// We can jump straight to the set covering algorithm
	// if we already have the matrix in the hdf5 dataset
	if (!is_matrix_created(args.filename))
	{
		TICK

			/**
			 * The dataset
			 */
			dataset_t dataset;
		init_dataset(&dataset);

		/**
		 * READ DATASET
		 */
		if (hdf5_read_dataset(args.filename, args.datasetname, &dataset) != OK)
		{
			// Error reading attributes
			return EXIT_FAILURE;
		}

		print_dataset_details(stdout, &dataset);

		fprintf(stdout, "Finished loading dataset ");
		TOCK(stdout)
		TICK

			// Sort dataset
			// We need to know the number of longs in each line of the dataset
			// so we can't use the standard qsort implementation
			sort_r(dataset.data, dataset.n_observations,
				   dataset.n_words * sizeof(word_t), compare_lines_extra,
				   &dataset.n_words);

		fprintf(stdout, "\nSorted dataset ");
		TOCK(stdout)
		TICK

			// Remove duplicates
			fprintf(stdout, "\nRemoving duplicates:\n");
		uint32_t duplicates = remove_duplicates(&dataset);
		fprintf(stdout, " - %d duplicate(s) removed ", duplicates);
		TOCK(stdout)
		TICK

			// Fill class arrays
			fprintf(stdout, "\nChecking classes:\n");

		if (fill_class_arrays(&dataset) != OK)
		{
			free_dataset(&dataset);
			return EXIT_FAILURE;
		}

		for (uint32_t i = 0; i < dataset.n_classes; i++)
		{
			fprintf(stdout, " - class %d: %d item(s)\n", i,
					dataset.n_observations_per_class[i]);
		}

		TOCK(stdout)
		TICK

			// Set JNSQ
			fprintf(stdout, "\nSetting up JNSQ attributes:\n");
		unsigned int max_jnsq = add_jnsqs(&dataset);
		fprintf(stdout, " - Max JNSQ: %d [%d bits] ", max_jnsq,
				dataset.n_bits_for_jnsqs);
		TOCK(stdout)
		TICK

			unsigned long matrix_lines
			= calculate_number_of_lines_of_disjoint_matrix(&dataset);

		double matrixsize
			= (matrix_lines * (dataset.n_attributes + dataset.n_bits_for_class))
			/ (1024 * 1024 * 8);

		fprintf(stdout, "\nBuilding disjoint matrix.\n");
		fprintf(stdout, "Estimated disjoint matrix size: %lu lines [%0.2fMB]\n",
				matrix_lines, matrixsize);

		// Build disjoint matrix and store it in the hdf5 file
		if (create_disjoint_matrix(args.filename, &dataset) != OK)
		{
			return EXIT_FAILURE;
		}

		fprintf(stdout, "Finished building disjoint matrix ");
		TOCK(stdout)

		/**
		 * From this point forward we no longer need the dataset
		 */
		free_dataset(&dataset);
	}

	cover_t cover;
	init_cover(&cover);

	TICK

		fprintf(stdout, "\nApplying set covering algorithm.\n");
	if (calculate_solution(args.filename, &cover) != OK)
	{
		return EXIT_FAILURE;
	}

	print_solution(stdout, &cover);

	free_cover(&cover);
	TOCK(stdout)

	fprintf(stdout, "All done! ");

	clock_gettime(CLOCK_MONOTONIC_RAW, &main_tock);
	fprintf(stdout, "[%0.3fs]\n",
			(main_tock.tv_nsec - main_tick.tv_nsec) / 1000000000.0
				+ (main_tock.tv_sec - main_tick.tv_sec));

	return EXIT_SUCCESS;
}
