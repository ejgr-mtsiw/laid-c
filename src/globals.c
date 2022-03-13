/*
 ============================================================================
 Name        : globals.c
 Author      : Eduardo Ribeiro
 Description : Global variables initialization
 ============================================================================
 */

#include "globals.h"

/**
 * Dataset dimensions
 */
hsize_t g_dimensions[2] = { 0, 0 };

/**
 * Number of attributes
 */
uint_fast32_t g_n_attributes = 0;

/**
 * Number of observations
 */
uint_fast32_t g_n_observations = 0;

/**
 * Number of classes
 */
uint_fast8_t g_n_classes = 0;

/**
 * Number of bits used to store the class
 */
uint_fast8_t g_n_bits_for_class = 0;

/**
 * Number of longs needed to store one line of the dataset
 */
uint_fast32_t g_n_longs = 0;
