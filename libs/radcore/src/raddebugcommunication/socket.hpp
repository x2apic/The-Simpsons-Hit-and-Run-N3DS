//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        socket.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the an abstract interface to sockets.
//				We use this interface so we can hide the underlying socket
//				implementation. Sometimes we use real sockets, othertimes
//				emulation.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Mar 30, 2001
//
// Notes:       On the GameCube, there is not real implemtation of sockets.
//              All interfaces must be implemented.
//=============================================================================

#ifndef	SOCKET_HPP
#define SOCKET_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radmemory.hpp>

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Interface
//=============================================================================

#if defined( SN_TCPIP ) || defined( WIN32 ) || defined( RAD_XBOX )
//
// Game cubes socket is a pure abstract interface. The other platfroms should
// be as well and this should be cleaned up.
//
struct radSocket 
{
    virtual int socket( int af, int type, int protocol)
    {
        return( ::socket( af, type, protocol ) );
    }
    virtual int closesocket( int sock )
    {
        return( ::closesocket( sock ) );
    }
    virtual int setsockopt( int sock, int level, int optname, const char* optval, int optlen)
    {
        return( ::setsockopt( sock, level, optname, optval, optlen ) );
    }
    virtual int accept( int sock, struct sockaddr* addr, int* addrlen)
    {
        return( ::accept( sock, addr, addrlen) );
    }
    virtual int bind( int sock, struct sockaddr* addr, int addrlen)
    {
        return( ::bind( sock, addr, addrlen ) );
    }
    virtual int connect( int sock, const struct sockaddr* addr, int addrlen)
    {
        return( ::connect( sock, addr, addrlen ) );
    }
    virtual int listen( int sock, int backlog)
    {
        return( ::listen( sock, backlog) );
    }
    virtual int recv( int sock, char* buf, int len, int flags)
    {
        return( ::recv( sock, buf, len, flags) );
    }
    virtual int send( int sock, const char* buf, int len, int flags)
    {
        return( ::send( sock, buf, len, flags) );
    }
    virtual int lasterror( int sock )
    {
        #if defined( RAD_WIN32 ) || defined( RAD_XBOX )
        return( WSAGetLastError( ) );
        #endif
        #ifdef RAD_PS2
        return( sn_errno( sock ) );
        #endif
    }
    virtual ~radSocket( void ) 
    {
        
    } 
    #if defined( RAD_WIN32 ) || defined( RAD_XBOX )
    virtual int ioctlsocket( SOCKET s, long cmd, u_long *argp )
    {
        return( ::ioctlsocket( s,  cmd,  argp ) );
    }
    #ifndef RAD_XBOX
    virtual int AsyncSelect( SOCKET s, HWND Wnd, unsigned int wMsg, long levent )
    {
        return( ::WSAAsyncSelect( s, Wnd, wMsg, levent ) );
    }
    #endif
    #endif

    //
    // This placement operator is used by the host side only
    //
    void* operator new( size_t size, void* p )
    {
        return( p );
    }    

    void* operator new( size_t size, radMemoryAllocator alloc )
    {
        return( radMemoryAlloc( alloc, size ) );
    }    

    void operator delete( void * pMemory )
    {
	    radMemoryFree( pMemory );
    }

};

#else

//
// GameCube does not have real socket implementation. Define a bunch of socket definitions
// normally found on platforms that have sockets. 
//
struct linger
{
    int l_onoff;                   /* option on/off */
    int l_linger;                  /* linger time */
};
#define SOL_SOCKET      0               /* options for socket level */
#define SO_LINGER       0x0080          /* linger on close if data present */
#define SO_KEEPALIVE    0x0008          /* keep connections alive */
#define WSAEWOULDBLOCK  6       
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define SO_REUSEADDR    0x0004          /* allow local address reuse */
#define SOCK_STREAM     1               /* stream socket */
#define WSAENOTCONN     9

struct in_addr
{
    unsigned int       s_addr;             /* IP address */
};

#define INADDR_ANY      0               /* Match any internet address */

/* Socket address structure as used for internet TCP/IP protocol */

#define INVALID_SOCKET -1

struct sockaddr_in
{
    unsigned short       sin_family;         /* Address family = AF_INET */
    unsigned short       sin_port;           /* TCP: protocol port */
    struct in_addr  sin_addr;           /* TCP: IP address */
    char         sin_zero[8];        /* TCP: unused (0) */
};

#define htonl(l)        ((unsigned int)(l))
#define htons(s)        ((unsigned short)(s))

struct radSocket 
{
    virtual int socket( int af, int type, int protocol) = 0;
    virtual int closesocket( int sock ) = 0;
    virtual int setsockopt( int sock, int level, int optname, const char* optval, int optlen) = 0;
    virtual int accept( int sock, struct sockaddr* addr, int* addrlen) = 0;
    virtual int bind( int sock, struct sockaddr* addr, int addrlen)= 0;
    virtual int listen( int sock, int backlog)= 0;
    virtual int recv( int sock, char* buf, int len, int flags)= 0;
    virtual int send( int sock, const char* buf, int len, int flags)= 0;
    virtual int lasterror( int sock )= 0;

    void* operator new( size_t size, radMemoryAllocator alloc )
    {
        return( radMemoryAlloc( alloc, size ) );
    }    

    void operator delete( void * pMemory )
    {
	    radMemoryFree( pMemory );
    }

    virtual ~radSocket( void ) 
    {
        
    } 

};

#endif

#endif
