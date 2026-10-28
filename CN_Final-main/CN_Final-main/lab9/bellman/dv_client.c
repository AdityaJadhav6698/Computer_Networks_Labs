#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_VERTICES 100
#define MAX_EDGES 500

struct Edge {
    int source;
    int destination;
    int cost;
};

struct DV_Request {
    int vertices;
    int edge_count;
    int source_router;
    struct Edge edges[MAX_EDGES];
};

int main(void) {
    int sock;
    struct sockaddr_in server_addr;
    struct DV_Request req;
    char ack;

    /* Socket Creation */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Gather Network Topology */
    printf("Enter number of routers (1-%d): ", MAX_VERTICES);
    if (scanf("%d", &req.vertices) != 1 || req.vertices < 1 || req.vertices > MAX_VERTICES) {
        fprintf(stderr, "Invalid number of routers.\n");
        return 1;
    }

    printf("Enter number of directed links (1-%d): ", MAX_EDGES);
    if (scanf("%d", &req.edge_count) != 1 || req.edge_count < 0 || req.edge_count > MAX_EDGES) {
        fprintf(stderr, "Invalid number of links.\n");
        return 1;
    }

    printf("Enter each link as: source destination cost\n");
    for (int i = 0; i < req.edge_count; i++) {
        if (scanf("%d %d %d", &req.edges[i].source, &req.edges[i].destination, &req.edges[i].cost) != 3 ||
            req.edges[i].source < 0 || req.edges[i].source >= req.vertices ||
            req.edges[i].destination < 0 || req.edges[i].destination >= req.vertices) {
            fprintf(stderr, "Invalid link. Router IDs must be between 0 and %d.\n", req.vertices - 1);
            return 1;
        }
    }

    printf("Enter source router (0-%d): ", req.vertices - 1);
    if (scanf("%d", &req.source_router) != 1 || req.source_router < 0 || req.source_router >= req.vertices) {
        fprintf(stderr, "Invalid source router.\n");
        return 1;
    }

    /* Connect and Send Data */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    send(sock, &req, sizeof(req), 0);

    /* Wait for server acknowledgment */
    recv(sock, &ack, sizeof(ack), 0);
    
    printf("\nNetwork topology sent successfully!\n");
    printf("Check the server console for the distance-vector routing table updates.\n");

    close(sock);
    return 0;
}