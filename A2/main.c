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

    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);

    //initialise the tables with table numbers
    for (int i=0; i<10; i++){
        (base+i)->num = 100+i;
        strcpy((base+i)->name, "\0");
    }
    for (int j=0; j<10; j++){
        (base+j+10)->num = 200+j;
        strcpy((base+j+10)->name, "\0");
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
    printf(" ---- Section A ---- \n");
    for (int i=0; i<10; i++){
        printf("Table %d - %s\n", (base+i)->num, (base+i)->name);
    }
    printf(" ---- Section B ---- \n");
    for (int j=0; j<10; j++){
        printf("Table %d - %s\n", (base+j+10)->num, (base+j+10)->name);
    }


    //perform a random sleep
    sleep(rand() % 2);

    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);

    return;
}


char * readTable(struct table *base, int tableIndex) {
    return (base+tableIndex)->name;
}

int writeTable(struct table *base, int tableIndex, char *nameHld) {
    if(!strcmp(readTable(base, tableIndex), "\0")) {
        // table is availble
        strcpy((base+tableIndex)->name, nameHld);
        return 1;
    } else {
        printf("Cannot reserve table %d - %s\n", (base+tableIndex)->num, (base+tableIndex)->name);
        return 0;
    }
}

void reserveSpecificTable(struct table *base, char *nameHld, char *section, int tableNo)
{

    printf("reserveSpecificTable %s %s %d\n", nameHld, section, tableNo);

    //check if table number belongs to section specified
    //if not: print Invalid table number
    if(tableNo == 0 || tableNo > 9) {
        printf("%s\n", "Invalid table number");
        return;
    }

    switch (section[0])
    {
        case 'A':
            sem_wait(mutexA);
            writeTable(base, tableNo, nameHld);
            sem_post(mutexA);

            break;
        case 'B':
            sem_wait(mutexB);
            writeTable(base, tableNo + BUFF_SIZE/2, nameHld);
            sem_post(mutexB);

            break;
    }
    return;
}

void reserveSomeTable(struct table *base, char *nameHld, char *section)
{
    int success = 0;
    int i;
    switch (section[0])
    {
        case 'A':
            //capture mutex for section A
            sem_wait(mutexA);

            //look for empty table and reserve it ie copy name to that struct
            for (int i = 0; i < 10; i++) {
                if(success = writeTable(base, i, nameHld)) {
                    break;
                }
            }

            //release mutex for section A
            sem_post(mutexA);

            break;
        case 'B':
            //capture mutex for section B
            sem_wait(mutexB);

            //look for empty table and reserve it ie copy name to that struct
            for (int i = 0; i < 10; i++) {
                if(success = writeTable(base, i + BUFF_SIZE/2, nameHld)) {
                    break;
                }
            }

            //if no empty table print : Cannot find empty table
            if (!success) {
                printf("Cannot find empty table\n");
            }

            //release mutex for section A
            sem_post(mutexB);
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
            if (tableChar != NULL) {
                tableNo = atoi(tableChar);
                reserveSpecificTable(base, nameHld, section, tableNo);
            } else {
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
        dup2(0,fdstdin);
        //perform stdin rewiring as done in assign 1
        close(0);
        open(argv[1], O_RDONLY);
    }
    //open mutex BUFF_MUTEX_A and BUFF_MUTEX_B with inital value 1 using sem_open
    mutexA = sem_open(BUFF_MUTEX_A, O_CREAT, 0777, 1);
    mutexB = sem_open(BUFF_MUTEX_B, O_CREAT, 0777, 1);

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
    base = mmap(NULL, sizeof(struct table)*BUFF_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);

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
        dup2(fdstdin,0);
    }

    //unmap the shared memory
    munmap(base, sizeof(struct table)*BUFF_SIZE);
    close(shm_fd);
    return 0;
}
