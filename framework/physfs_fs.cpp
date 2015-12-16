#include "framework/fs.h"
#include "framework/logger.h"
#include "framework/ignorecase.h"

#include <physfs.h>

#ifndef _WIN32
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

namespace
{

using namespace OpenApoc;

static UString GetCorrectCaseFilename(const UString &Filename)
{
	std::string u8Filename = Filename.str();
	std::unique_ptr<char[]> buf(new char[u8Filename.length() + 1]);
	strncpy(buf.get(), u8Filename.c_str(), u8Filename.length());
	buf[Filename.length()] = '\0';
	if (PHYSFSEXT_locateCorrectCase(buf.get()))
	{
		LogInfo("Failed to find file \"%s\"", Filename.c_str());
		return "";
	}
	return buf.get();
}

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
	virtual ~PhysfsIFileImpl()
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
				assert(0);
		}

		if (mode & std::ios_base::in)
		{
			setg(egptr(), egptr(), egptr());
		}

		if (mode & std::ios_base::out)
		{
			LogError("ios::out set on read-only IFile \"%s\"", this->suppliedPath.c_str());
			assert(0);
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
			assert(0);
			setp(buffer.get(), buffer.get());
		}

		return PHYSFS_tell(file);
	}

	int_type overflow(int_type c = traits_type::eof()) override
	{
		LogError("overflow called on read-only IFile \"%s\"", this->suppliedPath.c_str());
		assert(0);
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

IFileImpl::~IFileImpl() {}

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
		LogError("Failed to allocate memory for %llu bytes", (long long unsigned)memsize);
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

IFile::~IFile() {}

FileSystem::FileSystem(std::vector<UString> paths)
{
	// Paths are supplied in inverse-search order (IE the last in 'paths' should be the first
	// searched)
	for (auto &p : paths)
	{
		if (!PHYSFS_mount(p.c_str(), "/", 0))
		{
			LogWarning("Failed to add resource dir \"%s\"", p.c_str());
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

FileSystem::~FileSystem() {}

UString FileSystem::getCorrectCaseFilename(const UString &path)
{
	return GetCorrectCaseFilename(path);
}

IFile FileSystem::open(const UString &path)
{
	IFile f;
	UString foundPath = GetCorrectCaseFilename(path);
	if (foundPath == "")
	{
		LogInfo("Failed to find \"%s\"", path.c_str());
		assert(!f);
		return f;
	}
	f.f.reset(new PhysfsIFileImpl(foundPath, path));
	f.rdbuf(dynamic_cast<PhysfsIFileImpl *>(f.f.get()));
	LogInfo("Loading \"%s\" from \"%s\"", path.c_str(), f.systemPath().c_str());
	return f;
}

} // namespace OpenApoc
