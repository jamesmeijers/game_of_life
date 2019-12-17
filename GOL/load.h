#ifndef _load_h 
#define _load_h

#include <stdio.h>
#include <stdint.h>
/**
 * From the file stream "input", load the dimensions of the board
 * (storing them in *nrows and *ncols, respectively) and then return a
 * malloc'd pointer to the board (which is stored as a 1-D
 * column-oriented array).
 */
uint64_t*
load_board (FILE* input, int* nrows, int* ncols);

/**
 * Return a malloc'd pointer to a blank board of dimension nrows by
 * ncols.  The board is initialized to contain nonsense values, which
 * may be useful for debugging.
 */
uint64_t*
make_board (const int nrows, const int ncols);





#endif /* _load_h */
