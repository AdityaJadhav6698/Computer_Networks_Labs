#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_PACKETS 100

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind error");
        exit(1);
    }

    listen(server_fd, 5);

    printf("Waiting for sender...\n");

    addr_len = sizeof(client_addr);

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &addr_len);

    printf("Sender connected.\n\n");

    int packets;
    int lost_count;
    int lost[MAX_PACKETS];

    recv(client_fd, &packets, sizeof(packets), 0);

    recv(client_fd, &lost_count, sizeof(lost_count), 0);

    recv(client_fd,
         lost,
         sizeof(int) * lost_count,
         0);

    /*
       first_attempt[i] = 1 means that the first
       transmission of packet i should be lost.
    */

    int first_attempt[MAX_PACKETS] = {0};

    for (int i = 0; i < lost_count; i++)
    {
        first_attempt[lost[i]] = 1;
    }

    printf("Number of packets : %d\n", packets);
    printf("Lost packets      : ");

    for (int i = 0; i < lost_count; i++)
    {
        printf("%d ", lost[i]);
    }

    printf("\n\n");

    while (1)
    {
        int packet;

        int bytes = recv(client_fd,
                         &packet,
                         sizeof(packet),
                         0);

        if (bytes <= 0)
            break;

        printf("Received Packet %d\n", packet);

        /*
           If this packet is specified as lost,
           drop ONLY its first transmission.

           The retransmission will be accepted.
        */

        if (first_attempt[packet] == 1)
        {
            printf("Packet %d LOST at receiver\n",
                   packet);

            first_attempt[packet] = 0;

            /*
               No ACK is sent.
            */

            continue;
        }

        /*
           Packet successfully received.
        */

        printf("Packet %d accepted\n", packet);

        int ack = packet;

        send(client_fd,
             &ack,
             sizeof(ack),
             0);

        printf("ACK %d sent\n\n", ack);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}

