#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5003
#define BUF_SIZE 128
#define MAX_PACKETS 1000000

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

/* Display the current sliding window */
void displayWindow(int base, int ws, int total, int seq_space)
{
    int j;

    printf("Window: [ ");

    for (j = base; j < base + ws && j < total; j++)
        printf("%d ", j % seq_space);

    printf("]\n");
}

int main(void)
{
    int sock, bits = 0, seq_space, ws, max_packets;
    int base, i, end, received;
    double tt, tp, a, max_value, total;

    struct sockaddr_in addr;
    socklen_t len;

    Packet p;
    Ack ack;

    printf("====================================================\n");
    printf("       SLIDING WINDOW PROTOCOL - NOISELESS\n");
    printf("====================================================\n");

    printf("Enter Tt (ms): ");
    if (scanf("%lf", &tt) != 1)
        return 1;

    printf("Enter Tp (ms): ");
    if (scanf("%lf", &tp) != 1)
        return 1;

    if (!isfinite(tt) || !isfinite(tp) ||
        tt <= 0 || tp < 0) {
        printf("Invalid input. Tt must be positive and Tp non-negative.\n");
        return 1;
    }

    /* Calculations from the notebook */
    a = tp / tt;
    max_value = 1.0 + 2.0 * a;

    if (!isfinite(max_value) || max_value > MAX_PACKETS) {
        printf("Calculated packet count is too large.\n");
        return 1;
    }

    max_packets = (int)floor(max_value + 1e-10);

    /*
       Minimum bits needed to represent max_packets
       distinct packet sequence numbers.
    */
    while ((1LL << bits) < max_packets)
        bits++;

    if (bits == 0)
        bits = 1;

    seq_space = 1 << bits;

    /* Window size from the notebook formula */
    ws = (max_packets < seq_space)  
             ? max_packets : seq_space;

    /* Pipelined total time */
    total = max_packets * tt + 2.0 * tp;

    printf("\nCALCULATIONS\n");
    printf("----------------------------------------------------\n");
    printf("a = Tp / Tt               = %.4f\n", a);
    printf("1 + 2a                    = %.4f\n", max_value);
    printf("Maximum packet count      = %d\n", max_packets);
    printf("Sequence number bits      = %d\n", bits);
    printf("Sequence number space     = %d\n", seq_space);
    printf("Window size (Ws)          = %d\n", ws);
    printf("Total theoretical time    = %.2f ms\n", total);
    printf("----------------------------------------------------\n");

    /* Initial array demonstration */
    printf("\nSLIDING WINDOW ARRAY DEMONSTRATION\n");
    printf("Initial ");
    displayWindow(0, ws, max_packets, seq_space);

    /* Create UDP socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    printf("\nTRANSMISSION AND ACKNOWLEDGEMENTS\n");
    printf("Ensure the receiver is running first.\n");

    for (base = 0; base < max_packets; base += ws) {

        end = base + ws;

        if (end > max_packets)
            end = max_packets;

        printf("\nSending window: packets %d to %d\n",
               base + 1, end);

        /* Send all packets in this window */
        for (i = base; i < end; i++) {
            memset(&p, 0, sizeof(p));

            p.packet_no = i + 1;
            p.seq = i % seq_space;
            p.tt = tt;
            p.tp = tp;

            snprintf(p.data, sizeof(p.data),
                     "SW-PACKET-%d", p.packet_no);

            printf("Sending Packet %d (Seq=%d)\n",
                   p.packet_no, p.seq);

            if (sendto(sock, &p, sizeof(p), 0,
                       (struct sockaddr *)&addr,
                       sizeof(addr)) < 0) {
                perror("sendto");
                close(sock);
                return 1;
            }
        }

        /* Receive one ACK for each packet in this window */
        received = 0;

        while (received < end - base) {
            len = sizeof(addr);

            if (recvfrom(sock, &ack, sizeof(ack), 0,
                         (struct sockaddr *)&addr, &len)
                < (ssize_t)sizeof(ack)) {
                perror("recvfrom");
                close(sock);
                return 1;
            }

            received++;

            printf("\nACK %d received for Packet %d\n",
                   ack.ack, ack.packet_no);

            printf("ACK arrival time = %.2f ms\n",
                   ack.packet_no * tt + 2.0 * tp);

            /*
               The next unacknowledged packet is at index
               ack.packet_no, because packet numbers start at 1.
            */
            printf("Updated ");
            displayWindow(ack.packet_no, ws,
                          max_packets, seq_space);
        }
    }

    printf("\n====================================================\n");
    printf("SUMMARY\n");
    printf("Maximum packet count   : %d\n", max_packets);
    printf("Sequence number bits   : %d\n", bits);
    printf("Sequence number space  : %d\n", seq_space);
    printf("Window size            : %d\n", ws);
    printf("Total theoretical time : %.2f ms\n", total);
    printf("All packets acknowledged; no packet loss.\n");
    printf("====================================================\n");

    close(sock);
    return 0;
}

