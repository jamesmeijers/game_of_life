/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <stdio.h>
//#define DEBUG
#define POS(x, y, row_mask, col_mask, words_per_row) ((y&row_mask) * words_per_row + (x&col_mask))

//#define CHANGE_TRACKING

#define EAST_BITS_MASK 0x000000000000000F
#define WEST_BITS_MASK 0xF000000000000000

/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/
static uint64_t calculate_word_next_gen(unsigned y, unsigned x, unsigned row_mask, unsigned col_mask, unsigned words_per_row, uint64_t * board);


/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
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
        char * outboard_update_map)
{

  #ifdef CHANGE_TRACKING
  int num_skips = 0;
  int num_calcs = 0;
  #endif

  unsigned i, j, generation, cycles_per_row;

  const unsigned row_mask = nrows - 1;
  const unsigned col_mask = (ncols/16) - 1;

  cycles_per_row = nrows/16;

  for (i = start_row + 1; i < (start_row + rows_per_thread-1); i++)
  {
    for (j = 0; j < cycles_per_row; j++)
    {
      inboard_update_map[POS(j, i, row_mask, col_mask, cycles_per_row)] = 1;
    }
  }



  for(generation = 0; generation < gens_max; generation++)
  {
    
    for (i = start_row + 1; i < (start_row + rows_per_thread-1); i++)
    {
      for (j = 0; j < cycles_per_row; j++)
      {
        //if there's a chance it may have changed
        if( inboard_update_map[POS(j, i, row_mask, col_mask, cycles_per_row)] )
        {
          #ifdef CHANGE_TRACKING
          num_calcs++;
          #endif
          //update it to assume it hasn't changed
          inboard_update_map[POS(j, i, row_mask, col_mask, cycles_per_row)] = 0;
          uint64_t old_val = inboard[POS(j, i, row_mask, col_mask, cycles_per_row)];
          uint64_t new_val = calculate_word_next_gen(i, j, row_mask, col_mask, cycles_per_row, inboard);
          outboard[POS(j, i, row_mask, col_mask, cycles_per_row)] = new_val;
          if(new_val != old_val)
          {
            //self
            outboard_update_map[POS(j, i, row_mask, col_mask, cycles_per_row)] = 1;
            //north
            outboard_update_map[POS(j, (i - 1), row_mask, col_mask, cycles_per_row)] = 1;
            //south
            outboard_update_map[POS(j, (i + 1), row_mask, col_mask, cycles_per_row)] = 1;
            if ((new_val & EAST_BITS_MASK) != (old_val & EAST_BITS_MASK))
            {
              //east
              outboard_update_map[POS((j + 1), i, row_mask, col_mask, cycles_per_row)] = 1;
              //north east
              outboard_update_map[POS((j + 1), (i - 1), row_mask, col_mask, cycles_per_row)] = 1;
              //south east
              outboard_update_map[POS((j + 1), (i + 1), row_mask, col_mask, cycles_per_row)] = 1;

            }
            if ((new_val & WEST_BITS_MASK) != (old_val & WEST_BITS_MASK))
            {
              //west
              outboard_update_map[POS((j - 1), i, row_mask, col_mask, cycles_per_row)] = 1;
              //north west
              outboard_update_map[POS((j - 1), (i - 1), row_mask, col_mask, cycles_per_row)] = 1;
              //south west
              outboard_update_map[POS((j - 1), (i + 1), row_mask, col_mask, cycles_per_row)] = 1;
            }

          }


        }
        else 
        {
          #ifdef CHANGE_TRACKING
          num_skips++;
          #endif
          outboard[POS(j, i, row_mask, col_mask, cycles_per_row)] = inboard[POS(j, i, row_mask, col_mask, cycles_per_row)];
        }
      }

    }

    int tmp_count = *my_count;


    /*must wait for other thread's last gen to perform this threads current gen*/
    while(*above_thread_count < tmp_count)
    {
#ifdef DEBUG
      printf("start: %d, m: %d, a: %d\n", start_row, tmp_count, *above_thread_count);
#endif
    }

    i = start_row;
      for (j = 0; j < cycles_per_row; j++)
      {
          uint64_t old_val = inboard[POS(j, i, row_mask, col_mask, cycles_per_row)];
          uint64_t new_val = calculate_word_next_gen(i, j, row_mask, col_mask, cycles_per_row, inboard);
          outboard[POS(j, i, row_mask, col_mask, cycles_per_row)] = new_val;
          if(new_val != old_val)
          {
            //south
            outboard_update_map[POS(j, (i + 1), row_mask, col_mask, cycles_per_row)] = 1;
            if ((new_val & EAST_BITS_MASK) != (old_val & EAST_BITS_MASK))
            {
              //south east
              outboard_update_map[POS((j + 1), (i + 1), row_mask, col_mask, cycles_per_row)] = 1;

            }
            if ((new_val & WEST_BITS_MASK) != (old_val & WEST_BITS_MASK))
            {
              //south west
              outboard_update_map[POS((j - 1), (i + 1), row_mask, col_mask, cycles_per_row)] = 1;
            }

          }
      }

    while(*below_thread_count < tmp_count);
    i = (start_row + rows_per_thread-1);
      for (j = 0; j < cycles_per_row; j++)
      {
          //update it to assume it hasn't changed
          uint64_t old_val = inboard[POS(j, i, row_mask, col_mask, cycles_per_row)];
          uint64_t new_val = calculate_word_next_gen(i, j, row_mask, col_mask, cycles_per_row, inboard);
          outboard[POS(j, i, row_mask, col_mask, cycles_per_row)] = new_val;
          if(new_val != old_val)
          {
            //north
            outboard_update_map[POS(j, (i - 1), row_mask, col_mask, cycles_per_row)] = 1;
            if ((new_val & EAST_BITS_MASK) != (old_val & EAST_BITS_MASK))
            {
              //north east
              outboard_update_map[POS((j + 1), (i - 1), row_mask, col_mask, cycles_per_row)] = 1;

            }
            if ((new_val & WEST_BITS_MASK) != (old_val & WEST_BITS_MASK))
            {
              //north west
              outboard_update_map[POS((j - 1), (i - 1), row_mask, col_mask, cycles_per_row)] = 1;
            }

          }
      }

    /*update count to tell other threads that I'm done this generation, only this thread can
      write, so no need for locking*/
    *my_count = *my_count + 1;

    uint64_t * tmp = inboard;
    inboard = outboard;
    outboard = tmp;

    char * tmp_2 = inboard_update_map;
    inboard_update_map = outboard_update_map;
    outboard_update_map = tmp_2;
  }

  #ifdef CHANGE_TRACKING
  printf("num_skips: %d, num_calcs: %d\n", num_skips, num_calcs);
  #endif


  return inboard;

}

#define BIG_SHIFT 60
#define SMALL_SHIFT 4
#define VAL_MASK 0xF

#ifdef DEBUG
int first_round = 1;
#endif

static uint64_t calculate_word_next_gen(unsigned y, unsigned x, unsigned row_mask, unsigned col_mask, unsigned words_per_row, uint64_t * board)
{
  uint64_t current, N, NE, E, SE, S, SW, W, NW, sum, ee_3 = 0, ee_4 = 0, final;

  current = board[POS(x, y, row_mask, col_mask, words_per_row)];
  N = board[POS(x, (y - 1), row_mask, col_mask, words_per_row)];
  NE = board[POS((x + 1), (y - 1), row_mask, col_mask, words_per_row)];
  NW = board[POS((x - 1), (y - 1), row_mask, col_mask, words_per_row)];
  E = board[POS((x + 1), y, row_mask, col_mask, words_per_row)];
  SE = board[POS((x + 1), (y + 1), row_mask, col_mask, words_per_row)];
  S = board[POS(x, (y + 1), row_mask, col_mask, words_per_row)];
  SW = board[POS((x - 1), (y + 1), row_mask, col_mask, words_per_row)];
  W = board[POS((x - 1), y, row_mask, col_mask, words_per_row)];

  E = (current << SMALL_SHIFT) | (E >> BIG_SHIFT);
  NE = (N << SMALL_SHIFT) | (NE >> BIG_SHIFT);
  SE = (S << SMALL_SHIFT) | (SE >> BIG_SHIFT);

  W = (current >> SMALL_SHIFT) | (W << BIG_SHIFT);
  NW = (N >> SMALL_SHIFT) | (NW << BIG_SHIFT);
  SW = (S >> SMALL_SHIFT) | (SW << BIG_SHIFT);

  sum = current + N + NE + E + SE + S + SW + W + NW;
  #ifdef DEBUG
  printf("C :%016lx\nN :%016lx\nNE:%016lx\nE :%016lx\nSE:%016lx\nS :%016lx\nSW:%016lx\nW :%016lx\nNW:%016lx\n", current, N, NE, E, SE, S, SW, W, NW);
  printf("SM:%016lx\n", sum);
  #endif

  unsigned i;
  for (i = 0; i < 32; i+=4)
  {
    ee_3 |= ((sum & VAL_MASK) == 3) << i;
    ee_4 |= ((sum & VAL_MASK) == 4) << i;
    sum >>= 4;
  }

/*
There seems to be a compiler bug (or some undiscovered error on my part) preventing
me from shifting a 64 bit value by 32 bits. So, I'm seperating it into two loops


*/

  uint64_t ee_3_b = 0, ee_4_b = 0;

  for (i = 0; i < 32; i+=4)
  {
    ee_3_b |= ((sum & VAL_MASK) == 3) << i;
    ee_4_b |= ((sum & VAL_MASK) == 4) << i;
    sum >>= 4;
  }

  ee_3 |= ee_3_b << 32;
  ee_4 |= ee_4_b << 32;

  final = (current & ee_4) | ee_3;
  #ifdef DEBUG
  first_round = 0;
  printf("E3:%016lx\n", ee_3);
  printf("E4:%016lx\n", ee_4);
  printf("FN:%016lx\n", final);
  #endif
  return final;
  
}


/*convert struct to regular function inputs*/
void *    
game_of_life_MT(void * data)
{
  GOL_MT_DATA * d = (GOL_MT_DATA *) data;
  game_of_life(d->outboard, d->inboard, d->start_row, d->nrows, d->rows_per_thread, d->ncols, d->gens_max, \
    d->above_thread_count, d->below_thread_count, d->my_count, d->inboard_update_map, d->outboard_update_map);
  return NULL;
}



