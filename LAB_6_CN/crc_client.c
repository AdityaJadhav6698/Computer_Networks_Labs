#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define KEY "1011"   // Generator polynomial — same in sender & receiver

int main()
{
    int senderSocket;
    struct sockaddr_in receiverAddress;

    senderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (senderSocket < 0)
        printf("socket is not created\n");
    else
        printf("socket is created successfully\n");

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_port = htons(9000);
    receiverAddress.sin_addr.s_addr = INADDR_ANY;

    int constatus = connect(senderSocket, (struct sockaddr *)&receiverAddress, sizeof(receiverAddress));
    if (constatus == -1)
    {
        printf("there was an error in the connection\n");
        return 1;
    }
    else
        printf("connection is established.\n");

    char data[100], temp[150];
    int i, j;
    int n = 0, k = strlen(KEY);

    printf("\n--- CRC SENDER ---\n");
    printf("Generator (key): %s\n", KEY);
    printf("Enter data: ");
    scanf("%s", data);
    n = strlen(data);

    strcpy(temp, data);

    // Append k-1 zeros
    for (i = 0; i < k - 1; i++)
        temp[n + i] = '0';
    temp[n + k - 1] = '\0';

    // CRC division
    for (i = 0; i < n; i++)
    {
        if (temp[i] == '1')
        {
            for (j = 0; j < k; j++)
            {
                temp[i + j] = (temp[i + j] == KEY[j]) ? '0' : '1';
            }
        }
    }

    printf("CRC Checksum: ");
    for (i = n; i < n + k - 1; i++)
        printf("%c", temp[i]);
    printf("\n");

    // Construct codeword = data + CRC
    char codeword[150];
    strcpy(codeword, data);
    for (i = n; i < n + k - 1; i++)
        codeword[i] = temp[i];
    codeword[n + k - 1] = '\0';

    printf("Generated Codeword: %s\n", codeword);

    int inject = 0;
    printf("Do you want to inject error? (0: No, 1: Yes): ");
    scanf("%d", &inject);
    if (inject == 1)
    {
        codeword[0] = (codeword[0] == '0') ? '1' : '0';
        printf("Codeword with error injected: %s\n", codeword);
    }

    // Send only the codeword — key is already known to receiver
    send(senderSocket, codeword, strlen(codeword) + 1, 0);
    printf("Codeword sent to receiver.\n");

    char ack[30];
    recv(senderSocket, ack, sizeof(ack), 0);
    printf("Receiver Result: %s\n\n", ack);

    close(senderSocket);
    return 0;
}