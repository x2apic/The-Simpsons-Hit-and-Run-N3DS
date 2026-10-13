#ifndef BUFFERLOADER_HPP
#define BUFFERLOADER_HPP

#include <radfile.hpp>
#include <radlinkedclass.hpp>
#include <radsound_hal.hpp>  
#include <radsoundobject.hpp>

class radSoundBufferLoaderCtr
	:
    public IRadSoundHalDataSourceCallback,
    public radLinkedClass< radSoundBufferLoaderCtr >,
    public radSoundObject
{
    public:

        IMPLEMENT_REFCOUNTED( "radSoundBufferLoaderCtr" )

        radSoundBufferLoaderCtr(
            IRefCount * pIRefCount_Owner,
            void * pBuffer,
		    IRadSoundHalDataSource * pIRadSoundHalDataSource,
            IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
            unsigned int numberOfFrames,
            IRadSoundHalBufferLoadCallback * pISoundBufferCallback );

        virtual void OnDataSourceFramesLoaded( unsigned int framesActuallyRead );

    static void CancelOperations( IRefCount * pIRefCount_Owner );

    void Cancel( void );

    private:
        
        void Start( void );
        void Finish( void );

        ref< IRadSoundHalBufferLoadCallback >	m_xIRadSoundHalBufferLoadCallback;
        ref< IRadSoundHalDataSource >			m_xIRadSoundHalDataSource;
        ref< IRefCount >                        m_xIRefCount_Owner;

        unsigned int m_NumberOfFrames;
        void * m_pBuffer;
        bool m_Cancelled;
};
#endif