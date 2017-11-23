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

#include "json_query.h" /* file from ../yeecontrol/ */

#define ARGV_EXE_NAME	0
#define ARGV_IP_ADDR	1
#define ARGV_COMMAND	2
#define ARGV_PARAMS		3

#define COMMAND_BUFFER_SIZE 100

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
	wprintf(L"       set_bright <brightness level from 1 to 100>\n");
	wprintf(L"       set_power <on/off>\n");
	wprintf(L"       toggle\n");
}

unsigned long ArgumentToAddr(wchar_t *wcIP)
{
	int wcLen;
	int wctombResult;
	char outputBuffer[20];

	wcLen = (int) wcslen(wcIP);

	if(wcLen > 15) /* AAA.BBB.CCC.DDD can't be longer than 15 chars */
		return INADDR_NONE;

	wctombResult = WideCharToMultiByte(CP_ACP, 0, wcIP, wcLen, outputBuffer, 20, NULL, NULL);
	
	if(wctombResult == wcLen)
	{	
		outputBuffer[wcLen] = '\0';
		return inet_addr(outputBuffer);
	}
	
	return INADDR_NONE;
}

int wmain(int argc, wchar_t **argv, wchar_t **envp)
{
	char buffer[COMMAND_BUFFER_SIZE];
	unsigned long addr;
	WSADATA wsaData;
	SOCKET tcpSocket;
	struct sockaddr_in lampAddr;

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

		if(br > 0 && br < 101)
			GenSetBrightRequest(buffer, COMMAND_BUFFER_SIZE, br);
		else
		{
			wprintf(L"Argument for \"set_bright\" must be from 1 to 100\n");
			return 1;
		}
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

	addr = ArgumentToAddr(argv[ARGV_IP_ADDR]);

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

	if(send(tcpSocket, buffer, strlen(buffer), 0) != SOCKET_ERROR)
		wprintf(L"Data was successfully sent!\n");

	shutdown(tcpSocket, SD_SEND);

	while(1)
	{
		if(recv(tcpSocket, buffer, COMMAND_BUFFER_SIZE, 0) == 0)
			break;
	}

	closesocket(tcpSocket);
	WSACleanup();
	return 0;
}
