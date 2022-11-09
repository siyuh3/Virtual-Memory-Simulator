#include <stdio.h>      
#include <stdlib.h>  
#include <unistd.h>

#include "pmmalloc.h"

int main() {
    int size = 20 * 1024;
    init_malloc(size);
    printf("***************************\nThe original heap:\n");
    print_heap();
    printf("**************************\n\n");

    printf("We first have one process occupied all the memory\n");

    void *first_process[10];
    void * second_process[10];
    for (int i = 0; i < 4; i++) {
        first_process[i] = pm_malloc(sizeof(char) * 200, 0);
        pm_write(first_process[i]);
        pm_get(first_process[i]);
        sleep(1);
        second_process[i] = pm_malloc(sizeof(int) * 100, 1);
        pm_write(second_process[i]);
        pm_get(second_process[i]);
        sleep(1);
    }
    first_process[5] = pm_malloc(sizeof(char) * 200, 0);
    pm_write(first_process[5]);
    pm_get(first_process[5]);
    print_heap();
    printf("\n");


    printf("We anticipate the following write action result a swap out the first one\n");
    first_process[6] = pm_malloc(sizeof(char) * 200, 0);
    pm_write(first_process[6]);
    pm_get(first_process[6]);
    printf("Sleep for 5 seconds.\nPlease go and check newly created file in secondary_storage\n\n");
    sleep(5);
    

    printf("We anticipate the following get action result swap in the first one and out the second one\n");
    pm_get(first_process[0]);
    printf("Sleep for 5 seconds.\nPlease go and check newly created file in secondary_storage\n\n");
    sleep(5);

    printf("We anticipate the following write action result a swap out the third one\n");
    second_process[5] = pm_malloc(sizeof(int) * 100, 1);
    printf("Finished\n");
    pm_write(second_process[5]);
    pm_get(second_process[5]);
    printf("Sleep for 5 seconds.\nPlease go and check newly created file in secondary_storage\n\n");
    sleep(5);

    printf("We anticipate the following write action result a swap out the forth one\n");
    second_process[6] = pm_malloc(sizeof(double) * 10, 1);
    pm_write(second_process[6]);
    pm_get(second_process[6]);
    printf("Sleep for 5 seconds.\nPlease go and check newly created file in secondary_storage\n\n");
    sleep(5);

    destroy_malloc();
    printf("********Done*********\n");
    return 0;
}