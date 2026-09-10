#include "aesdsocket.h"
#include <asm-generic/socket.h>
#include <bits/pthreadtypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/queue.h>

volatile sig_atomic_t caught_signal = 0;
typedef struct thread_node_s{
    pthread_t thread_id;
    int completed;
    SLIST_ENTRY(thread_node_s) entries;
} thread_node_t;
SLIST_HEAD(slisthead, thread_node_s)    ;
typedef struct {
    int client_fd;
    pthread_mutex_t *file_mtx; 
    thread_node_t *node;
} thread_args_t;
pthread_mutex_t file_mtx = PTHREAD_MUTEX_INITIALIZER;

void signal_handler(int signal_number) {
    if (signal_number == SIGINT || signal_number == SIGTERM) {
        caught_signal = 1;
    }
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void *timer_append(void *arg){
    thread_args_t *args = (thread_args_t *)arg;
    pthread_mutex_t *file_mtx = args->file_mtx;
    free(args);
    int fd = -1;
    int slept = 0;
    while(!caught_signal){
        if(caught_signal){
            break;
        }
        while(slept < 10 && !caught_signal){
            sleep(1);
            slept++;
            continue;
        }
        time_t rawtime;
        struct tm *timeinfo;
        char time_stamp[100];
        char formatted_time[150];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(time_stamp, sizeof(time_stamp), "%a, %d %b %Y %H:%M:%S %z", timeinfo);
        snprintf(formatted_time, sizeof(formatted_time), "timestamp:%s\n", time_stamp);
        pthread_mutex_lock(file_mtx);
        fd = open(PATHFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if(fd < 0){
            perror("Couldn't open file to write timestamp");
        }
        else{
            write(fd, formatted_time, strlen(formatted_time));
        }
        slept = 0;
        pthread_mutex_unlock(file_mtx);
    }
    return NULL;
}
void *handle_connection(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int client_fd = args->client_fd;
    pthread_mutex_t *file_mtx = args->file_mtx;
    thread_node_t *node = args->node;
    int fd;
    free(args);
    char recv_buf[MAXDATASIZE];
    char *packet_buf = NULL;
    size_t packet_size = 0;
    ssize_t bytes_received;
    char *newline_ptr = NULL; 
    while ((bytes_received = recv(client_fd, recv_buf, sizeof(recv_buf), 0)) > 0) {
        char *new_buf = realloc(packet_buf, packet_size + bytes_received);
        if (!new_buf) {
            perror("Failed to allocate memory");
            free(packet_buf);
            close(client_fd);
            return NULL;
        }
        packet_buf = new_buf;
        memcpy(packet_buf + packet_size, recv_buf, bytes_received);
        packet_size += bytes_received;
        newline_ptr = memchr(packet_buf, '\n', packet_size);
        if (newline_ptr != NULL) {
            break;
        }
    }
    if (bytes_received < 0) {
        perror("recv");
        free(packet_buf);
        close(client_fd);
        return NULL;
    }
    if (packet_buf == NULL) {
        close(client_fd);
        return NULL;
    }
    pthread_mutex_lock(file_mtx);
    fd = open(PATHFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Couldn't open file");
        close(client_fd);
        pthread_mutex_unlock(file_mtx);
        return NULL;
    }
    size_t len = (newline_ptr != NULL) ? (size_t)(newline_ptr - packet_buf) + 1: packet_size;
    ssize_t bytes_written = write(fd, packet_buf, len);
    if (bytes_written != (ssize_t)len) {
        close(fd);
        syslog(LOG_ERR, "Failed to write to file");
        free(packet_buf);
        pthread_mutex_unlock(file_mtx);
        return NULL;
    }
    free(packet_buf);
    close(fd);
    fd = open(PATHFILE, O_RDONLY);
    if (fd >= 0) {
        ssize_t bytes_read;
        char read_buf[MAXDATASIZE];
        while ((bytes_read = read(fd, read_buf, sizeof(read_buf))) > 0) {
            if (send(client_fd, read_buf, bytes_read, 0) < 0) {
                perror("send to client");
                break;
            }
        }
        close(fd);
    } else {
        perror("Erro while opening read file");
    }
    pthread_mutex_unlock(file_mtx);
    node->completed = 1;
    close(client_fd);
    return NULL;
}
int main(int argc, char *argv[]){
    int daemonize = 0;
    int c;
    while ((c = getopt(argc, argv, "d")) != -1) {
        if (c == 'd') daemonize = 1;
        else { 
            return -1; 
        }
    }
    if (daemonize) {
        pid_t pid = fork();
        if (pid < 0) { 
            perror("fork"); 
            return -1; 
        }
        if (pid > 0) 
            return 0;
        if (setsid() < 0) { 
            perror("setsid");
            return -1; 
        }
        int nullfd = open("/dev/null", O_RDWR);
        if (nullfd >= 0) {
            dup2(nullfd, STDIN_FILENO);
            dup2(nullfd, STDOUT_FILENO);
            dup2(nullfd, STDERR_FILENO);
            if (nullfd > STDERR_FILENO) close(nullfd);
        }
   }
   
   int sock_fd, client_fd = -1, optval = 1;
   struct sockaddr_in server_addr;
   struct sockaddr_storage claddr;
   socklen_t len;
   char claddrStr[INET_ADDRSTRLEN];
   struct sigaction sa;
   int s;
   memset(&sa, 0, sizeof(sa));
   sa.sa_flags = 0;
   sa.sa_handler = signal_handler;
   sigemptyset(&sa.sa_mask);
   openlog("aesdsocket", LOG_PID, LOG_USER);
   if(sigaction(SIGINT, &sa, NULL) != 0){
       perror("Error registrating SIGINT");
       return -1;
   } 
   if(sigaction(SIGTERM, &sa, NULL) != 0){
       perror("Error registrating SIGINT");
       return -1;
   }
   pthread_t thread_timer;
   int j;
   thread_args_t *args = malloc(sizeof(thread_args_t));
   args->file_mtx = &file_mtx;
   j = pthread_create(&thread_timer, NULL, timer_append, (void *)args);
   if(j < 0){
      perror("Failed to create thread for timer");
      free(args);
   }
   sock_fd = socket(AF_INET, SOCK_STREAM, 0);
   if(sock_fd < 0){
       perror("Failed to open socket");
       return -1;   
   }
   memset(&server_addr , 0, sizeof(struct sockaddr_in));
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(PORT);   
   server_addr.sin_addr.s_addr = INADDR_ANY;
   if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
       close(sock_fd);    
       perror("setsockopt");
           return -1;
   }
   if(bind(sock_fd, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) < 0){
       close(sock_fd); 
       perror("Couldn't bind socket to port");
        return -1;
   }
   if(listen(sock_fd, BACKLOG) < 0){
       close(sock_fd);
       perror("Error initializing socket in listen mode");
       return -1;
   }
   struct slisthead head;
   SLIST_INIT(&head);
   while(!caught_signal){ 
       len = sizeof(struct sockaddr_storage);
       client_fd = accept(sock_fd, (struct sockaddr *) &claddr, &len);
       if(client_fd < 0){
            perror("Couldn't connect to client");
            continue;
       }
       thread_node_t *node = malloc(sizeof(thread_node_t));
       node->completed = 0;
       thread_args_t *args = malloc(sizeof(thread_args_t));
       args->client_fd = client_fd;
       args->file_mtx = &file_mtx;
       args->node = node;
       inet_ntop(claddr.ss_family, get_in_addr((struct sockaddr *) &claddr), claddrStr, sizeof(claddrStr));
       syslog(LOG_INFO, "Accepted conthread vs processnection from %s", claddrStr);
       s = pthread_create(&node->thread_id, NULL, handle_connection, (void *)args);
       if(s < 0){
            perror("Failed to create thread");
            free(args);
            free(node);
            close(client_fd);
       }
       else {
            SLIST_INSERT_HEAD(&head, node, entries);
       }
       thread_node_t *elem = SLIST_FIRST(&head);
       thread_node_t *tmp = NULL;
       while (elem != NULL) {
           tmp = SLIST_NEXT(elem, entries); 
           if (elem->completed == 1) {
             pthread_join(elem->thread_id, NULL);
             SLIST_REMOVE(&head, elem, thread_node_s, entries);
             free(elem);
           }
    elem = tmp; 
    }     
       syslog(LOG_INFO, "Closed connection from %s", claddrStr);
    }
    syslog(LOG_INFO, "Caught signal, exiting");
    if(sock_fd != -1)
        close(sock_fd);
    if (client_fd != -1)
        close(client_fd);
    pthread_join(thread_timer, NULL);
    thread_node_t *elem;
    while (!SLIST_EMPTY(&head)) {
        elem = SLIST_FIRST(&head);
        pthread_join(elem->thread_id, NULL);
        SLIST_REMOVE_HEAD(&head, entries);
        free(elem);
    }
    pthread_mutex_destroy(&file_mtx);
    remove(PATHFILE);
    closelog();
}

