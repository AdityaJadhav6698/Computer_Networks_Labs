
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_PACKETS 100

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

int main(void) {
  int sock;
  struct sockaddr_in addr;
  float tp, tt;
  int packets, lost_count;
  int lost[MAX_PACKETS];
  int lost_flag[MAX_PACKETS + 1] = {0};

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket");
    return 1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Connect");
    close(sock);
    return 1;
  }

  printf("Enter propagation time (tp): ");
  if (scanf("%f", &tp) != 1 || tp < 0)
    return 1;

  printf("Enter transmission time (tt): ");
  if (scanf("%f", &tt) != 1 || tt < 0)
    return 1;

  printf("Enter total number of packets: ");
  if (scanf("%d", &packets) != 1 || packets < 1 || packets > MAX_PACKETS)
    return 1;

  printf("Enter number of lost packets: ");
  if (scanf("%d", &lost_count) != 1 || lost_count < 0 || lost_count > packets)
    return 1;

  printf("Enter lost packet numbers:\n");

  for (int i = 0; i < lost_count; i++) {
    if (scanf("%d", &lost[i]) != 1 || lost[i] < 1 || lost[i] > packets ||
        lost_flag[lost[i]]) {
      printf("Invalid or duplicate packet number.\n");
      close(sock);
      return 1;
    }
    lost_flag[lost[i]] = 1;
  }

  if (!send_int(sock, packets) || !send_int(sock, lost_count)) {
    perror("Send configuration");
    close(sock);
    return 1;
  }

  for (int i = 0; i < lost_count; i++) {
    if (!send_int(sock, lost[i])) {
      perror("Send lost list");
      close(sock);
      return 1;
    }
  }

  float rtt = 2.0f * (tp);
  float timeout = 3.0f * rtt;
  float current_time = 1.0f;
  int retransmissions = 0;

  printf("\n========== STOP-AND-WAIT ARQ ==========\n");
  printf("tp = %.2f ms, tt = %.2f ms\n", tp, tt);
  printf("RTT = %.2f ms\n", rtt);
  printf("Timeout = %.2f ms\n", timeout);
  printf("ACK send delay = 1 ms (simulation)\n");

  for (int i = 1; i <= packets; i++) {
    int ack;

    printf("\n--------------------------------------\n");
    printf("PACKET %d\n", i);
    printf("--------------------------------------\n");

    float send_time = current_time;
    float receive_time = send_time + tp;
    float ack_send_time = receive_time + 1.0f;
    float ack_receive_time = ack_send_time + tp;

    printf("[%.2f ms] Sender: Send Packet %d\n", send_time, i);

    if (!send_int(sock, i)) {
      perror("Send packet");
      break;
    }

    if (!lost_flag[i]) {
      if (!recv_int(sock, &ack)) {
        printf("Connection closed before ACK.\n");
        break;
      }

      printf("[%.2f ms] Receiver: Receive Packet %d\n", receive_time, i);
      printf("[%.2f ms] Receiver: Send ACK %d\n", ack_send_time, i);
      printf("[%.2f ms] Sender: Receive ACK %d\n", ack_receive_time, ack);

      current_time = ack_receive_time + tt;
    } else {
      /*
       * First attempt is dropped by the server.
       * No ACK arrives; wait for timeout.
       */
      printf("[%.2f ms] Channel: Packet %d LOST\n", receive_time, i);

      float timeout_time = send_time + timeout;
      printf("[%.2f ms] Sender: Timeout\n", timeout_time);

      printf("[%.2f ms] Sender: Retransmit Packet %d\n", timeout_time, i);

      if (!send_int(sock, i)) {
        perror("Retransmission");
        break;
      }

      float retry_receive = timeout_time + tp;
      float retry_ack_send = retry_receive + 1.0f;
      float retry_ack_receive = retry_ack_send + tp;

      if (!recv_int(sock, &ack)) {
        printf("Connection closed before retransmission ACK.\n");
        break;
      }

      printf("[%.2f ms] Receiver: Receive retransmitted Packet %d\n",
             retry_receive, i);
      printf("[%.2f ms] Receiver: Send ACK %d\n", retry_ack_send, i);
      printf("[%.2f ms] Sender: Receive ACK %d\n", retry_ack_receive, ack);

      current_time = retry_ack_receive + tt;
      retransmissions++;
    }

    printf("Packet %d completed at %.2f ms\n", i, current_time);
  }

  printf("\n========== FINAL RESULT ==========\n");
  printf("Packets: %d\n", packets);
  printf("Configured lost packets: %d\n", lost_count);
  printf("Retransmissions: %d\n", retransmissions);
  printf("Final simulated time: %.2f ms\n", current_time);
  printf("Elapsed simulated time: %.2f ms\n", current_time - 1.0f);

  close(sock);
  return 0;
}
