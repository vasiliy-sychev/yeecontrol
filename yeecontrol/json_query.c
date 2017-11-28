/*
*  This file (json_query.c) is a part of:
*  YeeControl - native Windows client for YeeLight Smart LED Bulbs
*
*
*  Simple JSON generator for YeeLight Smart Lamps
*
*  This is NOT a fully-featured JSON stream generator, so please
*  do not use it for other "serious" tasks :)
*
*  Written by Vasiliy Sychev in 2017, Kyiv, Ukraine
*/

#include <stdlib.h>
#include <string.h>

#include "json_query.h"

/* Possible states */
#define NO_BUFFER_SPACE			0
#define QUERY_INITIALIZED		1
#define ITEM_ADDED				2
#define LIST_OPENED				3
#define LIST_ITEM_ADDED			4
#define LIST_CLOSED				5
#define QUERY_FINALIZED			6

int BeginQuery(JSONQUERY *query, char *buffer, int bufferSizeBytes)
{
	if(bufferSizeBytes < 3) /* At least we need three bytes: '{' + '}' + NULL */
	{
		query->state = NO_BUFFER_SPACE;
		return 1;
	}

	buffer[0] = '{';
	buffer[1] = '\0';

	query->buffer = buffer;
	query->bufferSize = bufferSizeBytes;
	query->bytesUsed = 1; /* One byte used by '{' character */
	query->state = QUERY_INITIALIZED;
	return 0;
}


int BeginList(JSONQUERY *query, char *name)
{
	int newDataLen;
	int i = query->bytesUsed;

	switch(query->state)
	{
	case NO_BUFFER_SPACE:
		return 1;

	case QUERY_INITIALIZED:
		newDataLen = (int) strlen(name) + 4; /* Length of block name + "":[ */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->state = LIST_OPENED;
		break;

	case ITEM_ADDED:
	case LIST_CLOSED:
		newDataLen = (int) strlen(name) + 5; /* Length of block name + ,"":[ */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->state = LIST_OPENED;
		query->buffer[i++] = ',';
	}

	query->buffer[i++] = '\"';

	while(*name != '\0')
	{
		query->buffer[i++] = *name;
		name++;
	}

	query->buffer[i++] = '\"';
	query->buffer[i++] = ':';
	query->buffer[i++] = '[';
	query->buffer[i] = '\0';
	query->bytesUsed = i;
	return 0;
}

int EndQueryEndList(JSONQUERY *query)
{
	int i = query->bytesUsed;

	if(query->state == NO_BUFFER_SPACE)
		return 1;

	if(query->bufferSize - i < 2)
	{
		query->state = NO_BUFFER_SPACE;
		return 1;
	}

	switch(query->state)
	{
	case QUERY_INITIALIZED:
	case ITEM_ADDED:
	case LIST_CLOSED:
		query->state = QUERY_FINALIZED;
		query->buffer[i++] = '}';
		break;

	case LIST_OPENED:
	case LIST_ITEM_ADDED:
		query->state = LIST_CLOSED;
		query->buffer[i++] = ']';
		break;
	}

	query->buffer[i] = '\0';
	query->bytesUsed = i;
	return 0;
}

int WriteNumber(JSONQUERY *query, char *name, int number)
{
	char itoaOutBuffer[20];
	int newDataLen;
	int bp = 0;
	int i = query->bytesUsed;

	itoa(number, itoaOutBuffer, 10);

	switch(query->state)
	{
	case NO_BUFFER_SPACE:
		return 1;

	case QUERY_FINALIZED: /* After EndQuery() we must ignore all calls */
		return 0;

	case QUERY_INITIALIZED: /* First item, so we don't need comma character */
		newDataLen = (int) (strlen(name) + strlen(itoaOutBuffer) + 3); /* Length of strings + "": */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->state = ITEM_ADDED; /* QUERY_INITIALIZED -> ITEM_ADDED */
		query->buffer[i++] = '\"';

		while(*name != '\0')
		{
			query->buffer[i++] = *name;
			name++;
		}

		query->buffer[i++] = '\"';
		query->buffer[i++] = ':';
		break;

	case ITEM_ADDED:
	case LIST_CLOSED:
		newDataLen = (int) (strlen(name) + strlen(itoaOutBuffer) + 4); /* Length of strings + ,"": */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->state = ITEM_ADDED;
		query->buffer[i++] = ',';
		query->buffer[i++] = '\"';

		while(*name != '\0')
		{
			query->buffer[i++] = *name;
			name++;
		}

		query->buffer[i++] = '\"';
		query->buffer[i++] = ':';
		break;

	case LIST_OPENED:
		newDataLen = (int) strlen(itoaOutBuffer); /* Only length of itoa() result */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->state = LIST_ITEM_ADDED; /* LIST_OPENED -> LIST_ITEM_ADDED */
		break;

	case LIST_ITEM_ADDED:
		newDataLen = (int) (strlen(itoaOutBuffer) + 1); /* itoa() result + separator (,) */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->buffer[i++] = ',';
		break;
	}

	while(itoaOutBuffer[bp] != '\0') /* Let's append number */
	{
		query->buffer[i++] = itoaOutBuffer[bp];
		bp++;
	}

	query->buffer[i] = '\0';
	query->bytesUsed = i;
	return 0;
}

int WriteString(JSONQUERY *query, char *name, char *string)
{
	int newDataLen;
	int i = query->bytesUsed;

	switch(query->state)
	{
	case NO_BUFFER_SPACE:
		return 1;

	case QUERY_FINALIZED: /* After EndQuery() we must ignore all calls */
		return 0;

	case QUERY_INITIALIZED: /* First item, so we don't need comma character */
		newDataLen = (int) (strlen(name) + strlen(string) + 5);

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->state = ITEM_ADDED; /* QUERY_INITIALIZED -> ITEM_ADDED */
		query->buffer[i++] = '\"';

		while(*name != '\0')
		{
			query->buffer[i++] = *name;
			name++;
		}

		query->buffer[i++] = '\"';
		query->buffer[i++] = ':';
		query->buffer[i++] = '\"';
		break;

	case ITEM_ADDED:
	case LIST_CLOSED:
		newDataLen = (int) (strlen(name) + strlen(string) + 6); /* Length of name + ,"":"" */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}
		query->state = ITEM_ADDED;

		query->buffer[i++] = ',';
		query->buffer[i++] = '\"';

		while(*name != '\0')
		{
			query->buffer[i++] = *name;
			name++;
		}

		query->buffer[i++] = '\"';
		query->buffer[i++] = ':';
		query->buffer[i++] = '\"';
		break;

	case LIST_OPENED:
		newDataLen = (int) (strlen(string) + 2); /* Data + two double quotes */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->state = LIST_ITEM_ADDED; /* LIST_OPENED -> LIST_ITEM_ADDED */
		query->buffer[i++] = '\"';
		break;

	case LIST_ITEM_ADDED:
		newDataLen = (int) (strlen(string) + 3); /* Data + separator + two double quotes */

		if(query->bufferSize - i < newDataLen + 1)
		{
			query->state = NO_BUFFER_SPACE;
			return 1;
		}

		query->buffer[i++] = ',';
		query->buffer[i++] = '\"';
		break;
	}

	while(*string != '\0')
	{
		query->buffer[i++] = *string;
		string++;
	}

	query->buffer[i++] = '\"';
	query->buffer[i] = '\0';
	query->bytesUsed = i;
	return 0;
}
