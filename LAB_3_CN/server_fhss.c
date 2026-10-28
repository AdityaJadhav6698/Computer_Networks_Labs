#include "common.h"

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    printf("Server listening on port %d...\n", PORT);

    char chunks[100][4];
    int count = 0;

    Packet p;
    while (1) {
        recvfrom(sock, &p, sizeof(p), 0, NULL, NULL);
        if (p.seqNo == END) break;

        printf("Received | Seq: %d | Freq: %d Hz | Data: %s\n", p.seqNo, p.frequency, p.data);
        strcpy(chunks[p.seqNo], p.data);
        if (p.seqNo + 1 > count) count = p.seqNo + 1;
    }

    // ---- Reassemble ----
    char spread[300] = "";
    for (int i = 0; i < count; i++)
        strcat(spread, chunks[i]);
    printf("\nSpread data: %s\n", spread);

    //despread - any chunk containing a 1 means original bit was 1 
    char original[100] = "";
    for (int i = 0; i < strlen(spread); i += 3) {
        char chunk[4];
        strncpy(chunk, spread + i, 3);
        chunk[3] = '\0';
        strcat(original, strchr(chunk, '1') ? "1" : "0");
    }

    printf("Recovered data: %s\n", original);

    close(sock);
    return 0;
}