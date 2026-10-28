#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

struct ip_result {
    char class_name;
    int nid_bits;
    int hid_bits;
    unsigned long long networks;
    unsigned long long hosts;
    int subnet_mask[4];
    int network[4];
    int first_usable[4];
    int last_usable[4];
    int broadcast[4];
};

int valid_octets(int address[4]) {
    for (int i = 0; i < 4; i++) {
        if (address[i] < 0 || address[i] > 255)
            return 0;
    }
    return 1;
}

void calculate_ip_details(int address[4], struct ip_result *result) {
    int mask_bits;

    result->class_name = 'I';

    if (address[0] >= 1 && address[0] <= 126) {
        result->class_name = 'A';
        result->nid_bits = 8;
        result->hid_bits = 24;
        result->networks = 126;
        result->hosts = 16777214;
        mask_bits = 1;
    } else if (address[0] >= 128 && address[0] <= 191) {
        result->class_name = 'B';
        result->nid_bits = 16;
        result->hid_bits = 16;
        result->networks = 16384;
        result->hosts = 65534;
        mask_bits = 2;
    } else if (address[0] >= 192 && address[0] <= 223) {
        result->class_name = 'C';
        result->nid_bits = 24;
        result->hid_bits = 8;
        result->networks = 2097152;
        result->hosts = 254;
        mask_bits = 3;
    } else if (address[0] >= 224 && address[0] <= 239) {
        result->class_name = 'D';
        result->nid_bits = 0;
        result->hid_bits = 0;
        result->networks = 0;
        result->hosts = 0;
        mask_bits = 0;
    } else {
        result->class_name = 'E';
        result->nid_bits = 0;
        result->hid_bits = 0;
        result->networks = 0;
        result->hosts = 0;
        mask_bits = 0;
    }

    for (int i = 0; i < 4; i++) {
        result->subnet_mask[i] = i < mask_bits ? 255 : 0;
        result->network[i] = address[i] & result->subnet_mask[i];
        result->broadcast[i] =
            result->network[i] | (result->subnet_mask[i] ^ 255);
        result->first_usable[i] = result->network[i];
        result->last_usable[i] = result->broadcast[i];
    }

    if (mask_bits > 0) {
        result->first_usable[3]++;
        result->last_usable[3]--;
    }
}

int main(void) {
    int socket_fd, client_fd;
    int address[4];
    int option = 1;
    struct sockaddr_in server, client;
    struct ip_result result;
    socklen_t client_length;
    char ack = 'K'; // Acknowledgment character

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(socket_fd, (struct sockaddr *)&server, sizeof(server));
    listen(socket_fd, 1);

    printf("Waiting for client...\n");
    client_length = sizeof(client);
    client_fd = accept(socket_fd, (struct sockaddr *)&client, &client_length);
    
    recv(client_fd, address, sizeof(address), 0);

    /* Output processing and printing on SERVER side */
    printf("\nReceived IP: %d.%d.%d.%d\n", address[0], address[1], address[2], address[3]);

    if (valid_octets(address)) {
        calculate_ip_details(address, &result);
        
        if (result.class_name == 'I') {
            printf("Result: Invalid IP address\n");
        } else {
            printf("Class: %c\n", result.class_name);
            printf("Number of bits in NID: %d\n", result.nid_bits);
            printf("Number of bits in HID: %d\n", result.hid_bits);
            printf("Number of networks: %llu\n", result.networks);
            printf("Number of hosts per network: %llu\n", result.hosts);
            printf("Classful subnet mask: %d.%d.%d.%d\n",
                   result.subnet_mask[0], result.subnet_mask[1],
                   result.subnet_mask[2], result.subnet_mask[3]);
            printf("Network address: %d.%d.%d.%d\n", result.network[0], result.network[1], result.network[2], result.network[3]);
            printf("First usable address: %d.%d.%d.%d\n", result.first_usable[0], result.first_usable[1], result.first_usable[2], result.first_usable[3]);
            printf("Last usable address: %d.%d.%d.%d\n", result.last_usable[0], result.last_usable[1], result.last_usable[2], result.last_usable[3]);
            printf("Directed broadcast address: %d.%d.%d.%d\n", result.broadcast[0], result.broadcast[1], result.broadcast[2], result.broadcast[3]);
        }
    } else {
        printf("Result: Invalid IP address format (octets out of range).\n");
    }

    /* Send a simple acknowledgment back to client so it can close cleanly */
    send(client_fd, &ack, sizeof(ack), 0);

    close(client_fd);
    close(socket_fd);
    return 0;
}