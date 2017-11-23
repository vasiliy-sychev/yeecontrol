/*
*  This file (json_query.h) is a part of:
*  YeeControl - native Windows client for YeeLight Smart LED Bulbs
*
*  Written by Vasiliy Sychev in 2017, Kyiv, Ukraine
*/

#ifndef JSON_QUERY_H
#define JSON_QUERY_H

/* Structures */
struct JSONQuery
{
	char *buffer;
	int bufferSize;
	int bytesUsed;
	int state;
};

/* Custom data types */
typedef struct JSONQuery JSONQUERY;

/* Function prototypes */
int BeginQuery(JSONQUERY *query, char *buffer, int bufferSizeBytes);
int BeginList(JSONQUERY *query, char *name);
int EndQueryEndList(JSONQUERY *query);

int WriteNumber(JSONQUERY *query, char *name, int number);
int WriteString(JSONQUERY *query, char *name, char *string);

#endif
