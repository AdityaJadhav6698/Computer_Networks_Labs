
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_VERTICES 100

struct LS_Request {
    int vertices;
    int graph[MAX_VERTICES][MAX_VERTICES];
    int source_router;
};

void print_path(int parent[], int vertex) {
    if (parent[vertex] == -1) {
        printf("%d", vertex);
        return;
    }

    print_path(parent, parent[vertex]);
    printf(" -> %d", vertex);
}

void handle_client(int client_fd) {
    struct LS_Request req;
    char ack = 'K';
    size_t received = 0;

    while (received < sizeof(req)) {
        ssize_t n = recv(client_fd, (char *)&req + received,
                         sizeof(req) - received, 0);

        if (n <= 0) {
            printf("Client disconnected or receive failed.\n");
            close(client_fd);
            return;
        }

        received += (size_t)n;
    }

    if (req.vertices < 1 || req.vertices > MAX_VERTICES ||
        req.source_router < 0 ||
        req.source_router >= req.vertices) {
        printf("Invalid request received.\n");
        close(client_fd);
        return;
    }

    printf("\n--- Link State Routing ---\n");
    printf("Number of routers: %d\n", req.vertices);
    printf("Source router: %d\n\n", req.source_router);

    int distance[MAX_VERTICES];
    int parent[MAX_VERTICES];
    int visited[MAX_VERTICES] = {0};

    for (int i = 0; i < req.vertices; i++) {
        distance[i] = INT_MAX;
        parent[i] = -1;
    }

    distance[req.source_router] = 0;

    // Dijkstra's algorithm
    for (int step = 0; step < req.vertices; step++) {
        int current = -1;

        for (int i = 0; i < req.vertices; i++) {
            if (!visited[i] && distance[i] != INT_MAX &&
                (current == -1 ||
                 distance[i] < distance[current])) {
                current = i;
            }
        }

        if (current == -1)
            break;

        visited[current] = 1;

        for (int neighbor = 0; neighbor < req.vertices; neighbor++) {
            int cost = req.graph[current][neighbor];

            if (!visited[neighbor] && cost >= 0 &&
                distance[current] <= INT_MAX - cost &&
                distance[current] + cost < distance[neighbor]) {

                distance[neighbor] = distance[current] + cost;
                parent[neighbor] = current;
            }
        }
    }

    printf("Routing Table (Dijkstra / OSPF Model)\n");
    printf("Destination\tCost\tPath\n");

    for (int i = 0; i < req.vertices; i++) {
        printf("%d\t\t", i);

        if (distance[i] == INT_MAX) {
            printf("INF\tUnreachable\n");
        } else {
            printf("%d\t", distance[i]);
            print_path(parent, i);
            printf("\n");
        }
    }

    printf("\n--- Processing Complete ---\n\n");

    if (send(client_fd, &ack, sizeof(ack), 0) < 0)
        perror("Acknowledgment failed");

    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    int opt = 1;
    struct sockaddr_in server_addr, client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("Link State Server listening on port %d...\n", PORT);

    while (1) {
        socklen_t addr_len = sizeof(client_addr);

        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &addr_len);

        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}
