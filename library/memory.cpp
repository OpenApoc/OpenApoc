
#include "memory.h"

#include "framework/includes.h"

namespace OpenApoc
{

Memory::Memory(size_t InitialSize) : data(InitialSize) {}

Memory::~Memory() {}

size_t Memory::GetSize() { return data.size(); }

void Memory::Resize(size_t length) { data.resize(length); }

void Memory::AppendData(void *data, size_t length)
{
	size_t startSize = this->GetSize();
	this->Resize(startSize + length);
	memcpy(this->GetDataOffset(startSize), data, length);
}

void *Memory::GetData() { return data.data(); }

void Memory::Clear() { data.clear(); }

void *Memory::GetDataOffset(size_t offset) { return &(data.data()[offset]); }

}; // namespace OpenApoc
