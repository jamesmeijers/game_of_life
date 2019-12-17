#include "load.h"
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

uint64_t *
make_board (const int nrows, const int ncols)
{
  uint64_t* board = NULL;

  /* Allocate the board and fill in with 'Z' (instead of a number, so
     that it's easy to diagnose bugs */
  //4 bits per element => 16 elements per 64 bits
  board = malloc ( nrows * ncols * sizeof(uint64_t)/16);
  assert (board != NULL);

  return board;
}

static void
load_dimensions (FILE* input, int* nrows, int* ncols)
{
  int ngotten = 0;
  
  ngotten = fscanf (input, "P1\n%d %d\n", nrows, ncols);
  if (ngotten < 1)
    {
      fprintf (stderr, "*** Failed to read 'P1' and board dimensions ***\n");
      fclose (input);
      exit (EXIT_FAILURE);
    }
  if (*nrows < 1)
    {
      fprintf (stderr, "*** Number of rows %d must be positive! ***\n", *nrows);
      fclose (input);
      exit (EXIT_FAILURE);
    }
  if (*ncols < 1)
    {
      fprintf (stderr, "*** Number of cols %d must be positive! ***\n", *ncols);
      fclose (input);
      exit (EXIT_FAILURE);
    }
}

static uint64_t *
load_board_values (FILE* input, const int nrows, const int ncols)
{
  uint64_t * board = NULL;
  int ngotten = 0;
  int i, j;

  /* Make a new board */
  board = make_board (nrows, ncols);

  /* Fill in the board with values from the input file */
  for (i = 0; i < (nrows * ncols)/16; i+=1)
  {
    uint64_t tmp = 0;
    for(j = 0; j < 16; j++)
    { 
      char c;
      ngotten = fscanf (input, "%c\n", &c);
      if (ngotten < 1)
	    {
	      fprintf (stderr, "*** Ran out of input at item %d ***\n", i);
	      fclose (input);
	      exit (EXIT_FAILURE);
	    }
      else{
	      /* ASCII '0' is not zero; do the conversion */
        c -= '0';
      }
      tmp <<= 4;
      tmp += c;
    }
    board[i] = tmp;
  }
  return board;
}

uint64_t *
load_board (FILE* input, int* nrows, int* ncols)
{
  load_dimensions (input, nrows, ncols);
  return load_board_values (input, *nrows, *ncols);
}


