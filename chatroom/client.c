#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LENGTH 2048
#define WIDTH 50 
// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];

void clearScreen() {
    printf("\033[2J\033[H");  // Clear screen and move cursor to home
}

// Function to create a border
void printBorder() {
    printf("\033[1;31m");  // Red color for the border
    for (int i = 0; i < WIDTH + 2; i++) {
        printf("-");
    }
    printf("\n\033[0m");  // Reset to normal color
}

// Function to scroll "WELCOME" with colorful bold borders
void scrollWelcome() {
    char message[] = "   WELCOME   ";
    int len = strlen(message);
    
    char display[WIDTH + 1];
    memset(display, ' ', WIDTH);
    display[WIDTH] = '\0';

    int pos = 0;
    int x=0;
    while (x!=10) {
        clearScreen();

        // Print the border
        printBorder();

        // Shift characters for the rolling effect
        for (int i = 0; i < WIDTH; i++) {
            display[i] = message[(pos + i) % len];
        }

        // Print the message in bold and with different colors
        printf("\033[1;33m");  // Yellow color
        printf("|");
        printf("\033[1;34m");  // Blue color
        printf("%s", display);
        printf("\033[1;32m");  // Green color
        printf("|");
        printf("\n\033[0m");     // Reset color

        // Print the bottom border
        printBorder();

        usleep(150000);  // Delay for smooth scrolling (adjust if needed)
        pos = (pos + 1) % len;  // Move position
        x++;
    }
}

void str_overwrite_stdout() {
  printf("You%s", "> ");
  fflush(stdout);
}

void str_trim_lf(char *arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void catch_ctrl_c_and_exit(int sig) { flag = 1; }

void send_msg_handler() {
  char message[LENGTH] = {};
  char buffer[LENGTH + 32] = {};

  while (1) {
    str_overwrite_stdout();
    fgets(message, LENGTH, stdin);
    str_trim_lf(message, LENGTH);

    if (strcmp(message, "exit") == 0) {
      break;
    } else {
      snprintf(buffer, sizeof(buffer), "%s: %s\n", name, message);
      send(sockfd, buffer, strlen(buffer), 0);
    }

    bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
  catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() {
  char message[LENGTH] = {};
  while (1) {
    int receive = recv(sockfd, message, LENGTH, 0);
    if (receive > 0) {
      printf("%s", message);
      str_overwrite_stdout();
    } else if (receive == 0) {
      break;
    } else {
      // -1
    }
    memset(message, 0, sizeof(message));
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

  signal(SIGINT, catch_ctrl_c_and_exit);

  printf("\n=== Welcome to the Chatroom Client ===\n");
  printf("Please enter your name (between 2 and 32 characters): ");
  fgets(name, 32, stdin);
  str_trim_lf(name, strlen(name));

  // Check name length
  if (strlen(name) > 32 || strlen(name) < 2) {
    printf("Error: Name must be between 2 and 32 characters.\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server_addr;

  /* Socket settings */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  // Connect to Server
  int err =
      connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
    printf("ERROR: Unable to connect to the server.\n");
    return EXIT_FAILURE;
  }

  // Send name
  send(sockfd, name, 32, 0);

  // Welcome message
  scrollWelcome();
  printf("You are now connected as '%s'. You can start chatting now.\n", name);
  printf("Type 'exit' to leave the chatroom at any time.\n");
  printf("Have fun chatting with others!\n\n");

  pthread_t send_msg_thread;
  if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) !=
      0) {
    printf("ERROR: Unable to create send message thread.\n");
    return EXIT_FAILURE;
  }

  pthread_t recv_msg_thread;
  if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) !=
      0) {
    printf("ERROR: Unable to create receive message thread.\n");
    return EXIT_FAILURE;
  }

  while (1) {
    if (flag) {
      printf("\nBye! You have left the chatroom.\n");
      break;
    }
  }

  close(sockfd);

  return EXIT_SUCCESS;
}

