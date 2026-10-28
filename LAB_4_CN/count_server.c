#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

#define PORT 8081
#define MAX_STREAM 2048

int recv_all(int socket, void *buffer, int length)
{
    int received = 0;

    while (received < length)
    {
        int n = recv(socket,
                     (char *)buffer + received,
                     length - received,
                     0);

        if (n <= 0)
            return -1;

        received += n;
    }

    return 0;
}

int main()
{
    int serverSocket;
    int clientSocket;
    int opt = 1;

    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

    socklen_t addrSize = sizeof(clientAddr);

    int32_t frameCount;
    int32_t streamLength;

    unsigned char stream[MAX_STREAM];

    /* Create server socket */
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* Allow port reuse */
    setsockopt(serverSocket,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    /* Server details */
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    /* Bind socket */
    if (bind(serverSocket,
             (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    /* Listen for client */
    listen(serverSocket, 5);

    printf("Byte Count Server waiting...\n");

    /* Accept client */
    clientSocket = accept(serverSocket,
                           (struct sockaddr *)&clientAddr,
                           &addrSize);

    if (clientSocket < 0)
    {
        perror("Accept failed");
        close(serverSocket);
        return 1;
    }

    printf("Client connected.\n");

    /* Receive number of frames */
    if (recv_all(clientSocket,
                 &frameCount,
                 sizeof(frameCount)) < 0)
    {
        perror("Receiving frame count failed");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    /* Receive total stream length */
    if (recv_all(clientSocket,
                 &streamLength,
                 sizeof(streamLength)) < 0)
    {
        perror("Receiving stream length failed");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    /* Validate stream length */
    if (streamLength <= 0 || streamLength > MAX_STREAM)
    {
        printf("Invalid stream length.\n");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    /* Receive actual byte-count stream */
    if (recv_all(clientSocket,
                 stream,
                 streamLength) < 0)
    {
        perror("Receiving data failed");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    printf("\n--- Receiver Side ---\n");

    /*
       Display received frames:
       [3ABC3DEF3GHI]
    */

    printf("Received frames: [");

    int pos = 0;

    for (int i = 0; i < frameCount; i++)
    {
        /* First byte is count */
        int count = stream[pos++];

        /* Print count */
        printf("%d", count);

        /* Print data according to count */
        for (int j = 0; j < count; j++)
        {
            putchar(stream[pos++]);
        }
    }

    printf("]\n");

    /*
       Display original data:
       [ABCDEFGHI]
    */

    printf("Original data: [");

    pos = 0;

    for (int i = 0; i < frameCount; i++)
    {
        /* Read count */
        int count = stream[pos++];

        /* Print only data */
        for (int j = 0; j < count; j++)
        {
            putchar(stream[pos++]);
        }
    }

    printf("]\n");

    close(clientSocket);
    close(serverSocket);

    return 0;
}