typedef struct pm_stc {
    int length;
    int used;
    struct pm_stc *pre;
    struct pm_stc *nx;
    char *buffer;
    int pages;
} pm_stc;

typedef struct pte {
    void *virtual_address;
    void * physical_address;
    int removed;
    int process_index;
    long index;
} pte;

typedef struct pt_head {
    int count;
    pte page_table_entry[31];
    int offset[6];
} pt_head;

typedef struct fifo_node {
    void *virtual_address;
} fifo_node;

void init_malloc(int size);
void destroy_malloc();
void *pm_malloc(int size, int process);
void pm_free(void *virtual_address);
void print_heap();
int page_out();
void page_in(pte *cur_pte);
void pm_write(void *virtual_address);
void pm_get(void *virtual_address);