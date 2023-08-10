/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

#include "dataset.h"
#include "dataset_hdf5.h"
#include "disjoint_matrix.h"
#include "jnsq.h"
#include "set_cover.h"
#include "set_cover_hdf5.h"
#include "types/cover_t.h"
#include "types/dataset_hdf5_t.h"
#include "types/dataset_t.h"
#include "types/dm_t.h"
#include "types/steps_t.h"
#include "types/word_t.h"
#include "utils/bit.h"
#include "utils/clargs.h"
#include "utils/sort_r.h"
#include "utils/timing.h"

#include "hdf5.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

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
	/**
	 * Command line arguments set by the user
	 */
	clargs_t args;

	/**
	 * Parse command line arguments
	 */
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK)
	{
		return EXIT_FAILURE;
	}

	/**
	 * Timing for the full operation
	 */
	SETUP_TIMING_GLOBAL;

	/**
	 * Local timing structures
	 */
	SETUP_TIMING;

	/**
	 * The dataset
	 */
	dataset_t dataset;
	init_dataset(&dataset);

	/**
	 * The HDF5 dataset file
	 */
	dataset_hdf5_t hdf5_dset;

	/**
	 * The disjoint matrix info
	 */
	dm_t dm;

	// Open dataset file
	printf("Using dataset '%s'\n", args.filename);

	if (hdf5_open_dataset(args.filename, args.datasetname, &hdf5_dset) == NOK)
	{
		return EXIT_FAILURE;
	}

	/*We can jump straight to the set covering algorithm
	  if we already have the matrix in the hdf5 dataset*/
	uint8_t skip_dm_creation
		= hdf5_dataset_exists(hdf5_dset.file_id, DM_LINE_DATA);

	if (skip_dm_creation)
	{
		// We don't have to build the disjoint matrix!
		printf("Disjoint matrix dataset found.\n\n");

		goto apply_set_cover;
	}

	// We have to build the disjoint matrices.
	printf("Disjoint matrix dataset not found.\n\n");

	printf("Reading dataset: ");
	TICK;

	// Setup dataset

	dataset.n_observations = hdf5_dset.dimensions[0];
	dataset.n_words		   = hdf5_dset.dimensions[1];

	// Allocate memory for dataset data
	dataset.data = (word_t*) malloc(dataset.n_words * dataset.n_observations
									* sizeof(word_t));

	// Load dataset attributes
	hdf5_read_dataset_attributes(hdf5_dset.dataset_id, &dataset);

	// Load dataset data
	hdf5_read_dataset_data(hdf5_dset.dataset_id, dataset.data);

	TOCK;

	// Print dataset details
	printf("  Classes = %d", dataset.n_classes);
	printf(" [%d bits]\n", dataset.n_bits_for_class);
	printf("  Attributes = %d \n", dataset.n_attributes);
	printf("  Observations = %d \n", dataset.n_observations);

	// Sort dataset
	printf("Sorting dataset: ");
	TICK;

	/*
	  We need to know the number of longs in each line of the dataset
	  so we can't use the standard qsort implementation
	 */
	sort_r(dataset.data, dataset.n_observations,
		   dataset.n_words * sizeof(word_t), compare_lines_extra,
		   &dataset.n_words);

	TOCK;

	// Remove duplicates
	printf("Removing duplicates: ");
	TICK;

	unsigned int duplicates = remove_duplicates(&dataset);

	TOCK;
	printf("  %d duplicate(s) removed\n", duplicates);

	// Fill class arrays
	printf("Checking classes: ");
	TICK;

	/**
	 * Array that stores the number of observations for each class
	 */
	dataset.n_observations_per_class
		= (uint32_t*) calloc(dataset.n_classes, sizeof(uint32_t));
	assert(dataset.n_observations_per_class != NULL);

	/**
	 * Matrix that stores the list of observations per class
	 */
	dataset.observations_per_class = (word_t**) calloc(
		dataset.n_classes * dataset.n_observations, sizeof(word_t*));
	assert(dataset.observations_per_class != NULL);

	if (fill_class_arrays(&dataset) != OK)
	{
		return EXIT_FAILURE;
	}

	TOCK;

	for (unsigned int i = 0; i < dataset.n_classes; i++)
	{
		printf("  Class %d: ", i);
		printf("%d item(s)\n", dataset.n_observations_per_class[i]);
	}

	// Set JNSQ
	printf("Setting up JNSQ attributes: ");
	TICK;

	uint32_t max_inconsistency = add_jnsqs(&dataset);

	// Update number of bits needed for jnsqs
	if (max_inconsistency > 0)
	{
		// How many bits are needed for jnsq attributes
		uint8_t n_bits_for_jnsq = ceil(log2(max_inconsistency + 1));

		dataset.n_bits_for_jnsqs = n_bits_for_jnsq;
	}

	TOCK;
	printf("  Max JNSQ: %d", max_inconsistency);
	printf(" [%d bits]\n", dataset.n_bits_for_jnsqs);

	// JNSQ attributes are treated just like all the other attributes from
	// this point forward
	dataset.n_attributes += dataset.n_bits_for_jnsqs;

	// TODO: CONFIRM FIX: n_words may have changed?
	// If we have 5 classes (3 bits) and only one bit is in the last word
	// If we only use 2 bits for jnsqs then we need one less n_words
	// This could impact all the calculations that assume n_words - 1
	// is the last word!
	dataset.n_words = dataset.n_attributes / WORD_BITS
		+ (dataset.n_attributes % WORD_BITS != 0);

	// End setup dataset

	// Calculate disjoint matrix lines
	printf("Building disjoint matrix: ");
	TICK;

	// Calculate the number of disjoint matrix lines
	dm.n_matrix_lines = get_dm_n_lines(&dataset);

	dm.steps = (steps_t*) malloc(dm.n_matrix_lines * sizeof(steps_t));
	generate_steps(&dataset, &dm);

	TOCK;

	printf("  Number of lines in the disjoint matrix: %d\n", dm.n_matrix_lines);

	double matrix_size = ((double) dm.n_matrix_lines * dataset.n_attributes)
		/ (1024.0 * 1024 * 1024 * 8);
	printf("  Estimated disjoint matrix size: %3.2fGB (x2)\n", matrix_size);

	TICK;

	// Build the disjoint matrix and store it in the hdf5 file
	create_line_dataset(&hdf5_dset, &dataset, &dm);

	printf("  Line dataset done: ");
	TOCK;

	TICK;

	create_column_dataset(&hdf5_dset, &dataset, &dm);

	printf("  Column dataset done: ");
	TOCK;

	/*
	  We no longer need the original dataset
	 */
	free_dataset(&dataset);

	free(dm.steps);
	dm.steps = NULL;

apply_set_cover:

	printf("Applying set covering algorithm:\n");
	TICK;

	// We no longer need to keep the original dataset open
	H5Dclose(hdf5_dset.dataset_id);

	/**
	 *  - Setup line covered array -> 0
	 *  - Setup attributes totals -> 0
	 *  - Read the global attributes totals
	 *loop:
	 *  - Select the best attribute and blacklists it
	 *  - Black list the lines covered by this attribute
	 *  - Update atributes totals
	 *  - if there are still lines to blacklist:
	 *   - Goto loop
	 *  - else:
	 *   - Show solution
	 */

	cover_t cover;
	init_cover(&cover);

	// Open the line dataset
	hid_t d_id = H5Dopen(hdf5_dset.file_id, DM_LINE_DATA, H5P_DEFAULT);
	assert(d_id != NOK);

	dataset_hdf5_t line_dset_id;
	line_dset_id.file_id	= hdf5_dset.file_id;
	line_dset_id.dataset_id = d_id;
	hdf5_get_dataset_dimensions(d_id, line_dset_id.dimensions);

	// Open the column dataset
	d_id = H5Dopen(hdf5_dset.file_id, DM_COLUMN_DATA, H5P_DEFAULT);
	assert(d_id != NOK);

	dataset_hdf5_t column_dset_id;
	column_dset_id.file_id	  = hdf5_dset.file_id;
	column_dset_id.dataset_id = d_id;
	hdf5_get_dataset_dimensions(d_id, column_dset_id.dimensions);

	/**
	 * If we skipped the matriz generation, dataset and dm are empty.
	 * So we need to read the attributes from the dataset
	 */
	hdf5_read_attribute(line_dset_id.dataset_id, N_MATRIX_LINES_ATTR,
						H5T_NATIVE_UINT32, &cover.n_matrix_lines);
	hdf5_read_attribute(line_dset_id.dataset_id, N_ATTRIBUTES_ATTR,
						H5T_NATIVE_UINT32, &cover.n_attributes);

	cover.n_words_in_a_line = line_dset_id.dimensions[1];

	cover.n_words_in_a_column = cover.n_matrix_lines / WORD_BITS
		+ (cover.n_matrix_lines % WORD_BITS != 0);

	// The covered lines
	cover.covered_lines
		= (word_t*) calloc(cover.n_words_in_a_column, sizeof(word_t));

	// The sum for the attributes
	cover.attribute_totals = (uint32_t*) calloc(
		cover.n_words_in_a_line * WORD_BITS, sizeof(uint32_t));

	/**
	 * Column data to process
	 */
	word_t* column
		= (word_t*) calloc(cover.n_words_in_a_column, sizeof(word_t));

	/**
	 * Number of uncovered lines.
	 */
	// uint32_t n_uncovered_lines = 0;

	cover.selected_attributes
		= (word_t*) calloc(cover.n_words_in_a_line, sizeof(word_t));

	read_initial_attribute_totals(hdf5_dset.file_id, cover.attribute_totals);

	// No line is covered so far
	cover.n_uncovered_lines = cover.n_matrix_lines;

	while (true)
	{
		int64_t best_attribute = 0;

		best_attribute = get_best_attribute_index(cover.attribute_totals,
												  cover.n_attributes);

		printf("  Selected attribute #%ld, ", best_attribute);
		printf("covers %d lines ", cover.attribute_totals[best_attribute]);
		TOCK;
		TICK;

		mark_attribute_as_selected(&cover, best_attribute);

		// Update number of lines remaining
		cover.n_uncovered_lines -= cover.attribute_totals[best_attribute];

		// If we covered all of them, we can leave.
		if (cover.n_uncovered_lines == 0)
		{
			break;
		}

		/**
		 * If this attribute covers more lines than what remains
		 * to be covered we calculate the sum of the remaining lines.
		 * If the attribute covers only a few lines we remove the
		 * contribution of the covered lines.
		 * The objetive is to reduce the number of lines read
		 * from the dataset.
		 */
		oknok_t sum_uncovered_lines = NOK;
		if (cover.attribute_totals[best_attribute] > cover.n_uncovered_lines)
		{
			// We'll check the uncovered lines
			sum_uncovered_lines = OK;
		}

		// Read the column data for the best attribute
		get_column(column_dset_id.dataset_id, best_attribute,
				   cover.n_words_in_a_column, column);

		if (sum_uncovered_lines == OK)
		{
			// Update covered lines array
			update_covered_lines(&cover, column);

			// Calculate the totals for all the attributes
			// for the remaining uncovered lines
			update_attribute_totals_add(&cover, &line_dset_id);
		}
		else
		{
			// Remove contribution from newly covered lines
			update_attribute_totals_sub(&cover, &line_dset_id, column);

			// Update covered lines array
			update_covered_lines(&cover, column);
		}
	}

	print_solution(stdout, &cover);
	printf("All done! ");

	PRINT_TIMING_GLOBAL;

	free(column);
	column = NULL;
	free_cover(&cover);

	// Close dataset files
	H5Dclose(line_dset_id.dataset_id);
	H5Dclose(column_dset_id.dataset_id);
	H5Fclose(hdf5_dset.file_id);

	return EXIT_SUCCESS;
}
