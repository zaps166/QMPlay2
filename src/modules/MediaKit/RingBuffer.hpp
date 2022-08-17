#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <OS.h>

class RingBuffer {

public:
     RingBuffer(int size);
     ~RingBuffer();
     int Read( unsigned char* dataPtr, int numBytes );
     int Write( unsigned char *dataPtr, int numBytes );
     
     bool Empty( void );
     int GetSize( );
     int GetWriteAvailable( );
     int GetReadAvailable( );
     status_t InitCheck( );
private:
     unsigned char *Buffer;
     int BufferSize;
     int reader;
     int writer;
     int writeBytesAvailable;
     
     sem_id locker;
     
     bool 	initialized;
};

#endif
