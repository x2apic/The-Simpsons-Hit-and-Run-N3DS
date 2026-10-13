//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical Text Display Component.
//
// Description: This file implements a simple text display on the console
//              TV screen. This is useful for debugging on comsumer units.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <radtime.hpp>
#include <radtextdisplay.hpp>     // interface specification
#include <radobject.hpp>

#include "textdisplay.hpp"        // class specification

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



//===========================================================================
// Globals
//===========================================================================
#ifdef RAD_PS2

//
// Misc constants copied from PS2 boot loader.
//
#define PS2_SUNIT		0x01
#define PS2_PACKETSIZE	( 0x100 * PS2_SUNIT )
#define PS2_WORKSIZE	( 0x80 )
#define PS2_WORKBASE	0x70000000
#define PS2_SUPPER		( PS2_WORKBASE + PS2_WORKSIZE )
#define PS2_SLOWER		( PS2_WORKBASE + PS2_WORKSIZE + PS2_PACKETSIZE )

#define PS2_SCREEN_WIDTH        512 // Width in pixels of the mode we use.
#define PS2_SCREEN_HEIGHT       224 // Height in pixels of the mode we use.
#define PS2_BUF_OFFSET_X		( ( ( 4096 - PS2_SCREEN_WIDTH ) / 2 ) << 4 )
#define PS2_BUF_OFFSET_Y		( ( ( 4096 - PS2_SCREEN_HEIGHT ) / 2 ) << 4 )
#define PS2_CONSOLE_WIDTH		( PS2_SCREEN_WIDTH / 8 ) // Width in characters.
#define PS2_CONSOLE_HEIGHT		26  // Height in characters
#define PS2_SCREEN_BORDER_RIGHT 2   // Unusable chars on right side - offscreen.

#endif // RAD_PS2

#ifdef RAD_GAMECUBE

//
// Border around text on RAD_GAMECUBE - defines screen area actually visible.
//
#define GCN_SCREEN_BORDER_LEFT 2
#define GCN_SCREEN_BORDER_RIGHT 0
#define GCN_SCREEN_BORDER_TOP 14
#define GCN_SCREEN_BORDER_BOTTOM 24

//
// Font data
//
static char 
    g_TextDisplayGcnFontData[ 128 ][ GCN_TEXTDISPLAY_FONT_HEIGHT ][ GCN_TEXTDISPLAY_FONT_WIDTH ] 
        = GCN_TEXTDISPLAY_FONT;

#endif // RAD_GAMECUBE

//
// The display singleton
//
static radTextDisplay s_theDisplay;

bool radTextDisplay::m_Initialized = false;
radMemoryAllocator radTextDisplay::m_Alloc = RADMEMORY_ALLOC_DEFAULT;


//===========================================================================
// radTextDisplayGet
//===========================================================================
// Description: Return an interface handle on the text display object.
//              This addrefs the object as well. If the display has not been
//              initialized yet, that is done here.
//
// Constraints:	The caller is responsible for calling Release on the returned
//              object when finished with it.
//
// Parameters:	pNewWatch - place to store pointer to new object.
//              timeUnit - time unit for the new watch to use.
//
//===========================================================================

void radTextDisplayGet
( 
    IRadTextDisplay** pDisplay,
    radMemoryAllocator alloc
)
{
    rAssert( pDisplay != NULL );

    if( !s_theDisplay.m_Initialized )
    {
        s_theDisplay.Initialize( alloc );
    }

    IRadTextDisplay* display = &s_theDisplay;
    *pDisplay = display;
    radAddRef( *pDisplay, NULL );
}



//=============================================================================
// Function:    radTextDisplay::Initialize
//=============================================================================
// Description: Allocates a frame buffer, initializes the display using 
//              default colors, clears the screen etc.
//
// Constraints: Generally should only be called once at program startup.
//
// Parameters:  alloc - memory allocator to use internally.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Initialize( radMemoryAllocator alloc )
{
    rAssertMsg( !m_Initialized, "radTextDisplay already initialized." );

    //
    // Initialize easy stuff.
    //
    m_AutoSwap = true;
    m_Alloc = alloc;
    m_TextBuffer = NULL;
    m_BackgroundColorRGB = 0x000000;
    m_TextColorRGB = 0xffffff;
    m_CursorX = 0;
    m_CursorY = 0;
    m_CurFrame = 0;
    m_Width = 80;
    m_Height = 25;
    #ifdef RAD_PS2
    m_Console = 0;
    m_Width = PS2_CONSOLE_WIDTH - PS2_SCREEN_BORDER_RIGHT;
    m_Height = PS2_CONSOLE_HEIGHT;
    #endif // RAD_PS2
    #ifdef RAD_GAMECUBE
    m_First = true;
    m_FrameBufferSize = 0;
    m_FrameBuffer1 = NULL;
    m_FrameBuffer2 = NULL;
    m_CurBuffer = NULL;
    m_ScreenMode = &GXNtsc480Int;
    m_BackgroundColorYUV = RGBtoYUV( m_BackgroundColorRGB );
    m_TextColorYUV = RGBtoYUV( m_TextColorRGB );
    m_Width = ( ( m_ScreenMode->fbWidth - GCN_SCREEN_BORDER_LEFT - GCN_SCREEN_BORDER_RIGHT ) / 2 ) 
              / GCN_TEXTDISPLAY_FONT_WIDTH;
    m_Height = ( ( m_ScreenMode->xfbHeight - GCN_SCREEN_BORDER_TOP - GCN_SCREEN_BORDER_BOTTOM ) / 2 ) 
              / GCN_TEXTDISPLAY_FONT_HEIGHT;
    #endif // RAD_GAMECUBE

	#ifdef RAD_XBOX
	m_pD3D = NULL;
	m_pd3dDevice = NULL;
	m_pXFont = NULL;
	#endif // RAD_XBOX

    //
    // Set up the frame buffer.
    //
    m_TextBuffer = ( char* ) radMemoryAlloc( m_Alloc, sizeof( char ) * m_Width * m_Height );
    rAssert( m_TextBuffer != NULL );

    //
    // Clear and paint the display.
    //
    InitDisplay( );
    m_Initialized = true;
    Clear( );

    //
    // If this platform is unsupported, print a warning.
    //
    #if !defined RAD_PS2 && !defined RAD_GAMECUBE && !defined RAD_XBOX
    rDebugString( "WARNING: radTextDisplay is not supported on this platform.\n" );
    rDebugString( "         Screen output will be redirected to the debug channel.\n" );
    #endif // !RAD_PS2 & !RAD_GAMECUBE

}

   
//=============================================================================
// Function:    radTextDisplay::Terminate
//=============================================================================
// Description: Frees memory used by the text display and shuts down the video
//              output system.
//
// Constraints: Generally should only be called once, at program shutdown.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Terminate( void )
{
    rAssertMsg( m_Initialized, "radTextDisplay already terminated." );

    //
    // Do whatever is needed to shut down the hardware.
    //
    CloseDisplay( );

    //
    // Free the frame buffers
    //
    if( m_TextBuffer != NULL )
    {
        radMemoryFree( m_Alloc, m_TextBuffer );
        m_TextBuffer = NULL;
    }

    #ifdef RAD_GAMECUBE
    if( m_FrameBuffer1 != NULL )
    {
        radMemoryFreeAligned( m_Alloc, m_FrameBuffer1 );
        m_FrameBuffer1 = NULL;
    }
    if( m_FrameBuffer2 != NULL )
    {
        radMemoryFreeAligned( m_Alloc, m_FrameBuffer2 );
        m_FrameBuffer2 = NULL;
    }
    #endif // RAD_GAMECUBE

    //
    // Close down other state stuff.
    //
    m_Initialized = false;
}

//=============================================================================
// Function:    radTextDisplay::SetAutoSwap
//=============================================================================

void radTextDisplay::SetAutoSwap( bool on )
{
    m_AutoSwap = on;
}

//=============================================================================
// Function:    radTextDisplay::SwapBuffers
//=============================================================================

void radTextDisplay::SwapBuffers( void )
{
    Paint( );
}

//=============================================================================
// Function:    radTextDisplay::AddRef
//=============================================================================
// Description: Increment the reference count of this object.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::AddRef( void )
{
    m_ReferenceCount++;
}


//=============================================================================
// Function:    radTextDisplay::Release
//=============================================================================
// Description: Decrement reference count of this object.
//
// Parameters:  None.
//              
// Returns:     None.
//
// Notes:       If the reference count reaches zero, the display system
//              is automatically shut down.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount < 1 )
    {
        Terminate( );
    }
}


//=============================================================================
// Function:    radTextDisplay::SetBackgroundColor
//=============================================================================
// Description: Change the background color of the display.
//
// Parameters:  color - the 24-bit RGB color to use.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::SetBackgroundColor( unsigned int color )
{
    m_BackgroundColorRGB = color;

    #ifdef RAD_PS2
	m_DoubleBufferInfo.clear0.rgbaq.R = ( color >> 16 ) & 0xff;
	m_DoubleBufferInfo.clear0.rgbaq.G = ( color >> 8 ) & 0xff;
	m_DoubleBufferInfo.clear0.rgbaq.B = color & 0xff;
	m_DoubleBufferInfo.clear1.rgbaq.R = ( color >> 16 ) & 0xff;
	m_DoubleBufferInfo.clear1.rgbaq.G = ( color >> 8 ) & 0xff;
	m_DoubleBufferInfo.clear1.rgbaq.B = color & 0xff;
    #endif // RAD_PS2

    #ifdef RAD_GAMECUBE
    m_BackgroundColorYUV = RGBtoYUV( color );
    #endif // RAD_GAMECUBE

    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::SetTextColor
//=============================================================================
// Description: Change the text color of the display.
//
// Parameters:  color - the 24-bit RGB color to use.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::SetTextColor( unsigned int color )
{
    m_TextColorRGB = color;

    #ifdef RAD_PS2
    sceDevConsSetColor( m_Console, 7, ( color >> 16 ) & 0xff, ( color >> 8 ) & 0xff, color & 0xff );
    #endif // RAD_PS2

    #ifdef RAD_GAMECUBE
    m_TextColorYUV = RGBtoYUV( color );
    #endif // RAD_GAMECUBE

	#ifdef RAD_XBOX
    m_pXFont->SetTextColor( D3DCOLOR_XRGB( ( color >> 16 )& 0xff, ( color >> 8 ) & 0xff, color & 0xff ) );
	#endif

    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::Clear
//=============================================================================
// Description: Clear all text from the display and home the cursor.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Clear( void )
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );

    //
    // Clear the text buffer.
    //
    for( int i = 0; i < m_Width * m_Height; i++ )
    {
        m_TextBuffer[ i ] = '\0';
    }

    //
    // Home the cursor.
    //
    SetCursorPosition( 0, 0 );

    //
    // Redraw the screen.
    //
    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::GetDimensions
//=============================================================================
// Description: Get the width and height of the usable text display area.
//
// Parameters:  pWidth - place to store width
//              pHeight - place to store height
//              
// Returns:     None.
//
// Notes:       Dimensions are in character units, not pixels.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::GetDimensions( unsigned int* pWidth, unsigned int* pHeight ) const
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );

    if( pWidth != NULL )
    {
        *pWidth = ( unsigned int ) m_Width;
    }

    if( pHeight != NULL )
    {
        *pHeight = ( unsigned int ) m_Height;
    }
}


//=============================================================================
// Function:    radTextDisplay::SetCursorPosition
//=============================================================================
// Description: Change the cursor position for the next text update.
//
// Constraints: The position will be clamped to legal coordinates.
//
// Parameters:  x, y = new cursor position in character units.
//              
// Returns:     None.
//
// Notes:       (0,0) is the upper left corner. X increases right, Y down.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::SetCursorPosition( unsigned int x, unsigned int y )
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );

    if( ( int ) x < 0 )
    {
        m_CursorX = 0;
    }
    else if( ( int ) x >= m_Width )
    {
        m_CursorX = m_Width - 1;
    }
    else
    {
        m_CursorX = ( int ) x;
    }

    if( ( int ) y < 0 )
    {
        m_CursorY = 0;
    }
    else if( ( int ) y >= m_Height )
    {
        m_CursorY = m_Height - 1;
    }
    else
    {
        m_CursorY = ( int ) y;
    }
}


//=============================================================================
// Function:    radTextDisplay::TextOut
//=============================================================================
// Description: Prints a string on the display starting at the current cursor
//              position. Long lines will be wrapped at the screen width, and
//              the screen will scroll if the bottom is passed. The cursor
//              position is updated to the end of the string.
//
// Parameters:  pText - string to print.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::TextOut( const char*  pText )
{
    rAssertMsg( m_Initialized, "radTextDisplay not initialized!" );
    rAssert( pText != NULL );

    //
    // If this platform is unsupported, just echo the input to the debug channel.
    //
    #if !defined RAD_PS2 && !defined RAD_GAMECUBE && !defined RAD_XBOX
    rDebugString( pText );
    #endif // !RAD_PS2 & !RAD_GAMECUBE

    //
    // Loop over input characters.
    //
    char* ptr = ( char* )pText;
    while( *ptr != '\0' )
    {
        //
        // If it's a printable character, just output it.
        //
        if( isprint( *ptr ) )
        {
            WriteLetter( m_CursorX, m_CursorY, *ptr );
            m_CursorX++;
        }
        else
        {
            //
            // Otherwise, handle special control characters.
            //
            char c = *ptr;
            switch( c )
            {
                case 8:     // backspace
                {
                    if( m_CursorX > 0 )
                    {
                        m_CursorX--;
                    }
                    break;
                }
                case 9:     // tab
                {
                    m_CursorX += 4;
                    break;
                }
                case 10:    // linefeed
                {
                    m_CursorY++;
                    m_CursorX = 0;
                    break;
                }
                case 13:    // carriage return
                {
                    m_CursorY++;
                    m_CursorX = 0;
                    break;
                }
                case 127:   // backspace & delete
                {
                    if( m_CursorX > 0 )
                    {
                        m_CursorX--;
                        WriteLetter( m_CursorX, m_CursorY, 0 );
                    }
                    break;
                }
                default:    // anything else
                {
                    WriteLetter( m_CursorX, m_CursorY, c );
                    m_CursorX++;
                    break;
                }
            }
        }

        //
        // Handle line wrap.
        //
        while( m_CursorX >= m_Width )
        {
            m_CursorX -= m_Width;
            m_CursorY++;
        }

        //
        // Handle scrolling.
        //
        while( m_CursorY >= m_Height )
        {
            ScrollUp( );
        }

        ptr++;
    }

    //
    // Done with input - repaint the screen.
    //
    PaintIfAutoSwapOn( );
}


//=============================================================================
// Function:    radTextDisplay::TextOutAt
//=============================================================================
// Description: Prints a string on the display at a specified starting 
//              coordinate on the display. Wrapping and scrolling will still
//              occur as with TextOut, but the cursor position will not be
//              updated.
//
// Parameters:  pText - string to print.
//              x, y - location to print it at.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::TextOutAt( const char* pText, int x, int y )
{
    int cx = m_CursorX;
    int cy = m_CursorY;
    SetCursorPosition( x, y );
    TextOut( pText );
    SetCursorPosition( cx, cy );
}


//=============================================================================
// Private member functions
//=============================================================================

//=============================================================================
// Function:    radTextDisplay::WriteLetter
//=============================================================================
// Description: Stores a character in the memory map of the text on screen.
//              Does not actually update the visible display.
//
// Parameters:  x, y - location to print it at.
//              c - character to store.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::WriteLetter( int x, int y, char c )
{
    if( x < 0 || x >= m_Width )
    {
        return;
    }

    if( y < 0 || y >= m_Height )
    {
        return;
    }

    if( m_TextBuffer == NULL )
    {
        return;
    }

    m_TextBuffer[ x + y * m_Width ] = c;
}


//=============================================================================
// Function:    radTextDisplay::ScrollUp
//=============================================================================
// Description: Scrolls the text buffer up one line. Does not actually update
///             the visible display. Does update the cursor vertical position.
//
// Parameters:  None.
//              
// Returns:     None.
//
//------------------------------------------------------------------------------
    
void radTextDisplay::ScrollUp( void )
{
    //
    // Cursor scrolls with text.
    //
    m_CursorY--;

    if( m_TextBuffer == NULL )
    {
        return;
    }

    //
    // Copy all lines up one.
    //
    for( int y = 0; y < m_Height - 1; y++ )
    {
        for( int x = 0; x < m_Width; x++ )
        {
            m_TextBuffer[ x + y * m_Width ] = m_TextBuffer[ x + ( y + 1 ) * m_Width ];
        }
    }

    //
    // Clear the bottom line.
    //
    for( int x = 0; x < m_Width; x++ )
    {
        m_TextBuffer[ x + ( m_Height - 1 ) * m_Width ] = '\0';
    }
}


//=============================================================================
// Function:    radTextDisplay::RGBtoYUV
//=============================================================================
// Description: Converts from RGB color space to YUV color space for GameCube.
//
// Parameters:  color - RGB color in 24-bit format.
//              
// Returns:     Equivalent color in RAD_GAMECUBE 32-bit format (YUV color stored as YUYV).
//
//------------------------------------------------------------------------------
#ifdef RAD_GAMECUBE
    
unsigned int radTextDisplay::RGBtoYUV( unsigned int color ) const
{
    int red, green, blue;
    int lum, chrom, var;

    red = ( color >> 16 ) & 0xff;
    green = ( color >> 8 ) & 0xff;
    blue = color & 0xff;

    lum = 299 * green + 587 * red + 114 * blue;
    chrom = -147 * green - 289 * red + 437 * blue;
    var = 615 * green - 515 * red - 100 * blue;

    lum /= 1000;
    chrom /= 1000;
    chrom += 128;
    var /= 1000;
    var += 128;

    unsigned int yuv = ( lum & 0xff ) << 24;
    yuv += ( chrom & 0xff ) << 16;
    yuv += ( lum & 0xff ) << 8;
    yuv += ( var & 0xff );

    return( yuv );
}
#endif // RAD_GAMECUBE

//=============================================================================
// Function:    radTextDisplay::ReDrawScreenIfAutoPaintOn
//=============================================================================

void radTextDisplay::PaintIfAutoSwapOn( void )
{
    if ( m_AutoSwap == true )
    {
        Paint( );
    }
}

//=============================================================================
// Function:    radTextDisplay::ReDrawScreen
//=============================================================================
// Description: Repaints the entire text display with the current contents of
//              the text buffer.
//
// Parameters:  None.
//              
// Returns:     None
//
//------------------------------------------------------------------------------
    
void radTextDisplay::Paint( void )
{
    //
    // PS2 version.
    //
    #ifdef RAD_PS2

    char line[ 256 ];
    rAssert( m_Width < 255 );

    //
    // Clear the screen.
    //
	sceDevConsClear( m_Console );

    //
    // Loop over text buffer vertically.
    //
    for( int y = 0; y < m_Height; y++ )
    {
        //
        // Copy each line into a temp buffer and trim 
        // whitespace from the right end.
        //
        int x;
        for( x = 0; x < m_Width; x++ )
        {
            char c = m_TextBuffer[ x + m_Width * y ];
            if( c == 0 || ( c & 128 ) != 0 )
            {
                c = ' ';
            }
            line[ x ] = c;
        }
        line[ m_Width ] = '\0';
        x = m_Width - 1;
        while( line[ x ] == ' ' && x >= 0 )
        {
            line[ x ] = '\0';
            x--;
        }

        //
        // Output the text buffer as a string.
        //
    	sceDevConsLocate( m_Console, 0, y );
	    sceDevConsPrintf( m_Console, line );
    }

    //
    // Show the updated frame buffer.
    //
	sceDevConsDraw( m_Console );
    Ps2SwapBuffers( );

    #endif // RAD_PS2

    //
    // GameCube version.
    //
    #ifdef RAD_GAMECUBE

    //
    // Clear the screen.
    //
    VIWaitForRetrace( );
    GcnFillFrameBuffer( m_BackgroundColorYUV );

    //
    // Loop over entire text buffer; draw characters individually.
    //
    for( int y = 0; y < m_Height; y++ )
    {
        for( int x = 0; x < m_Width; x++ )
        {
            char c = m_TextBuffer[ x + m_Width * y ];
            if( ( c & 128 ) != 0 )
            {
                c = 0;
            }
            if( c != 0 )
            {
                PaintChar( x, y, c );
            }
        }
    }

    //
    // Show the updated frame buffer.
    //
    GcnSwapBuffers( );

    #endif // RAD_GAMECUBE

	#ifdef RAD_XBOX
    IDirect3DSurface8 * pD3DSurface = NULL;
	m_pd3dDevice->GetRenderTarget( & pD3DSurface );

	unsigned int uFontW = 0;
	m_pXFont->GetTextExtent( L"a", -1, & uFontW );
	unsigned int uFontH = m_pXFont->GetTextHeight( );

    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 
                         D3DCOLOR_XRGB( ( m_BackgroundColorRGB >> 16 ) & 0xff, ( m_BackgroundColorRGB >> 8 ) & 0xff, m_BackgroundColorRGB & 0xff ), 1.0f, 0L );

    //
    // Loop over entire text buffer; draw characters individually.
    //
    for( int y = 0; y < m_Height; y++ )
    {
		unsigned int nCurrentXPos = 0;
		unsigned int nCharWidth = 0;
        for( int x = 0; x < m_Width; x++ )
        {
            char c = m_TextBuffer[ x + m_Width * y ];
            if( ( c & 128 ) != 0 )
            {
                c = 0;
            }

            if( c != 0 )
            {
				WCHAR cChar = c;
                m_pXFont->TextOut( pD3DSurface, &cChar, 1, nCurrentXPos, y * uFontH );
				m_pXFont->GetTextExtent( &cChar, 1, & nCharWidth );
				nCurrentXPos += nCharWidth;
            }
			else
			{
				nCurrentXPos += uFontW;
			}
        }
    }
	pD3DSurface->Release( );
    m_pd3dDevice->Present( NULL, NULL, NULL, NULL );

	#endif
}


//=============================================================================
// Function:    radTextDisplay::PaintChar
//=============================================================================
// Description: Paints a character one pixel at a time on the screen.
//
// Parameters:  cx, cy - character coordinates to paint at.
//              c - letter th draw.
//              
// Returns:     None
//
//------------------------------------------------------------------------------
#ifdef RAD_GAMECUBE
    
void radTextDisplay::PaintChar( int cx, int cy, char c )
{
    //
    // Calculate character origin in pixel coordinates.
    //
    int left = GCN_SCREEN_BORDER_LEFT + cx * GCN_TEXTDISPLAY_FONT_WIDTH;
    int top = GCN_SCREEN_BORDER_TOP + cy * GCN_TEXTDISPLAY_FONT_HEIGHT;
    int index = ( int )( unsigned char ) c;
    u8*         ptr;

    //
    // Loop over font mask for this character.
    //
    for( int y = 0; y < GCN_TEXTDISPLAY_FONT_HEIGHT; y++ )
    {
        for( int x = 0; x < GCN_TEXTDISPLAY_FONT_WIDTH; x++ )
        { 
            //
            // If the font mask is a 1, plot a pixel in the text color.
            //
            if( g_TextDisplayGcnFontData[ index ][ y ][ x ] != 0 )
            {
                ptr = m_CurBuffer + ( left + x + ( y + top ) * m_ScreenMode->fbWidth ) * VI_DISPLAY_PIX_SZ * 2;
                * (u32*) ptr = m_TextColorYUV;
            }
        }
    }
}
#endif // RAD_GAMECUBE


//=============================================================================
// Function:    radTextDisplay::InitDisplay
//=============================================================================
// Description: Calls the appropriate lib functions to init display hardware.
//
// Parameters:  None.
//              
// Returns:     None
//
//------------------------------------------------------------------------------
void radTextDisplay::InitDisplay( void )
{
    //
    // PS2 version.
    //
    #ifdef RAD_PS2

    //
    // Initialize the screen into text console mode.
    //
    Ps2InitConsole( );

    //
    // Show the empty screen.
    //
    Ps2SwapBuffers( );

    //
    // Init the SCE dev console kit.
    //
    sceDevConsInit( );
    m_Console = sceDevConsOpen( PS2_BUF_OFFSET_X + ( 8 << 4 ), 
                                PS2_BUF_OFFSET_Y + ( 8 << 4 ), 
                                PS2_CONSOLE_WIDTH, 
                                PS2_CONSOLE_HEIGHT );

    //
    // Set the foreground color.
    //
    sceDevConsSetColor( m_Console, 7, ( m_TextColorRGB >> 16 ) & 0xff, 
                        ( m_TextColorRGB >> 8 ) & 0xff, m_TextColorRGB & 0xff );


    #endif // RAD_PS2

    //
    // GameCube version.
    //
    #ifdef RAD_GAMECUBE

    //
    // Call another function to set everything up.
    //
    GcnInitConsole( );

    #endif // RAD_GAMECUBE


	//------------------------------------------------------------------------
	// Initialize XBox D3D
	//------------------------------------------------------------------------
	#ifdef RAD_XBOX
    // Create the D3D object, which is used to create the D3DDevice.
    if( NULL == ( m_pD3D = Direct3DCreate8( D3D_SDK_VERSION ) ) )
	{
		rAssertMsg( false, "radTextDisplay: Error: failed to create D3DDevice.\n" );
	}

    // Set up the structure used to create the D3DDevice.
    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    
    // Set fullscreen 640x480x32 mode
    d3dpp.BackBufferWidth        = 640;
    d3dpp.BackBufferHeight       = 480;
    d3dpp.BackBufferFormat       = D3DFMT_X8R8G8B8;

    // Create one backbuffer and a zbuffer
    d3dpp.BackBufferCount        = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    // Set up how the backbuffer is "presented" to the frontbuffer each frame
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;

    // Create the Direct3D device. Hardware vertex processing is specified 
    // since all vertex processing takes place on Xbox hardware.
    if( FAILED( m_pD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL,
                                      D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                      &d3dpp, &m_pd3dDevice ) ) )
	{
		rAssertMsg( false, "radTextDisplay: Error: failed to create D3DDevice.\n" );
	}
    XFONT_OpenDefaultFont( & m_pXFont );

	#endif
}

//=============================================================================
// Function:    radTextDisplay::CloseDisplay
//=============================================================================
// Description: Calls the appropriate lib functions to shut down display hardware.
//
// Parameters:  None.
//              
// Returns:     None
//
//------------------------------------------------------------------------------

void radTextDisplay::CloseDisplay( void )
{
    //
    // PS2 version.
    //
    #ifdef RAD_PS2

    //
    // Shut down dev console.
    //
    sceDevConsClose( m_Console );
    //sceSifExitCmd( );

    //
    // Flush caches.
    //
    //DI( );
    FlushCache( 0 );
    //FlushCache( 2 );

    #endif // RAD_PS2

    //
    // GameCube version.
    //
    #ifdef RAD_GAMECUBE

    //
    // Don't need to do anything special here.
    //

    #endif // RAD_GAMECUBE

	#ifdef RAD_XBOX
	m_pd3dDevice->Release( );
	m_pD3D->Release( );
	m_pd3dDevice = NULL;
	m_pD3D = NULL;
	#endif
}



//=============================================================================
// Function:    radTextDisplay::Ps2InitConsole
//=============================================================================
// Description: This routine was stolen from code stolen from PS2 sample code. 
//              It initializes the PS2 display to output text.
// 
// Parameters:  None.
//
// Returns:     None.
//
//------------------------------------------------------------------------------
#ifdef RAD_PS2

void radTextDisplay::Ps2InitConsole( void )
{
    //
    // Reset a bunch of things.
    //
	int sindex;
	sceVif1Packet packet[ 2 ];
	sceDmaEnv env;
	sceDmaChan *p1;
	u_long giftagAD[ 2 ] = { SCE_GIF_SET_TAG( 0, 1, 0, 0, 0, 1 ), 0x000000000000000eL };
	sceDevVif0Reset( );
	sceDevVu0Reset( );
	sceGsResetPath( );
	sceDmaReset( 1 );

	sceVif1PkInit( &packet[ 0 ], ( u_long128*) PS2_SUPPER );
	sceVif1PkInit( &packet[ 1 ], ( u_long128*) PS2_SLOWER );

    //
    // Set up some DMA stuff.
    //
	sceDmaGetEnv( &env );
	env.notify = 1 << SCE_DMA_VIF1;
	sceDmaPutEnv( &env );

	p1 = sceDmaGetChan( SCE_DMA_VIF1 );
	p1->chcr.TTE = 1;

    //
    // Reset graphics system and enable double buffering.
    //
	sceGsResetGraph( 0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME );
	sceGsSetDefDBuff( &m_DoubleBufferInfo, SCE_GS_PSMCT32, PS2_SCREEN_WIDTH, PS2_SCREEN_HEIGHT,
	                  SCE_GS_ZGEQUAL, SCE_GS_PSMZ24, SCE_GS_CLEAR );

    //
    // Set background color for both frame buffers.
    //
	m_DoubleBufferInfo.clear0.rgbaq.R = ( m_BackgroundColorRGB >> 16 ) & 0xff;
	m_DoubleBufferInfo.clear0.rgbaq.G = ( m_BackgroundColorRGB >> 8 ) & 0xff;
	m_DoubleBufferInfo.clear0.rgbaq.B = m_BackgroundColorRGB & 0xff;
	m_DoubleBufferInfo.clear1.rgbaq.R = ( m_BackgroundColorRGB >> 16 ) & 0xff;
	m_DoubleBufferInfo.clear1.rgbaq.G = ( m_BackgroundColorRGB >> 8 ) & 0xff;
	m_DoubleBufferInfo.clear1.rgbaq.B = m_BackgroundColorRGB & 0xff;

	FlushCache( 0 );

    //
    // Set up some other mysterious thingies.
    //
	sindex = 0;
	sceVif1PkReset( &packet[ sindex ] );
	sceVif1PkCnt( &packet[ sindex ], 0 );
	sceVif1PkOpenDirectCode( &packet[ sindex ], 0 );
	sceVif1PkOpenGifTag( &packet[ sindex ], *( u_long128* ) &giftagAD[ 0 ] );

	sceVif1PkReserve( &packet[ sindex ],
		sceGsSetDefAlphaEnv( ( sceGsAlphaEnv*) packet[ sindex ].pCurrent, 0 ) * 4);
	sceVif1PkCloseGifTag( &packet[ sindex ] );
	sceVif1PkCloseDirectCode( &packet[ sindex ] );
	sceVif1PkEnd( &packet[ sindex ], 0 );
	sceVif1PkTerminate( &packet[ sindex ] );

    //
    // Tell the Gs to go init itself.
    //
	sceDmaSend( p1, ( u_int* )
		( ( ( u_int ) packet[ sindex ].pBase & 0x0fffffff ) | 0x80000000 ) );

    //
    // Wait for vsync.
    //
    while( !sceGsSyncV( 0 ) ) { }
}
#endif // RAD_PS2


//=============================================================================
// Function:    radTextDisplay::Ps2SwapBuffers
//=============================================================================
// Description: This function swaps text draw buffers on the PS2.
// 
// Parameters:  None.
//
// Returns:     None.
//
//------------------------------------------------------------------------------
#ifdef RAD_PS2

void radTextDisplay::Ps2SwapBuffers( void )
{
    //
    // Set up for double-buffer swap.
    //
	sceGsSyncPath( 0, 0 );
	sceGsSwapDBuff( &m_DoubleBufferInfo, m_CurFrame );
	m_CurFrame++;

    //
    // Show the appropriate buffer.
    //
	if( m_CurFrame & 0x01 )
	{	/* interlace half pixel adjust */
		sceGsSetHalfOffset( &m_DoubleBufferInfo.draw1, 2048, 2048, sceGsSyncV( 0 ) ^ 0x01 );
	}
	else
	{
		sceGsSetHalfOffset( &m_DoubleBufferInfo.draw0, 2048, 2048, sceGsSyncV( 0 ) ^ 0x01 );
	}

    //
    // Flush caches.
    //
	FlushCache( 0 );
	sceGsSyncPath( 0, 0 );
}
#endif // RAD_PS2




//=============================================================================
// Function:    radTextDisplay::GcnAllocateFrameBuffers
//=============================================================================
// Description: Allocates two graphical frame buffers.
// 
// Parameters:  None.
//
// Returns:     None.
//
//------------------------------------------------------------------------------
#ifdef RAD_GAMECUBE

void radTextDisplay::GcnAllocateFrameBuffer( void )
{
    void*       arenaLo;

    //
    // Allocate two 32-byte aligned frame buffers.
    //
    m_FrameBuffer1 = ( u8* ) radMemoryAllocAligned( RADMEMORY_ALLOC_DEFAULT, 
                                      m_FrameBufferSize*VI_DISPLAY_PIX_SZ * 2, 32 );
    rAssert( m_FrameBuffer1 != NULL );

    m_FrameBuffer2 = ( u8* ) radMemoryAllocAligned( RADMEMORY_ALLOC_DEFAULT, 
                                      m_FrameBufferSize*VI_DISPLAY_PIX_SZ * 2, 32 );
    rAssert( m_FrameBuffer2 != NULL );

    //
    // Init double-buffering pointer.
    //
    m_CurBuffer = m_FrameBuffer1;
}
#endif // RAD_GAMECUBE


//=============================================================================
// Function:    radTextDisplay::GcnFillFrameBuffer
//=============================================================================
// Description: Fill a frame buffer with a solid color.
// 
// Parameters:  color - YUYV color to use.
//
// Returns:     None.
//
//------------------------------------------------------------------------------
#ifdef RAD_GAMECUBE

void radTextDisplay::GcnFillFrameBuffer( u32 color )
{
    u8*         ptr;

    for (ptr = m_CurBuffer; ptr < m_CurBuffer + m_FrameBufferSize; ptr += VI_DISPLAY_PIX_SZ * 2)
    {
        *( u32* ) ptr = color;
    }
}
#endif // RAD_GAMECUBE


//=============================================================================
// Function:    radTextDisplay::GcnInitConsole
//=============================================================================
// Description: Initializes the GameCube display system for displaying text.
// 
// Parameters:  None.
//
// Returns:     None.
//
//------------------------------------------------------------------------------
#ifdef RAD_GAMECUBE

void radTextDisplay::GcnInitConsole( void )
{
    //
    // Init the video system.
    // We don't call OSInit here because that is done in radplatform.
    //
    VIInit( );

    //
    // Calculate frame buffer size.
    // Note that each line width should be a multiple of 16.
    m_FrameBufferSize = ( u32 )( VIPadFrameBufferWidth( m_ScreenMode->fbWidth )
                      * m_ScreenMode->xfbHeight * VI_DISPLAY_PIX_SZ );

    //
    // Allocate us a coupla frame buffers.
    //
    GcnAllocateFrameBuffer( );

    //
    // Configure the video system for 640x480 non-interlaced mode.
    //
    VIConfigure( m_ScreenMode );

    //
    // Need to "flush" so that the VI changes so far takes effect
    // from the following field.
    //
    VIFlush( );
    VIWaitForRetrace( );

    //
    // Since the tv mode is not interlaced after VIInit,
    // we don't need to wait for one more frame to make sure
    // that the mode is switched from interlace to non-interlace
    //
    //VIWaitForRetrace( );
}
#endif // RAD_GAMECUBE


//=============================================================================
// Function:    radTextDisplay::GcnSwapBuffers
//=============================================================================
// Description: Swap display buffers on the GameCube.
// 
// Parameters:  None.
//
// Returns:     None.
//
//------------------------------------------------------------------------------
#ifdef RAD_GAMECUBE

void radTextDisplay::GcnSwapBuffers( void )
{
    // 
    // Copy the newly drawn frame buffer to the graphics hardware.
    //
    DCStoreRange( ( void* ) m_CurBuffer, m_FrameBufferSize );
    VISetNextFrameBuffer( ( void* ) m_CurBuffer);

    //
    // If this is the first time, we need to kick the hardware.
    //
    if( m_First )
    {                
        VISetBlack( FALSE );
        m_First = false;
    }

    //
    // Flush video commands.
    //
    VIFlush( );

    //
    // Draw into the other frame buffer now.
    //
    m_CurFrame++;
    m_CurBuffer = ( m_CurFrame & 0x1 ) ? m_FrameBuffer2 : m_FrameBuffer1;
}
#endif // RAD_GAMECUBE
