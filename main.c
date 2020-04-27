/*********************
 * POS - shell       *
 * tom shell (tsh)   *
 * Tomas Willaschek  *
 * xwilla00          *
 *********************/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#define BUFF_SIZE

char buffer[BUFF_SIZE];
pthread_mutex_t mutex;
pthread_cond_t cond;

void sigint_handler(){
    printf("Caught\n");
    _exit(0);
}

int main() {
    signal(SIGINT, sigint_handler);
    int pid = fork();
    if (pid == 0){

    }
    else{

    }
    return 0;
}
