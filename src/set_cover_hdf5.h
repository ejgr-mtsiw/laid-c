/*
 ============================================================================
 Name        : set_cover_hdf5.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to apply the set cover algorithm
			   using hdf5
 ============================================================================
 */

#ifndef SET_COVER_HDF5_H
#define SET_COVER_HDF5_H

#include "types/cover_t.h"
#include "types/dataset_hdf5_t.h"
#include "types/oknok_t.h"
#include "types/word_t.h"

#include "hdf5.h"

#include <stdint.h>

/**
 * Reads attribute data
 */
oknok_t get_column(const hid_t dataset, const uint32_t attribute,
				   const uint32_t count, word_t* column);

/**
 *
 */
oknok_t update_attribute_totals_add(cover_t* cover,
									dataset_hdf5_t* line_dataset);

oknok_t update_attribute_totals_sub(cover_t* cover,
									dataset_hdf5_t* line_dataset,
									word_t* column);

#endif // SET_COVER_HDF5_H
