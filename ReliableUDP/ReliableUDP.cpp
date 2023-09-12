// FILE          : ReliableUDP.cpp
// PROJECT       : ReliableUDP
// programmer    : Yujung Park
// FIRST VERSION : 2023-02-05
// DESCRIPTION   : The program is used reliableUDP from Reliability and Flo
// The file transfer function is added into the program


/*
	Reliability and Flo
	Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <time.h>

#include "Net.h"

#pragma warning(suppress : 4996)

#define SHOW_ACKS 1

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

class FlowControl
{
public:

	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}

	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}

	void Update(float deltaTime, float rtt)
	{
		const float RTT_Threshold = 250.0f;

		if (mode == Good)
		{
			if (rtt > RTT_Threshold)
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f)
					penalty_time = 1.0f;
				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		if (mode == Bad)
		{
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;

			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}

	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

private:

	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};

// ----------------------------------------------

int main(int argc, char* argv[])
{
	// parse command line
	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;
	

	////// W1 ReliableUDP.exe 127.0.0.1
	if (argc >= 2)
	{
		int a, b, c, d;
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
	}


	//**** A1
	// Retrieving additional command line arguments
	// argument for a requested file name
	// No hard-coded parameters (i.e. IP address, port). You must include sane defaults.
	// e.g. ReliableUDP.exe 127.0.0.1 hello.txt
	// argv[2] = file name

	char inputFileName[PacketSize] = { 0 };
	if (argc >= 3)
	{
		int a, b, c, d;
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
		strcpy(inputFileName, argv[2]);
	}


	// initialize
	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;

	if (!connection.Start(port))
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}

	if (mode == Client)
		connection.Connect(address);
	else
		connection.Listen();

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;

	bool A1FileRet = false;

	clock_t time;

	while (true)
	{
		// update flow control

		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();

		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;
		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}

		
		// send and receive packets
		sendAccumulator += DeltaTime;

		while (sendAccumulator > 1.0f / sendRate)
		{
			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet));


			//**** A1
			if (mode == Client)
			{
				time = clock();
				// A1 C1. Retrieving the file from disk
				FILE* ifp = NULL;
				ifp = fopen((char*)inputFileName, "rb");
				if (ifp == NULL)
				{
					printf("ERROR: open file error\n");
				}

				// A1 C2. Sending file metadata
				makePacket(ifp, inputFileName, packet, 'm');
				connection.SendPacket(packet, sizeof(packet));
				sendAccumulator -= 1.0f / sendRate;
				// A1 C3. Breaking the file in pieces to send
				// A1 C4. Sending the pieces
				while (!feof(ifp))
				{
					makePacket(ifp, inputFileName, packet, 'f');
					connection.SendPacket(packet, sizeof(packet));			
					sendAccumulator -= 1.0f / sendRate;
				}
				if (fclose(ifp) != 0)
				{
					printf("ERROR: closing file error\n");
				}

				makePacket(ifp, inputFileName, packet, 's');
				
				time = clock() - time;
				float tTime = ((float)time) / CLOCKS_PER_SEC;
				struct stat fStatus;
				int statResult;
				if (stat(inputFileName, &fStatus) < 0)
				{
					statResult = -1;
				}
				else
				{
					statResult = fStatus.st_size;
				}
				
				printf("A1***********************************************\n");
				printf("Transmission time: %f seconds\n", tTime);
				printf("File size: %d bytes\n", statResult);
				printf("Transfer speed: %f Mbps\n", (float)statResult / 125000 / tTime);
				printf("A1***********************************************\n");
			}
			//**** A1

			
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;
		}
		

		while (true)
		{
			unsigned char packet[256];

			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read == 0)
				break;

		}

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);

		// show connection stats

		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth);

			statsAccumulator -= 0.25f;
		}

		net::wait(DeltaTime);
	}

	ShutdownSockets();

	return 0;
}