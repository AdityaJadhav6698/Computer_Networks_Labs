#ifndef COMMON_FHSS_H
#define COMMON_FHSS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9998
#define END -1

static const int freqBands[8] = {
    200, 300, 400, 500, 600, 700, 800, 900
};

typedef struct {
    int seqNo;
    int frequency;
    char data[4];
} Packet;

#endif
