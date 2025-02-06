#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

// Client Structure
typedef struct {
  struct sockaddr_in address;
  int sockfd;
  int uid; // unique for every client
  char name[NAME_LEN];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout() {
  printf("\r%s", ">");
  fflush(stdout);
}

void str_trim_lf(char *arr, int len) {
  for (int i = 0; i < len; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

// adding customers in queue
void queue_add(client_t *cl) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!clients[i]) {
      clients[i] = cl;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// remving from queue
void queue_remove(int uid) {
  pthread_mutex_lock(&clients_mutex);

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i]) {
      if (clients[i]->uid == uid) {
        clients[i] = NULL;
        break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// sending message to all the clients except sender itsellf
void send_message(char *s, int uid) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i]) {
      if (clients[i]->uid != uid) // not the sender itself
      {
        if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
          printf("ERROR: Write to descriptor failed\n");
          break;
        }
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
  char buffer[BUFFER_SZ];
  char name[NAME_LEN];
  int leave_flag = 0;

  cli_count++;
  client_t *cli = (client_t *)arg;

  // Name:
  if (recv(cli->sockfd, name, NAME_LEN, 0) == 0 || strlen(name) < 2 ||
      strlen(name) == NAME_LEN - 1) {
    printf("ERROR: Enter the name correctly\n");
    leave_flag = 1;
  } else {
    strcpy(cli->name, name);
    sprintf(buffer, "%s has joined\n", cli->name);
    printf("%s", buffer);
    send_message(buffer, cli->uid);
  }

  bzero(buffer, BUFFER_SZ);
  while (1) {
    if (leave_flag) {
      break;
    }
    int receive = recv(cli->sockfd, buffer, BUFFER_SZ, 0);

    if (receive > 0) {
      if (strlen(buffer) > 0){
      send_message(buffer, cli->uid);
      str_trim_lf(buffer, strlen(buffer));
      printf("%s -> %s\n", buffer, cli->name);
    } 
    }
    else if (receive == 0 || strcmp(buffer, "exit") == 0) {
      sprintf(buffer, "%s has left\n", cli->name);
      printf("%s", buffer);
      send_message(buffer, cli->uid);
      leave_flag = 1;
    } else {
      printf("ERROR:-1\n");
      leave_flag = 1;
    }
    bzero(buffer, BUFFER_SZ);
  }
  close(cli->sockfd);
  queue_remove(cli->uid);
  free(cli);
  cli_count--;
  pthread_detach(pthread_self());
  return NULL;
}

void print_ip_addr(struct sockaddr_in addr) {
  printf("%d.%d.%d.%d\n", addr.sin_addr.s_addr & 0xff,
         (addr.sin_addr.s_addr & 0xff00) >> 8,
         (addr.sin_addr.s_addr & 0xff0000) >> 16,
         (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return 1;
  }
  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);
  int option = 1;
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  // socket settings
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  // Signals
  signal(SIGPIPE, SIG_IGN);
  if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEADDR | SO_REUSEPORT),
                 (char *)&option, sizeof(option)) < 0) {
    perror("Error setting socket options");
    return 1;
  }
  // Bind
  if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
    printf("ERROR: Socket binding failed\n");
    return 1;
  }

  // Listen
  if (listen(listenfd, 10) < 0) {
    printf("ERROR:Listen failed\n");
    return 1;
  }
  printf("Server started\n");
  printf("===========WELCOME TO THE CHATROOM===========\n");

  while (1) {
    socklen_t clilen = sizeof(cli_addr);
    connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

    // Check for max_clients

    if ((cli_count + 1) == MAX_CLIENTS) {
      printf("Max clients reached. Connection rejected : ");
      print_ip_addr(cli_addr);
      close(connfd);
      continue;
    }

    // client settings
    client_t *cli = (client_t *)malloc(sizeof(client_t));
    cli->address = cli_addr;
    cli->sockfd = connfd;
    cli->uid = uid++;

    // Add client to queue
    queue_add(cli);
    pthread_create(&tid, NULL, &handle_client, (void *)cli);
    // reduce cpu usage
    sleep(1);
  }
  return 0; // successful exit
}
