#if __has_include("common_dsss.h")
#include "common_dsss.h"
#elif __has_include("fhss/common_dsss.h")
#include "fhss/common_dsss.h"
#elif __has_include("../fhss/common_dsss.h")
#include "../fhss/common_dsss.h"
#else
#error "common_dsss.h not found"
#endif

// ---- Despread: XOR received chunk with PN_CODE again to recover the bit ----
char despreadBit(const char *chunk) {
    char result[PN_LEN + 1];
    for (int i = 0; i < PN_LEN; i++) {
        int c = chunk[i] - '0';
        int n = PN_CODE[i] - '0';
        result[i] = (c ^ n) + '0';   // XOR cancels PN_CODE, leaves original bit repeated
    }
    result[PN_LEN] = '\0';
    return result[0];   // every position holds the same recovered bit
}

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    printf("Server listening on port %d...\n", PORT);
    printf("Using PN code: %s\n\n", PN_CODE);

    char chunks[100][PN_LEN + 1];
    int count = 0;

    Packet p;
    while (1) {
        recvfrom(sock, &p, sizeof(p), 0, NULL, NULL);
        if (p.seqNo == END) break;

        printf("Received | Seq: %d | Freq: %d Hz | Chunk: %s\n", p.seqNo, p.frequency, p.data);
        strcpy(chunks[p.seqNo], p.data);
        if (p.seqNo + 1 > count) count = p.seqNo + 1;
    }

    // ---- Despread each chunk back into the original bit ----
    char original[100] = "";
    for (int i = 0; i < count; i++) {
        char bit = despreadBit(chunks[i]);
        strncat(original, &bit, 1);
    }

    printf("\nRecovered data: %s\n", original);

    close(sock);
    return 0;
}