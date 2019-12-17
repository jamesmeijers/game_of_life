#ifndef _life_h
#define _life_h
#include <stdint.h>
/**
 * Given the initial board state in inboard and the board dimensions
 * nrows by ncols, evolve the board state gens_max times by alternating
 * ("ping-ponging") between outboard and inboard.  Return a pointer to 
 * the final board; that pointer will be equal either to outboard or to
 * inboard (but you don't know which).
 */
uint64_t *
game_of_life (uint64_t * outboard, 
	      uint64_t * inboard,
        const int start_row,
	      const int nrows,
        const int rows_per_thread,
	      const int ncols,
	      const int gens_max,
        volatile int * above_thread_count,
        volatile int * below_thread_count,
        volatile int * my_count,
        char * inboard_update_map,
        char * outboard_update_map);


typedef struct 
{
        uint64_t * outboard;
        uint64_t * inboard;
        int start_row;
        int nrows;
        int rows_per_thread;
        int ncols;
        int gens_max;
        int * above_thread_count;
        int * below_thread_count;
        int * my_count;
        char * inboard_update_map;
        char * outboard_update_map;
} GOL_MT_DATA;


void * 
game_of_life_MT(void * data);


/**
 * Same output as game_of_life() above, except this is not
 * parallelized.  Useful for checking output.
 */
char*
sequential_game_of_life (char* outboard, 
			 char* inboard,
			 const int nrows,
			 const int ncols,
			 const int gens_max);


#endif /* _life_h */
