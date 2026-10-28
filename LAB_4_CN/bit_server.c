#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_FRAME 2048
#define FLAG "01111110"
#define FLAG_LEN 8

int bitDestuff(const char stuffed[], char data[], size_t dataSize)
{
    size_t j = 0;
    int count = 0;
    size_t stuffedLength = strlen(stuffed);

    if (stuffedLength < 2 * FLAG_LEN ||
        strncmp(stuffed, FLAG, FLAG_LEN) != 0 ||
        strncmp(stuffed + stuffedLength - FLAG_LEN, FLAG, FLAG_LEN) != 0)
        return -1;

    for (size_t i = FLAG_LEN; i < stuffedLength - FLAG_LEN; i++)
    {
        if (j + 1 >= dataSize)
            return -1;

        data[j++] = stuffed[i];

        if (stuffed[i] == '1')
        {
            count++;

            if (count == 5)
            {
                if (i + 1 >= stuffedLength - FLAG_LEN ||
                    stuffed[i + 1] != '0')
                    return -1;
                i++;
                count = 0;
            }
        }
        else
        {
            count = 0;
        }
    }

    data[j] = '\0';
    return 0;
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize = sizeof(clientAddr);

    char stuffed[MAX_FRAME];
    char data[MAX_FRAME];

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if(bind(serverSocket,
            (struct sockaddr*)&serverAddr,
            sizeof(serverAddr)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    listen(serverSocket, 5);

    printf("Bit Stuffing Server waiting...\n");

    clientSocket = accept(serverSocket,
                           (struct sockaddr*)&clientAddr,
                           &addrSize);

    printf("Client connected.\n");

    size_t received = 0;
    while (received < sizeof(stuffed) - 1)
    {
        ssize_t n = recv(clientSocket, stuffed + received,
                         sizeof(stuffed) - 1 - received, 0);
        if (n <= 0)
        {
            if (n < 0)
                perror("Receive failed");
            else
                fprintf(stderr, "Client closed the connection before sending a complete frame.\n");
            close(clientSocket);
            close(serverSocket);
            return 1;
        }

        received += (size_t)n;
        if (memchr(stuffed, '\0', received) != NULL)
            break;
    }
    stuffed[received] = '\0';

    printf("\nFramed data received: %s\n", stuffed);

    if (bitDestuff(stuffed, data, sizeof(data)) < 0)
    {
        fprintf(stderr, "Invalid frame: missing flags or malformed stuffed data.\n");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    printf("Destuffed data: %s\n", data);

    close(clientSocket);
    close(serverSocket);

    return 0;
}