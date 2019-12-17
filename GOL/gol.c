#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "life.h"
#include "load.h"
#include "save.h"

#ifdef VERIFY_FLAG
#define DO_VERIFY 1
#else // VERIFY_FLAG
#define DO_VERIFY 0
#endif // VERIFY_FLAG


#define NUM_THREADS 8 


int thread_complete_counters[NUM_THREADS] = {0};

//#define PRINT_BOARD

static int
to_int (int* num, const char* s)
{
  errno = 0;
  *num = strtol (s, NULL, 10);
  if (errno != 0)
    {
      errno = 0; /* Reset errno */
      return -1;
    }
  else
    return 0;
}

static void 
print_usage (const char argv0[])
{
  fprintf (stderr, 
	   "\nUsage: %s <num_generations> <infilename> <outfilename>\n\n"
	   "\t<num_generations>: nonnegative number of generations\n"
	   "\t<infilename>:      file from which to load initial board configuration\n"
	   "\t<outfilename>:     file to which to save final board configuration;\n"
           "\t                   if not provided or just a single hyphen (-), then \n"
	   "\t                   board is output to stdout\n"
	   "\n\n", argv0);
}

#ifdef PRINT_BOARD
void print_board(uint64_t * board, int nrows, int ncols)
{
    int i, j, k;

    for(i = 0; i < nrows; i++)
    {
      for(j = 0; j < ncols/16; j++)
      {
        uint64_t tmp = board[i*ncols/16 + j];
        for (k = 15; k >= 0; k--)
        {
          char a = (char) ((tmp >> (k*4)) & 0xF);
          printf("%c", a + '0');
        }

      }
      printf("\n");
    }

    printf("\n\n\n");

}
#endif

int 
main (int argc, char* argv[])
{
  const int argc_min = 3;
  const int argc_max = 4;

  int gens_max = 0;
  uint64_t * inboard = NULL;
  uint64_t * outboard = NULL;
  uint64_t * final_board = NULL;
  int nrows = 0;
  int ncols = 0;
  FILE* input = NULL;
  FILE* output = NULL;
  int err = 0;
  char * inboard_update_map;
  char * outboard_update_map;

  /* Parse command-line arguments */
  if (argc < argc_min || argc > argc_max)
    {
      fprintf (stderr, "*** Wrong number of command-line arguments; "
	       "got %d, need at least %d and no more than %d ***\n", 
	       argc - 1, argc_min - 1, argc_max - 1);
      print_usage (argv[0]);
      exit (EXIT_FAILURE);
    }
  
  err = to_int (&gens_max, argv[1]);
  if (err != 0)
    {
      fprintf (stderr, "*** <num_generations> argument \'%s\' "
	       "must be a nonnegative integer! ***\n", argv[1]);
      print_usage (argv[0]);
      exit (EXIT_FAILURE);
    }

  /* Open input and output files */ 
  input = fopen (argv[2], "r");
  if (input == NULL)
    {
      fprintf (stderr, "*** Failed to open input file \'%s\' for reading! ***\n", argv[2]);
      print_usage (argv[0]);
      exit (EXIT_FAILURE);
    }

  if (argc < argc_max || 0 == strcmp (argv[3], "-"))
    output = stdout;
  else
    {
      output = fopen (argv[3], "w");
      if (output == NULL)
	{
	  fprintf (stderr, "*** Failed to open output file \'%s\' for writing! ***\n", argv[3]);
	  print_usage (argv[0]);
	  exit (EXIT_FAILURE);
	}
    }

  /* Load the initial board state from the input file */
  inboard = load_board (input, &nrows, &ncols);

#ifdef PRINT_BOARD
  print_board(inboard, nrows, ncols);
#endif

  fclose (input);

  /* Create a second board for ping-ponging */
  outboard = make_board (nrows, ncols);

  /* 
   * Evolve board gens_max ticks, and time the evolution.  You will
   * parallelize the game_of_life() function for this assignment.
   */

  inboard_update_map = malloc(nrows * (ncols/16)  * sizeof(char));
  outboard_update_map = malloc(nrows * (ncols/16)  * sizeof(char));

  int rows_per_thread = nrows/NUM_THREADS;
  GOL_MT_DATA mt_data[NUM_THREADS];  


  pthread_t threads[NUM_THREADS];

  int t;
  for (t = 0; t < NUM_THREADS; t++)
  {
    mt_data[t].outboard = outboard;
    mt_data[t].inboard = inboard;
    mt_data[t].start_row = rows_per_thread * t;
    mt_data[t].nrows = nrows;
    mt_data[t].rows_per_thread = rows_per_thread;
    mt_data[t].ncols = ncols;
    mt_data[t].gens_max = gens_max;
    mt_data[t].inboard_update_map = inboard_update_map;
    mt_data[t].outboard_update_map = outboard_update_map;

    if (t == 0)
      mt_data[t].above_thread_count = &thread_complete_counters[NUM_THREADS-1];
    else
      mt_data[t].above_thread_count = &thread_complete_counters[t-1];

    if (t == (NUM_THREADS - 1))
      mt_data[t].below_thread_count = &thread_complete_counters[0];
    else
      mt_data[t].below_thread_count = &thread_complete_counters[t+1];
  
    mt_data[t].my_count = &thread_complete_counters[t];

    pthread_create(&threads[t], NULL, game_of_life_MT, (void *) &mt_data[t]); 
  }
  for (t = 0; t < NUM_THREADS; t++)
  {
    pthread_join(threads[t], NULL);
  }
  //final_board = game_of_life (outboard, inboard, nrows, ncols, gens_max);

  if ((gens_max & 0x1))
  {
    final_board = outboard;
  }
  else
  {
    final_board = inboard;
  }

#ifdef PRINT_BOARD
  print_board(final_board, nrows, ncols);
#endif
  /* Print (or save, depending on command-line argument <outfilename>)
     the final board */
  save_board (output, final_board, nrows, ncols);
  if (output != stdout && output != stderr)
    fclose (output);

  /* Clean up */
  if (inboard != NULL)
    free (inboard);
  if (outboard != NULL)
    free (outboard);
  return 0;
}

