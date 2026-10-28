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
    int server_fd, client_fd;

    struct sockaddr_in server, client;
    socklen_t addrlen = sizeof(client);

    Frame frame;

    int packets;

    int received[MAX_FRAMES] = {0};

    /*
     * Keeps track of whether a -ve ACK
     * has already been sent for a packet.
     */
    int nackSent[MAX_FRAMES] = {0};

    int expected = 0;

    /* ==============================
       CREATE SOCKET
       ============================== */

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket error");
        exit(1);
    }

    int opt = 1;

    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    /* ==============================
       BIND
       ============================== */

    if (bind(server_fd,
             (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind error");
        close(server_fd);
        exit(1);
    }

    /* ==============================
       LISTEN
       ============================== */

    listen(server_fd, 5);

    printf("=====================================\n");
    printf("      SELECTIVE REPEAT SERVER\n");
    printf("=====================================\n");

    printf("Waiting for client...\n");

    /* ==============================
       ACCEPT
       ============================== */

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client,
                       &addrlen);

    if (client_fd < 0)
    {
        perror("Accept error");
        close(server_fd);
        exit(1);
    }

    printf("Client connected.\n\n");

    /* ==============================
       RECEIVE NUMBER OF PACKETS
       ============================== */

    recv(client_fd,
         &packets,
         sizeof(packets),
         0);

    /* ==============================
       RECEIVE CONNECTION REQUEST
       ============================== */

    recv(client_fd,
         &frame,
         sizeof(frame),
         0);

    if (frame.type == U_FRAME &&
        frame.control == SABM)
    {
        printf("Connection request received.\n");

        memset(&frame, 0, sizeof(frame));

        frame.type = U_FRAME;
        frame.control = UA;

        send(client_fd,
             &frame,
             sizeof(frame),
             0);

        printf("Connection established.\n");
    }

    /* ==============================
       RECEIVE DATA
       ============================== */

    while (1)
    {
        memset(&frame, 0, sizeof(frame));

        int bytes = recv(client_fd,
                         &frame,
                         sizeof(frame),
                         0);

        if (bytes <= 0)
            break;

        /* ==============================
           CONNECTION TERMINATION
           ============================== */

        if (frame.type == U_FRAME &&
            frame.control == DISC)
        {
            printf("\nConnection termination request received.\n");

            memset(&frame, 0, sizeof(frame));

            frame.type = U_FRAME;
            frame.control = UA;

            send(client_fd,
                 &frame,
                 sizeof(frame),
                 0);

            printf("Connection terminated.\n");

            break;
        }

        /* ==============================
           I-FRAME
           ============================== */

        if (frame.type == I_FRAME)
        {
            int seq = frame.seq;

            printf("\nPacket %d received.\n", seq);

            printf("Data: %s\n", frame.data);

            /* --------------------------
               Check duplicate
               -------------------------- */

            if (!received[seq])
            {
                received[seq] = 1;

                /*
                 * Packet has now arrived,
                 * so its previous -ve ACK
                 * condition is cleared.
                 */
                nackSent[seq] = 0;

                printf("Packet %d accepted.\n", seq);
            }
            else
            {
                printf("Duplicate packet %d received.\n",
                       seq);
            }

            /* --------------------------
               Check for missing packet
               -------------------------- */

            if (seq > expected &&
                !received[expected] &&
                !nackSent[expected])
            {
                printf("-ve ACK: Packet %d missing.\n",
                       expected);

                Frame nack;

                memset(&nack, 0, sizeof(nack));

                nack.type = S_FRAME;
                nack.control = SREJ;
                nack.seq = expected;

                send(client_fd,
                     &nack,
                     sizeof(nack),
                     0);

                /*
                 * Prevent sending the same
                 * -ve ACK repeatedly.
                 */
                nackSent[expected] = 1;
            }

            /* --------------------------
               Send ACK for received packet
               -------------------------- */

            Frame ack;

            memset(&ack, 0, sizeof(ack));

            ack.type = S_FRAME;
            ack.control = RR;
            ack.seq = seq;

            send(client_fd,
                 &ack,
                 sizeof(ack),
                 0);

            printf("ACK: Packet %d\n", seq);

            /* --------------------------
               Move expected forward
               -------------------------- */

            while (expected < packets &&
                   received[expected])
            {
                expected++;
            }
        }
    }

    printf("\nAll communication completed.\n");

    close(client_fd);
    close(server_fd);

    return 0;
}