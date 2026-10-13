//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical Text Display Console.
//
// Description: This file implements a text display on the console TV screen.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __TEXTDISPLAY_H
#define __TEXTDISPLAY_H

//===========================================================================
// Nested Includes
//===========================================================================

#include <radmemory.hpp>
#include <raddebug.hpp>
#ifdef RAD_PS2
#include <eekernel.h>
#include <libgraph.h>
#include <libdev.h>
#include <libdma.h>
#include <libpkt.h>
#include <sifcmd.h>
#endif // RAD_PS2
#ifdef RAD_GAMECUBE
#include <dolphin.h>
#include "font.hpp"
#endif // RAD_GAMECUBE
#ifdef RAD_XBOX
#include <xtl.h>
#include <xfont.h>
#endif // RAD_XBOX

//===========================================================================
// Class Definitions
//===========================================================================

//===========================================================================
//
// Description: This class implements the IRadTextDisplay interface.
//
//===========================================================================
class radTextDisplay : public IRadTextDisplay
{
    public:

    //
    // Setup and tear-down functions.
    //
    void Initialize( radMemoryAllocator alloc );
    void Terminate( void );

    //
    // IRefCount stuff
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // IRadTextDisplay stuff.
    //

    virtual void SetAutoSwap( bool on );
    virtual void SwapBuffers( void );

    virtual void SetBackgroundColor( unsigned int color );  
	virtual void SetTextColor( unsigned int color );
    virtual void Clear( void );

    virtual void GetDimensions( unsigned int* pWidth, unsigned int* pHeight ) const;
    virtual void SetCursorPosition( unsigned int x, unsigned int y );

    virtual void TextOutAt( const char* pText, int x, int y );
    virtual void TextOut( const char*  pText );

    //
    // Initialization flag.
    //
    static bool m_Initialized;

    //
    // Allocator for internal memory buffers.
    //
    static radMemoryAllocator m_Alloc;

    private:

    //
    // Store a character in the text buffer.
    //
    void WriteLetter( int x, int y, char c );

    //
    // Scroll the screen up one line.
    //
    void ScrollUp( void );

    //
    // Init/shut down display hardware.
    //
    void InitDisplay( void );
    void CloseDisplay( void );

    //
    // Redraw and redisplay video screen.
    //
    void PaintIfAutoSwapOn( void );
    void Paint( void );


    //
    // ===========================================
    // PS2-specific functions.
    //
    #ifdef RAD_PS2

    //
    // Initialize the PS2 text display system.
    //
    void Ps2InitConsole( void );

    //
    // Display a newly drawn batch of text.
    //
    void Ps2SwapBuffers( void );

    #endif // RAD_PS2


    //
    // ===========================================
    // GameCube-specific functions.
    //
    #ifdef RAD_GAMECUBE
    //
    // Convert color spaces.
    //
    unsigned int RGBtoYUV( unsigned int color ) const;

    //
    // Paint one character on the screen.
    //
    void PaintChar( int cx, int cy, char c );

    //
    // Allocate storage for frame buffers.
    //
    void GcnAllocateFrameBuffer( void );

    //
    // Init the GameCube text display system.
    //
    void GcnInitConsole( void );

    //
    // Fill frame buffer with a color.
    //
    void GcnFillFrameBuffer( u32 code );

    //
    // Swap display buffers.
    //
    void GcnSwapBuffers( void );

    #endif // RAD_GAMECUBE


    //
    // ===========================================
    // Data members.
    //

    //
    // Reference counting member variable.
    //
    int m_ReferenceCount;


    bool m_AutoSwap;

    //
    // Buffer for storing the displayed text.
    //
    char* m_TextBuffer;

    //
    // Display controls.
    //
    unsigned int m_BackgroundColorRGB; // Background color.
    unsigned int m_TextColorRGB;       // Text color.
    int          m_CursorX;   // Cursor location, 0-left-0-top-based.
    int          m_CursorY;
    int          m_Width;     // Screen width in characters.
    int          m_Height;    // Screen height in characters.

    //
    // Current redraw frame number (controls selection of frame buffer.
    //
    int m_CurFrame;
    
    //
    // ===========================================
    // PS2-specific data members.
    //
    #ifdef RAD_PS2

    int m_Console;   // Console number.

    //
    // Double-buffer structure needed for redisplay.
    //
    sceGsDBuff m_DoubleBufferInfo;

    #endif // RAD_PS2


    //
    // ===========================================
    // GameCube-specific data members.
    //
    #ifdef RAD_GAMECUBE

    unsigned int m_BackgroundColorYUV;  // Background color in GCN color space.
    unsigned int m_TextColorYUV;        // Text color in GCN color space.

    //
    // Frame buffers and accounting data.
    //
    u8*  m_FrameBuffer1;
    u8*  m_FrameBuffer2;
    u8*  m_CurBuffer;
    bool m_First;
    u32  m_FrameBufferSize;

    //
    // Screen video mode description data.
    //
    GXRenderModeObj* m_ScreenMode;

    #endif // RAD_GAMECUBE

    //
    // ===========================================
    // XBox specific data members
    //
    #ifdef RAD_XBOX
	LPDIRECT3D8             m_pD3D;			// Used to create the D3DDevice
	LPDIRECT3DDEVICE8       m_pd3dDevice;	// Our rendering device
	XFONT *                 m_pXFont;		// Our xbox font
	#endif // RAD_XBOX
};

#endif // file