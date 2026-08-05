#include "vehicle_tcp_server.h"
#include "vehicle_state.h"
#include "vehicle_state_store.h"

#include <pthread.h>
#include <stdatomic.h>

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

static pthread_t server_thread;
static atomic_bool server_running;

static int server_fd = -1;
static int client_fd = -1;

static uint16_t configured_port;

static void signal_handler(int signal_number) {
  (void)signal_number;

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
  vehicle_state_store_update(&state);
  vehicle_state_print(&state);
}

static void receive_client_data(int socket_fd) {
  char receive_buffer[RECEIVE_BUFFER_SIZE];
  char line_buffer[LINE_BUFFER_SIZE];
  size_t line_length = 0;

  while (atomic_load(&server_running)) {
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

static void *server_thread_main(void *arg) {
  (void)arg;
  // signal(SIGINT, signal_handler);
  // signal(SIGTERM, signal_handler);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("socket");
    return NULL;
  }

  int reuse_address = 1;

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
                 sizeof(reuse_address)) < 0) {
    perror("setsockopt");
    close(server_fd);
    return NULL;
  }

  struct sockaddr_in server_address = {.sin_family = AF_INET,
                                       .sin_port = htons(SERVER_PORT),
                                       .sin_addr.s_addr = htonl(INADDR_ANY)};

  if (bind(server_fd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    perror("bind");
    close(server_fd);
    return NULL;
  }

  if (listen(server_fd, 1) < 0) {
    perror("listen");
    close(server_fd);
    return NULL;
  }

  printf("Listening on 0.0.0.0:%d\n", SERVER_PORT);

  while (atomic_load(&server_running)) {
    struct sockaddr_in client_address;
    socklen_t client_length = sizeof(client_address);

    client_fd =
        accept(server_fd, (struct sockaddr *)&client_address, &client_length);

    if (client_fd < 0) {
      if (!atomic_load(&server_running)) {
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
  return NULL;
}

bool vehicle_tcp_server_start(uint16_t port) {
  if (atomic_load(&server_running)) {
    return true;
  }

  configured_port = port;
  atomic_store(&server_running, true);

  int result = pthread_create(&server_thread, NULL, server_thread_main, NULL);

  if (result != 0) {
    atomic_store(&server_running, false);
    return false;
  }

  return true;
}
void vehicle_tcp_server_stop(void) {
  if (!atomic_exchange(&server_running, false)) {
    return;
  }

  if (client_fd >= 0) {
    shutdown(client_fd, SHUT_RDWR);
  }

  if (server_fd >= 0) {
    shutdown(server_fd, SHUT_RDWR);
  }

  pthread_join(server_thread, NULL);

  client_fd = -1;
  server_fd = -1;
}
