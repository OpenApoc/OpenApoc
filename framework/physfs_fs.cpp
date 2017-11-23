#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "framework/data.h"
#include "framework/framework.h"
#include "framework/fs.h"
#include "framework/logger.h"
#include "framework/trace.h"
#include <physfs.h>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef ANDROID
#define be16toh(x) htobe16(x)
#define be32toh(x) htobe32(x)
#define be64toh(x) htobe64(x)
#define le16toh(x) htole16(x)
#define le32toh(x) htole32(x)
#define le64toh(x) htole64(x)
#elif defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE)
#include <endian.h>
#else
/* We assume all other platforms are little endian for now */
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
	UString suppliedPath;

	PHYSFS_File *file;

	PhysfsIFileImpl(const UString &path, size_t bufferSize = 512)
	    : bufferSize(bufferSize), buffer(new char[bufferSize]), suppliedPath(path)
	{
		file = PHYSFS_openRead(path.cStr());
		if (!file)
		{
			LogError("Failed to open file \"%s\" : \"%s\"", path,
			         PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
			return;
		}
		systemPath = PHYSFS_getRealDir(path.cStr());
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
			LogError("ios::out set on read-only IFile \"%s\"", this->systemPath);
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
			LogError("ios::out set on read-only IFile \"%s\"", this->systemPath);
			LogAssert(0);
			setp(buffer.get(), buffer.get());
		}

		return PHYSFS_tell(file);
	}

	int_type overflow(int_type) override
	{
		LogError("overflow called on read-only IFile \"%s\"", this->systemPath);
		LogAssert(0);
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
		if (!PHYSFS_mount(p.cStr(), "/", 0))
		{
			LogInfo("Failed to add resource dir \"%s\", error: %s", p,
			        PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
			continue;
		}
		else
			LogInfo("Resource dir \"%s\" mounted to \"%s\"", p, PHYSFS_getMountPoint(p.cStr()));
	}
	this->writeDir = PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME);
	LogInfo("Setting write directory to \"%s\"", this->writeDir);
	PHYSFS_setWriteDir(this->writeDir.cStr());
	// Finally, the write directory trumps all
	PHYSFS_mount(this->writeDir.cStr(), "/", 0);
}

FileSystem::~FileSystem() = default;

IFile FileSystem::open(const UString &path)
{
	TRACE_FN_ARGS1("PATH", path);
	IFile f;

	auto lowerPath = path.toLower();
	if (path != lowerPath)
	{
		LogError("Path \"%s\" contains CAPITAL - cut it out!", path);
	}

	if (!PHYSFS_exists(path.cStr()))
	{
		LogInfo("Failed to find \"%s\"", path);
		LogAssert(!f);
		return f;
	}
	f.f.reset(new PhysfsIFileImpl(path));
	f.rdbuf(dynamic_cast<PhysfsIFileImpl *>(f.f.get()));
	LogInfo("Loading \"%s\" from \"%s\"", path, f.systemPath());
	return f;
}

std::list<UString> FileSystem::enumerateDirectory(const UString &basePath,
                                                  const UString &extension) const
{
	std::list<UString> result;
	bool filterByExtension = !extension.empty();

	char **elements = PHYSFS_enumerateFiles(basePath.cStr());
	for (char **element = elements; *element != NULL; element++)
	{
		if (!filterByExtension)
		{
			result.push_back(*element);
		}
		else
		{
			const UString elementString = (*element);
			if (elementString.endsWith(extension))
			{
				result.push_back(elementString);
			}
		}
	}
	PHYSFS_freeList(elements);
	return result;
}

static std::list<UString> recursiveFindFilesInDirectory(const FileSystem &fs, UString path,
                                                        const UString &extension)
{
	std::list<UString> foundFiles;
	auto list = fs.enumerateDirectory(path, "");
	for (auto &entry : list)
	{
		if (entry.endsWith(extension))
		{
			foundFiles.push_back(path + "/" + entry);
		}
		else
		{
			auto subdirFiles = recursiveFindFilesInDirectory(fs, path + "/" + entry, extension);
			foundFiles.insert(foundFiles.end(), subdirFiles.begin(), subdirFiles.end());
		}
	}
	return foundFiles;
}

std::list<UString> FileSystem::enumerateDirectoryRecursive(const UString &basePath,
                                                           const UString &extension) const
{
	return recursiveFindFilesInDirectory(*this, basePath, extension);
}

} // namespace OpenApoc
