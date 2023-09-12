#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "checksum.h"
//#include "Net.h"

#pragma warning(disable : 4996)

const int kBlockSize = 256;

void makePacket(FILE* file, char* fname, unsigned char* buffer, char packetType);
void WriteFile(unsigned char* data, unsigned int pcktSeq, unsigned int ack, const char* fname);