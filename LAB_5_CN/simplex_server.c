#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

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

    recv(client_fd, &packets, sizeof(packets), 0);

    printf("Number of packets = %d\n\n", packets);

    for (int i = 1; i <= packets; i++)
    {
        int packet;

        recv(client_fd, &packet, sizeof(packet), 0);

        printf("Received Packet %d\n", packet);

        int ack = packet;

        send(client_fd, &ack, sizeof(ack), 0);

        printf("ACK %d sent\n", ack);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}