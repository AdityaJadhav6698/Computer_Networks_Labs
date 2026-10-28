#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_FRAME_SIZE 120
#define MAX_MSG_SIZE 100
#define MAX_FINAL_FRAME 1000

int stringLength(char str[])
{
    int length = 0;

    while (str[length] != '\0')
        length++;

    return length;
}

void clearMemory(void *address, int size)
{
    char *bytes = (char *)address;

    for (int i = 0; i < size; i++)
        bytes[i] = 0;
}

int stringToNumber(char str[], int end)
{
    int number = 0;

    for (int i = 0; i < end; i++)
    {
        if (str[i] < '0' || str[i] > '9')
            return -1;

        number = number * 10 + (str[i] - '0');
    }

    return number;
}

int separateFrame(char frame[], char msg[], int *byteCount)
{
    int msgIndex = 0;

    if (frame[0] < '0' || frame[0] > '9')
        return 0;

    *byteCount = frame[0] - '0';

    for (int i = 1; frame[i] != '\0'; i++)
        msg[msgIndex++] = frame[i];

    msg[msgIndex] = '\0';

    return msgIndex == *byteCount;
}

void separateFinalFrame(char finalFrame[])
{
    int finalFrameLength = stringLength(finalFrame);
    int frameStart = 0;
    int frameNumber = 1;

    while (frameStart < finalFrameLength)
    {
        char msg[MAX_MSG_SIZE];
        int msgIndex = 0;

        if (finalFrame[frameStart] < '0' || finalFrame[frameStart] > '9')
        {
            printf("Invalid frame: byte count is not valid.\n");
            return;
        }

        int byteCount = finalFrame[frameStart] - '0';

        if (byteCount >= MAX_MSG_SIZE)
        {
            printf("Invalid frame: message is too large.\n");
            return;
        }

        if (frameStart + byteCount >= finalFrameLength)
        {
            printf("Invalid frame: byte count mismatch.\n");
            return;
        }

        for (int i = frameStart + 1; i <= frameStart + byteCount; i++)
            msg[msgIndex++] = finalFrame[i];

        msg[msgIndex] = '\0';

        printf("Frame %d\n", frameNumber);
        printf("Byte count: %d\n", byteCount);
        printf("Message: %s\n", msg);

        frameStart = frameStart + byteCount + 1;
        frameNumber++;
    }
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    clearMemory(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9000);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Binding failed");
        close(serverSocket);
        return 1;
    }

    printf("Binding successful\n");

    if (listen(serverSocket, 3) < 0)
    {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    printf("Waiting for client connection...\n");

    clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket < 0)
    {
        perror("Accept failed");
        close(serverSocket);
        return 1;
    }

    printf("Client connected successfully.\n");

    char finalFrame[MAX_FINAL_FRAME];
    int n = recv(clientSocket, finalFrame, sizeof(finalFrame) - 1, 0);
    if (n <= 0)
    {
        perror("Receive failed");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    finalFrame[n] = '\0';

    printf("Received final frame: %s\n", finalFrame);
    separateFinalFrame(finalFrame);

    char ack[] = "OK";
    send(clientSocket, ack, stringLength(ack) + 1, 0);

    close(clientSocket);
    close(serverSocket);

    return 0;
}
