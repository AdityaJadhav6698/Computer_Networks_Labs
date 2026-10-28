#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(void) {
    int socket_fd;
    int address[4];
    struct sockaddr_in server;
    char ack;

    printf("Enter IP address: ");
    if (scanf("%d.%d.%d.%d", &address[0], &address[1], &address[2], &address[3]) != 4) {
        printf("Invalid input format.\n");
        return 1;
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = htonl(0x7f000001); // 127.0.0.1

    if (connect(socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return 1;
    }

    /* Send IP to server */
    send(socket_fd, address, sizeof(address), 0);
    
    /* Wait for acknowledgment from server */
    recv(socket_fd, &ack, sizeof(ack), 0);

    printf("IP sent successfully. Check the server console for the output.\n");

    close(socket_fd);
    return 0;
}