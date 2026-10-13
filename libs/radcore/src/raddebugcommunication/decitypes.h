#ifndef	DECITYPES_H
#define DECITYPES_H

typedef unsigned char byte;				// 8bit
typedef unsigned short half;			// 16bit
typedef unsigned int word;				// 32bit
typedef struct { word xa[2]; } twin;    // 64bit
typedef struct { word xa[4]; } quad;    // 128bit

#ifdef RAD_GAMECUBE
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
#endif

#endif