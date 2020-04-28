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
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>

#define BUFF_SIZE 512

pid_t pid;

struct monitor {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int too_long;
    char buffer[BUFF_SIZE];
    int end;
};

void sigint_handler(){
    kill(pid, SIGTERM);
}

void *read_thread(void *arg) {
    char end[5];
    struct monitor *m = (struct monitor *)arg;
    end[4] = '\0';
    while (!m->end){
        pthread_mutex_lock(&m->mutex);
        memset(m->buffer, 0, sizeof(m->buffer));
        printf("tsh $ ");
        fflush(stdout);
        int bytes = read(0, m->buffer, sizeof(m->buffer) + 1);
        if(bytes > BUFF_SIZE)
            m->too_long = 1;
        for(int i = 0; i < 4 && !m->too_long; i++)
            end[i] = m->buffer[i];

        if(strcmp(end, "exit") == 0)
            m->end = 1;
        pthread_cond_signal(&m->cond);
        pthread_mutex_unlock(&m->mutex);
        usleep(1);
    }
    pthread_exit(0);
}

char *get_prog_path(char *prog);
char **parse_args(const char *buffer);

int main() {
    struct monitor *m = (struct monitor*)malloc(sizeof(struct monitor));
    if(pthread_mutex_init(&m->mutex, NULL) != 0 || pthread_cond_init(&m->cond, NULL) != 0){
        perror("Failed to init mutex or cond");
        exit(1);
    }
    m->too_long = m->end = pid = 0;
    printf("*****************************\n");
    printf("* Welcome to TomShell (tsh) *\n");
    printf("* Author: Tomas Willaschek  *\n");
    printf("*         (xwilla00)        *\n");
    printf("* POS 2020 - shell          *\n");
    printf("*****************************\n\n");
    pthread_t t_read;
    pthread_create(&t_read, NULL, read_thread, m);
    while (1){
        pthread_cond_wait(&m->cond, &m->mutex);
        if(m->end)
            break;
        if(m->too_long){
            printf("Příliš dlouhý vstup\n");
            m->too_long = 0;
        }
        else{
            int detached = 0;
            for(int i = 0; i < BUFF_SIZE || m->buffer[i] == '\n'; i++)
                if(m->buffer[i] == '&'){
                    detached = 1;
                    break;
                }
            parse_args(m->buffer);
            printf("%s", m->buffer);
//            char **prog_args = parse_args(m->buffer);
//            if((pid = fork()) == 0){
//                signal(SIGINT, sigint_handler);
//                execv(prog_args[0], prog_args);
//            }
//            else{
//                if(!detached)
//                    wait(NULL);
//
//            }
        }
        pthread_mutex_unlock(&m->mutex);
    }
    pthread_join(t_read, NULL);
    free(m);
    return 0;
}

char **parse_args(const char *buffer){
    int arg_len;
    for(arg_len = 0; arg_len < BUFF_SIZE; arg_len++){
        if(buffer[arg_len] == '\n' || buffer[arg_len] == '<' || buffer[arg_len] == '>' || buffer[arg_len] == '&')
            break;
    }
    printf("%d\n", arg_len);
    char input[arg_len+1];
    int token_cnt = 1;
    for(int i = 0; i < arg_len; i++){
        input[i] = buffer[i];
        if(buffer[i] == ' ' && i < arg_len - 1)
            token_cnt++;
    }

    input[arg_len] = '\0';
    char *token = strtok(input, " ");
    if(token != NULL){

    }
    while (token != NULL){
        printf("token: '%s'\n", token);
        token = strtok(NULL, " ");
    }


    return NULL;
}

char *get_prog_path(char *prog){
    char *path = getenv("PATH");
    if(path != NULL){
        char *token;
        token = strtok(path, ":");
        while(token != NULL){
            struct dirent *dir;
            DIR *d = opendir(token);
            if(d){
                while ((dir = readdir(d)) != NULL){
                    if(strcmp(prog, dir->d_name) == 0){
                        printf("\n%s\n", dir->d_name);
                        printf("%s\n", token);
                        int len = strlen(token) + strlen(dir->d_name) + 2;
                        int i = 0;
                        char result[len];
                        for(i; i < strlen(token); i++)
                            result[i] = token[i];
                        result[i++] = '/';
                        for(int j = 0; j < strlen(dir->d_name); j++)
                            result[i++] = dir->d_name[j];
                        result[i++] = '\0';
                        return result;
                    }
                }
                closedir(d);
            }


            token = strtok(NULL, ":");
        }
    }
    return NULL;
}
