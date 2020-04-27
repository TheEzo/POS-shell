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

#define BUFF_SIZE 512

struct monitor {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int val;
    char buffer[BUFF_SIZE];
    int end;
};

void sigint_handler(){
    printf("Caught\n");
    _exit(0);
}

void read_thread(struct monitor *m) {
    while (!m->end){
        pthread_mutex_lock(&m->mutex);
        m->val++;
        if(m->val == 10)
            m->end = 1;
        pthread_cond_signal(&m->cond);
        pthread_mutex_unlock(&m->mutex);
        usleep(10);
    }
    pthread_exit(0);
}

int main() {
    struct monitor *m = (struct monitor*)malloc(sizeof(struct monitor));
    if(pthread_mutex_init(&m->mutex, NULL) != 0 || pthread_cond_init(&m->cond, NULL) != 0){
        perror("Failed to init mutex or cond");
        exit(1);
    }

    signal(SIGINT, sigint_handler);
    m->val = m->end = 0;
    pthread_t t_read;
    pthread_create(&t_read, NULL, (void *)read_thread, m);
    while (!m->end){
        pthread_cond_wait(&m->cond, &m->mutex);
        printf("%d\n", m->val);
        pthread_mutex_unlock(&m->mutex);
    }
    pthread_join(t_read, NULL);
    free(m);
    return 0;
}
