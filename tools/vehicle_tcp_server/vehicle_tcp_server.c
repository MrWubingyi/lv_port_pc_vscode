#include "vehicle_state.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 19090
#define RECEIVE_BUFFER_SIZE 1024
#define LINE_BUFFER_SIZE 4096

static volatile sig_atomic_t running = 1;
static int server_fd = -1;
static int client_fd = -1;

static void signal_handler(int signal_number) {
  (void)signal_number;
  running = 0;

  if (client_fd >= 0) {
    shutdown(client_fd, SHUT_RDWR);
  }

  if (server_fd >= 0) {
    shutdown(server_fd, SHUT_RDWR);
  }
}

static void process_vehicle_frame(const char *frame) {
  vehicle_state_t state;

  if (!vehicle_state_parse_json(frame, &state)) {
    fprintf(stderr, "Discarded frame: %s\n", frame);
    return;
  }

  vehicle_state_print(&state);
}

static void receive_client_data(int socket_fd) {
  char receive_buffer[RECEIVE_BUFFER_SIZE];
  char line_buffer[LINE_BUFFER_SIZE];
  size_t line_length = 0;

  while (running) {
    ssize_t received =
        recv(socket_fd, receive_buffer, sizeof(receive_buffer), 0);

    if (received == 0) {
      printf("Client disconnected\n");
      break;
    }

    if (received < 0) {
      if (errno == EINTR) {
        continue;
      }

      perror("recv");
      break;
    }

    for (ssize_t i = 0; i < received; i++) {
      char value = receive_buffer[i];

      if (value == '\n') {
        line_buffer[line_length] = '\0';

        if (line_length > 0) {
          process_vehicle_frame(line_buffer);
        }

        line_length = 0;
        continue;
      }

      if (value == '\r') {
        continue;
      }

      if (line_length >= sizeof(line_buffer) - 1) {
        fprintf(stderr, "Frame too long, discarded\n");
        line_length = 0;
        continue;
      }

      line_buffer[line_length++] = value;
    }
  }
}

int main(void) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }

  int reuse_address = 1;

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
                 sizeof(reuse_address)) < 0) {
    perror("setsockopt");
    close(server_fd);
    return EXIT_FAILURE;
  }

  struct sockaddr_in server_address = {.sin_family = AF_INET,
                                       .sin_port = htons(SERVER_PORT),
                                       .sin_addr.s_addr = htonl(INADDR_ANY)};

  if (bind(server_fd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    perror("bind");
    close(server_fd);
    return EXIT_FAILURE;
  }

  if (listen(server_fd, 1) < 0) {
    perror("listen");
    close(server_fd);
    return EXIT_FAILURE;
  }

  printf("Listening on 0.0.0.0:%d\n", SERVER_PORT);

  while (running) {
    struct sockaddr_in client_address;
    socklen_t client_length = sizeof(client_address);

    client_fd =
        accept(server_fd, (struct sockaddr *)&client_address, &client_length);

    if (client_fd < 0) {
      if (!running) {
        break;
      }

      perror("accept");
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));

    printf("Connected: %s:%u\n", client_ip, ntohs(client_address.sin_port));

    receive_client_data(client_fd);

    close(client_fd);
    client_fd = -1;
  }

  if (server_fd >= 0) {
    close(server_fd);
    server_fd = -1;
  }

  printf("Server stopped\n");
  return EXIT_SUCCESS;
}
