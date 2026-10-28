#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    float tp, tt;
    int packets;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

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

    send(sock, &packets, sizeof(packets), 0);

    float rtt = 2 * (tp + tt);

    /*
       Start clock at t = 1
    */
    float current_time = 1;

    printf("\n========== STOP AND WAIT SIMPLEX ==========\n");

    printf("RTT = 2 * (tp + tt)\n");
    printf("RTT = 2 * (%.2f + %.2f) = %.2f ms\n\n",
           tp, tt, rtt);

    for (int i = 1; i <= packets; i++)
    {
        int packet = i;
        int ack;

        float start = current_time;

        printf("Packet %d\n", i);

        printf("  Sending Packet %d at %.2f ms\n",
               i, start);

        send(sock, &packet, sizeof(packet), 0);

        recv(sock, &ack, sizeof(ack), 0);

        /*
           A successful packet takes exactly 1 RTT
           according to our timing model.
        */

        current_time += rtt;

        printf("  ACK %d received at %.2f ms\n",
               ack, current_time);

        printf("  Time taken = %.2f ms\n\n", rtt);
    }

    printf("===========================================\n");
    printf("Start time              : 1 ms\n");
    printf("Final time              : %.2f ms\n",
           current_time);
    printf("Total transmission time : %.2f ms\n",
           current_time - 1);
    printf("===========================================\n");

    close(sock);

    return 0;
}