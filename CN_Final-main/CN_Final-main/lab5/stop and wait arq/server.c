
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_PACKETS 100

static int recv_int(int fd, int *value) {
  char *p = (char *)value;
  size_t left = sizeof(*value);

  while (left) {
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

  while (left) {
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
  struct sockaddr_in addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Socket");
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Bind");
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, 1) < 0) {
    perror("Listen");
    close(server_fd);
    return 1;
  }

  printf("Waiting for sender on port %d...\n", PORT);

  client_fd = accept(server_fd, NULL, NULL);
  if (client_fd < 0) {
    perror("Accept");
    close(server_fd);
    return 1;
  }

  int packets, lost_count;
  int lost[MAX_PACKETS];
  int drop_first[MAX_PACKETS + 1] = {0};

  if (!recv_int(client_fd, &packets) || !recv_int(client_fd, &lost_count) ||
      packets < 1 || packets > MAX_PACKETS || lost_count < 0 ||
      lost_count > packets) {
    printf("Invalid configuration.\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }

  for (int i = 0; i < lost_count; i++) {
    if (!recv_int(client_fd, &lost[i]) || lost[i] < 1 || lost[i] > packets) {
      printf("Invalid lost packet list.\n");
      close(client_fd);
      close(server_fd);
      return 1;
    }
    drop_first[lost[i]] = 1;
  }

  printf("\nSender connected.\n");
  printf("Total packets: %d\n", packets);
  printf("Lost packet numbers: ");

  if (lost_count == 0)
    printf("None");
  for (int i = 0; i < lost_count; i++)
    printf("%d ", lost[i]);
  printf("\n\n");

  int accepted = 0;

  while (accepted < packets) {
    int packet;

    if (!recv_int(client_fd, &packet))
      break;

    if (packet < 1 || packet > packets) {
      printf("Invalid packet received.\n");
      break;
    }

    printf("Received Packet %d\n", packet);

    if (drop_first[packet]) {
      printf("Packet %d LOST: first attempt dropped; no ACK\n\n", packet);
      drop_first[packet] = 0;
      continue;
    }

    printf("Packet %d accepted\n", packet);

    if (!send_int(client_fd, packet)) {
      perror("Send ACK");
      break;
    }

    printf("ACK %d sent\n\n", packet);
    accepted++;
  }

  printf("Server finished. Accepted packets: %d\n", accepted);

  close(client_fd);
  close(server_fd);
  return 0;
}
