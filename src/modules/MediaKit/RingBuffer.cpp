#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RingBuffer.hpp"

RingBuffer::RingBuffer( int size )
{
    initialized = false;
    Buffer = new unsigned char[size];
    if(Buffer) {
        memset( Buffer, 0, size );
        BufferSize = size;
    } else {
        BufferSize = 0;
    }
    reader = 0;
    writer = 0;
    writeBytesAvailable = size;
    if((locker=create_sem(1,"locker")) >= B_OK) {
        initialized = true;
    } else {
        delete[] Buffer;
    }
}

RingBuffer::~RingBuffer( )
{
    if(initialized) {
        delete[] Buffer;
        delete_sem(locker);
    }
}

bool
RingBuffer::Empty( void )
{
    memset( Buffer, 0, BufferSize );
    reader = 0;
    writer = 0;
    writeBytesAvailable = BufferSize;
    return true;
}

int
RingBuffer::Read( unsigned char *data, int size )
{
    acquire_sem(locker);

    if( data == 0 || size <= 0 || writeBytesAvailable == BufferSize ) {
        release_sem(locker);
        return 0;
    }

    int readBytesAvailable = BufferSize - writeBytesAvailable;

    if( size > readBytesAvailable ) {
        size = readBytesAvailable;
    }

    if(size > BufferSize - reader) {
        int len = BufferSize - reader;
        memcpy(data, Buffer + reader, len);
        memcpy(data + len, Buffer, size-len);
    } else {
        memcpy(data, Buffer + reader, size);
    }

    reader = (reader + size) % BufferSize;
    writeBytesAvailable += size;

    release_sem(locker);
    return size;
}

int
RingBuffer::Write( unsigned char *data, int size )
{
    acquire_sem(locker);

    if( data == 0 || size <= 0 || writeBytesAvailable == 0 ) {
        release_sem(locker);
        return 0;
    }

    if( size > writeBytesAvailable ) {
        size = writeBytesAvailable;
    }

    if(size > BufferSize - writer) {
        int len = BufferSize - writer;
        memcpy(Buffer + writer, data, len);
        memcpy(Buffer, data+len, size-len);
    } else {
        memcpy(Buffer + writer, data, size);
    }

    writer = (writer + size) % BufferSize;
    writeBytesAvailable -= size;

    release_sem(locker);
    return size;
}

int
RingBuffer::GetSize( void )
{
    return BufferSize;
}

int
RingBuffer::GetWriteAvailable( void )
{
    return writeBytesAvailable;
}

int
RingBuffer::GetReadAvailable( void )
{
    return BufferSize - writeBytesAvailable;
}

status_t
RingBuffer::InitCheck( void )
{
    return initialized?B_OK:B_ERROR;
}
