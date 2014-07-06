
#include "memory.h"

//#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Memory::Memory( size_t InitialSize )
{
	if( InitialSize <= 0 )
	{
		data_ptr = 0;
		data_len = 0;
	} else {
		data_ptr = (char*)malloc( InitialSize );
		data_len = InitialSize;
	}
}

Memory::~Memory()
{
	Clear();
}

size_t Memory::GetSize()
{
	return data_len;
}

void Memory::AppendData( void* data, size_t length )
{
	void* ptr;

	if( data_ptr == 0 )
		ptr = malloc( length );
	else
		ptr = realloc( data_ptr, data_len + length );

	if( ptr == 0 )
		return;
	data_ptr = (char*)ptr;
	memcpy( &(data_ptr[data_len]), data, length );
	data_len += length;
}

void* Memory::GetData()
{
	return data_ptr;
}

void Memory::Clear()
{
	if( data_ptr != 0 )
		free( data_ptr );
	data_ptr = 0;
	data_len = 0;
}
