/*
 ============================================================================
 Name        : set_cover.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to apply the set cover algorithm
 ============================================================================
 */

#include "set_cover.h"

oknok_t calculate_solution(const char *filename, cover_t *cover) {

	oknok_t ret = OK;
	herr_t status = 1;

	word_t *line = NULL, *column = NULL;

	// OPEN LINE DATASET
	hdf5_dataset_t line_dataset;
	ret = hdf5_open_dataset(&line_dataset, filename, DM_DATASET_LINE_DATA);
	if (ret != OK) {
		return ret;
	}

	// OPEN COLUMN DATASET
	hdf5_dataset_t column_dataset;
	ret = hdf5_open_dataset(&column_dataset, filename, DM_DATASET_COLUMN_DATA);
	if (ret != OK) {
		goto out_close_free_line_dataset;
	}

	// READ ATTRIBUTES
	// Number of attributes
	uint32_t n_attributes = 0;
	hdf5_read_attribute(line_dataset.dataset_id, HDF5_N_ATTRIBUTES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_attributes);

	// Number of matrix lines
	uint32_t n_matrix_lines = 0;
	hdf5_read_attribute(line_dataset.dataset_id, HDF5_N_MATRIX_LINES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_matrix_lines);

	uint32_t n_words_in_a_line = (uint32_t) (n_attributes / WORD_BITS)
			+ (n_attributes % WORD_BITS != 0);

	uint32_t n_words_in_a_column = (uint32_t) (n_matrix_lines / WORD_BITS)
			+ (n_matrix_lines % WORD_BITS != 0);

	// SETUP COVERED LINES
	// Fill with zeroes: they are all uncovered so far
	word_t *covered_lines = (word_t*) calloc(n_words_in_a_column,
			sizeof(word_t));
	if (covered_lines == NULL) {
		fprintf(stderr, "Error allocating covered lines array.\n");
		ret = NOK;
		goto out_close_free_column_dataset;
	}

	// SETUP SELECTED ATTRIBUTES
	word_t *selected_attributes = (word_t*) calloc(n_words_in_a_line,
			sizeof(word_t));
	if (selected_attributes == NULL) {
		fprintf(stderr, "Error allocating selected attributes array.\n");
		ret = NOK;
		goto out_close_free_column_dataset;
	}

	// LOAD INITIAL ATTRIBUTE TOTALS
	hdf5_dataset_t attribute_totals_dataset;
	ret = hdf5_open_dataset(&attribute_totals_dataset, filename,
	DM_DATASET_ATTRIBUTE_TOTALS);
	if (ret != OK) {
		goto out_close_free_selected_attributes;
	}

	// READ ATTRIBUTE TOTALS
	//uint32_t *attribute_totals = (uint32_t*) malloc(sizeof(uint32_t) * n_attributes);
	uint32_t *attribute_totals = (uint32_t*) calloc(
			n_words_in_a_line * WORD_BITS, sizeof(uint32_t));
	if (attribute_totals == NULL) {
		fprintf(stderr, "Error allocating attribute totals array.\n");
		ret = NOK;
		close_hdf5_dataset(&attribute_totals_dataset);
		goto out_close_free_selected_attributes;
	}

	status = H5Dread(attribute_totals_dataset.dataset_id, H5T_NATIVE_UINT,
	H5S_ALL, H5S_ALL, H5P_DEFAULT, attribute_totals);
	close_hdf5_dataset(&attribute_totals_dataset);
	if (status < 0) {
		ret = NOK;
		goto out_close_free_selected_attributes;
	}

	// Setup cover
	cover->n_attributes = n_attributes;
	cover->n_matrix_lines = n_matrix_lines;
	cover->n_words_in_a_line = n_words_in_a_line;
	cover->n_words_in_a_column = n_words_in_a_column;
	cover->covered_lines = covered_lines;
	cover->selected_attributes = selected_attributes;
	cover->attribute_totals = attribute_totals;

	line = (word_t*) malloc(sizeof(word_t) * n_words_in_a_line);
	if (line == NULL) {
		fprintf(stderr, "Error allocating line buffer.\n");
		ret = NOK;
		goto out_close_free_selected_attributes;
	}

	column = (word_t*) malloc(sizeof(word_t) * n_words_in_a_column);
	if (column == NULL) {
		fprintf(stderr, "Error allocating column buffer array.\n");
		ret = NOK;
		goto out_close_free_line_buffer;
	}

	uint32_t round = 1;
	int64_t best_attribute = 0;

	SETUP_TIMING

	while (true) {

		TICK

		best_attribute = get_best_attribute_index(cover);
		if (best_attribute == -1) {
			// No more attributes available, we're done here
			ret = OK;
			goto out_free_buffers;
		}

		fprintf(stdout, "\n - Selecting attribute round #%d\n", round++);

		// Mark best attribute as selected
		mark_attribute_as_selected(cover, best_attribute);
		fprintf(stdout, "  - Selected attribute #%lu ", best_attribute);

		// Update attributes totals

		// Read best attribute column
		ret = hdf5_read_line(&column_dataset, best_attribute,
				n_words_in_a_column, column);
		if (ret != OK) {
			// Error out
		}

		// Update attributes totals
		uint32_t current_line = 0;
		for (uint32_t w = 0; w < n_words_in_a_column; w++) {
			word_t lines = column[w];

			// Ignore lines already covered
			lines &= ~covered_lines[w];

			// Check the remaining lines
			for (int8_t bit = WORD_BITS - 1;
					bit >= 0 && current_line < n_matrix_lines;
					bit--, current_line++) {

				if (lines & AND_MASK_TABLE[bit]) {
					// This line is covered

					// Read line from dataset
					ret = hdf5_read_line(&line_dataset, current_line,
							n_words_in_a_line, line);
					if (ret != OK) {
						// Error out
						goto out_free_buffers;
					}

					// Decrement totals
					remove_line_contribution(cover, line);

				}
			}
		}

		// Mark lines covered by the best attribute
		update_covered_lines(cover, column);

		TOCK(stdout)
	}

out_close_free_selected_attributes:
	free(selected_attributes);
	selected_attributes = NULL;

out_free_buffers:
	free(column);

out_close_free_line_buffer:
	free(line);

out_close_free_column_dataset:
	close_hdf5_dataset(&column_dataset);

out_close_free_line_dataset:
	close_hdf5_dataset(&line_dataset);

	return ret;
}

int64_t get_best_attribute_index(cover_t *cover) {

	uint32_t max_total = 0;
	int64_t max_attribute = -1;

	for (uint32_t i = 0; i < cover->n_attributes; i++) {
		if (cover->attribute_totals[i] > max_total) {
			max_total = cover->attribute_totals[i];
			max_attribute = i;
		}
	}

	return max_attribute;
}

oknok_t remove_line_contribution(cover_t *cover, const word_t *line) {

	uint32_t current_attribute = 0;

	for (uint32_t w = 0; w < cover->n_words_in_a_line; w++) {
		for (int8_t bit = WORD_BITS - 1; bit >= 0; bit--, current_attribute++) {
			cover->attribute_totals[current_attribute] -= BIT_CHECK(line[w],
					bit);
		}
	}

	return OK;
}

oknok_t update_covered_lines(cover_t *cover, word_t *column) {

	for (uint32_t w = 0; w < cover->n_words_in_a_column; w++) {
		BITMASK_SET(cover->covered_lines[w], column[w]);
	}

	return OK;
}

oknok_t mark_attribute_as_selected(cover_t *cover, int64_t attribute) {

	uint32_t attribute_word = attribute / WORD_BITS;
	uint8_t attribute_bit = WORD_BITS - (attribute % WORD_BITS) - 1;

	BIT_SET(cover->selected_attributes[attribute_word], attribute_bit);

	return OK;
}

void print_solution(FILE *stream, cover_t *cover) {

	fprintf(stream, "\nSolution: { ");

	uint32_t current_attribute = 0;

	for (uint32_t w = 0; w < cover->n_words_in_a_line; w++) {
		for (int8_t bit = WORD_BITS - 1;
				bit >= 0 && current_attribute < cover->n_attributes;
				bit--, current_attribute++) {
			if (cover->selected_attributes[w] & AND_MASK_TABLE[bit]) {
				// This attribute is set so it's part of the solution
				fprintf(stream, "%d ", current_attribute);
			}
		}
	}
	fprintf(stream, "}\n");
}

void free_cover(cover_t *cover) {

	cover->n_attributes = 0;
	cover->n_matrix_lines = 0;
	cover->n_words_in_a_line = 0;
	cover->n_words_in_a_column = 0;

	free(cover->covered_lines);
	free(cover->selected_attributes);
	free(cover->attribute_totals);

	cover->covered_lines = NULL;
	cover->selected_attributes = NULL;
	cover->attribute_totals = NULL;
}
