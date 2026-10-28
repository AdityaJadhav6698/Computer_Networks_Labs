
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BITS 100

int main()
{
    int clientsocket;
    struct sockaddr_in serveraddress = {0};

    char data_str[MAX_BITS];
    int data[MAX_BITS];
    int codeword[MAX_BITS];
    int m, r = 0;

    /* SOCKET SETUP */
    clientsocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientsocket < 0)
    {
        perror("Socket error");
        exit(1);
    }

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(PORT);
    serveraddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientsocket, (struct sockaddr *)&serveraddress, sizeof(serveraddress)) < 0)
    {
        perror("Connection error");
        close(clientsocket);
        exit(1);
    }

    /* INPUT */
    printf("Enter data bits (e.g., 1011): ");
    scanf("%99s", data_str);

    m = strlen(data_str);

    for (int i = 0; i < m; i++)
    {
        data[i] = data_str[i] - '0';
    }

    /* CALCULATE PARITY BITS */
    while ((1 << r) < (m + r + 1))
    {
        r++;
    }

    printf("\nData bits (m)   : %d\n", m);
    printf("Parity bits (r) : %d\n", r);
    printf("Total bits      : %d\n", m + r);

    /* CONSTRUCT CODEWORD */
    int j = 0;
    int k = 0;

    for (int i = 1; i <= m + r; i++)
    {
        if (i == (1 << k))
        {
            codeword[i] = 0;
            k++;
        }
        else
        {
            codeword[i] = data[j++];
        }
    }

    /* CALCULATE EVEN PARITY */
    for (int i = 0; i < r; i++)
    {
        int pos = (1 << i);
        int parity = 0;

        for (int x = 1; x <= m + r; x++)
        {
            if ((x & pos) != 0)
            {
                parity ^= codeword[x];
            }
        }

        codeword[pos] = parity;
    }

    printf("\nGenerated Codeword: ");

    for (int i = 1; i <= m + r; i++)
    {
        printf("%d ", codeword[i]);
    }

    printf("\n");

    /* INJECT ERROR */
    int err_pos;

    printf("\nEnter position to inject error (1 to %d) or 0 for no error: ", m + r);
    scanf("%d", &err_pos);

    if (err_pos > 0 && err_pos <= m + r)
    {
        codeword[err_pos] ^= 1;

        printf("Error injected at position %d.\n", err_pos);
        printf("Transmitted Codeword: ");

        for (int i = 1; i <= m + r; i++)
        {
            printf("%d ", codeword[i]);
        }

        printf("\n");
    }
    else
    {
        printf("No error injected.\n");
    }

    /* SEND TO SERVER */
    send(clientsocket, &m, sizeof(m), 0);
    send(clientsocket, &r, sizeof(r), 0);
    send(clientsocket, codeword, sizeof(int) * (m + r + 1), 0);

    printf("\nCodeword sent to server successfully.\n");

    close(clientsocket);

    return 0;
}
