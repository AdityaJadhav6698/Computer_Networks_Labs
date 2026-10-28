#include "common_dsss.h"

// ---- Spread one data bit: repeat it 11 times, then XOR with PN_CODE ----
void spreadBit(char bit, char *out) {
    for (int i = 0; i < PN_LEN; i++) {
        int b = bit - '0';                 // the data bit, same value repeated
        int n = PN_CODE[i] - '0';           // PN code bit at this position
        out[i] = (b ^ n) + '0';             // XOR, converted back to char
    }
    out[PN_LEN] = '\0';
}

// ---- Convert an 11-bit chunk into a decimal value, used only for freq allocation ----
int chunkToValue(const char *chunk) {
    int value = 0;
    for (int i = 0; i < PN_LEN; i++)
        value = value * 2 + (chunk[i] - '0');
    return value;
}

int main() {
    char bits[100];
    printf("Enter binary string: ");
    scanf("%s", bits);

    int numBits = strlen(bits);

    // ---- Setup UDP socket ----
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    printf("Using PN code: %s\n\n", PN_CODE);

    // ---- Spread each bit individually, allocate frequency, send ----
    for (int i = 0; i < numBits; i++) {
        Packet p;
        p.seqNo = i;

        spreadBit(bits[i], p.data);                 // XOR spreading happens here

        int value = chunkToValue(p.data);
        p.frequency = freqBands[value % 8];          // map to one of 8 bands

        sendto(sock, &p, sizeof(p), 0, (struct sockaddr*)&server, sizeof(server));
        printf("Packet %d | Bit: %c | Spread: %s | Freq: %d Hz\n",
               i+1, bits[i], p.data, p.frequency);
        usleep(500000);
    }

    Packet end = {END, 0, ""};
    sendto(sock, &end, sizeof(end), 0, (struct sockaddr*)&server, sizeof(server));

    close(sock);
    return 0;
}