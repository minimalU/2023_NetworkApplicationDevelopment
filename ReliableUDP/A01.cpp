// FILE          : A01.cpp
// PROJECT       : ReliableUDP
// programmer    : Yujung Park
// FIRST VERSION : 2023-02-05
// DESCRIPTION   : This is modular function code for assignment 01.
// makePacket() and writeFile() functions are added for file transfer functionality.

#include "A1.h"

// FUNCTION   : makePacket()
// DESCRIPTION: This function takes parameters of input file pointer, fliename string pointer, 
// buffer string poinnter, char of packet type and 
// make data packet for file transfer
// PARAMETERS : FILE* file	input file name
//				char* fname file name
//				unsigned char* buffer
//				char packetType
// RETURN     : nothing
void makePacket(FILE* file, char* fname, unsigned char* buffer, char packetType)
{
	char data[kBlockSize] = { 0 };
	unsigned char dataReadFromFile[kBlockSize - 10] = { 0 };
	int  numBytesRead = 0;
	uint16_t checksum = 0;

	// matadata: D0 + packetsize + Filename
	if (packetType == 'm')
	{

		numBytesRead = strlen(fname);
		memset((char*)buffer, '\0', kBlockSize);
		sprintf((char*)buffer, "D0%03d-%s\0", numBytesRead, fname);
	}
	// filedata : D1 + packetsize + Filedata
	if (packetType == 'f')
	{
		if ((numBytesRead = fread(dataReadFromFile, sizeof(char), kBlockSize - 11, file)) != 0)
		{
			checksum = crc_16(dataReadFromFile, 16);
			dataReadFromFile[numBytesRead] = '\0';
			// printf("  >>> %d bytes READ \n", numBytesRead);
			sprintf((char*)buffer, "D1%03d-%s", numBytesRead, dataReadFromFile);

		}

	}
	// checksum data : D2 + packetsize + crc
	if (packetType == 's')
	{
		int checksum = (int)mycrc_16(fname);
		sprintf((char*)buffer, "D2%03d-%05d\0", numBytesRead, checksum);
	}
}

// FUNCTION   : WriteFile()
// DESCRIPTION: This function takes parameters of a pointer to data buffer, packet sequence, 
// ack, a pointer to the file name and get the data from buffer and write the file
// PARAMETERS : unsigned char* data 
//				unsigned int pcktSeq  packet sequence
//				unsigned int ack
//				const char* fname  file name
// RETURN     : nothing
void WriteFile(unsigned char* data, unsigned int pcktSeq, unsigned int ack, const char* fname)
{
	char pckReceiveRet[kBlockSize] = { 0 };
	char dataType[4] = { 0 };
	int numBytes = 0;
	int checksum = 0;
	int crcRet = 0;
	char dataReceived[kBlockSize] = { 0 };
	
	char content[kBlockSize] = { 0 };

	if (data[0] == 'D')
	{
		memcpy(dataReceived, data, kBlockSize);
		// printf("dataReceived: %s\n", dataReceived);
		// dataReceived[kBlockSize] = '\0';
		char* token = strchr(dataReceived, '-');
		memcpy(content, dataReceived + 2, 3);
		numBytes = atoi(content);
		memcpy(content, token + 1, numBytes);

		// Receiving the file metadata
		if (data[1] == '0')
		{
			memset((char*)fname, '\0', kBlockSize);
			memcpy((char*)fname, content, numBytes);
			//strcpy((char*)fname, content);
		}
		// Receiving the file pieces and Writing the pieces out to disk
		if (data[1] == '1')
		{
			if (ack == 0)
			{
				FILE* ofp = NULL;
				ofp = fopen(fname, "ab");
				if (ofp == NULL)
				{
					printf("ERROR: file open error\n");
				}
				else
				{
					if (fwrite(content, sizeof(char), numBytes, ofp) == 0)
					{
						printf("ERROR: writing to output file\n");
					}
				}
				if (fclose(ofp) != 0)
				{
					printf("ERROR: file closing error\n");
				}
			}
		}
		// Verifying the file integrity
		if (data[1] == '2')
		{
			checksum = atoi(content);
			crcRet = (int)mycrc_16((char*)fname);

			if (checksum != crcRet)
			{
				// printf("File is currupted");
			}
		}
	}
}