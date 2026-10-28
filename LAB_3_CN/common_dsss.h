#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT    9999
#define END     -1
#define PN_LEN  11

// ---- 11-bit PN spreading code (change this string to use a different code) ----
const char *PN_CODE = "11010010111";

int freqBands[8] = {200, 300, 400, 500, 600, 700, 800, 900};

typedef struct {
    int  seqNo;
    int  frequency;
    char data[PN_LEN + 1];   // 11 chip bits + '\0'
} Packet;

#endif