#include <stdio.h>
#include <limits.h>

#define MAX_VERTICES 100
#define MAX_EDGES 500

struct Edge {
    int source;
    int destination;
    int cost;
};

static void print_path(int parent[], int vertex)
{
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

int main(void)
{
    struct Edge edges[MAX_EDGES];
    int vertices;
    int edge_count;
    int source;
    long long distance[MAX_VERTICES];
    int parent[MAX_VERTICES];

    printf("Enter number of routers (1-%d): ", MAX_VERTICES);
    if (scanf("%d", &vertices) != 1 || vertices < 1 || vertices > MAX_VERTICES) {
        fprintf(stderr, "Invalid number of routers.\n");
        return 1;
    }

    printf("Enter number of directed links (1-%d): ", MAX_EDGES);
    if (scanf("%d", &edge_count) != 1 || edge_count < 0 || edge_count > MAX_EDGES) {
        fprintf(stderr, "Invalid number of links.\n");
        return 1;
    }

    printf("Enter each link as: source destination cost\n");
    for (int i = 0; i < edge_count; i++) {
        if (scanf("%d %d %d", &edges[i].source, &edges[i].destination, &edges[i].cost) != 3 ||
            edges[i].source < 0 || edges[i].source >= vertices ||
            edges[i].destination < 0 || edges[i].destination >= vertices) {
            fprintf(stderr, "Invalid link. Router IDs must be between 0 and %d.\n", vertices - 1);
            return 1;
        }
    }

    printf("Enter source router (0-%d): ", vertices - 1);
    if (scanf("%d", &source) != 1 || source < 0 || source >= vertices) {
        fprintf(stderr, "Invalid source router.\n");
        return 1;
    }

    for (int i = 0; i < vertices; i++) {
        distance[i] = LLONG_MAX;
        parent[i] = -1;
    }
    distance[source] = 0;

    /* Bellman-Ford relaxations model repeated distance-vector updates. */
    for (int round = 1; round < vertices; round++) {
        int changed = 0;

        for (int i = 0; i < edge_count; i++) {
            struct Edge edge = edges[i];
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

    for (int i = 0; i < edge_count; i++) {
        struct Edge edge = edges[i];
        if (distance[edge.source] != LLONG_MAX &&
            distance[edge.source] + edge.cost < distance[edge.destination]) {
            printf("\nNegative-weight cycle detected reachable from router %d.\n", source);
            printf("Shortest paths are undefined.\n");
            return 2;
        }
    }

    printf("\nDistance-vector routing table (Bellman-Ford/RIP model)\n");
    printf("Destination\tCost\tPath\n");
    for (int i = 0; i < vertices; i++) {
        printf("%d\t\t", i);
        if (distance[i] == LLONG_MAX) {
            printf("INF\tUnreachable\n");
        } else {
            printf("%lld\t", distance[i]);
            print_path(parent, i);
            putchar('\n');
        }
    }

    return 0;
}
