
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

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

int main(void) {
  int sock;
  struct sockaddr_in server_addr;
  float tp, tt;
  int packets;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket error");
    return 1;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connection error");
    close(sock);
    return 1;
  }

  printf("Enter propagation time (tp): ");
  if (scanf("%f", &tp) != 1 || tp < 0)
    return 1;

  printf("Enter transmission time (tt): ");
  if (scanf("%f", &tt) != 1 || tt < 0)
    return 1;

  printf("Enter number of packets: ");
  if (scanf("%d", &packets) != 1 || packets < 1 || packets > 100000)
    return 1;

  if (!send_int(sock, packets)) {
    perror("Send packet count");
    close(sock);
    return 1;
  }

  float rtt = 2.0f * (tp + tt);
  float current_time = 1.0f;

  printf("\n========== STOP-AND-WAIT SIMPLEX ==========\n");
  printf("tp = %.2f ms, tt = %.2f ms\n", tp, tt);
  printf("RTT = 2 * (tp + tt) = %.2f ms\n", rtt);
  printf("ACK send delay = 1 ms (simulation)\n\n");

  for (int i = 1; i <= packets; i++) {
    int ack;
    float send_time = current_time;
    float receive_time = send_time + tp;
    float ack_send_time = receive_time + 1.0f;
    float ack_receive_time = ack_send_time + tp;

    printf("--------------------------------------\n");
    printf("PACKET %d\n", i);
    printf("--------------------------------------\n");

    printf("[%.2f ms] Sender: Send Packet %d\n", send_time, i);

    if (!send_int(sock, i)) {
      perror("Send packet");
      break;
    }

    if (!recv_int(sock, &ack)) {
      printf("Connection closed before ACK.\n");
      break;
    }

    printf("[%.2f ms] Receiver: Receive Packet %d\n", receive_time, i);

    printf("[%.2f ms] Receiver: Send ACK %d\n", ack_send_time, i);

    printf("[%.2f ms] Sender: Receive ACK %d\n", ack_receive_time, ack);

    /*
       Continue the same timeline.
       Start the next packet tt ms after ACK arrival.
    */
    current_time = ack_receive_time + tt;

    printf("Packet %d completed at %.2f ms\n\n", i, current_time);
  }

  printf("===========================================\n");
  printf("Final simulated time : %.2f ms\n", current_time);
  printf("Elapsed simulated time: %.2f ms\n", current_time - 1.0f);
  printf("===========================================\n");

  close(sock);
  return 0;
}
