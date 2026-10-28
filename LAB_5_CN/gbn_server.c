#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int server_fd, client_fd;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t addr_len;

    int window_size;
    int total_packets;
    int loss_n;

    int expected = 1;

    /*
        Create socket
    */

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket error");
        exit(1);
    }

    /*
        Server address
    */

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /*
        Bind
    */

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind error");
        exit(1);
    }

    /*
        Listen
    */

    listen(server_fd, 5);

    printf("Waiting for sender...\n");

    /*
        Accept
    */

    addr_len = sizeof(client_addr);

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &addr_len);

    if (client_fd < 0)
    {
        perror("Accept error");
        exit(1);
    }

    printf("Sender connected.\n\n");

    /*
        Receive inputs
    */

    recv(client_fd,
         &window_size,
         sizeof(window_size), 0);

    recv(client_fd,
         &total_packets,
         sizeof(total_packets), 0);

    recv(client_fd,
         &loss_n,
         sizeof(loss_n), 0);

    printf("Window size   : %d\n",
           window_size);

    printf("Total packets : %d\n",
           total_packets);

    printf("Every nth lost: %d\n\n",
           loss_n);

    /*
        --------------------------------------------------
        RECEIVE PACKETS
        --------------------------------------------------
    */

    while (1)
    {
        int packet;
        int type;

        /*
            Receive packet number
        */

        int bytes = recv(client_fd,
                         &packet,
                         sizeof(packet), 0);

        if (bytes <= 0)
            break;

        /*
            End signal
        */

        if (packet == -1)
            break;

        /*
            Receive type

            1 = new packet
            2 = retransmission
        */

        recv(client_fd,
             &type,
             sizeof(type), 0);

        if (type == 1)
        {
            printf("Received Packet %d\n",
                   packet);
        }
        else
        {
            printf("Received Retransmitted Packet %d\n",
                   packet);
        }

        /*
            Correct packet
        */

        if (packet == expected)
        {
            printf("Packet %d accepted\n",
                   packet);

            expected++;

            /*
                Cumulative ACK
            */

            int ack = packet;

            send(client_fd,
                 &ack,
                 sizeof(ack), 0);

            printf("ACK %d sent\n\n",
                   ack);
        }
        else if (packet > expected)
        {
            /*
                Out-of-order packet.

                GBN receiver discards it.
            */

            printf("Packet %d DISCARDED\n",
                   packet);

            /*
                ACK last correctly received packet.
            */

            int ack = expected - 1;

            send(client_fd,
                 &ack,
                 sizeof(ack), 0);

            printf("Duplicate ACK %d sent\n\n",
                   ack);
        }
        else
        {
            /*
                Duplicate packet
            */

            printf("Duplicate Packet %d\n",
                   packet);

            int ack = packet;

            send(client_fd,
                 &ack,
                 sizeof(ack), 0);

            printf("ACK %d sent\n\n",
                   ack);
        }
    }

    printf("\nAll packets processed.\n");

    close(client_fd);
    close(server_fd);

    return 0;
}