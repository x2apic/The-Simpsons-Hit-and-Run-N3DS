//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Font data for Radical Text Display Console.
//
// Description: This file contains a bitmapped font for drawing text on
//              the GameCube display.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __TEXTDISPLAYFONT_H
#define __TEXTDISPLAYFONT_H

//===========================================================================
// Data Definitions
//===========================================================================

//
// This constant data is used to initialize the radTextDisplay's font data
// for plotting on the GameCube screen. The first 128 ASCII characters are
// defined. Each is a 5x7 fixed-width character with a border included,
// making the actual bitmap 6x8 pixels.
//

#define GCN_TEXTDISPLAY_FONT_WIDTH  6
#define GCN_TEXTDISPLAY_FONT_HEIGHT 8

// null
#define GCN_TEXTDISPLAY_FONT_00 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// SOH
#define GCN_TEXTDISPLAY_FONT_01 { \
        { 0,1,1,1,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,1,0,1,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// For now most of the seldom-used characters are just copies of the SOH happy face.
// STX
#define GCN_TEXTDISPLAY_FONT_02 GCN_TEXTDISPLAY_FONT_01

// ETX
#define GCN_TEXTDISPLAY_FONT_03 GCN_TEXTDISPLAY_FONT_01

// EOI
#define GCN_TEXTDISPLAY_FONT_04 GCN_TEXTDISPLAY_FONT_01

// ENQ
#define GCN_TEXTDISPLAY_FONT_05 GCN_TEXTDISPLAY_FONT_01

// ACK
#define GCN_TEXTDISPLAY_FONT_06 GCN_TEXTDISPLAY_FONT_01

// BEL
#define GCN_TEXTDISPLAY_FONT_07 GCN_TEXTDISPLAY_FONT_01

// BS
#define GCN_TEXTDISPLAY_FONT_08 GCN_TEXTDISPLAY_FONT_01

// HT
#define GCN_TEXTDISPLAY_FONT_09 GCN_TEXTDISPLAY_FONT_01

// LF
#define GCN_TEXTDISPLAY_FONT_0A GCN_TEXTDISPLAY_FONT_01

// VT
#define GCN_TEXTDISPLAY_FONT_0B GCN_TEXTDISPLAY_FONT_01

// FF
#define GCN_TEXTDISPLAY_FONT_0C GCN_TEXTDISPLAY_FONT_01

// CR
#define GCN_TEXTDISPLAY_FONT_0D GCN_TEXTDISPLAY_FONT_01

// S0
#define GCN_TEXTDISPLAY_FONT_0E GCN_TEXTDISPLAY_FONT_01

// S1
#define GCN_TEXTDISPLAY_FONT_0F GCN_TEXTDISPLAY_FONT_01

// SLE
#define GCN_TEXTDISPLAY_FONT_10 GCN_TEXTDISPLAY_FONT_01

// CS1
#define GCN_TEXTDISPLAY_FONT_11 GCN_TEXTDISPLAY_FONT_01

// DC2
#define GCN_TEXTDISPLAY_FONT_12 GCN_TEXTDISPLAY_FONT_01

// DC3
#define GCN_TEXTDISPLAY_FONT_13 GCN_TEXTDISPLAY_FONT_01

// DC4
#define GCN_TEXTDISPLAY_FONT_14 GCN_TEXTDISPLAY_FONT_01

// NAK
#define GCN_TEXTDISPLAY_FONT_15 GCN_TEXTDISPLAY_FONT_01

// SYN
#define GCN_TEXTDISPLAY_FONT_16 GCN_TEXTDISPLAY_FONT_01

// ETB
#define GCN_TEXTDISPLAY_FONT_17 GCN_TEXTDISPLAY_FONT_01

// CAN
#define GCN_TEXTDISPLAY_FONT_18 GCN_TEXTDISPLAY_FONT_01

// EM
#define GCN_TEXTDISPLAY_FONT_19 GCN_TEXTDISPLAY_FONT_01

// SUB
#define GCN_TEXTDISPLAY_FONT_1A GCN_TEXTDISPLAY_FONT_01

// ESC
#define GCN_TEXTDISPLAY_FONT_1B GCN_TEXTDISPLAY_FONT_01

// FS
#define GCN_TEXTDISPLAY_FONT_1C GCN_TEXTDISPLAY_FONT_01

// GS
#define GCN_TEXTDISPLAY_FONT_1D GCN_TEXTDISPLAY_FONT_01

// RS
#define GCN_TEXTDISPLAY_FONT_1E GCN_TEXTDISPLAY_FONT_01

// US
#define GCN_TEXTDISPLAY_FONT_1F GCN_TEXTDISPLAY_FONT_01

// space
#define GCN_TEXTDISPLAY_FONT_20 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// !
#define GCN_TEXTDISPLAY_FONT_21 { \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// "
#define GCN_TEXTDISPLAY_FONT_22 { \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// #
#define GCN_TEXTDISPLAY_FONT_23 { \
        { 0,1,0,1,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// $
#define GCN_TEXTDISPLAY_FONT_24 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,0,1,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// %
#define GCN_TEXTDISPLAY_FONT_25 { \
        { 0,0,0,0,1, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// &
#define GCN_TEXTDISPLAY_FONT_26 { \
        { 0,1,0,0,0, 0 },   \
        { 1,0,1,0,0, 0 },   \
        { 1,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 0,1,1,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// '
#define GCN_TEXTDISPLAY_FONT_27 { \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// (
#define GCN_TEXTDISPLAY_FONT_28 { \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// )
#define GCN_TEXTDISPLAY_FONT_29 { \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// *
#define GCN_TEXTDISPLAY_FONT_2A { \
        { 0,0,0,0,0, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// +
#define GCN_TEXTDISPLAY_FONT_2B { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// ,
#define GCN_TEXTDISPLAY_FONT_2C { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// -
#define GCN_TEXTDISPLAY_FONT_2D { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// .
#define GCN_TEXTDISPLAY_FONT_2E { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// /
#define GCN_TEXTDISPLAY_FONT_2F { \
        { 0,0,0,0,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 0
#define GCN_TEXTDISPLAY_FONT_30 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,1,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,1,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 1
#define GCN_TEXTDISPLAY_FONT_31 { \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 2
#define GCN_TEXTDISPLAY_FONT_32 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 3
#define GCN_TEXTDISPLAY_FONT_33 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 4
#define GCN_TEXTDISPLAY_FONT_34 { \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 5
#define GCN_TEXTDISPLAY_FONT_35 { \
        { 1,1,1,1,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,1,1,1,0, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 6
#define GCN_TEXTDISPLAY_FONT_36 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 7
#define GCN_TEXTDISPLAY_FONT_37 { \
        { 1,1,1,1,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 8
#define GCN_TEXTDISPLAY_FONT_38 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// 9
#define GCN_TEXTDISPLAY_FONT_39 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// :
#define GCN_TEXTDISPLAY_FONT_3A { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// ;
#define GCN_TEXTDISPLAY_FONT_3B { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// <
#define GCN_TEXTDISPLAY_FONT_3C { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// =
#define GCN_TEXTDISPLAY_FONT_3D { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// >
#define GCN_TEXTDISPLAY_FONT_3E { \
        { 0,0,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// ?
#define GCN_TEXTDISPLAY_FONT_3F { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// @
#define GCN_TEXTDISPLAY_FONT_40 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,1,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// A
#define GCN_TEXTDISPLAY_FONT_41 { \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// B
#define GCN_TEXTDISPLAY_FONT_42 { \
        { 1,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// C
#define GCN_TEXTDISPLAY_FONT_43 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// D
#define GCN_TEXTDISPLAY_FONT_44 { \
        { 1,1,1,0,0, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 1,1,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// E
#define GCN_TEXTDISPLAY_FONT_45 { \
        { 1,1,1,1,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,1,1,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// F
#define GCN_TEXTDISPLAY_FONT_46 { \
        { 1,1,1,1,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,1,1,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// G
#define GCN_TEXTDISPLAY_FONT_47 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,1,1,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// H
#define GCN_TEXTDISPLAY_FONT_48 { \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,1,1,1,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// I
#define GCN_TEXTDISPLAY_FONT_49 { \
        { 0,1,1,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// J
#define GCN_TEXTDISPLAY_FONT_4A { \
        { 0,0,1,1,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 0,1,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// K
#define GCN_TEXTDISPLAY_FONT_4B { \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 1,0,1,0,0, 0 },   \
        { 1,1,0,0,0, 0 },   \
        { 1,0,1,0,0, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// L
#define GCN_TEXTDISPLAY_FONT_4C { \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// M
#define GCN_TEXTDISPLAY_FONT_4D { \
        { 1,0,0,0,1, 0 },   \
        { 1,1,0,1,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// N
#define GCN_TEXTDISPLAY_FONT_4E { \
        { 1,0,0,0,1, 0 },   \
        { 1,1,0,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,0,1,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// O
#define GCN_TEXTDISPLAY_FONT_4F { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// P
#define GCN_TEXTDISPLAY_FONT_50 { \
        { 1,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,1,1,1,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// Q
#define GCN_TEXTDISPLAY_FONT_51 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 0,1,1,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// R
#define GCN_TEXTDISPLAY_FONT_52 { \
        { 1,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,1,1,1,0, 0 },   \
        { 1,0,1,0,0, 0 },   \
        { 1,0,0,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// S
#define GCN_TEXTDISPLAY_FONT_53 { \
        { 0,1,1,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// T
#define GCN_TEXTDISPLAY_FONT_54 { \
        { 1,1,1,1,1, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// U
#define GCN_TEXTDISPLAY_FONT_55 { \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// V
#define GCN_TEXTDISPLAY_FONT_56 { \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// W
#define GCN_TEXTDISPLAY_FONT_57 { \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// X
#define GCN_TEXTDISPLAY_FONT_58 { \
        { 1,0,0,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// Y
#define GCN_TEXTDISPLAY_FONT_59 { \
        { 1,0,0,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// Z
#define GCN_TEXTDISPLAY_FONT_5A { \
        { 1,1,1,1,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 1,0,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// [
#define GCN_TEXTDISPLAY_FONT_5B { \
        { 0,1,1,1,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// '\'
#define GCN_TEXTDISPLAY_FONT_5C { \
        { 1,0,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// ]
#define GCN_TEXTDISPLAY_FONT_5D { \
        { 0,1,1,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// ^
#define GCN_TEXTDISPLAY_FONT_5E { \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// _
#define GCN_TEXTDISPLAY_FONT_5F { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// `
#define GCN_TEXTDISPLAY_FONT_60 { \
        { 0,1,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// a
#define GCN_TEXTDISPLAY_FONT_61 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,1,1,1,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// b
#define GCN_TEXTDISPLAY_FONT_62 { \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// c
#define GCN_TEXTDISPLAY_FONT_63 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,1, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// d
#define GCN_TEXTDISPLAY_FONT_64 { \
        { 0,0,0,0,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,1,1,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,0,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// e
#define GCN_TEXTDISPLAY_FONT_65 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// f
#define GCN_TEXTDISPLAY_FONT_66 { \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,1, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,1,1, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// g
#define GCN_TEXTDISPLAY_FONT_67 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,0,1,1,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// h
#define GCN_TEXTDISPLAY_FONT_68 { \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// i
#define GCN_TEXTDISPLAY_FONT_69 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// j
#define GCN_TEXTDISPLAY_FONT_6A { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,1,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// k
#define GCN_TEXTDISPLAY_FONT_6B { \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,1,0,0, 0 },   \
        { 0,1,1,0,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// l
#define GCN_TEXTDISPLAY_FONT_6C { \
        { 0,1,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// m
#define GCN_TEXTDISPLAY_FONT_6D { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,1,0,1,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// n
#define GCN_TEXTDISPLAY_FONT_6E { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// o
#define GCN_TEXTDISPLAY_FONT_6F { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,0,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// p
#define GCN_TEXTDISPLAY_FONT_70 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// q
#define GCN_TEXTDISPLAY_FONT_71 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// r
#define GCN_TEXTDISPLAY_FONT_72 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,0,1,1,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// s
#define GCN_TEXTDISPLAY_FONT_73 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,1,0, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// t
#define GCN_TEXTDISPLAY_FONT_74 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// u
#define GCN_TEXTDISPLAY_FONT_75 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,0,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// v
#define GCN_TEXTDISPLAY_FONT_76 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// w
#define GCN_TEXTDISPLAY_FONT_77 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 1,0,1,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// x
#define GCN_TEXTDISPLAY_FONT_78 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 1,0,0,0,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// y
#define GCN_TEXTDISPLAY_FONT_79 { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,1,0,0,1, 0 },   \
        { 0,0,1,1,1, 0 },   \
        { 0,0,0,0,1, 0 },   \
        { 0,0,1,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// z
#define GCN_TEXTDISPLAY_FONT_7A { \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,1,1,1,1, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,1,1,1, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// {
#define GCN_TEXTDISPLAY_FONT_7B { \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// |
#define GCN_TEXTDISPLAY_FONT_7C { \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// }
#define GCN_TEXTDISPLAY_FONT_7D { \
        { 0,1,0,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,0,1,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,0,1,0,0, 0 },   \
        { 0,1,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// ~
#define GCN_TEXTDISPLAY_FONT_7E { \
        { 0,0,1,0,1, 0 },   \
        { 0,1,0,1,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
        { 0,0,0,0,0, 0 },   \
                            \
        { 0,0,0,0,0, 0 }    \
    }

// DEL
#define GCN_TEXTDISPLAY_FONT_7F GCN_TEXTDISPLAY_FONT_01


//
// This table combines all the above bitmaps into one enormous macro.
// This macro can be used to initialize a 128x8x6 table with static font data.
//
#define GCN_TEXTDISPLAY_FONT {  \
    GCN_TEXTDISPLAY_FONT_00,    \
    GCN_TEXTDISPLAY_FONT_01,    \
    GCN_TEXTDISPLAY_FONT_02,    \
    GCN_TEXTDISPLAY_FONT_03,    \
    GCN_TEXTDISPLAY_FONT_04,    \
    GCN_TEXTDISPLAY_FONT_05,    \
    GCN_TEXTDISPLAY_FONT_06,    \
    GCN_TEXTDISPLAY_FONT_07,    \
    GCN_TEXTDISPLAY_FONT_08,    \
    GCN_TEXTDISPLAY_FONT_09,    \
    GCN_TEXTDISPLAY_FONT_0A,    \
    GCN_TEXTDISPLAY_FONT_0B,    \
    GCN_TEXTDISPLAY_FONT_0C,    \
    GCN_TEXTDISPLAY_FONT_0D,    \
    GCN_TEXTDISPLAY_FONT_0E,    \
    GCN_TEXTDISPLAY_FONT_0F,    \
    GCN_TEXTDISPLAY_FONT_10,    \
    GCN_TEXTDISPLAY_FONT_11,    \
    GCN_TEXTDISPLAY_FONT_12,    \
    GCN_TEXTDISPLAY_FONT_13,    \
    GCN_TEXTDISPLAY_FONT_14,    \
    GCN_TEXTDISPLAY_FONT_15,    \
    GCN_TEXTDISPLAY_FONT_16,    \
    GCN_TEXTDISPLAY_FONT_17,    \
    GCN_TEXTDISPLAY_FONT_18,    \
    GCN_TEXTDISPLAY_FONT_19,    \
    GCN_TEXTDISPLAY_FONT_1A,    \
    GCN_TEXTDISPLAY_FONT_1B,    \
    GCN_TEXTDISPLAY_FONT_1C,    \
    GCN_TEXTDISPLAY_FONT_1D,    \
    GCN_TEXTDISPLAY_FONT_1E,    \
    GCN_TEXTDISPLAY_FONT_1F,    \
    GCN_TEXTDISPLAY_FONT_20,    \
    GCN_TEXTDISPLAY_FONT_21,    \
    GCN_TEXTDISPLAY_FONT_22,    \
    GCN_TEXTDISPLAY_FONT_23,    \
    GCN_TEXTDISPLAY_FONT_24,    \
    GCN_TEXTDISPLAY_FONT_25,    \
    GCN_TEXTDISPLAY_FONT_26,    \
    GCN_TEXTDISPLAY_FONT_27,    \
    GCN_TEXTDISPLAY_FONT_28,    \
    GCN_TEXTDISPLAY_FONT_29,    \
    GCN_TEXTDISPLAY_FONT_2A,    \
    GCN_TEXTDISPLAY_FONT_2B,    \
    GCN_TEXTDISPLAY_FONT_2C,    \
    GCN_TEXTDISPLAY_FONT_2D,    \
    GCN_TEXTDISPLAY_FONT_2E,    \
    GCN_TEXTDISPLAY_FONT_2F,    \
    GCN_TEXTDISPLAY_FONT_30,    \
    GCN_TEXTDISPLAY_FONT_31,    \
    GCN_TEXTDISPLAY_FONT_32,    \
    GCN_TEXTDISPLAY_FONT_33,    \
    GCN_TEXTDISPLAY_FONT_34,    \
    GCN_TEXTDISPLAY_FONT_35,    \
    GCN_TEXTDISPLAY_FONT_36,    \
    GCN_TEXTDISPLAY_FONT_37,    \
    GCN_TEXTDISPLAY_FONT_38,    \
    GCN_TEXTDISPLAY_FONT_39,    \
    GCN_TEXTDISPLAY_FONT_3A,    \
    GCN_TEXTDISPLAY_FONT_3B,    \
    GCN_TEXTDISPLAY_FONT_3C,    \
    GCN_TEXTDISPLAY_FONT_3D,    \
    GCN_TEXTDISPLAY_FONT_3E,    \
    GCN_TEXTDISPLAY_FONT_3F,    \
    GCN_TEXTDISPLAY_FONT_40,    \
    GCN_TEXTDISPLAY_FONT_41,    \
    GCN_TEXTDISPLAY_FONT_42,    \
    GCN_TEXTDISPLAY_FONT_43,    \
    GCN_TEXTDISPLAY_FONT_44,    \
    GCN_TEXTDISPLAY_FONT_45,    \
    GCN_TEXTDISPLAY_FONT_46,    \
    GCN_TEXTDISPLAY_FONT_47,    \
    GCN_TEXTDISPLAY_FONT_48,    \
    GCN_TEXTDISPLAY_FONT_49,    \
    GCN_TEXTDISPLAY_FONT_4A,    \
    GCN_TEXTDISPLAY_FONT_4B,    \
    GCN_TEXTDISPLAY_FONT_4C,    \
    GCN_TEXTDISPLAY_FONT_4D,    \
    GCN_TEXTDISPLAY_FONT_4E,    \
    GCN_TEXTDISPLAY_FONT_4F,    \
    GCN_TEXTDISPLAY_FONT_50,    \
    GCN_TEXTDISPLAY_FONT_51,    \
    GCN_TEXTDISPLAY_FONT_52,    \
    GCN_TEXTDISPLAY_FONT_53,    \
    GCN_TEXTDISPLAY_FONT_54,    \
    GCN_TEXTDISPLAY_FONT_55,    \
    GCN_TEXTDISPLAY_FONT_56,    \
    GCN_TEXTDISPLAY_FONT_57,    \
    GCN_TEXTDISPLAY_FONT_58,    \
    GCN_TEXTDISPLAY_FONT_59,    \
    GCN_TEXTDISPLAY_FONT_5A,    \
    GCN_TEXTDISPLAY_FONT_5B,    \
    GCN_TEXTDISPLAY_FONT_5C,    \
    GCN_TEXTDISPLAY_FONT_5D,    \
    GCN_TEXTDISPLAY_FONT_5E,    \
    GCN_TEXTDISPLAY_FONT_5F,    \
    GCN_TEXTDISPLAY_FONT_60,    \
    GCN_TEXTDISPLAY_FONT_61,    \
    GCN_TEXTDISPLAY_FONT_62,    \
    GCN_TEXTDISPLAY_FONT_63,    \
    GCN_TEXTDISPLAY_FONT_64,    \
    GCN_TEXTDISPLAY_FONT_65,    \
    GCN_TEXTDISPLAY_FONT_66,    \
    GCN_TEXTDISPLAY_FONT_67,    \
    GCN_TEXTDISPLAY_FONT_68,    \
    GCN_TEXTDISPLAY_FONT_69,    \
    GCN_TEXTDISPLAY_FONT_6A,    \
    GCN_TEXTDISPLAY_FONT_6B,    \
    GCN_TEXTDISPLAY_FONT_6C,    \
    GCN_TEXTDISPLAY_FONT_6D,    \
    GCN_TEXTDISPLAY_FONT_6E,    \
    GCN_TEXTDISPLAY_FONT_6F,    \
    GCN_TEXTDISPLAY_FONT_70,    \
    GCN_TEXTDISPLAY_FONT_71,    \
    GCN_TEXTDISPLAY_FONT_72,    \
    GCN_TEXTDISPLAY_FONT_73,    \
    GCN_TEXTDISPLAY_FONT_74,    \
    GCN_TEXTDISPLAY_FONT_75,    \
    GCN_TEXTDISPLAY_FONT_76,    \
    GCN_TEXTDISPLAY_FONT_77,    \
    GCN_TEXTDISPLAY_FONT_78,    \
    GCN_TEXTDISPLAY_FONT_79,    \
    GCN_TEXTDISPLAY_FONT_7A,    \
    GCN_TEXTDISPLAY_FONT_7B,    \
    GCN_TEXTDISPLAY_FONT_7C,    \
    GCN_TEXTDISPLAY_FONT_7D,    \
    GCN_TEXTDISPLAY_FONT_7E,    \
    GCN_TEXTDISPLAY_FONT_7F     \
    }

#endif // file