
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_VERTICES 100

struct LS_Request {
    int vertices;
    int graph[MAX_VERTICES][MAX_VERTICES];
    int source_router;
};

int main() {
    int sock;
    struct sockaddr_in server_addr;
    struct LS_Request req;
    char ack;

    printf("Enter number of routers (1-%d): ", MAX_VERTICES);
    if (scanf("%d", &req.vertices) != 1 ||
        req.vertices < 1 || req.vertices > MAX_VERTICES) {
        printf("Invalid number of routers.\n");
        return 1;
    }

    printf("Enter the adjacency matrix (-1 means no link, 0 on diagonal):\n");
    for (int i = 0; i < req.vertices; i++) {
        for (int j = 0; j < req.vertices; j++) {
            if (scanf("%d", &req.graph[i][j]) != 1 ||
                req.graph[i][j] < -1 ||
                (i == j && req.graph[i][j] != 0)) {
                printf("Invalid matrix value. Use costs >= 0, -1 for no link, and 0 on the diagonal.\n");
                return 1;
            }
        }
    }

    printf("Enter source router (0-%d): ", req.vertices - 1);
    if (scanf("%d", &req.source_router) != 1 ||
        req.source_router < 0 ||
        req.source_router >= req.vertices) {
        printf("Invalid source router.\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    size_t sent = 0;

    while (sent < sizeof(req)) {
        ssize_t n = send(sock, (char *)&req + sent,
                         sizeof(req) - sent, 0);

        if (n <= 0) {
            perror("Send failed");
            close(sock);
            return 1;
        }

        sent += (size_t)n;
    }

    if (recv(sock, &ack, sizeof(ack), MSG_WAITALL) == sizeof(ack)
        && ack == 'K') {
        printf("\nNetwork topology sent successfully!\n");
        printf("Check server terminal for routing table.\n");
    } else {
        printf("Acknowledgment not received.\n");
    }

    close(sock);
    return 0;
}
