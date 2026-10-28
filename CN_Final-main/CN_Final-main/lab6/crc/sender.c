
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define KEY "1011"
#define PORT 9000

int main()
{
    int clientsocket;
    struct sockaddr_in serveraddress = {0};

    clientsocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientsocket < 0)
    {
        printf("Socket is not created\n");
        return 1;
    }
    else
        printf("Socket is created successfully\n");

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(PORT);
    serveraddress.sin_addr.s_addr = INADDR_LOOPBACK;

    int constatus = connect(clientsocket, (struct sockaddr *)&serveraddress, sizeof(serveraddress));

    if (constatus == -1)
    {
        printf("Connection error\n");
        close(clientsocket);
        return 1;
    }
    else
        printf("Connection is established.\n");

    char data[100], temp[150];
    int i, j;
    int n, k = strlen(KEY);

    printf("\n--- CRC SENDER ---\n");
    printf("Generator (key): %s\n", KEY);
    printf("Enter data: ");
    scanf("%99s", data);

    n = strlen(data);

    strcpy(temp, data);

    /* APPEND k-1 ZEROS */
    for (i = 0; i < k - 1; i++)
        temp[n + i] = '0';

    temp[n + k - 1] = '\0';

    /* CRC DIVISION */
    for (i = 0; i < n; i++)
    {
        if (temp[i] == '1')
        {
            for (j = 0; j < k; j++)
            {
                if (temp[i + j] == KEY[j])
                    temp[i + j] = '0';
                else
                    temp[i + j] = '1';
            }
        }
    }

    printf("CRC Checksum: ");

    for (i = n; i < n + k - 1; i++)
        printf("%c", temp[i]);

    printf("\n");

    /* CONSTRUCT CODEWORD */
    char codeword[150];

    strcpy(codeword, data);

    for (i = n; i < n + k - 1; i++)
        codeword[i] = temp[i];

    codeword[n + k - 1] = '\0';

    printf("Generated Codeword: %s\n", codeword);

    /* INJECT ERROR */
    int inject;

    printf("Do you want to inject error? (0: No, 1: Yes): ");
    scanf("%d", &inject);

    if (inject == 1)
    {
        if (codeword[0] == '0')
            codeword[0] = '1';
        else
            codeword[0] = '0';

        printf("Codeword with error injected: %s\n", codeword);
    }

    /* SEND CODEWORD */
    send(clientsocket, codeword, strlen(codeword) + 1, 0);

    printf("Codeword sent to receiver.\n");

    /* RECEIVE RESULT */
    char ack[30];

    int bytes = recv(clientsocket, ack, sizeof(ack) - 1, 0);

    if (bytes > 0)
    {
        ack[bytes] = '\0';
        printf("Receiver Result: %s\n", ack);
    }
    else
        printf("Failed to receive receiver result.\n");

    close(clientsocket);

    return 0;
}
