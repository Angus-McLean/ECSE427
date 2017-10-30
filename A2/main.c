#define _SVID_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE 500
#define _XOPEN_SORUCE 600
#define _XOPEN_SORUCE 600

#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define BUFF_SIZE 20
#define BUFF_SHM "/OS_BUFF"
#define BUFF_MUTEX_A "/OS_MUTEX_A"
#define BUFF_MUTEX_B "/OS_MUTEX_B"

//declaring semaphores names for local usage
sem_t *mutexA;
sem_t *mutexB;

//declaring the shared memory and base address
int shm_fd;
void *base;

//structure for indivdual table
struct table
{
    int num;
    char name[10];
};

void initTables(struct table *base)
{
    struct table *tempTable = malloc(sizeof(struct table));

    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);

    //initialise the tables with table numbers
    for (int i = 1; i <= 20; i++) {

        tempTable->num = i;
        *tempTable->name = '\0';

        memcpy(&base + i*sizeof(struct table), tempTable, sizeof(struct table));
    }

    //perform a random sleep
    sleep(rand() % 2);

    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);

    return;
}

void printTableInfo(struct table *base)
{
    struct table *tempTable = malloc(sizeof(struct table));

    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);

    //print the tables with table numbers and name
    for (int i = 1; i <= 20; i++) {
        memcpy(tempTable, &base + i*sizeof(struct table), sizeof(struct table));
        printf("Table %d - %s\n", tempTable->num, tempTable->name);
    }

    //perform a random sleep
    sleep(rand() % 2);

    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);

    return;
}

void reserveSpecificTable(struct table *base, char *nameHld, char *section, int tableNo)
{
    struct table *tempTable = malloc(sizeof(struct table));

    printf("reserveSpecificTable %s %s %d\n", nameHld, section, tableNo);

    switch (section[0])
    {
        case 'A':
            //capture mutex for section A
            sem_wait(mutexA);


            //check if table number belongs to section specified
            //if not: print Invalid table number

            //reserve table for the name specified
            //if cant reserve (already reserved by someone) : print "Cannot reserve table"
            memcpy(tempTable, &base + tableNo*sizeof(struct table), sizeof(struct table));
            printf("Checking table %d for %s\n", tempTable->num, tempTable->name);

            //
            if(strcmp(tempTable->name, "\0") == 0) {
                *tempTable->name = *nameHld;
            	sprintf(tempTable->name, "%s", nameHld);
                memcpy(&base + tableNo*sizeof(struct table), tempTable, sizeof(struct table));
                printf("Reserved table %d for %s\n", tempTable->num, tempTable->name);
            } else {
                printf("Cannot reserve table %d - %s\n", tempTable->num, tempTable->name);
            }

            // release mutex
            sem_post(mutexA);

            break;
        case 'B':
            //capture mutex for section B

            //check if table number belongs to section specified
            //if not: print Invalid table number

            //reserve table for the name specified ie copy name to that struct
            //if cant reserve (already reserved by someone) : print "Cannot reserve table"

            // release mutex
            break;
    }
    return;
}

void reserveSomeTable(struct table *base, char *nameHld, char *section)
{
    int idx = -1;
    int i;
    switch (section[0])
    {
        case 'A':
        //capture mutex for section A
        sem_wait(mutexA);

        //look for empty table and reserve it ie copy name to that struct

        //if no empty table print : Cannot find empty table


        //release mutex for section A
        sem_post(mutexA);

        break;
        case 'B':
        //capture mutex for section A

        //look for empty table and reserve it ie copy name to that struct

        //if no empty table print : Cannot find empty table


        //release mutex for section A
        break;
    }
}

int processCmd(char *cmd, struct table *base)
{
    char *token;
    char *nameHld;
    char *section;
    char *tableChar;
    int tableNo;
    token = strtok(cmd, " ");
    switch (token[0])
    {
        case 'r':
        nameHld = strtok(NULL, " ");
        section = strtok(NULL, " ");
        tableChar = strtok(NULL, " ");
        if (tableChar != NULL)
        {
            tableNo = atoi(tableChar);
            reserveSpecificTable(base, nameHld, section, tableNo);
        }
        else
        {
            reserveSomeTable(base, nameHld, section);
        }
        sleep(rand() % 2);
        break;
        case 's':
        printTableInfo(base);
        break;
        case 'i':
        initTables(base);
        break;
        case 'e':
        return 0;
    }
    return 1;
}

int main(int argc, char * argv[])
{
    int fdstdin;
    // file name specifed then rewire fd 0 to file
    if(argc>1)
    {
        //store actual stdin before rewiring using dup in fdstdin

        //perform stdin rewiring as done in assign 1

    }
    //open mutex BUFF_MUTEX_A and BUFF_MUTEX_B with inital value 1 using sem_open
    mutexA = sem_open(BUFF_MUTEX_A, O_CREAT | O_EXCL, 0777, 1);
    mutexB = sem_open(BUFF_MUTEX_B, O_CREAT | O_EXCL, 0777, 1);

    if(mutexA == (void *)-1) {
        printf("sem_open() failed");
        exit(1);
    }
    if(mutexB == (void *)-1) {
        printf("sem_open() failed");
        exit(1);
    }

    //opening the shared memory buffer ie BUFF_SHM using shm open
    shm_fd = shm_open(BUFF_SHM, O_RDWR|O_CREAT, 0666);
    if (shm_fd == -1)
    {
        printf("prod: Shared memory failed: %s\n", strerror(errno));
        exit(1);
    }

    //configuring the size of the shared memory to sizeof(struct table) * BUFF_SIZE usinf ftruncate
    ftruncate(shm_fd, sizeof(struct table) * BUFF_SIZE);

    //map this shared memory to kernel space
    base = mmap(NULL, sizeof(struct table)*BUFF_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

    if (base == MAP_FAILED)
    {
        printf("prod: Map failed: %s\n", strerror(errno));
        // close and shm_unlink?
        exit(1);
    }

    //intialising random number generator
    time_t now;
    srand((unsigned int)(time(&now)));

    //array in which the user command is held
    char cmd[100];
    int cmdType;
    int ret = 1;
    while (ret)
    {
        printf("\n>>");
        gets(cmd);
        if(argc>1)
        {
            printf("Executing Command : %s\n",cmd);
        }
        printf("Executing Command : %s\n",cmd);
        ret = processCmd(cmd, base);
    }

    //close the semphores
    sem_close(mutexA);
    sem_close(mutexB);


    //reset the standard input
    if(argc>1)
    {
        //using dup2
    }

    //unmap the shared memory
    munmap(base, sizeof(struct table)*BUFF_SIZE);
    close(shm_fd);
    return 0;
}
