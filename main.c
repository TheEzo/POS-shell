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
#include <sys/mman.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFF_SIZE

struct monitor {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int val;
    char *buffer[BUFF_SIZE];
};

void sigint_handler(){
    printf("Caught\n");
    _exit(0);
}

void read_thread(struct monitor *m) {
    while (m->val < 10){
        pthread_mutex_lock(&m->mutex);
        m->val++;
        pthread_cond_signal(&m->cond);
        pthread_mutex_unlock(&m->mutex);
        usleep(10);
    }
    pthread_exit(0);
}

int main() {
    struct monitor *m = (struct monitor*)malloc(sizeof(struct monitor));
    int stat;
    stat = pthread_mutex_init(&m->mutex, NULL);
    if(stat != 0){
        perror("Failed to init mutex");
        exit(1);
    }
    stat = pthread_cond_init(&m->cond, NULL);
    if(stat != 0){
        perror("Failed to init cond");
        exit(1);
    }
    signal(SIGINT, sigint_handler);

    pthread_t t_read;
    pthread_create(&t_read, NULL, (void *)read_thread, m);
//    pthread_join(t_read, NULL);
    while (m->val < 10){
        pthread_cond_wait(&m->cond, &m->mutex);
        printf("%d\n", m->val);
        pthread_mutex_unlock(&m->mutex);
    }
    pthread_join(t_read, NULL);
    free(m);
    return 0;
}
