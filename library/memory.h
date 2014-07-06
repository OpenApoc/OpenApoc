
#pragma once

#include <cstdlib>

class Memory
{

	private:
		char*		data_ptr;
		size_t	data_len;

	public:
		Memory( size_t InitialSize );
		~Memory();

    void Clear();
		void AppendData( void* data, size_t length );
		size_t GetSize();
		void* GetData();
};
