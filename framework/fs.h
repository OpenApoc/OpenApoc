#pragma once

#include <cstdint>
#include <map>
#include <mutex>

#include "library/sp.h"
#include "library/strings.h"

#define PROGRAM_NAME "OpenApoc"
#define PROGRAM_ORGANISATION "OpenApoc"

namespace OpenApoc
{

class IFileImpl
{
  public:
	virtual ~IFileImpl();
};

class IFile : public std::istream
{
  private:
	std::unique_ptr<IFileImpl> f;
	IFile();
	friend class FileSystem;

  public:
	~IFile();
	size_t size() const;
	std::unique_ptr<char[]> readAll();
	bool readule16(uint16_t &val);
	bool readule32(uint32_t &val);
	const UString &fileName() const;
	const UString &systemPath() const;
	IFile(IFile &&other);
};

class FileSystem
{
  private:
	UString writeDir;

	// Some operating systems (windows, android) seem to have be somewhat inefficient stat() or
	// similay, so the current implementation of getCorrectCaseFilename can take a surprisingly
	// large amount of the startup time.
	//
	// So, for now keep a massive map of < toupper(path), systemPath > for things we've already
	// enumerated
	// FIXME: When we allow adding/removing paths from the search path list, this cache will likely
	// have to be invalidated
	std::map<UString, UString> pathCache;
	std::recursive_mutex pathCacheLock;

  public:
	FileSystem(std::vector<UString> paths);
	~FileSystem();
	IFile open(const UString &path);
	UString getCorrectCaseFilename(const UString &path);
	std::list<UString> enumerateDirectory(const UString &path, const UString &extension) const;
};

} // namespace OpenApoc
