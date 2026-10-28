#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_FRAMES 100

#define I_FRAME 'I'
#define S_FRAME 'S'
#define U_FRAME 'U'

#define RR 1
#define SREJ 2

#define SABM 1
#define UA 2
#define DISC 3

typedef struct
{
    char type;
    int control;
    int seq;
    char data[100];
} Frame;

int main()
{
    int sock;
    struct sockaddr_in server;

    Frame frame;

    int bits;
    int window_size;
    int packets;

    int lost[MAX_FRAMES] = {0};
    int acked[MAX_FRAMES] = {0};

    int lost_count;

    int retransmissions = 0;

    /* ==============================
       INPUT
       ============================== */

    printf("=====================================\n");
    printf("      SELECTIVE REPEAT ARQ\n");
    printf("=====================================\n");

    printf("Enter number of bits: ");
    scanf("%d", &bits);

    window_size = 1 << (bits - 1);

    printf("Window size = 2^(%d-1) = %d\n",
           bits,
           window_size);

    printf("\nEnter number of packets: ");
    scanf("%d", &packets);

    printf("Enter number of lost packets: ");
    scanf("%d", &lost_count);

    printf("Enter lost packet numbers (0-based):\n");

    for (int i = 0; i < lost_count; i++)
    {
        int x;

        scanf("%d", &x);

        if (x >= 0 && x < packets)
        {
            lost[x] = 1;
        }
    }

    /* ==============================
       CREATE SOCKET
       ============================== */

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* ==============================
       CONNECT
       ============================== */

    if (connect(sock,
                (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        perror("Connection error");
        close(sock);
        exit(1);
    }

    printf("\nConnected to server.\n");

    /* ==============================
       SEND NUMBER OF PACKETS
       ============================== */

    send(sock,
         &packets,
         sizeof(packets),
         0);

    /* ==============================
       CONNECTION REQUEST
       ============================== */

    memset(&frame, 0, sizeof(frame));

    frame.type = U_FRAME;
    frame.control = SABM;

    send(sock,
         &frame,
         sizeof(frame),
         0);

    recv(sock,
         &frame,
         sizeof(frame),
         0);

    if (frame.type == U_FRAME &&
        frame.control == UA)
    {
        printf("Connection established.\n");
    }

    /* ==============================
       SELECTIVE REPEAT
       ============================== */

    int base = 0;

    while (base < packets)
    {
        int end = base + window_size - 1;

        if (end >= packets)
            end = packets - 1;

        printf("\n-------------------------------------\n");
        printf("Window: %d to %d\n",
               base,
               end);
        printf("-------------------------------------\n");

        /* ==============================
           SEND CURRENT WINDOW
           ============================== */

        for (int i = base; i <= end; i++)
        {
            if (lost[i])
            {
                printf("Packet %d is LOST.\n", i);
                continue;
            }

            memset(&frame, 0, sizeof(frame));

            frame.type = I_FRAME;
            frame.seq = i;

            sprintf(frame.data,
                    "DATA%d",
                    i);

            send(sock,
                 &frame,
                 sizeof(frame),
                 0);

            printf("Packet %d sent.\n", i);
        }

        /*
         * Count packets that were actually
         * sent initially.
         */
        int normal_frames = 0;

        for (int i = base; i <= end; i++)
        {
            if (!lost[i])
                normal_frames++;
        }

        int responses = 0;

        /* ==============================
           RECEIVE ACK / -ve ACK
           ============================== */

        while (responses < normal_frames)
        {
            memset(&frame, 0, sizeof(frame));

            recv(sock,
                 &frame,
                 sizeof(frame),
                 0);

            /* --------------------------
               Positive ACK
               -------------------------- */

            if (frame.type == S_FRAME &&
                frame.control == RR)
            {
                int seq = frame.seq;

                printf("ACK received for packet %d\n",
                       seq);

                /*
                 * Count only the first ACK
                 * for a packet that was sent
                 * normally.
                 */
                if (!acked[seq])
                {
                    acked[seq] = 1;

                    /*
                     * If this packet was originally
                     * lost, its ACK should not be
                     * counted here.
                     */
                    if (!lost[seq])
                    {
                        responses++;
                    }
                }
            }

            /* --------------------------
               Negative ACK
               -------------------------- */

            else if (frame.type == S_FRAME &&
                     frame.control == SREJ)
            {
                int missing = frame.seq;

                printf("-ve ACK received for packet %d\n",
                       missing);

                /*
                 * Retransmit only if it has
                 * not already been acknowledged.
                 */
                if (!acked[missing])
                {
                    memset(&frame, 0, sizeof(frame));

                    frame.type = I_FRAME;
                    frame.seq = missing;

                    sprintf(frame.data,
                            "DATA%d",
                            missing);

                    send(sock,
                         &frame,
                         sizeof(frame),
                         0);

                    retransmissions++;

                    printf("Retransmitting packet %d\n",
                           missing);
                }
            }
        }

        /* ==============================
           HANDLE LOST PACKETS
           ============================== */

        /*
         * If a lost packet was at the END
         * of the window, the server could not
         * detect it because no later packet
         * arrived.
         *
         * Therefore retransmit any lost
         * packet which still has no ACK.
         */

        for (int i = base; i <= end; i++)
        {
            if (lost[i] && !acked[i])
            {
                memset(&frame, 0, sizeof(frame));

                frame.type = I_FRAME;
                frame.seq = i;

                sprintf(frame.data,
                        "DATA%d",
                        i);

                send(sock,
                     &frame,
                     sizeof(frame),
                     0);

                retransmissions++;

                printf("Retransmitting lost packet %d\n",
                       i);
            }
        }

        /* ==============================
           RECEIVE ACKS FOR RETRANSMITTED
           LOST PACKETS
           ============================== */

        int lost_remaining = 0;

        for (int i = base; i <= end; i++)
        {
            if (lost[i] && !acked[i])
            {
                lost_remaining++;
            }
        }

        while (lost_remaining > 0)
        {
            memset(&frame, 0, sizeof(frame));

            recv(sock,
                 &frame,
                 sizeof(frame),
                 0);

            if (frame.type == S_FRAME &&
                frame.control == RR)
            {
                int seq = frame.seq;

                printf("ACK received for packet %d\n",
                       seq);

                if (!acked[seq])
                {
                    acked[seq] = 1;

                    if (lost[seq])
                    {
                        lost_remaining--;
                    }
                }
            }

            /*
             * Ignore duplicate -ve ACKs here.
             * The lost packet has already been
             * retransmitted.
             */
        }

        printf("\nWindow %d to %d completed.\n",
               base,
               end);

        base = end + 1;
    }

    /* ==============================
       CONNECTION TERMINATION
       ============================== */

    memset(&frame, 0, sizeof(frame));

    frame.type = U_FRAME;
    frame.control = DISC;

    send(sock,
         &frame,
         sizeof(frame),
         0);

    recv(sock,
         &frame,
         sizeof(frame),
         0);

    printf("\nConnection closed.\n");

    /* ==============================
       SUMMARY
       ============================== */

    printf("\n=====================================\n");
    printf("       TRANSMISSION SUMMARY\n");
    printf("=====================================\n");

    printf("Number of bits       : %d\n",
           bits);

    printf("Window size          : %d\n",
           window_size);

    printf("Total packets        : %d\n",
           packets);

    printf("Total retransmissions: %d\n",
           retransmissions);

    printf("=====================================\n");

    close(sock);

    return 0;
}