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
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>

#define BUFF_SIZE 512
#define OUT_APPEND 1
#define OUT_NEW 2cat

int max_pid_cnt = 8;
pid_t *pid;

struct monitor {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int too_long;
    char buffer[BUFF_SIZE];
    int end;
};

void sigint_handler(){
    kill(pid, SIGCHLD);
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
char **parse_args(const char *buffer, char *outfile, int *out_flag);
void check_pids(pid_t *pids, char **progs);

int main() {
    struct monitor *m = (struct monitor*)malloc(sizeof(struct monitor));
    if(pthread_mutex_init(&m->mutex, NULL) != 0 || pthread_cond_init(&m->cond, NULL) != 0){
        perror("Failed to init mutex or cond");
        exit(1);
    }
    pid_t *running_pids = (int *)malloc(max_pid_cnt * sizeof(pid_t));
    char **progs = (char **)malloc(max_pid_cnt * sizeof(char *));
    int pid_index = 0;

    char outfile[1024];
    int open_flag = 0;

    signal(SIGINT, sigint_handler);
    pid = &running_pids[pid_index];
    m->too_long = m->end;
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
        check_pids(running_pids, progs);
        if(m->end){
            printf("May the force be with you\n");
            break;
        }
        if(m->too_long){
            printf("Input too long\n");
            m->too_long = 0;
        }
        else if(m->buffer[0] == '\n'){}
        else{
            int detached = 0;
            for(int i = 0; i < BUFF_SIZE || m->buffer[i] == '\n'; i++)
                if(m->buffer[i] == '&'){
                    detached = 1;
                    break;
                }
            if(detached || *pid != 0){
                int i;
                for(i = 0; i < max_pid_cnt; i++){
                    if(running_pids[i] == 0){
                        pid_index = i;
                        break;
                    }
                }
                if(i == max_pid_cnt) {
                    max_pid_cnt *= 2;
                    running_pids = (pid_t *) realloc(running_pids, max_pid_cnt * sizeof(pid_t));
                    progs = (char **) realloc(progs, max_pid_cnt * sizeof(char *));
                }
                pid = &running_pids[pid_index];
            }
            open_flag = 0;
            memset(outfile, 0, sizeof(outfile));
            char **user_args = parse_args(m->buffer, &outfile, &open_flag);
            if(user_args == NULL)
                printf("Unknown command: %s", m->buffer);
            else{
                if((*pid = fork()) == 0){
                    if(open_flag == OUT_NEW)
                        freopen(outfile,"w",stdout);
                    else if(open_flag == OUT_APPEND)
                        freopen(outfile, "a", stdout);
                    execv(user_args[0], user_args);
                }
                else{
                    if(detached){
                        printf("running %d\n", running_pids[pid_index]);
                        waitpid(*pid, NULL, WNOHANG);
                        progs[pid_index] = (char *)malloc(BUFF_SIZE * sizeof(char));
                        strcpy(progs[pid_index], m->buffer);
                        pid_index++;
                        if(pid_index == max_pid_cnt)
                            pid_index = 0;
                    }
                    else{
                        wait(NULL);
                        *pid = 0;
                    }
                    usleep(1);
                    int i = 0;
                    while(user_args[i] != NULL){
                        free(user_args[i]);
                        user_args[i] = NULL;
                        i++;
                    }
                    free(user_args);
                    user_args = NULL;
                }
            }

        }
        pthread_mutex_unlock(&m->mutex);
    }
    pthread_join(t_read, NULL);
    free(m);
    return 0;
}

char **parse_args(const char *buffer, char *outfile, int *open_flag){
    int out_cnt = 0;
    int outfile_pos = 0;
    int ws_loaded = 0;
    for(int i = 0; i < strlen(buffer); i++){
        if(out_cnt && (buffer[i] == '&' || buffer[i] == '<'))
            break;
        if(buffer[i] == '>')
            out_cnt++;
        else if(out_cnt){
            if(outfile_pos == 0 && buffer[i] == ' '){}
            else if(buffer[i] == ' ')
                ws_loaded = 1;
            else if(buffer[i] != '>' && !ws_loaded && buffer[i] != '\n')
                outfile[outfile_pos++] = buffer[i];
        }
    }
    if(out_cnt == 1)
        *open_flag = OUT_NEW;
    if(out_cnt == 2)
        *open_flag = OUT_APPEND;

    int arg_len;
    for(arg_len = 0; arg_len < BUFF_SIZE; arg_len++){
        if(buffer[arg_len] == '\n' || buffer[arg_len] == '<' || buffer[arg_len] == '>' || buffer[arg_len] == '&')
            break;
    }
    char input[arg_len+1];
    int token_cnt = 1;
    for(int i = 0; i < arg_len; i++){
        input[i] = buffer[i];
        if(buffer[i] == ' ' && i < arg_len - 1)
            token_cnt++;
    }

    input[arg_len] = '\0';
    char **result = (char **) malloc((token_cnt + 1) * sizeof(char *));
    char *first = strtok(input, " ");
    if(first == NULL)
        return NULL;

    char *token = strtok(NULL, " ");

    int i = 1;
    while (token != NULL){
        int len = strlen(token);
        *(result+i) = (char *)malloc(len * sizeof(char));
        strcpy(result[i], token);
        i++;
        token = strtok(NULL, " ");
    }
    if(first != NULL){
        result[0] = get_prog_path(first);
        if(result[0] == NULL){
            return NULL;
        }
    }

    return result;
}

char *get_prog_path(char *prog){
    char *env_path = getenv("PATH");
    char path[strlen(env_path)];
    strcpy(path, env_path);
    if(path != NULL){
        char *token;
        token = strtok(path, ":");
        while(token != NULL){
            struct dirent *dir;
            DIR *d = opendir(token);
            if(d){
                while ((dir = readdir(d)) != NULL){
                    if(strcmp(prog, dir->d_name) == 0){
                        int len = (int)(strlen(token) + strlen(dir->d_name) + 2);
                        char *result = (char *) malloc(len * sizeof(char));
                        unsigned int i = 0;
                        for(; i < strlen(token); i++)
                            result[i] = token[i];
                        result[i++] = '/';
                        for(unsigned int j = 0; j < strlen(dir->d_name); j++)
                            result[i++] = dir->d_name[j];
                        result[i] = '\0';
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

void check_pids(pid_t *pids, char **progs){
    for(int i = 0; i < max_pid_cnt; i++){
        if(pids[i] > 0){
            int stat;
            waitpid(pids[i], &stat, WNOHANG);
            if(stat == 0){
                printf("DONE\t\t%s", progs[i]);
                free(progs[i]);
                progs[i] = NULL;
                pids[i] = 0;
            }
        }
    }
}
