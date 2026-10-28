
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

static int recv_int(int fd, int *value) {
  char *p = (char *)value;
  size_t left = sizeof(*value);

  while (left > 0) {
    ssize_t n = recv(fd, p, left, 0);
    if (n <= 0)
      return 0;
    p += n;
    left -= (size_t)n;
  }
  return 1;
}

static int send_int(int fd, int value) {
  char *p = (char *)&value;
  size_t left = sizeof(value);

  while (left > 0) {
    ssize_t n = send(fd, p, left, 0);
    if (n <= 0)
      return 0;
    p += n;
    left -= (size_t)n;
  }
  return 1;
}

int main(void) {
  int server_fd, client_fd;
  struct sockaddr_in server_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Socket error");
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind error");
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, 5) < 0) {
    perror("Listen error");
    close(server_fd);
    return 1;
  }

  printf("Waiting for sender...\n");

  client_fd = accept(server_fd, NULL, NULL);
  if (client_fd < 0) {
    perror("Accept error");
    close(server_fd);
    return 1;
  }

  int packets;

  if (!recv_int(client_fd, &packets) || packets < 1 || packets > 100000) {
    printf("Invalid packet count.\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }

  printf("Sender connected.\n");
  printf("Number of packets = %d\n\n", packets);

  for (int i = 1; i <= packets; i++) {
    int packet;

    if (!recv_int(client_fd, &packet)) {
      printf("Connection closed while receiving packet.\n");
      break;
    }

    printf("Received Packet %d\n", packet);

    int ack = packet;

    if (!send_int(client_fd, ack)) {
      perror("Send ACK");
      break;
    }

    printf("ACK %d sent\n\n", ack);
  }

  close(client_fd);
  close(server_fd);
  return 0;
}
