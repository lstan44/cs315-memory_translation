/* Stanley Lalanne
* Virtual Memory
* Translation of logical_addresses addresses to physical_addresses addresses
*/

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlb_entry {
unsigned char logical_addresses;
unsigned char physical_addresses;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlb_entry tlb[TLB_SIZE];
int tlbindex = 0;

// pagetable[page_number] is the frame number for the logical_addresses. Value is -1 if it isn't yet in the table.
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

/* Returns the physical address from TLB or -1 if not present. */
int TLBsearch(unsigned char page_number) {
int i;
for (i = max((tlbindex - TLB_SIZE), 0); i < tlbindex; i++) {
struct tlb_entry *entry = &tlb[i % TLB_SIZE];

if (entry->logical_addresses == page_number) {
return entry->physical_addresses;
}
}

return -1;
}

/* inserts the mapping to the tlb (cache)*/
void insert_in_tlb(unsigned char logical_addresses, unsigned char physical_addresses) {
struct tlb_entry *entry = &tlb[tlbindex % TLB_SIZE];
tlbindex++;
entry->logical_addresses = logical_addresses;
entry->physical_addresses = physical_addresses;
}


/* print bits for testing purpose */

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

int main(int argc, const char *argv[])
{

  //Print an error message if arguments are not passed in the command line
if (argc != 3) {
fprintf(stderr, "Usage: ./main backingstore.bin adresses.txt \n");
exit(1);
}

const char *backing_file = argv[1]; // first argument backingstore.bin
int backing_store = open(backing_file, O_RDONLY);
backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_store, 0); /*creates a new mapping in the virtual address space
* PROT_READ -> pages may be read
* MAP_PRIVATE -> Creates a private copy-on-write mapping
*/

const char *input_file = argv[2];
FILE *input_fp = fopen(input_file, "r");

// Fill page table entries with -1 for initially empty table.
int i;
for (i = 0; i < PAGES; i++) {
pagetable[i] = -1;
}

// Buffer to read the inputs of addresses.txt
char buffer[BUFFER_SIZE];

// Data we need to keep track of to compute stats at end.
int total_addresses = 0;
int tlb_hits = 0;
int page_faults = 0;

// Number of the next unallocated physical_addresses page in main memory
unsigned char free_page = 0;

while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
total_addresses++;
int logical_addresses = atoi(buffer); /* holds the logical_addresses from addresses.txt */
int offset = logical_addresses & OFFSET_MASK; /* Compare the binary representations to mask */
//printBits(sizeof(offset),&offset);
int page_number = (logical_addresses >> OFFSET_BITS) & PAGE_MASK;

int frame_number = TLBsearch(page_number);
// TLB hit
if (frame_number != -1) {
tlb_hits++;
// TLB miss
} else {
frame_number = pagetable[page_number];

// Page fault
if (frame_number == -1) {
page_faults++;

frame_number = free_page;
free_page++;

// Copy page from backing file into physical_addresses memory
memcpy(main_memory + frame_number * PAGE_SIZE, backing + page_number * PAGE_SIZE, PAGE_SIZE);

pagetable[page_number] = frame_number;
}

insert_in_tlb(page_number, frame_number);
}

int physical_addresses = (frame_number << OFFSET_BITS) | offset;
signed char value = main_memory[frame_number * PAGE_SIZE + offset];

printf("Virtual address: %d physical_addresses address: %d Value: %d\n", logical_addresses, physical_addresses, value);
// printf("%s\n","Logical Address:" );
// //printBits(sizeof(logical_addresses), &logical_addresses);
// printf("%s\n","Physical Address:" );
// //printBits(sizeof(physical_addresses), &physical_addresses);
// printf("%s\n","Value:" );
// //printBits(sizeof(value), &value);
}

printf("Number of Addresses Translated = %d\n", total_addresses);
printf("Page Faults = %d\n", page_faults);
printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
printf("TLB Hits = %d\n", tlb_hits);
printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

return 0;
}
