#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

#define PORT 8081
#define MAX_FRAMES 20
#define MAX_DATA 50
#define MAX_STREAM 2048

int send_all(int socket, const void *buffer, int length)
{
    int sent = 0;

    while (sent < length)
    {
        int n = send(socket,
                     (const char *)buffer + sent,
                     length - sent,
                     0);

        if (n <= 0)
            return -1;

        sent += n;
    }

    return 0;
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;

    int n;

    char data[MAX_FRAMES][MAX_DATA];
    unsigned char stream[MAX_STREAM];

    int pos = 0;

    /* Create socket */
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* Server details */
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Connect to server */
    if (connect(clientSocket,
                (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }

    printf("Connected to server.\n");

    /* Number of frames */
    printf("Enter number of frames: ");
    scanf("%d", &n);

    if (n < 1 || n > MAX_FRAMES)
    {
        printf("Invalid number of frames.\n");
        close(clientSocket);
        return 1;
    }

    /* Input frame data */
    for (int i = 0; i < n; i++)
    {
        printf("Enter data for frame %d: ", i + 1);
        scanf("%49s", data[i]);
    }

    /* Create byte-count stream */
    printf("\n--- Sender Side ---\n");
    printf("Frames: [");

    for (int i = 0; i < n; i++)
    {
        int count = strlen(data[i]);

        /* Display count + data */
        printf("%d%s", count, data[i]);

        /* Store count in stream */
        stream[pos++] = count;

        /* Store frame data in stream */
        for (int j = 0; j < count; j++)
        {
            stream[pos++] = data[i][j];
        }
    }

    printf("]\n");

    /* Information sent to server */
    int32_t frameCount = n;
    int32_t streamLength = pos;

    send_all(clientSocket,
             &frameCount,
             sizeof(frameCount));

    send_all(clientSocket,
             &streamLength,
             sizeof(streamLength));

    send_all(clientSocket,
             stream,
             pos);

    printf("Data sent to server.\n");

    close(clientSocket);

    return 0;
}