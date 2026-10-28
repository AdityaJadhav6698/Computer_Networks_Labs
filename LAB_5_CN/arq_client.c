#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_PACKETS 100

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    float tp, tt;
    int packets;
    int lost_count;
    int lost[MAX_PACKETS];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection error");
        exit(1);
    }

    printf("Enter propogation time (tp): ");
    scanf("%f", &tp);

    printf("Enter transmission time (tt): ");
    scanf("%f", &tt);

    printf("Enter number of packets: ");
    scanf("%d", &packets);

    printf("Enter number of lost packets: ");
    scanf("%d", &lost_count);

    printf("Enter lost packet numbers:\n");

    for (int i = 0; i < lost_count; i++)
    {
        scanf("%d", &lost[i]);
    }

    /*
       Send information to server.
    */

    send(sock,
         &packets,
         sizeof(packets),
         0);

    send(sock,
         &lost_count,
         sizeof(lost_count),
         0);

    send(sock,
         lost,
         sizeof(int) * lost_count,
         0);

    /*
       Calculate RTT.
    */

    float rtt = 2 * (tp + tt);

    /*
       Clock starts at t = 1 ms.
    */

    float current_time = 1;

    int retransmissions = 0;

    printf("\n============================================\n");
    printf("       STOP AND WAIT ARQ - NOISY CHANNEL\n");
    printf("============================================\n");

    printf("tp  = %.2f ms\n", tp);
    printf("tt  = %.2f ms\n", tt);
    printf("RTT = 2 * (tp + tt)\n");
    printf("RTT = %.2f ms\n", rtt);

    printf("\nTiming rules:\n");
    printf("Normal packet = 1 RTT\n");
    printf("Lost packet   = 3 RTT timeout + 1 RTT retransmission\n");
    printf("Lost packet   = 4 RTT\n");

    printf("\n--------------------------------------------\n");

    for (int i = 1; i <= packets; i++)
    {
        int is_lost = 0;

        /*
           Check whether packet i is lost.
        */

        for (int j = 0; j < lost_count; j++)
        {
            if (lost[j] == i)
            {
                is_lost = 1;
                break;
            }
        }

        int packet = i;
        int ack;

        /*
           -------------------------
           FIRST TRANSMISSION
           -------------------------
        */

        float start = current_time;

        printf("\nPacket %d\n", i);

        printf("  Sending Packet %d at %.2f ms\n",
               i, current_time);

        send(sock,
             &packet,
             sizeof(packet),
             0);

        if (!is_lost)
        {
            /*
               Normal packet:
               1 RTT
            */

            recv(sock,
                 &ack,
                 sizeof(ack),
                 0);

            current_time += rtt;

            printf("  ACK %d received at %.2f ms\n",
                   ack, current_time);

            printf("  Packet time = %.2f ms\n",
                   rtt);
        }
        else
        {
            /*
               LOST PACKET
               
               Timer starts from the instant
               the lost packet is sent.

               Wait 3 RTT.
            */

            float timeout = 3 * rtt;

            current_time += timeout;

            printf("  Packet %d LOST\n", i);

            printf("  Waiting 3 RTT = %.2f ms\n",
                   timeout);

            printf("  Timeout at %.2f ms\n",
                   current_time);

            /*
               Retransmission
            */

            printf("  Retransmitting Packet %d at %.2f ms\n",
                   i, current_time);

            send(sock,
                 &packet,
                 sizeof(packet),
                 0);

            recv(sock,
                 &ack,
                 sizeof(ack),
                 0);

            /*
               Retransmitted packet takes
               1 RTT.
            */

            current_time += rtt;

            retransmissions++;

            printf("  ACK %d received at %.2f ms\n",
                   ack, current_time);

            printf("  Lost packet time = 3 RTT + 1 RTT\n");

            printf("  Packet time = %.2f ms\n",
                   4 * rtt);
        }

        printf("  Started at %.2f ms\n", start);
        printf("  Finished at %.2f ms\n",
               current_time);
    }

    printf("\n============================================\n");
    printf("                 RESULT\n");
    printf("============================================\n");

    printf("Number of packets       : %d\n", packets);
    printf("Lost packets            : %d\n", lost_count);
    printf("Retransmissions         : %d\n", retransmissions);
    printf("RTT                     : %.2f ms\n", rtt);
    printf("Start time              : 1 ms\n");
    printf("Final time              : %.2f ms\n",
           current_time);
    printf("Total transmission time : %.2f ms\n",
           current_time - 1);

    printf("============================================\n");

    close(sock);

    return 0;
}