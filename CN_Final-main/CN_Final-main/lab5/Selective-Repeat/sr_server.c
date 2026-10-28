#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_PACKETS 1000

typedef struct {
    int window, total;
    int lost_count;
    int lost_packets[MAX_PACKETS];
    double tp, tt;
} Setup;

typedef struct {
    int packet, type;
    double send_start;
} Packet;

typedef struct {
    int ack;
} Response;

int recv_all(int fd, void *buf, size_t len) {
    char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = recv(fd, p + done, len - done, 0);
        if (n <= 0) return 0;
        done += n;
    }
    return 1;
}

int send_all(int fd, const void *buf, size_t len) {
    const char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = send(fd, p + done, len - done, 0);
        if (n <= 0) return 0;
        done += n;
    }
    return 1;
}

int main(void) {
    int server_fd, client_fd;
    Setup s;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(server_fd, 1) < 0) {
        perror("bind/listen"); close(server_fd); return 1;
    }

    printf("Waiting for SR sender...\n");

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) { perror("accept"); close(server_fd); return 1; }

    if (!recv_all(client_fd, &s, sizeof(s))) {
        close(client_fd); close(server_fd); return 1;
    }

    printf("\nSR Window size: %d\n", s.window);
    printf("Total packets: %d\n", s.total);
    printf("Propagation time: %.3f\n", s.tp);
    printf("Transmission time: %.3f\n\n", s.tt);

    int first_attempt[MAX_PACKETS] = {0};
    int received[MAX_PACKETS] = {0};

    // Mark packets designated for loss
    for (int i = 0; i < s.lost_count; i++) {
        if (s.lost_packets[i] < MAX_PACKETS) {
            first_attempt[s.lost_packets[i]] = 1;
        }
    }

    while (1) {
        Packet p;
        Response r;

        if (!recv_all(client_fd, &p, sizeof(p))) break;
        if (p.packet == -1) break;

        double arrival_start = p.send_start + s.tp;
        double arrival_end = p.send_start + s.tt + s.tp;
        double ack_send_time = arrival_end;

        // Apply loss logic if it's the first time seeing this flagged packet
        if (p.type == 1 && first_attempt[p.packet] == 1) {
            printf("Receiver: Packet %d LOST on first attempt; arrival time=%.3f\n\n", p.packet, arrival_end);
            first_attempt[p.packet] = 0; // It will succeed next time
            r.ack = -2; // Signal client event loop to drop it
        } else {
            printf("Receiver: Packet %d arrives from t=%.3f to t=%.3f\n", p.packet, arrival_start, arrival_end);
            
            if (received[p.packet] == 0) {
                received[p.packet] = 1;
                printf("Receiver: Packet %d ACCEPTED/BUFFERED\n", p.packet);
            } else {
                printf("Receiver: Packet %d DISCARDED (Duplicate)\n", p.packet);
            }

            r.ack = p.packet; // Selective Repeat ACKs the specific packet directly
            printf("Receiver: ACK %d sent at t=%.3f\n\n", r.ack, ack_send_time);
        }

        if (!send_all(client_fd, &r, sizeof(r))) break;
    }

    printf("Receiver simulation completed.\n");

    close(client_fd);
    close(server_fd);
    return 0;
}