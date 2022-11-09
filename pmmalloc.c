#include <stdio.h>      
#include <stdlib.h>  
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "pmmalloc.h"

#define PAGESIZE 1024

pm_stc *head; 
char *all_buffer;
int pm_stc_size = sizeof(pm_stc);
int pte_size = sizeof(pte);

int free_page;
pt_head *page_table;
int pte_count = 0;
fifo_node *FIFO;
int fifo_end = 0;
int fifo_start = 0;


void init_malloc(int size) {
    all_buffer = malloc(size);
    head = (pm_stc *) all_buffer;
    head->length = size / 2 - pm_stc_size;
    head->used = 0;
    head->buffer = (char *) head + pm_stc_size;
    head->pre = head->nx = NULL;

    // Leave some space so thatwhen page is full, the final bookkeeping still has some space
    free_page = size / PAGESIZE / 2 - 1;
    head->pages = free_page;
    page_table = (pt_head *) ((char *) head + size * 5 / 8);
    FIFO = (fifo_node *) ((char *) head + size * 3 / 4);
    printf("Page_table starts from: %p\n", page_table);
    printf("FFIO is at: %p\n", FIFO);
   
}

void destroy_malloc() {
    free(all_buffer);
}

void *pm_malloc(int size, int process) {

    int page_count = ceil((pm_stc_size + size) / (float) PAGESIZE);

    if (page_count > 1) {
        printf("Call pm_malloc failed due to size too large\n");
    }
    int do_swap = 0;
    if (free_page <= 0) {
        do_swap = page_out();
    } 

    pm_stc *cur_ptr = head;
    while (cur_ptr != NULL && (cur_ptr->length < size + pm_stc_size || cur_ptr->used == 1)) {
        cur_ptr = cur_ptr->nx;
    }
    if (cur_ptr == NULL) {
        printf("Call pm_malloc failed\n");
        exit(1);
    }

    if (do_swap == 0) {
        pm_stc *new_ptr = (pm_stc *) ((char *)cur_ptr + page_count * PAGESIZE);
        new_ptr->length = cur_ptr->length - page_count * PAGESIZE;
        new_ptr->used = 0;
        new_ptr->buffer = (char *) new_ptr + pm_stc_size;
        new_ptr->pre = cur_ptr;
        new_ptr->nx = cur_ptr->nx;
        cur_ptr->length = page_count * PAGESIZE - pm_stc_size;
        if (cur_ptr->nx != NULL) {
            cur_ptr->nx->pre = new_ptr;
        }
        cur_ptr->nx = new_ptr;
        new_ptr->pages = cur_ptr->pages - page_count;
        cur_ptr->pages = page_count;
    }
    
    cur_ptr->used = 1;
    free_page -= page_count;

    pt_head *cur_page_table = page_table + process;

    pte *cur_page_table_entry = &(cur_page_table->page_table_entry)[cur_page_table->count];
    cur_page_table_entry->virtual_address = cur_page_table_entry;
    cur_page_table_entry->physical_address = cur_ptr->buffer;
    cur_page_table_entry->index = cur_page_table->count;
    cur_page_table_entry->process_index = process;
 
    fifo_node *location = FIFO + fifo_end;
    location->virtual_address = &(cur_page_table->page_table_entry)[cur_page_table->count];
    fifo_end += 1;

    
    cur_page_table->count += 1;
    return (cur_page_table->page_table_entry)[cur_page_table->count - 1].virtual_address;

}

void pm_free(void *virtual_address) {
    void *ptr = ((pte *) virtual_address)->physical_address;
    pm_stc *header = (pm_stc *) ((char *) ptr - pm_stc_size);
    if (header == NULL || header->used == 0) {
        printf("The pointer is invalid.");
        return;
    }
    header->used = 0;
    free_page += header->pages;
    if (header->nx && header->nx->used == 0) {
        header->length += header->nx->length + pm_stc_size;
        header->pages += header->nx->pages;
        header->nx = header->nx->nx;
        if (header->nx) {
            header->nx->pre = header;
        }
    }

    if (header->pre && header->pre->used == 0) {
        header->pre->length += header->length + pm_stc_size;
        header->pre->pages += header->pages;
        header->pre->nx = header->nx;
        if (header->nx) {
            header->nx->pre = header->pre;
        }
    }
    
}

void print_heap() {
    pm_stc *cur_block = head;
    while (cur_block != NULL) {
        printf("Block Address is: %p\nBlock Size is: %d\nBlock Used: %d\n", cur_block->buffer - pm_stc_size, cur_block->length, cur_block->used);
        cur_block = cur_block->nx;
        if (cur_block != NULL) {
            printf("-------------------\n");
        } else {
            if (free_page <= 0) {
                printf("The last page cannot be used to avoid segmentation\n");
            }
        }
    }
}

int page_out() {    
    pte *swap_out_pte = (FIFO + fifo_start)->virtual_address;
    printf("Swap out page with physical address: %p\n", swap_out_pte->physical_address);
    swap_out_pte->removed = 1;
    
    FILE *fp;
    char file_name[50];

    sprintf(file_name, "secondary_storage/%d_%ld.txt", swap_out_pte->process_index, swap_out_pte->index);
    fp = fopen(file_name, "w");
    fwrite((char *)swap_out_pte->physical_address, sizeof(char), PAGESIZE - pm_stc_size, fp);
    fclose(fp);
    pm_free(swap_out_pte->virtual_address);
    fifo_start += 1;
    return 1;
}

void page_in(pte *cur_pte) {

    void *physcial_add = ((pte *) (FIFO + fifo_start)->virtual_address)->physical_address;
    pm_stc *header = (pm_stc *) ((char *) physcial_add - pm_stc_size);

    page_out();
    char file_name[50];
    sprintf(file_name, "secondary_storage/%d_%ld.txt", cur_pte->process_index, cur_pte->index);
    printf("In page in%s\n", file_name);
    FILE *fileptr;
    char *buffer;
    long filelen;

    fileptr = fopen(file_name, "rb");  
    fseek(fileptr, 0, SEEK_END);          
    filelen = ftell(fileptr);             
    rewind(fileptr);                     

    buffer = (char *)malloc(filelen * sizeof(char));
    fread(physcial_add, filelen, 1, fileptr); 
    fclose(fileptr); 

    cur_pte->removed = 0;
    cur_pte->physical_address = physcial_add;

    fifo_node *location = FIFO + fifo_end;
    location->virtual_address = cur_pte;
    fifo_end += 1;
    free_page -= 1;

    printf("Swap in page to physical address: %p\n", cur_pte->physical_address);

}

void pm_write(void *virtual_address) {
    pte * cur_pte = (pte *) virtual_address;
    if (cur_pte->removed == 1) {
        page_in(cur_pte);
    }
    char write_words[200];
    time_t now;
    time(&now);
    sprintf(write_words, "Now: %s", ctime(&now));
    
    strcpy(cur_pte->physical_address, write_words);
}

void pm_get(void *virtual_address) {
    pte * cur_pte = (pte *) virtual_address;
    if (cur_pte->removed == 1) {
        page_in(cur_pte);
    }
    printf("The contain is: %s", (char *)cur_pte->physical_address);
}
