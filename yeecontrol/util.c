/*
*  This file (util.c) is a part of:
*  YeeControl - native Windows client for YeeLight Smart LED Bulbs
*
*  Written by Vasiliy Sychev in 2017, Kyiv, Ukraine
*/

#include <WinSock2.h>
#include <Windows.h>

unsigned long WCStringToAddr(wchar_t *wcIP)
{
	int wcLen;
	int wctombResult;
	char outputBuffer[20];

	wcLen = (int) wcslen(wcIP);

	if(wcLen > 15) /* AAA.BBB.CCC.DDD can't be longer than 15 chars */
		return INADDR_NONE;

	wctombResult = WideCharToMultiByte(CP_ACP, 0, wcIP, wcLen, outputBuffer, 20, NULL, NULL);

	if(wctombResult == wcLen) /* All characters converted successfully */
	{	
		outputBuffer[wcLen] = '\0';
		return inet_addr(outputBuffer);
	}

	return INADDR_NONE;
}
