#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5003
#define BUF_SIZE 128

typedef struct {
    int seq;
    int packet_no;
    double tt;
    double tp;
    char data[BUF_SIZE];
} Packet;

typedef struct {
    int ack;
    int packet_no;
} Ack;

int main(void)
{
    int sock;
    struct sockaddr_in addr, client;
    socklen_t len;

    Packet p;
    Ack ack;

    printf("====================================================\n");
    printf("            SLIDING WINDOW RECEIVER\n");
    printf("====================================================\n");

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    printf("Listening on port %d...\n\n", PORT);

    while (1) {
        len = sizeof(client);

        ssize_t bytes = recvfrom(
            sock, &p, sizeof(p), 0,
            (struct sockaddr *)&client, &len
        );

        if (bytes < 0) {
            perror("recvfrom");
            continue;
        }

        if (bytes != (ssize_t)sizeof(p)) {
            printf("Invalid packet received.\n");
            continue;
        }

        /*
           Theoretical packet arrival time:
           Arrival = packet number * Tt + Tp
        */
        double arrival_time =
            p.packet_no * p.tt + p.tp;

        printf("\n--------------------------------------------\n");
        printf("PACKET RECEIVED AT RECEIVER\n");
        printf("--------------------------------------------\n");

        printf("Packet Number  : %d\n", p.packet_no);
        printf("Sequence Number: %d\n", p.seq);
        printf("Packet Data    : %s\n", p.data);
        printf("Tt             : %.2f ms\n", p.tt);
        printf("Tp             : %.2f ms\n", p.tp);

        printf("Packet Arrival Time: %.2f ms\n",
               arrival_time);

        /* Send ACK immediately */
        ack.ack = p.seq;
        ack.packet_no = p.packet_no;

        if (sendto(sock, &ack, sizeof(ack), 0,
                   (struct sockaddr *)&client, len)
            < (ssize_t)sizeof(ack)) {
            perror("sendto ACK");
            continue;
        }

        printf("ACK %d sent for Packet %d\n",
               ack.ack, ack.packet_no);

        printf("--------------------------------------------\n");

        fflush(stdout);
    }

    close(sock);
    return 0;
}
