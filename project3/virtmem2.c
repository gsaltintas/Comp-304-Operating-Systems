/**
 * virtmem2.c  for Part 2
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "utils.c"

#define TLB_SIZE 16
#define PAGES 1024
#define PAGE_MASK /* TODO */

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK /* TODO */

#define MEMORY_SIZE 256 * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry
{
  int logical;
  int physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}
int is_fifo = 1;
int replace = 0;

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(int logical_page)
{
  for (int i = 0; i < TLB_SIZE; i++)
  {
    struct tlbentry tlb_entry = tlb[i];
    if (tlb_entry.logical == logical_page)
    {
      return tlb_entry.physical;
    }
  }
  return -1;
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(int logical, int physical)
{
  struct tlbentry *new_entry = malloc(sizeof(struct tlbentry));
  new_entry->logical = logical;
  new_entry->physical = physical;

  tlbindex = tlbindex % TLB_SIZE;
  tlb[tlbindex] = *new_entry;
  tlbindex++;
}

int main(int argc, const char *argv[])
{
  if (argc != 5)
  {
    fprintf(stderr, "Usage ./virtmem backingstore input -p 0\n");
    exit(1);
  }

  const char *backing_filename = argv[1];
  int backing_fd = open(backing_filename, O_RDONLY);
  int op = check_policy_arguments(argv);
  backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

  const char *input_filename = argv[2];
  FILE *input_fp = fopen(input_filename, "r");

  // Fill page table entries with -1 for initially empty table.
  int i;
  for (i = 0; i < PAGES; i++)
  {
    pagetable[i] = -1;
  }

  // Character buffer for reading lines of input file.
  char buffer[BUFFER_SIZE];

  // Data we need to keep track of to compute stats at end.
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;

  // Number of the next unallocated physical page in main memory
  int free_page = 0;

  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL)
  {
    total_addresses++;
    int logical_address = atoi(buffer);

    /* TODO 
    / Calculate the page offset and logical page number from logical_address */
    int offset = logical_address % 1024;
    int logical_page = (int)logical_address / 1024;
    ///////

    int physical_page = search_tlb(logical_page);
    // TLB hit
    if (physical_page != -1)
    {
      tlb_hits++;
      // TLB miss
    }
    else
    {
      physical_page = pagetable[logical_page];

      // Page fault
      if (physical_page == -1)
      {
        /* TODO */
        if (is_fifo == 1)
        {
          /* TODO */
          physical_page = free_page;
          free_page++;
          if (free_page > 255)
          {
            free_page = 0;
            replace = 1;
          }
          if (replace == 1)
          {
            //find virtual address - set pagetable[va] to -1.
            int virt_page = -1;
            for (int i = 0; i < 256; i++)
            {
              int phy_page = pagetable[i];
              if (phy_page == physical_page)
              {
                virt_page = i;
              }
            }
            if (virt_page > -1)
            {
              pagetable[virt_page] = -1;
            }
            //search tlb ????
            for (int i = 0; i < TLB_SIZE; i++)
            {
              struct tlbentry tlb_entry = tlb[i];
              if (tlb_entry.logical == virt_page)
              {
                tlb_entry.physical = -1;
                tlb[i] = tlb_entry;
                printf("TLB physical:%d\n", tlb[i].physical);
              }
            }
          }
          main_memory[physical_page * 256 + offset] = backing[logical_page * 256 + offset];
          pagetable[logical_page] = physical_page;
          page_faults++;
          printf("PAGR FAULTT %d\n", physical_page);
        }
      }

      add_to_tlb(logical_page, physical_page);
    }

    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * 256 + offset];

    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }

  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

  return 0;
}
