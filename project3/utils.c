#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * checks the fourth and fifth command line inputs if they are in the allowed format -p 0|1, otherwise prompts the user and exits with code 1
 */ 
int check_policy_arguments(const char *argv[])
{
  if (strcmp(argv[3], "-p") != 0)
  {
    fprintf(stderr, "Usage ./virtmem backingstore input -p X\n");
    exit(1);
  }
  int op = atoi(argv[4]);
  printf("%d", op);
  if (op != 0 && op != 1)
  {
    fprintf(stderr, "Usage ./virtmem backingstore input -p X\n\t X: 0 for FIRO, 1 for LRU\n");
    exit(1);
  }
  return op;
}
