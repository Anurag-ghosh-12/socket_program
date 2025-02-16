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

// Client Structure
typedef struct {
  struct sockaddr_in address;
  int sockfd;
  int uid;
  char name[NAME_LEN];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_trim_lf(char *arr, int len) {
  for (int i = 0; i < len; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

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

void queue_remove(int uid) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] && clients[i]->uid == uid) {
      clients[i] = NULL;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

char* get_clients_table() {
  static char table[BUFFER_SZ];
  memset(table, 0, BUFFER_SZ);
  strcat(table, "    Online users now:\n");
  strcat(table, "=============================\n");
  strcat(table, "|   UID   |     Name        |\n");
  strcat(table, "=============================\n");

  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i]) {
      char row[50];
      snprintf(row, sizeof(row), "|   %4d   |  %-12s  |\n", clients[i]->uid, clients[i]->name);
      strcat(table, row);
    }
  }
  pthread_mutex_unlock(&clients_mutex);

  strcat(table, "=============================\n");
  return table;
}

void send_message(char *s, int recipient_uid) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] && clients[i]->uid == recipient_uid) {
      if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
        printf("ERROR: Write to descriptor failed\n");
      }
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
  client_t *cli = (client_t *)arg;
  char buffer[BUFFER_SZ];
  
  snprintf(buffer, BUFFER_SZ, "%s has joined\n", cli->name);
  printf("%s", buffer);

  while (1) {
    char *table = get_clients_table();
    write(cli->sockfd, table, strlen(table));

    write(cli->sockfd, "Enter recipient UID: \n", 21);
    bzero(buffer, BUFFER_SZ);
    if (recv(cli->sockfd, buffer, BUFFER_SZ, 0) <= 0) {
      break;
    }
    int recipient_uid = atoi(buffer);

    write(cli->sockfd, "Enter message: \n", 15);
    bzero(buffer, BUFFER_SZ);
    if (recv(cli->sockfd, buffer, BUFFER_SZ, 0) <= 0) {
      break;
    }
    str_trim_lf(buffer, strlen(buffer));
    send_message(buffer, recipient_uid);
  }

  close(cli->sockfd);
  queue_remove(cli->uid);
  free(cli);
  pthread_exit(NULL);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return 1;
  }

  int port = atoi(argv[1]);
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t clilen = sizeof(cli_addr);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr("10.2.65.33");
  serv_addr.sin_port = htons(port);

  bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  listen(listenfd, 10);
  printf("Server started on port %d\n", port);

  while (1) {
    int client_sockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);
    if (client_sockfd < 0) {
      printf("ERROR: Accept failed\n");
      continue;
    }

    char name[NAME_LEN];
    if (recv(client_sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2) {
      printf("ERROR: Invalid name\n");
      close(client_sockfd);
      continue;
    }

    client_t *cli = (client_t *)malloc(sizeof(client_t));
    cli->sockfd = client_sockfd;
    cli->uid = client_sockfd;
    strcpy(cli->name, name);
    queue_add(cli);

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, (void *)cli) != 0) {
      printf("ERROR: Thread creation failed\n");
      free(cli);
    }
  }

  return 0;
}

