#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_DATA 1024
#define MAX_STUFFED 2048
#define FLAG "01111110"
#define FLAG_LEN 8

int bitStuff(const char data[], char stuffed[], size_t stuffedSize)
{
    size_t j = 0;
    int count = 0;

    for (size_t i = 0; data[i] != '\0'; i++)
    {
        if (j + 1 >= stuffedSize)
            return -1;

        stuffed[j++] = data[i];

        if (data[i] == '1')
        {
            count++;

            if (count == 5)
            {
                if (j + 1 >= stuffedSize)
                    return -1;

                stuffed[j++] = '0';
                count = 0;
            }
        }
        else
        {
            count = 0;
        }
    }

    stuffed[j] = '\0';
    return 0;
}

int sendAll(int socket, const char *buffer, size_t length)
{
    size_t sent = 0;

    while (sent < length)
    {
        ssize_t n = send(socket, buffer + sent, length - sent, 0);
        if (n <= 0)
            return -1;
        sent += (size_t)n;
    }

    return 0;
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;

    char data[MAX_DATA];
    char stuffed[MAX_STUFFED];
    char frame[MAX_STUFFED + 2 * FLAG_LEN + 1];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(clientSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(clientSocket,
               (struct sockaddr*)&serverAddr,
               sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }

    printf("Connected to server.\n");

    printf("Enter binary data: ");
    if (scanf("%1023s", data) != 1)
    {
        fprintf(stderr, "Failed to read binary data.\n");
        close(clientSocket);
        return 1;
    }

    for (size_t i = 0; data[i] != '\0'; i++)
    {
        if (data[i] != '0' && data[i] != '1')
        {
            fprintf(stderr, "Input must contain only 0 and 1.\n");
            close(clientSocket);
            return 1;
        }
    }

    if (bitStuff(data, stuffed, sizeof(stuffed)) < 0)
    {
        fprintf(stderr, "Stuffed data is too large.\n");
        close(clientSocket);
        return 1;
    }

    int frameLength = snprintf(frame, sizeof(frame), "%s%s%s",
                               FLAG, stuffed, FLAG);
    if (frameLength < 0 || (size_t)frameLength >= sizeof(frame))
    {
        fprintf(stderr, "Framed data is too large.\n");
        close(clientSocket);
        return 1;
    }

    printf("\nOriginal data: %s\n", data);
    printf("Stuffed data : %s\n", stuffed);
    printf("Framed data  : %s\n", frame);

    if (sendAll(clientSocket, frame, (size_t)frameLength + 1) < 0)
    {
        perror("Sending data failed");
        close(clientSocket);
        return 1;
    }

    printf("Data sent to server.\n");

    close(clientSocket);

    return 0;
}