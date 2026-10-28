#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
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

static void print_path(int parent[], int vertex) {
    int path[MAX_VERTICES];
    int length = 0;
    int current = vertex;

    while (current != -1 && length < MAX_VERTICES) {
        path[length++] = current;
        current = parent[current];
    }

    if (current != -1) {
        printf("cycle detected in predecessor chain");
        return;
    }

    for (int i = length - 1; i >= 0; i--) {
        printf("%d", path[i]);
        if (i > 0) {
            printf(" -> ");
        }
    }
}

void handle_client(int client_fd) {
    struct DV_Request req;
    char ack = 'K';
    long long distance[MAX_VERTICES];
    int parent[MAX_VERTICES];

    if (recv(client_fd, &req, sizeof(req), 0) <= 0) {
        close(client_fd);
        return;
    }

    printf("\n--- Processing Distance Vector Routing ---\n");
    printf("Total Routers: %d\n", req.vertices);
    printf("Total Links: %d\n", req.edge_count);
    printf("Source Router: %d\n\n", req.source_router);

    /* Initialize tables */
    for (int i = 0; i < req.vertices; i++) {
        distance[i] = LLONG_MAX;
        parent[i] = -1;
    }
    distance[req.source_router] = 0;

    /* Bellman-Ford relaxations */
    for (int round = 1; round < req.vertices; round++) {
        int changed = 0;

        for (int i = 0; i < req.edge_count; i++) {
            struct Edge edge = req.edges[i];
            if (distance[edge.source] != LLONG_MAX &&
                distance[edge.source] + edge.cost < distance[edge.destination]) {
                distance[edge.destination] = distance[edge.source] + edge.cost;
                parent[edge.destination] = edge.source;
                changed = 1;
            }
        }

        printf("Distance-vector update round %d: %s\n", round,
               changed ? "table changed" : "converged");
        if (!changed) {
            break;
        }
    }

    /* Check for negative-weight cycles */
    int has_cycle = 0;
    for (int i = 0; i < req.edge_count; i++) {
        struct Edge edge = req.edges[i];
        if (distance[edge.source] != LLONG_MAX &&
            distance[edge.source] + edge.cost < distance[edge.destination]) {
            printf("\nNegative-weight cycle detected reachable from router %d.\n", req.source_router);
            printf("Shortest paths are undefined.\n");
            has_cycle = 1;
            break;
        }
    }

    /* Print routing table if no cycle is found */
    if (!has_cycle) {
        printf("\nDistance-vector routing table (Bellman-Ford/RIP model)\n");
        printf("Destination\tCost\tPath\n");
        for (int i = 0; i < req.vertices; i++) {
            printf("%d\t\t", i);
            if (distance[i] == LLONG_MAX) {
                printf("INF\tUnreachable\n");
            } else {
                printf("%lld\t", distance[i]);
                print_path(parent, i);
                putchar('\n');
            }
        }
    }

    printf("--- Finished Processing ---\n\n");

    /* Send acknowledgment to client */
    send(client_fd, &ack, sizeof(ack), 0);
    close(client_fd);
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Distance Vector Server listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd >= 0) {
            handle_client(client_fd);
        }
    }
    return 0;
}