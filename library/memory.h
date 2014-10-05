
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Memory
{

	private:
		std::vector<char> data;

	public:
		Memory(size_t InitialSize = 0);
		~Memory();

		void Clear();
		void Resize( size_t length );
		void AppendData( void* data, size_t length );
		size_t GetSize();
		void* GetData();
		void* GetDataOffset( size_t offset );
};

}; //namespace OpenApoc
