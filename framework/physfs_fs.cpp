#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "framework/fs.h"
#include "framework/logger.h"
#include "framework/trace.h"

#include <physfs.h>

#ifndef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#ifdef ANDROID
#define be16toh(x) htobe16(x)
#define be32toh(x) htobe32(x)
#define be64toh(x) htobe64(x)
#define le16toh(x) htole16(x)
#define le32toh(x) htole32(x)
#define le64toh(x) htole64(x)
#endif
#include <endian.h>
#else
/* Windows is always little endian? */
static inline uint16_t le16toh(uint16_t val) { return val; }
static inline uint32_t le32toh(uint32_t val) { return val; }
#endif

#include "fs/physfs_archiver_cue.h"

namespace
{

using namespace OpenApoc;

class PhysfsIFileImpl : public std::streambuf, public IFileImpl
{
  public:
	size_t bufferSize;
	std::unique_ptr<char[]> buffer;
	UString systemPath;
	UString caseCorrectedPath;
	UString suppliedPath;

	PHYSFS_File *file;

	PhysfsIFileImpl(const UString &path, const UString &suppliedPath, size_t bufferSize = 512)
	    : bufferSize(bufferSize), buffer(new char[bufferSize]), suppliedPath(suppliedPath)
	{
		file = PHYSFS_openRead(path.c_str());
		if (!file)
		{
			LogError("Failed to open file \"%s\" : \"%s\"", path.c_str(), PHYSFS_getLastError());
			return;
		}
		systemPath = PHYSFS_getRealDir(path.c_str());
		systemPath += "/" + path;
	}
	~PhysfsIFileImpl() override
	{
		if (file)
			PHYSFS_close(file);
	}

	int_type underflow() override
	{
		if (PHYSFS_eof(file))
		{
			return traits_type::eof();
		}
		size_t bytesRead = PHYSFS_readBytes(file, buffer.get(), bufferSize);
		if (bytesRead < 1)
		{
			return traits_type::eof();
		}
		setg(buffer.get(), buffer.get(), buffer.get() + bytesRead);
		return static_cast<unsigned char>(*gptr());
	}

	pos_type seekoff(off_type pos, std::ios_base::seekdir dir,
	                 std::ios_base::openmode mode) override
	{
		switch (dir)
		{
			case std::ios_base::beg:
				PHYSFS_seek(file, pos);
				break;
			case std::ios_base::cur:
				PHYSFS_seek(file, (PHYSFS_tell(file) + pos) - (egptr() - gptr()));
				break;
			case std::ios_base::end:
				PHYSFS_seek(file, PHYSFS_fileLength(file) + pos);
				break;
			default:
				LogError("Unknown direction in seekoff (%d)", dir);
				LogAssert(0);
		}

		if (mode & std::ios_base::in)
		{
			setg(egptr(), egptr(), egptr());
		}

		if (mode & std::ios_base::out)
		{
			LogError("ios::out set on read-only IFile \"%s\"", this->suppliedPath.c_str());
			LogAssert(0);
			setp(buffer.get(), buffer.get());
		}

		return PHYSFS_tell(file);
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
	{
		PHYSFS_seek(file, pos);
		if (mode & std::ios_base::in)
		{
			setg(egptr(), egptr(), egptr());
		}

		if (mode & std::ios_base::out)
		{
			LogError("ios::out set on read-only IFile \"%s\"", this->suppliedPath.c_str());
			LogAssert(0);
			setp(buffer.get(), buffer.get());
		}

		return PHYSFS_tell(file);
	}

	int_type overflow(int_type c = traits_type::eof()) override
	{
		LogError("overflow called on read-only IFile \"%s\"", this->suppliedPath.c_str());
		LogAssert(0);
		std::ignore = c;
		return 0;
	}
};
} // anonymous namespace

namespace OpenApoc
{

IFile::IFile() : std::istream(nullptr) {}
// FIXME: MSVC needs this, GCC fails with it?
#ifdef _WIN32
IFile::IFile(IFile &&other) : std::istream(std::move(other))
{
	this->f = std::move(other.f);
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
}
#endif

IFileImpl::~IFileImpl() = default;

bool IFile::readule16(uint16_t &val)
{
	this->read(reinterpret_cast<char *>(&val), sizeof(val));
	val = le16toh(val);
	return !!(*this);
}

bool IFile::readule32(uint32_t &val)
{
	this->read(reinterpret_cast<char *>(&val), sizeof(val));
	val = le32toh(val);
	return !!(*this);
}

size_t IFile::size() const
{
	if (this->f)
		return PHYSFS_fileLength(dynamic_cast<PhysfsIFileImpl *>(f.get())->file);
	return 0;
}

const UString &IFile::fileName() const
{
	static const UString emptyString = "";
	if (this->f)
		return dynamic_cast<PhysfsIFileImpl *>(f.get())->suppliedPath;
	return emptyString;
}

const UString &IFile::systemPath() const
{
	static const UString emptyString = "";
	if (this->f)
		return dynamic_cast<PhysfsIFileImpl *>(f.get())->systemPath;
	return emptyString;
}

std::unique_ptr<char[]> IFile::readAll()
{
	auto memsize = this->size();
	std::unique_ptr<char[]> mem(new char[memsize]);
	if (!mem)
	{
		LogError("Failed to allocate memory for %llu bytes",
		         static_cast<long long unsigned>(memsize));
		return nullptr;
	}

	// We don't want this to change the state (such as the offset) of the file
	// stream so store off the current pos and restore it after the read
	auto currentPos = this->tellg();
	this->seekg(0, this->beg);

	this->read(mem.get(), memsize);

	this->seekg(currentPos);

	return mem;
}

IFile::~IFile() = default;

FileSystem::FileSystem(std::vector<UString> paths)
{
	// FIXME: Is this the right thing to do that?
	LogInfo("Registering external archivers...");
	PHYSFS_registerArchiver(getCueArchiver());
	// Paths are supplied in inverse-search order (IE the last in 'paths' should be the first
	// searched)
	for (auto &p : paths)
	{
		if (!PHYSFS_mount(p.c_str(), "/", 0))
		{
			LogInfo("Failed to add resource dir \"%s\", error: %s", p.c_str(),
			        PHYSFS_getLastError());
			continue;
		}
		else
			LogInfo("Resource dir \"%s\" mounted to \"%s\"", p.c_str(),
			        PHYSFS_getMountPoint(p.c_str()));
	}
	this->writeDir = PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME);
	LogInfo("Setting write directory to \"%s\"", this->writeDir.c_str());
	PHYSFS_setWriteDir(this->writeDir.c_str());
	// Finally, the write directory trumps all
	PHYSFS_mount(this->writeDir.c_str(), "/", 0);
}

FileSystem::~FileSystem() = default;

static bool CaseInsensitiveCompare(const UString &a, const UString &b)
{
	return a.toUpper() == b.toUpper();
}

static UString FindFile(const UString &basePath, std::list<UString> pathElements,
                        std::map<UString, UString> &expandedFiles)
{
	if (pathElements.empty())
	{
		return basePath;
	}
	UString currentPath = basePath;
	if (!currentPath.empty())
		currentPath += "/";
	UString currentElement = pathElements.front();
	pathElements.pop_front();

	UString foundPath;

	char **elements = PHYSFS_enumerateFiles(currentPath.c_str());

	for (char **element = elements; *element != NULL; element++)
	{
		auto elementPath = currentPath + *element;
		// Don't add directories to the expandedFiles map
		if (PHYSFS_exists(elementPath.c_str()))
		{
			expandedFiles[elementPath.toUpper()] = elementPath;
		}
		if (CaseInsensitiveCompare(currentElement, *element))
		{
			// We don't directly return here as we want to add the rest of the dir to cache, and
			// check for overlapping entries
			if (!foundPath.empty())
			{
				LogWarning("Multiple elements that match \"%s\"", currentPath.c_str());
			}
			foundPath = currentPath + *element;
		}
	}
	PHYSFS_freeList(elements);
	if (!foundPath.empty())
	{
		return FindFile(foundPath, pathElements, expandedFiles);
	}
	else
	{
		// Not found
		return "";
	}
}

UString FileSystem::getCorrectCaseFilename(const UString &path)
{
	TRACE_FN;
	std::lock_guard<std::recursive_mutex> l(this->pathCacheLock);

	UString cacheKey = path.toUpper();

	auto it = this->pathCache.find(cacheKey);
	if (it != this->pathCache.end())
	{
		return it->second;
	}

	return FindFile("", path.splitlist('/'), this->pathCache);
}

IFile FileSystem::open(const UString &path)
{
	TRACE_FN_ARGS1("PATH", path);
	IFile f;
	UString foundPath = this->getCorrectCaseFilename(path);
	if (foundPath == "")
	{
		LogInfo("Failed to find \"%s\"", path.c_str());
		LogAssert(!f);
		return f;
	}
	f.f.reset(new PhysfsIFileImpl(foundPath, path));
	f.rdbuf(dynamic_cast<PhysfsIFileImpl *>(f.f.get()));
	LogInfo("Loading \"%s\" from \"%s\"", path.c_str(), f.systemPath().c_str());
	return f;
}

std::list<UString> FileSystem::enumerateDirectory(const UString &basePath,
                                                  const UString &extension) const
{

	std::list<UString> result;
	bool filterByExtension = !extension.empty();
	UString uppercaseExtension = extension.toUpper();

	char **elements = PHYSFS_enumerateFiles(basePath.c_str());
	for (char **element = elements; *element != NULL; element++)
	{
		if (!filterByExtension)
		{
			result.push_back(*element);
		}
		else
		{
			const UString elementString = (*element);
			if (elementString.toUpper().endsWith(uppercaseExtension))
			{
				result.push_back(elementString);
			}
		}
	}
	PHYSFS_freeList(elements);
	return result;
}

} // namespace OpenApoc
