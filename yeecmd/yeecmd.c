/*
*  This file (yeecmd.c) is a part of:
*  YeeControl - native Windows client for YeeLight Smart LED Bulbs
*
*  Written by Vasiliy Sychev in 2017, Kyiv, Ukraine
*/

#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <string.h>

#include "json_query.h" /* files from ../yeecontrol/ */
#include "util.h"

/* Command line arguments */
#define ARGV_EXE_NAME	0
#define ARGV_IP_ADDR	1
#define ARGV_COMMAND	2
#define ARGV_PARAMS		3
#define ARGV_RED		3
#define ARGV_GREEN		4
#define ARGV_BLUE		5

/* I/O buffer size */
#define COMMAND_BUFFER_SIZE 100

/* Values from YeeLink documentation, do not modify! */
#define MIN_BRIGHTNESS 1
#define MAX_BRIGHTNESS 100

#define MIN_COLOR_TEMP 1700
#define MAX_COLOR_TEMP 6500

void GenSetBrightRequest(char *buffer, int bufSize, int brightness)
{
	JSONQUERY query;

	BeginQuery(&query, buffer, bufSize);
	WriteNumber(&query, "id", 1);
	WriteString(&query, "method", "set_bright");

	/* params:[brightness, "effect", duration] */
	BeginList(&query, "params");
	WriteNumber(&query, NULL, brightness);
	WriteString(&query, NULL, "smooth");
	WriteNumber(&query, NULL, 100);
	EndQueryEndList(&query);

	EndQueryEndList(&query);
}

void GenSetCTRequest(char *buffer, int bufSize, int ct)
{
	JSONQUERY query;

	BeginQuery(&query, buffer, bufSize);
	WriteNumber(&query, "id", 1);
	WriteString(&query, "method", "set_ct_abx");

	/* params:[color_temperature, "effect", duration] */
	BeginList(&query, "params");
	WriteNumber(&query, NULL, ct);
	WriteString(&query, NULL, "smooth");
	WriteNumber(&query, NULL, 100);
	EndQueryEndList(&query);

	EndQueryEndList(&query);
}

void GenSetRGBRequest(char *buffer, int bufSize, int r, int g, int b)
{
	JSONQUERY query;
	int color = (r * 65536) + (g * 256) + b;

	BeginQuery(&query, buffer, bufSize);
	WriteNumber(&query, "id", 1);
	WriteString(&query, "method", "set_rgb");

	/* params:[color, "effect", duration] */
	BeginList(&query, "params");
	WriteNumber(&query, NULL, color);
	WriteString(&query, NULL, "smooth");
	WriteNumber(&query, NULL, 100);
	EndQueryEndList(&query);

	EndQueryEndList(&query);
}

void GenSetPowerRequest(char *buffer, int bufSize, int power)
{
	JSONQUERY query;

	BeginQuery(&query, buffer, bufSize);
	WriteNumber(&query, "id", 1);
	WriteString(&query, "method", "set_power");

	/* params:["on/off", "effect", duration] */
	BeginList(&query, "params");

	if(power == 0)
		WriteString(&query, NULL, "off");
	else
		WriteString(&query, NULL, "on");

	WriteString(&query, NULL, "smooth");
	WriteNumber(&query, NULL, 100);	
	EndQueryEndList(&query);

	EndQueryEndList(&query);
}

void GenSetDefaultRequest(char *buffer, int bufSize)
{
	JSONQUERY query;

	BeginQuery(&query, buffer, bufSize);
	WriteNumber(&query, "id", 1);
	WriteString(&query, "method", "set_default");

	/* Empty list (no params required for set_default command) */
	BeginList(&query, "params");
	EndQueryEndList(&query);

	EndQueryEndList(&query);
}

void GenToggleRequest(char *buffer, int bufSize)
{
	JSONQUERY query;

	BeginQuery(&query, buffer, bufSize);
	WriteNumber(&query, "id", 1);
	WriteString(&query, "method", "toggle");

	/* Empty list (no params required for toggle command) */
	BeginList(&query, "params");
	EndQueryEndList(&query);

	EndQueryEndList(&query);
}

void PrintUsage()
{
	wprintf(L"Usage: yeecmd <IPv4 address> <command> [optional arguments]\n\n");
	wprintf(L"Available commands:\n");
	wprintf(L"       set_bright <brightness level from %d to %d>\n", (int) MIN_BRIGHTNESS, (int) MAX_BRIGHTNESS);
	wprintf(L"       set_ctemp <color temperature %d..%d>\n", (int) MIN_COLOR_TEMP, (int) MAX_COLOR_TEMP);
	wprintf(L"       set_color <red 0..255> <green 0..255> <blue 0..255>\n");
	wprintf(L"       set_power <on/off>\n");
	wprintf(L"       set_default\n");
	wprintf(L"       toggle\n\n");
}

int wmain(int argc, wchar_t **argv, wchar_t **envp)
{
	char buffer[COMMAND_BUFFER_SIZE];
	unsigned long addr;
	WSADATA wsaData;
	SOCKET tcpSocket;
	DWORD timeout;
	struct sockaddr_in lampAddr;
	int recvResult;
	int packetsRecvd = 0;

	wprintf(L"\nYeeControl for YeeLight Smart LED Bulbs\ngithub.com/vasiliy-sychev/yeecontrol\n\n");

	if(argc < 3)
	{
		PrintUsage();
		return 1;
	}

	if(wcscmp(argv[ARGV_COMMAND], L"set_bright") == 0)
	{
		int br;

		if(argc < 4)
		{
			wprintf(L"Required argument (brightness level) is missing\n");
			return 1;
		}

		br = _wtoi(argv[ARGV_PARAMS]);

		if(br < MIN_BRIGHTNESS || br > MAX_BRIGHTNESS)
		{
			wprintf(L"Argument for \"set_bright\" must be from %d to %d\n", (int) MIN_BRIGHTNESS, (int) MAX_BRIGHTNESS);
			return 1;
		}

		GenSetBrightRequest(buffer, COMMAND_BUFFER_SIZE, br);
	}
	else if(wcscmp(argv[ARGV_COMMAND], L"set_ctemp") == 0)
	{
		int ct;

		if(argc < 4)
		{
			wprintf(L"Required argument (color temperature) is missing\n");
			return 1;
		}

		ct = _wtoi(argv[ARGV_PARAMS]);

		if(ct < MIN_COLOR_TEMP || ct > MAX_COLOR_TEMP)
		{
			wprintf(L"Argument for \"set_ctemp\" must be from %d to %d\n", (int) MIN_COLOR_TEMP, (int) MAX_COLOR_TEMP);
			return 1;
		}

		GenSetCTRequest(buffer, COMMAND_BUFFER_SIZE, ct);
	}
	else if(wcscmp(argv[ARGV_COMMAND], L"set_color") == 0)
	{
		if(argc < 6)
		{
			wprintf(L"Required arguments (RED GREEN BLUE) is missing\n");
			return 1;
		}

		GenSetRGBRequest(buffer, COMMAND_BUFFER_SIZE, _wtoi(argv[ARGV_RED]), _wtoi(argv[ARGV_GREEN]), _wtoi(argv[ARGV_BLUE]));
	}
	else if(wcscmp(argv[ARGV_COMMAND], L"set_power") == 0)
	{
		if(argc < 4)
		{
			wprintf(L"Required argument (lamp state, on|off) is missing\n");
			return 1;
		}

		if(wcscmp(argv[ARGV_PARAMS], L"on") == 0)
			GenSetPowerRequest(buffer, COMMAND_BUFFER_SIZE, 1);
		else if(wcscmp(argv[ARGV_PARAMS], L"off") == 0)
			GenSetPowerRequest(buffer, COMMAND_BUFFER_SIZE, 0);
		else
		{
			wprintf(L"Argument for \"set_power\" must be \"on\" or \"off\"\n");
			return 1;
		}
	}
	else if(wcscmp(argv[ARGV_COMMAND], L"set_default") == 0)
	{
		GenSetDefaultRequest(buffer, COMMAND_BUFFER_SIZE);
	}
	else if(wcscmp(argv[ARGV_COMMAND], L"toggle") == 0)
	{
		GenToggleRequest(buffer, COMMAND_BUFFER_SIZE);
	}
	else
	{
		wprintf(L"Unknown command: %s\n", argv[ARGV_COMMAND]);
		PrintUsage();
		return 1;
	}

	addr = WCStringToAddr(argv[ARGV_IP_ADDR]);

	if(addr == INADDR_ANY || addr == INADDR_NONE)
	{
		wprintf(L"Incorrect IP address: %s\n", argv[ARGV_IP_ADDR]);
		return 1;
	}

	//printf("[DEBUG] Packet: %s\n\n", buffer);

	if(WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
	{
		wprintf(L"WSAStartup(v2.2) returned error\n");
		return 1;
	}

	tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(tcpSocket == INVALID_SOCKET)
	{
		wprintf(L"Error creating socket (socket() returned INVALID_SOCKET)\n");
		WSACleanup();
		return 1;
	}

	timeout = 1000;
	setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(DWORD));

	lampAddr.sin_family = AF_INET;
	lampAddr.sin_addr.s_addr = addr;
	lampAddr.sin_port = htons(55443);

	wprintf(L"Connecting to %s... ", argv[ARGV_IP_ADDR]); /* Let's print some information */

	if(connect(tcpSocket, (SOCKADDR *) &lampAddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		wprintf(L"failed\n");

		closesocket(tcpSocket);
		WSACleanup();
		return 1;
	}

	wprintf(L"connected!\n");

	strcat(buffer, "\r\n");

	if(send(tcpSocket, buffer, strlen(buffer), 0) == SOCKET_ERROR)
	{
		wprintf(L"Error sending data\n");
		closesocket(tcpSocket);
		WSACleanup();
		return 1;
	}

	wprintf(L"Data was successfully sent, waiting for response...\n");

	while(1)
	{
		recvResult = recv(tcpSocket, buffer, COMMAND_BUFFER_SIZE, 0);

		if(recvResult == 0 && packetsRecvd == 2)
		{
			wprintf(L"Zero bytes received, leaving loop...\n");
			break;
		}

		wprintf(L"Packet received!\n");

		if(++packetsRecvd == 2)
		{
			wprintf(L"Closing connection...\n");
			shutdown(tcpSocket, SD_SEND);
		}
	}

	closesocket(tcpSocket);
	WSACleanup();
	return 0;
}
