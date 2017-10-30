rm /dev/shm/sem.OS_MUTEX_*;
gcc main.c -o a.out -lpthread -lrt >_ignore_compile.log
./a.out
