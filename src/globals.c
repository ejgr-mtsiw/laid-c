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
unsigned long g_n_attributes = 0;

/**
 * Number of observations
 */
unsigned long g_n_observations = 0;

/**
 * Number of classes
 */
unsigned int g_n_classes = 0;

/**
 * Number of bits used to store the class
 */
unsigned int g_n_bits_for_class = 0;

/**
 * Number of longs needed to store one line of the dataset
 */
unsigned int g_n_longs = 0;
