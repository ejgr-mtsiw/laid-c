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

	hid_t file_id, line_dataset_id, column_dataset_id;

	herr_t status = 1;

	// OPEN LINE DATASET
	ret = hdf5_open_dataset(filename, DM_DATASET_LINE_DATA, &file_id,
			&line_dataset_id);
	if (ret != OK) {
		return ret;
	}

	// READ ATTRIBUTES
	// Number of attributes
	uint32_t n_attributes = 0;
	hdf5_read_attribute(line_dataset_id, HDF5_N_ATTRIBUTES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_attributes);

	// Number of matrix lines
	uint32_t n_matrix_lines = 0;
	hdf5_read_attribute(line_dataset_id, HDF5_N_MATRIX_LINES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_matrix_lines);

	uint32_t n_line_words = (uint32_t) (n_attributes / WORD_BITS)
			+ (n_attributes % WORD_BITS != 0);

	// Setup line dataspace
	hid_t line_dataspace_id = H5Dget_space(line_dataset_id);

	// Setup line memspace
	hsize_t line_mem_dimensions[1] = { n_line_words };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t line_memspace_id = H5Screate_simple(1, line_mem_dimensions, NULL);
	if (line_memspace_id < 0) {
		fprintf(stderr, "Error creating lines memory space\n");
		ret = NOK;
		goto out_close_line_dataset;
	}

	// OPEN COLUMN DATASET
	ret = hdf5_open_dataset(filename, DM_DATASET_COLUMN_DATA, &file_id,
			&column_dataset_id);
	if (ret != OK) {
		goto out_close_line_dataset;
	}

	uint32_t n_column_words = (uint32_t) (n_matrix_lines / WORD_BITS)
			+ (n_matrix_lines % WORD_BITS != 0);

	// Setup column dataspace
	hid_t column_dataspace_id = H5Dget_space(column_dataset_id);

	// Setup column memspace
	hsize_t column_mem_dimensions[1] = { n_column_words };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t column_memspace_id = H5Screate_simple(1, column_mem_dimensions, NULL);
	if (column_memspace_id < 0) {
		fprintf(stderr, "Error creating columns memory space\n");
		ret = NOK;
		goto out_close_column_dataset;
	}

	// OPEN LINE TOTALS DATASET
	hid_t line_totals_dataset_id;
	ret = hdf5_open_dataset(filename, DM_DATASET_LINE_TOTALS, &file_id,
			&line_totals_dataset_id);
	if (ret != OK) {
		goto out_close_column_dataset;
	}

	// READ LINE TOTALS
	uint32_t *line_totals = (uint32_t*) malloc(
			sizeof(uint32_t) * n_matrix_lines);
	if (line_totals == NULL) {
		fprintf(stderr, "Error allocating line totals array.\n");
		ret = NOK;
		goto out_close_column_dataset;
	}

	status = H5Dread(line_totals_dataset_id, H5T_NATIVE_UINT, H5S_ALL,
	H5S_ALL, H5P_DEFAULT, line_totals);

	H5Dclose(line_totals_dataset_id);

	if (status < 0) {
		ret = NOK;
		goto out_close_column_dataset;
	}

	// SETUP UNCOVERED_LINE
	word_t *uncovered_lines = (word_t*) malloc(sizeof(word_t) * n_column_words);
	if (uncovered_lines == NULL) {
		fprintf(stderr, "Error allocating uncovered lines array.\n");
		ret = NOK;
		goto out_close_column_dataset;
	}

	// Fill with ones: they are all uncovered so far
	for (uint32_t i = 0; i < n_column_words; i++) {
		uncovered_lines[i] = ~0UL;
	}

	// How many lines are there on the last word?
	uint32_t remaining_lines = n_matrix_lines % WORD_BITS;

	// Clear last bits
	word_t mask = ~0UL;
	mask >>= remaining_lines;
	mask = ~mask;
	uncovered_lines[n_column_words - 1] &= mask;

	// SETUP SELECTED_ATTRIBUTES
	uint8_t *selected_attributes = (uint8_t*) calloc(n_attributes,
			sizeof(uint8_t));
	if (selected_attributes == NULL) {
		fprintf(stderr, "Error allocating selected attributes array.\n");
		ret = NOK;
		goto out_close_column_dataset;
	}

	// Setup cover
	cover->n_attributes = n_attributes;
	cover->n_matrix_lines = n_matrix_lines;
	cover->line_totals = line_totals;
	cover->selected_attributes = selected_attributes;
	cover->uncovered_lines = uncovered_lines;

	word_t *best_line = (word_t*) malloc(sizeof(word_t) * n_line_words);
	word_t *attribute_line = (word_t*) malloc(sizeof(word_t) * n_column_words);

	uint32_t n_attributes_in_solution = 0;
	uint32_t *solution = NULL;

	do {
		// READ BEST LINE
		// Get best line index
		int64_t best_line_index = get_best_line_index(cover);
		if (best_line_index == -1) {
			// All lines are covered
			ret = OK;
			goto out_check_duplicates;
		}

		ret = hdf5_read_line(line_dataset_id, line_dataspace_id,
				line_memspace_id, best_line_index, n_line_words, best_line);
		if (ret != OK) {
			goto out_free_buffers;
		}

		// FIND BEST ATTRIBUTE

		uint32_t at_total = 0;
		uint32_t max_at_total = 0;
		uint32_t selected_at = 0;
		word_t attribute = 0;
		uint32_t c_at = 0;

		// Find next attribute set
		for (uint32_t i = 0; i < n_line_words; i++) {
			attribute = best_line[i];
			at_total = 0;

			for (int8_t bit = WORD_BITS - 1; bit >= 0 && c_at < n_attributes;
					bit--, c_at++) {
				if (selected_attributes[c_at] == NOT_BLACKLISTED
						&& !!(AND_MASK_TABLE[bit] & attribute)) {

					// Calculate total for this attribute
					// Read attribute line from dataset
					hdf5_read_line(column_dataset_id, column_dataspace_id,
							column_memspace_id, c_at, n_column_words,
							attribute_line);

					for (uint32_t w = 0; w < n_column_words; w++) {
						at_total += __builtin_popcountl(
								attribute_line[w] & uncovered_lines[w]);
					}

					if (at_total > max_at_total) {
						max_at_total = at_total;
						selected_at = c_at;
					}
				}
			}
		}

		n_attributes_in_solution++;

		// UPDATE COVERED_LINE
		hdf5_read_line(column_dataset_id, column_dataspace_id,
				column_memspace_id, selected_at, n_column_words,
				attribute_line);

		for (uint32_t w = 0; w < n_column_words; w++) {
			uncovered_lines[w] &= ~attribute_line[w];
		}

		selected_attributes[selected_at] = BLACKLISTED;
	} while (true);

out_check_duplicates:
	/**
	 * Check for duplicated attributes
	 */
	solution = (uint32_t*) malloc(sizeof(uint32_t) * n_attributes_in_solution);

	uint32_t current_attribute = 0;

	do {

		// Update solution
		uint32_t *c_s = solution;

		for (uint32_t i = 0; i < n_attributes; i++) {
			if (selected_attributes[i] == BLACKLISTED) {
				*c_s = i;
				c_s++;
			}
		}

		// Reset uncovered lines
		for (uint32_t i = 0; i < n_column_words; i++) {
			uncovered_lines[i] = 0UL;
		}

		uint32_t remaining_bits = n_matrix_lines % WORD_BITS;
		word_t mask = ~0UL;
		mask >>= remaining_bits;
		uncovered_lines[n_column_words - 1] |= mask;

		// Fazer | de todos menos este
		for (uint32_t j = 0; j < n_attributes_in_solution; j++) {
			if (current_attribute == j) {
				continue;
			}

			hdf5_read_line(column_dataset_id, column_dataspace_id,
					column_memspace_id, solution[j], n_column_words,
					attribute_line);

			for (uint32_t w = 0; w < n_column_words; w++) {
				uncovered_lines[w] |= attribute_line[w];
			}
		}

		hdf5_read_line(column_dataset_id, column_dataspace_id,
				column_memspace_id, solution[current_attribute], n_column_words,
				attribute_line);

		// Verificar se era preciso
		bool needed = false;
		for (uint32_t w = 0; w < n_column_words; w++) {
			if (uncovered_lines[w] != ~0UL) {
				needed = true;
				break;
			}
		}

		if (needed) {
			current_attribute++;
		} else {
			fprintf(stdout, "Attribute %d was NOT needed!\n", current_attribute);
			selected_attributes[solution[current_attribute]] = NOT_BLACKLISTED;
			n_attributes_in_solution--;
			current_attribute = 0;
		}
	} while (current_attribute < n_attributes_in_solution);

out_free_buffers:
	free(best_line);
	free(attribute_line);

	// CLOSE DATASETS
out_close_column_dataset:
	H5Dclose(column_dataset_id);
	H5Sclose(column_dataspace_id);
	H5Sclose(column_memspace_id);

out_close_line_dataset:
	H5Dclose(line_dataset_id);
	H5Sclose(line_dataspace_id);
	H5Sclose(line_memspace_id);

	H5Fclose(file_id);

	return ret;
}

oknok_t hdf5_read_line(const hid_t dataset_id, const hid_t dataspace_id,
		const hid_t memspace_id, uint32_t index, const uint32_t n_words,
		word_t *line) {

	//Setup offset
	hsize_t offset[2] = { index, 0 };
	//Setup count
	hsize_t count[2] = { 1, n_words };

	// Select hyperslab on file dataset
	H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset, NULL, count,
	NULL);

	// Read line from dataset
	herr_t ret = H5Dread(dataset_id, H5T_NATIVE_ULONG, memspace_id,
			dataspace_id, H5P_DEFAULT, line);
	if (ret < 0) {
		fprintf(stderr, "[hdf5_read_line] Error reading from dataset!\n");
		return NOK;
	}

	return OK;
}

int64_t get_best_line_index(const cover_t *cover) {

	uint32_t min_line_total = 0xffffffff; // No uncovered line can have 0 as total
	int64_t min_line_index = -1;

	uint32_t n_matrix_lines = cover->n_matrix_lines;
	word_t *uncovered_lines = cover->uncovered_lines;
	uint32_t *line_totals = cover->line_totals;

	uint32_t n_words = (uint32_t) (n_matrix_lines / WORD_BITS)
			+ (n_matrix_lines % WORD_BITS != 0);

	uint32_t current_line = 0;

	for (uint32_t w = 0; w < n_words; w++) {
		for (int8_t bit = WORD_BITS - 1;
				bit >= 0 && current_line < n_matrix_lines;
				bit--, current_line++) {
			if (!!(AND_MASK_TABLE[bit] & uncovered_lines[w])) {
				if (line_totals[current_line] < min_line_total) {
					min_line_total = line_totals[current_line];
					min_line_index = current_line;
				}
			}
		}
	}

	return min_line_index;
}

//herr_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
//		const hid_t memory_space_id, const cover_t *cover,
//		const uint32_t attribute_to_blacklist) {
//
//	SETUP_TIMING
//	TICK
//
//	// Attribute to blacklist is in long n
//	uint32_t n = attribute_to_blacklist / WORD_BITS;
//
//	// Attribute to blacklist is at
//	uint8_t bit = WORD_BITS - 1 - attribute_to_blacklist % WORD_BITS;
//
//	// Number of longs in a line
//	uint32_t n_words = cover->n_words;
//
//	// Number of attributes
//	uint32_t n_attributes = cover->n_attributes;
//
//	// Calculate number of lines of disjoint matrix
//	uint32_t n_lines = cover->matrix_n_lines;
//
//	uint8_t *line_blacklist = cover->line_blacklist;
//
//	uint32_t *sum = cover->sum;
//
//	// Alocate buffer
//	word_t *buffer = (word_t*) malloc(sizeof(word_t) * n_words);
//
//	// We will read one line at a time
//	hsize_t offset[2] = { 0, 0 };
//	hsize_t count[2] = { 1, n_words };
//
//	// Current long
//	word_t c_long = 0;
//
//	// Current attribute
//	uint32_t c_attribute = 0;
//
//	const uint32_t n_lines_to_blacklist = sum[attribute_to_blacklist];
//	uint32_t next_output = 0;
//
//	for (uint32_t i = 0; i < n_lines; i++) {
//
//		if (line_blacklist[i] != BLACKLISTED) {
//
//			// Update offset
//			offset[0] = i;
//
//			// Select hyperslab on file dataset
//			H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
//			NULL, count, NULL);
//
//			// Read line to dataset
//			H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id,
//					dataset_space_id,
//					H5P_DEFAULT, buffer);
//
//			if (buffer[n] & AND_MASK_TABLE[bit]) {
//
//				// The bit is set: Blacklist this line
//				line_blacklist[i] = BLACKLISTED;
//
//				// Update sum removing the contribution from this line
//				for (uint32_t l = 0; l < n_words; l++) {
//
//					c_long = buffer[l];
//					c_attribute = l * WORD_BITS;
//
//					for (int bit = WORD_BITS - 1;
//							c_attribute < n_attributes && bit >= 0;
//							bit--, c_attribute++) {
//
//						// Update sum
//						sum[c_attribute] -= !!(c_long & AND_MASK_TABLE[bit]);
//					}
//				}
//			}
//		}
//
//		if (i > next_output) {
//			fprintf(stdout, "  - Blacklisting lines %0.0f%%.\r",
//					((double) i) / n_lines * 100);
//			fflush( stdout);
//
//			next_output += n_lines / 10;
//		}
//	}
//
//	fprintf(stdout, "  - Blacklisted %d lines ", n_lines_to_blacklist);
//	TOCK(stdout)
//
//	free(buffer);
//
//	return 0;
//}
//
///**
// * Finds the next attribute to blacklist
// */
//void calculate_initial_sum(const hid_t dataset_id, const hid_t dataset_space_id,
//		const hid_t memory_space_id, const cover_t *cover) {
//
//	SETUP_TIMING
//	TICK
//
//	uint32_t n_words = cover->n_words;
//
//	unsigned long n_lines = cover->matrix_n_lines;
//
//	uint32_t n_attributes = cover->n_attributes;
//
//	uint32_t *sum = cover->sum;
//
//	// Alocate buffer
//	word_t *buffer = (word_t*) malloc(sizeof(word_t) * n_words);
//
//	// We will read one line at a time
//	hsize_t offset[2] = { 0, 0 };
//	hsize_t count[2] = { 1, n_words };
//
//	// Current long
//	word_t c_long = 0;
//
//	// Current attribute
//	uint32_t c_attribute = 0;
//
//	uint32_t next_output = 0;
//
//	// Calculate totals
//	for (uint32_t i = 0; i < n_lines; i++) {
//
//		// Update offset
//		offset[0] = i;
//
//		// Select hyperslab on file dataset
//		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
//		NULL, count, NULL);
//
//		// Read line to dataset
//		H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id,
//		H5P_DEFAULT, buffer);
//
//		for (uint32_t l = 0; l < n_words; l++) {
//			c_long = buffer[l];
//			c_attribute = l * WORD_BITS;
//
//			for (int bit = WORD_BITS - 1;
//					bit >= 0 && c_attribute < n_attributes;
//					bit--, c_attribute++) {
//
//				// Add to sum
//				sum[c_attribute] += !!(c_long & AND_MASK_TABLE[bit]);
//			}
//		}
//
//		if (i > next_output) {
//			fprintf(stdout, " - Computing initial sum %0.0f%%.\r",
//					((double) i) / n_lines * 100);
//			fflush( stdout);
//
//			next_output += n_lines / 10;
//		}
//
//	}
//
//	fprintf(stdout, " - Calculated initial sum ");
//	TOCK(stdout)
//
//	free(buffer);
//}

void print_solution(FILE *stream, cover_t *cover) {

	fprintf(stream, "\nSolution: { ");
	for (uint32_t i = 0; i < cover->n_attributes; i++) {
		if (cover->selected_attributes[i] == BLACKLISTED) {
			// This attribute is set so it's part of the solution
			fprintf(stream, "%d ", i);
		}
	}
	fprintf(stream, "}\n");
}

void free_cover(cover_t *cover) {

	free(cover->selected_attributes);
	free(cover->uncovered_lines);
	free(cover->line_totals);

	cover->selected_attributes = NULL;
	cover->uncovered_lines = NULL;
	cover->line_totals = NULL;

	cover->n_matrix_lines = 0;
	cover->n_attributes = 0;
}
