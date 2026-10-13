//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radsoundmath.hpp> // (Reverse Endian Function)
#include <radsoundfile.hpp>

#ifdef RAD_GAMECUBE
    #include <radsound_gcn.hpp>
#endif

void radSoundHalFileHeader::ConvertToPlatformEndian
(
	void
)
{	
	#ifdef RAD_GAMECUBE
  
		m_Channels      = radSoundReverseEndian( m_Channels );
		m_BitResolution = radSoundReverseEndian( m_BitResolution );
		m_SamplingRate  = radSoundReverseEndian( m_SamplingRate );

        for ( int i = 0; i < GCN_ADPCM_MAX_CHANNELS; i ++ )
        {
            m_gcnAdpcmInfo[i].coef[0][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[0][0] );
            m_gcnAdpcmInfo[i].coef[1][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[1][0] );
            m_gcnAdpcmInfo[i].coef[2][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[2][0] );
            m_gcnAdpcmInfo[i].coef[3][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[3][0] );
            m_gcnAdpcmInfo[i].coef[4][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[4][0] );
            m_gcnAdpcmInfo[i].coef[5][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[5][0] );
            m_gcnAdpcmInfo[i].coef[6][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[6][0] );
            m_gcnAdpcmInfo[i].coef[7][0] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[7][0] );
            m_gcnAdpcmInfo[i].coef[0][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[0][1] );
            m_gcnAdpcmInfo[i].coef[1][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[1][1] );
            m_gcnAdpcmInfo[i].coef[2][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[2][1] );
            m_gcnAdpcmInfo[i].coef[3][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[3][1] );
            m_gcnAdpcmInfo[i].coef[4][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[4][1] );
            m_gcnAdpcmInfo[i].coef[5][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[5][1] );
            m_gcnAdpcmInfo[i].coef[6][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[6][1] );
            m_gcnAdpcmInfo[i].coef[7][1] = radSoundReverseEndian( m_gcnAdpcmInfo[i].coef[7][1] );

            m_gcnAdpcmInfo[i].gain                    = radSoundReverseEndian( m_gcnAdpcmInfo[i].gain );
            m_gcnAdpcmInfo[i].pred_scale              = radSoundReverseEndian( m_gcnAdpcmInfo[i].pred_scale );
            m_gcnAdpcmInfo[i].yn1                     = radSoundReverseEndian( m_gcnAdpcmInfo[i].yn1 );
            m_gcnAdpcmInfo[i].yn2                     = radSoundReverseEndian( m_gcnAdpcmInfo[i].yn2 );
            m_gcnAdpcmInfo[i].loop_pred_scale         = radSoundReverseEndian( m_gcnAdpcmInfo[i].loop_pred_scale );
            m_gcnAdpcmInfo[i].loop_yn1                = radSoundReverseEndian( m_gcnAdpcmInfo[i].loop_yn1 );
            m_gcnAdpcmInfo[i].loop_yn2                = radSoundReverseEndian( m_gcnAdpcmInfo[i].loop_yn2 );
    	}
	#endif
}

IRadSoundHalAudioFormat::Encoding radSoundHalFileHeader::GetEncodingFromChars( char chars[ 4 ] )
{
	if ( radSoundMemCmp( chars, "VAG ", 4 ) )
    {
        return IRadSoundHalAudioFormat::VAG;
    }
	if ( radSoundMemCmp( chars, "PCM ", 4 ) )
    {
        return IRadSoundHalAudioFormat::PCM;
    }
	if ( radSoundMemCmp( chars, "PCMB", 4 ) )
    {
        return IRadSoundHalAudioFormat::PCM_BIGENDIAN;
    }
	if ( radSoundMemCmp( chars, "XADP", 4 ) )
	{
		return 	IRadSoundHalAudioFormat::XBOXADPCM;
	}
	if ( radSoundMemCmp( chars, "GADP", 4 ) )
	{
		return 	IRadSoundHalAudioFormat::GCNADPCM;
	}
    if ( radSoundMemCmp( chars, "RADP", 4 ) )
    {
        return IRadSoundHalAudioFormat::RadicalAdpcm;
    }

	rAssert( 0 );

	return (IRadSoundHalAudioFormat::Encoding) 0xFFFFFFFF;
}

void radSoundHalFileHeader::InitializeAudioFormat
(
	IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
    radMemoryAllocator allocator
)
{
	rAssert( pIRadSoundHalAudioFormat != NULL );
	
	IRadSoundHalAudioFormat::Encoding encoding = GetEncodingFromChars( m_SoundDataType );

    ref< IRefCount > xIRefCount_CustomData = NULL;

    #if defined RAD_GAMECUBE

        ref< IRadSoundHalAudioFormatGcn > xIRshaf = ::radSoundHalAudioFormatGcnCreate( allocator );

        xIRefCount_CustomData = xIRshaf;
  
    #endif

	pIRadSoundHalAudioFormat->Initialize( encoding, xIRefCount_CustomData, m_SamplingRate, m_Channels, m_BitResolution );
	
    #ifdef RAD_GAMECUBE
    
        xIRshaf->Initialize( m_gcnAdpcmInfo, pIRadSoundHalAudioFormat );	
        
    #endif
}
