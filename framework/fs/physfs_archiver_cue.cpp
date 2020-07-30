//
// Created by sf on 4/15/16.
//

#ifdef __ANDROID__
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

#include "framework/filesystem.h"
#include "framework/fs/physfs_archiver_cue.h"
#include "framework/logger.h"
#include "library/sp.h"
#include "library/strings.h"
#include <SDL_endian.h> // endianness check
#include <cstddef>
#include <cstring> // for std::memcmp
#include <fstream>
#include <inttypes.h>
#include <map>
#include <physfs.h>

using namespace OpenApoc;

namespace
{

// We actually only use BINARY here, but just for the sake of completion
enum class CueFileType
{
	FT_UNDEFINED, // Should only be set if parsing failed
	FT_BINARY,
	FT_MOTOROLA,
	FT_AIFF,
	FT_WAVE,
	FT_MP3
};

// FIXME: Add more (all?) supported formats?
enum class CueTrackMode
{
	MODE_UNDEFINED, // Should only be set if parsing failed
	MODE1_2048,
	MODE1_2352,
	MODE2_2048,
	MODE2_2324,
	MODE2_2336,
	MODE2_2352
};

// FIXME: This is a very incomplete CueSheet parser!
class CueParser
{
  private:
	enum
	{
		PARSER_START,
		PARSER_FILE,
		PARSER_TRACK,
		PARSER_FINISH,
		PARSER_ERROR
	} parserState;

	/*    static std::regex commandRegex ;
	    static std::regex fileArgRegex ;
	    static std::regex trackArgRegex;
	    static std::regex indexArgRegex;*/

	UString dataFileName;
	CueFileType fileType;
	CueTrackMode trackMode;

	// Parse command while not being in a specific context
	bool parseStart(std::string command, std::string arg)
	{
		// Waiting for "FILE" command
		UString cmd(command);
		if (to_upper(cmd) != "FILE")
		{
			LogInfo("Encountered unexpected command: \"%s\", ignoring", cmd);
			return false;
		}
		// auto matchIter = std::sregex_iterator(arg.begin(), arg.end(), fileArgRegex);
		// FILE argument might be in quotation marks - better handle that

		size_t first_char = 0, last_char = arg.size() - 1;
		if (arg[first_char] == '"')
		{
			while ((last_char > 0) && (arg[last_char] != '"'))
			{
				last_char--;
			}
			// Trim quotation marks
			last_char -= 1;
			first_char += 1;
		}
		else
		{
			// Just find the last non-whitespace character
			last_char = first_char + 1;
			while ((last_char < arg.size()) && !isspace(arg[last_char]))
			{
				last_char++;
			}
			if (last_char == arg.size())
			{
				LogError("Malformed argument for FILE command (arguments are: \"%s\")",
				         arg.c_str());
				return false;
			}
			last_char -= 1;
		}
		if (last_char >= first_char)
		{
			dataFileName = arg.substr(first_char, last_char - first_char + 1);
			LogInfo("Reading from \"%s\"", dataFileName);
		}
		else
		{
			LogError("Bad file name for FILE command (arguments are: \"%s\")", arg.c_str());
			return false;
		}

		// Find the file type string
		first_char = last_char + 1;
		while ((first_char < arg.size()) && !isalnum(arg[first_char]))
		{
			first_char++;
		}
		if (first_char == arg.size())
		{
			LogError("File type not specified for \"%s\" (arguments are: \"%s\")", dataFileName,
			         arg.c_str());
			return false;
		}
		last_char = arg.size() - 1;
		while ((last_char > first_char) && isspace(arg[last_char]))
		{
			last_char--;
		}

		UString fileTypeStr(std::string(arg, first_char, last_char - first_char + 1));

		if (to_upper(fileTypeStr) != "BINARY")
		{
			LogError("Unsupported file type: \"%s\"", fileTypeStr);
			parserState = PARSER_ERROR;
			fileType = CueFileType::FT_UNDEFINED;
			return false;
		}
		fileType = CueFileType::FT_BINARY;
		return true;
	}
	// Parse command while being in a FILE context
	bool parseFile(std::string command, std::string arg)
	{
		// Waiting for the "TRACK" command
		UString cmd(command);
		if (to_upper(cmd) != "TRACK")
		{
			// According to
			// https://www.gnu.org/software/ccd2cue/manual/html_node/FILE-_0028CUE-Command_0029.html#FILE-_0028CUE-Command_0029
			// only TRACK is allowed after FILE
			LogError("Encountered unexpected command: \"%s\" (only TRACK is allowed)", cmd);
			parserState = PARSER_ERROR;
			fileType = CueFileType::FT_UNDEFINED;
			return false;
		}

		// Read track number
		size_t first_char = 0;
		while ((first_char < arg.size()) && isspace(arg[first_char]))
		{
			first_char++;
		}
		size_t last_char = first_char;
		while ((last_char < arg.size()) && isdigit(arg[last_char]))
		{
			last_char++;
		}
		int trackNumber = std::stoi(arg.substr(first_char, last_char - first_char + 1));

		if (trackNumber > 1)
		{
			LogWarning("First track is not numbered 1 (actual number is %d)", trackNumber);
		}

		// Read track mode
		first_char = last_char + 1;
		last_char = arg.size() - 1;
		while ((first_char <= last_char) && isspace(arg[first_char]))
		{
			first_char++;
		}
		while ((last_char >= first_char) && isspace(arg[last_char]))
		{
			last_char--;
		}
		UString modeStr(std::string(arg, first_char, last_char - first_char + 1));
		trackMode = CueTrackMode::MODE_UNDEFINED;
		modeStr = to_upper(modeStr);
		if (modeStr == "MODE1/2048")
			trackMode = CueTrackMode::MODE1_2048;
		else if (modeStr == "MODE1/2352")
			trackMode = CueTrackMode::MODE1_2352;
		else if (modeStr == "MODE2/2048")
			trackMode = CueTrackMode::MODE2_2048;
		else if (modeStr == "MODE2/2324")
			trackMode = CueTrackMode::MODE2_2324;
		else if (modeStr == "MODE2/2336")
			trackMode = CueTrackMode::MODE2_2336;
		else if (modeStr == "MODE2/2352")
			trackMode = CueTrackMode::MODE2_2352;
		if (trackMode == CueTrackMode::MODE_UNDEFINED)
		{
			LogError("Unknown/unimplemented mode \"%s\"", modeStr);
			parserState = PARSER_ERROR;
			return false;
		}
		return true;
	}

	// Parse command while being in a TRACK context
	bool parseTrack(std::string command, std::string)
	{
		UString cmd(command);
		// TODO: check for possible commands, put parser into an "error" state if command is not
		// valid
		if (to_upper(cmd) != "INDEX")
		{
			LogInfo("Encountered unexpected/unknown command: \"%s\", ignoring", cmd);
			return false;
		}
		// FIXME: I seriously could not make heads or tails of these indices.
		return true;
	}

	bool parse(UString cueFilename)
	{
		fs::path cueFilePath(cueFilename.c_str());

		std::ifstream cueFile(cueFilename, std::ios::in);
		if (!cueFile)
		{
			// Stream is unusable, bail out
			return false;
		}
		while (cueFile)
		{
			std::string cueLine;
			std::getline(cueFile, cueLine);

			// Cut the leading whitespaces
			size_t lead_whitespace = 0;
			while ((lead_whitespace < cueLine.size()) && isspace(cueLine[lead_whitespace]))
			{
				lead_whitespace++;
			}
			if (lead_whitespace == cueLine.size())
			{
				continue;
			}
			auto first_whitespace = cueLine.find(" ", lead_whitespace);
			if (first_whitespace == std::string::npos)
			{
				continue;
			}
			std::string command(cueLine, lead_whitespace, first_whitespace - lead_whitespace);
			auto last_whitespace = first_whitespace;
			while ((last_whitespace < cueLine.size()) && isspace(cueLine[last_whitespace]))
			{
				last_whitespace++;
			}
			std::string arg(cueLine, last_whitespace);
			switch (parserState)
			{
				case PARSER_START:
					if (parseStart(command, arg))
					{
						parserState = PARSER_FILE;
					}
					break;
				case PARSER_FILE:
					if (parseFile(command, arg))
					{
						parserState = PARSER_TRACK;
					}
					break;
				case PARSER_TRACK:
					if (parseTrack(command, arg))
					{
						parserState = PARSER_FINISH;
					}
					break;
				default:
					LogError("Invalid CueParser state!");
			}
			if ((parserState == PARSER_FINISH) || (parserState == PARSER_ERROR))
				return parserState == PARSER_FINISH;
		}
		return parserState == PARSER_FINISH;
	}

  public:
	CueParser(UString cueFile)
	    : parserState(PARSER_START), fileType(CueFileType::FT_UNDEFINED),
	      trackMode(CueTrackMode::MODE_UNDEFINED)
	{
		parse(cueFile);
	}
	bool isValid() { return parserState == PARSER_FINISH; }
	UString getDataFileName() { return dataFileName; }
	CueFileType getDataFileType() { return fileType; }
	CueTrackMode getTrackMode() { return trackMode; }
};

// --- iso9660 reader follows

// lsb-msb type as defined by iso9660

struct Int16LsbMsb
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	uint16_t val;       // lsb
	uint16_t __padding; // msb
#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
	uint16_t __padding;
	uint16_t val;
#else
#error Unknown endianness!
#endif
};

struct Sint16LsbMsb
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	int16_t val;
	int16_t __padding;
#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
	int16_t __padding;
	int16_t val;
#else
#error Unknown endianness!
#endif
};

struct Int32LsbMsb
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	uint32_t val;
	uint32_t __padding;
#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
	uint32_t __padding;
	uint32_t val;
#else
#error Unknown endianness!
#endif
};

struct Sint32LsbMsb
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	int32_t val;
	int32_t __padding;
#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
	int32_t __padding;
	int32_t val;
#else
#error Unknown endianness!
#endif
};

struct DecDatetime
{
	char year[4];
	char month[2];
	char day[2];
	char hour[2];
	char minute[2];
	char second[2];
	char hndSecond[2]; // hundredth of a second
	uint8_t gmtOff;
	// return value will be good for is checking whether two files on the same disk
	// were created at the same moment!
	PHYSFS_sint64 toUnixTime() // Convert to a saner (?) time representation
	{
		// The following is clearly an example of now NOT to do time stuff
		// The spec states that all fields are ASCII... we're gonna abuse that
		int year_int =
		    (year[0] - '0') * 1000 + (year[1] - '0') * 100 + (year[2] - '0') * 10 + (year[3] - '0');
		int month_int = (month[0] - '0') * 10 + (month[1] - '0');
		int day_int = (day[0] - '0') * 10 + (day[1] - '0');
		int hour_int = (hour[0] - '0') * 10 + (hour[1] - '0');
		int minute_int = (minute[0] - '0') * 10 + (minute[1] - '0');
		int second_int = (second[0] - '0') * 10 + (second[1] - '0');
		// int hndsec_int = (hndSecond[0] - '0') * 10 +
		//                 (hndSecond[1] - '0');
		// The resulting number is very obviously erroneous, because I don't
		// account for leap years/seconds correctly
		tm time_struct;
		time_struct.tm_sec = second_int;
		time_struct.tm_min = minute_int;
		time_struct.tm_hour = hour_int;
		time_struct.tm_mday = day_int;
		time_struct.tm_mon = month_int;
		time_struct.tm_year = year_int - 1900;
		time_struct.tm_isdst = 0;

		PHYSFS_sint64 unixSeconds = mktime(&time_struct);
		return unixSeconds;
	}
};

// Okay, TWO different datetime formats?
struct DirDatetime
{
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t gmtOffset;
	PHYSFS_sint64 toUnixTime()
	{
		tm time_struct;
		time_struct.tm_sec = second;
		time_struct.tm_min = minute;
		time_struct.tm_hour = hour;
		time_struct.tm_mday = day;
		time_struct.tm_mon = month;
		time_struct.tm_year = year;
		time_struct.tm_isdst = 0;

		PHYSFS_sint64 unixSeconds = mktime(&time_struct);

		return unixSeconds;
	}
};

static_assert(sizeof(Int16LsbMsb) == 4, "Unexpedted int16_lsb_msb_size!");
static_assert(sizeof(Sint16LsbMsb) == 4, "Unexpedted int16_lsb_msb_size!");
static_assert(sizeof(Int32LsbMsb) == 8, "Unexpedted int16_lsb_msb_size!");
static_assert(sizeof(Sint32LsbMsb) == 8, "Unexpedted int16_lsb_msb_size!");
static_assert(sizeof(DecDatetime) == 17, "Unexpected dec_datetime size!");
static_assert(sizeof(DirDatetime) == 7, "Unexpected dir_datetime size!");

class CueIO
{
  private:
	friend class CueArchiver;

	UString imageFile;  // For stream duplication
	int32_t lbaStart;   // Starting LBA for this stream
	int32_t lbaCurrent; // Current block for this stream
	int32_t posInLba;   // Current position in lba
	int64_t length;     // Allowed length of the stream
	CueFileType fileType;
	CueTrackMode trackMode;
	std::ifstream fileStream;

	CueIO(const UString &fileName, uint32_t lbaStart, int64_t length,
	      CueFileType fileType = CueFileType::FT_BINARY,
	      CueTrackMode trackMode = CueTrackMode::MODE1_2048)
	    : imageFile(fileName), lbaStart(lbaStart), lbaCurrent(lbaStart), posInLba(0),
	      length(length), fileType(fileType), trackMode(trackMode)
	{
		fileStream.open(fileName, std::ios::in | std::ios::binary);
		fileStream.seekg(lbaToByteOffset(lbaStart));
	}

	// Convert LBA to actual position in the stream
	int64_t lbaToByteOffset(uint32_t lba)
	{
		switch (trackMode)
		{
			// Each block is 2048 bytes, translation is trivial
			case CueTrackMode::MODE1_2048:
			case CueTrackMode::MODE2_2048:
				return lba * 2048;
			// Each block is 2048 bytes, but there's a prefix and
			// a postfix area for each sector
			case CueTrackMode::MODE1_2352:
				return lba * 2352 + 12 + 4; // 12 sync bytes, 4 header bytes
			// FIXME: Reality check?
			// For mode2, each block might be slightly larger than for mode1
			// These cases are dealing with "cooked" data
			case CueTrackMode::MODE2_2324:
				return lba * 2324; // Each sector is 2324 bytes
			// Strangely enough, mode2/2336 is the same as mode2/2352 without header?
			case CueTrackMode::MODE2_2336:
				return lba * 2336 + 8;
			case CueTrackMode::MODE2_2352:
				return lba * 2048 + 12 + 4 + 8;
			default:
				LogError("Unknown track mode set!");
				// Return negative offset to indicate error
				return -1;
		}
	}

	// Get the "user data" block size
	int32_t blockSize()
	{
		// FIXME: Reality check?
		switch (trackMode)
		{
			case CueTrackMode::MODE1_2048:
			case CueTrackMode::MODE2_2048:
			case CueTrackMode::MODE1_2352:
			case CueTrackMode::MODE2_2352:
			// Some docs say mode2 contains 2336 bytes of user data per block,
			// others insist on 2048 bytes...
			case CueTrackMode::MODE2_2336:
				return 2048;
			case CueTrackMode::MODE2_2324:
				return 2324;
			default:
				LogError("Bad track mode!");
		}
		// Unsupported track mode
		return -1;
	}

	// Get the "binary" block size
	int32_t binBlockSize()
	{
		switch (trackMode)
		{
			case CueTrackMode::MODE1_2048:
			case CueTrackMode::MODE2_2048:
				return 2048;
			case CueTrackMode::MODE1_2352:
			case CueTrackMode::MODE2_2352:
				return 2352;
			case CueTrackMode::MODE2_2336:
				return 2336;
			case CueTrackMode::MODE2_2324:
				return 2324;
			default:
				LogError("Bad track mode!");
		}
		// Unsupported track mode
		return -1;
	}

	// Offset of the user data portion of the block
	int32_t binDataOffset()
	{
		switch (trackMode)
		{
			// FIXME: Check mode2 correctness??
			case CueTrackMode::MODE1_2048:
			case CueTrackMode::MODE2_2048:
				return 0; // Only user data is present here
			case CueTrackMode::MODE2_2324:
				return 0;
			case CueTrackMode::MODE1_2352:
				return 12 + 4; // 12 bytes sync, 4 bytes header
			case CueTrackMode::MODE2_2352:
				return 12 + 4 + 8; // 12 bytes sync, 4 bytes header, 8 bytes subheader
			case CueTrackMode::MODE2_2336:
				return 8; // 8 bytes subheader (?)
			default:
				LogError("Bad track mode!");
		}
		// Unsupported track mode
		return -1;
	}

	int64_t read(void *buf, int64_t len)
	{
		// Ignore size 0 reads
		if (!len)
			return 0;
		// Since we probably will have to read in parts,
		// we have to make the buffer seekable
		char *bufWrite = (char *)buf;
#if 0 // FIXME: This code won't work, actually.
      // If the data is "cooked", just read it.
        if (trackMode == CUE_TrackMode::MODE1_2048 ||
            trackMode == CUE_TrackMode::MODE2_2048)
        {
            // FIXME: This won't correctly handle multi-extent files
            lbaCurrent += len / 2048;
            int64_t start = fileStream.tellg();
            fileStream.read(bufWrite, len);
            return fileStream.tellg() - start;
        }
#endif
		int64_t remainLength = length - (lbaCurrent - lbaStart) * blockSize() - posInLba;
		if (remainLength < 0)
		{
			LogError("Trying to read past end of stream!");
			return -1;
		}
		if (len > remainLength)
		{
			// FIXME: This produces way too much output as well, though we could use it somehow?
			// LogWarning("Requested read of size %" PRIu64 " is bigger than remaining %" PRIu64
			//           " bytes",
			//           len, remainLength);
			len = remainLength;
		}
		int64_t totalRead = 0;
		do
		{
			int64_t readSize = std::min(len - totalRead, int64_t(blockSize() - posInLba));
			fileStream.read(bufWrite + totalRead, readSize);
			totalRead += fileStream.gcount();
			if (fileStream.gcount() != readSize)
			{
				LogWarning("Read buffer underrun! Wanted %" PRId64 " bytes, got %" PRId64, readSize,
				           fileStream.gcount());
				return totalRead;
			}
			posInLba += readSize;
			if (posInLba >= blockSize())
			{
				posInLba = 0;
				lbaCurrent += 1;
				// fileStream.seekg(lbaToByteOffset(lbaCurrent), std::ios::beg);
				seek((lbaCurrent - lbaStart) * blockSize());
			}
		} while (len > totalRead);
		return totalRead;
	}

	int seek(int64_t offset)
	{
		if (offset > length)
		{
			return 0;
		}
		// FIXME: This assumes the offset is more or less *sane*
		uint32_t blockOffset = offset / blockSize();
		uint32_t posInBlock = offset % blockSize();

		lbaCurrent = lbaStart + blockOffset;
		posInLba = posInBlock;
		uint64_t binOffset = lbaToByteOffset(lbaCurrent) + posInLba;
		fileStream.seekg(binOffset, std::ios::beg);
		return fileStream.good();
	}

	PHYSFS_sint64 tell()
	{
		// return lbaToByteOffset(lbaCurrent - lbaStart) + posInLba;
		return blockSize() * (lbaCurrent - lbaStart) + posInLba;
	}

	CueIO(const CueIO &other)
	    : imageFile(other.imageFile), lbaStart(other.lbaStart), lbaCurrent(other.lbaCurrent),
	      posInLba(other.posInLba), length(other.length), fileType(other.fileType),
	      trackMode(other.trackMode)
	{
		fileStream.open(imageFile, std::ios::in | std::ios::binary);
		fileStream.seekg(lbaToByteOffset(lbaStart) + posInLba);
	}

	~CueIO() { fileStream.close(); }

	static PHYSFS_Io *createIo()
	{
		return new PHYSFS_Io{0,         nullptr,     cueIoRead,      cueIoWrite, cueIoSeek,
		                     cueIoTell, cueIoLength, cueIoDuplicate, cueIoFlush, cueIoDestroy};
	}

  public:
	static PHYSFS_sint64 cueIoRead(PHYSFS_Io *io, void *buffer, PHYSFS_uint64 len)
	{
		CueIO *cio = (CueIO *)io->opaque;
		return cio->read(buffer, len);
	}

	// We always ignore write requests
	static PHYSFS_sint64 cueIoWrite(PHYSFS_Io *io, const void *buffer, PHYSFS_uint64 len)
	{
		std::ignore = io;
		std::ignore = buffer;
		std::ignore = len;
		return -1;
	}

	static int cueIoSeek(PHYSFS_Io *io, PHYSFS_uint64 offset)
	{
		CueIO *cio = (CueIO *)io->opaque;
		return cio->seek(offset);
	}

	static PHYSFS_sint64 cueIoTell(PHYSFS_Io *io)
	{
		CueIO *cio = (CueIO *)io->opaque;
		return cio->tell();
	}

	static PHYSFS_sint64 cueIoLength(PHYSFS_Io *io)
	{
		CueIO *cio = (CueIO *)io->opaque;
		return cio->length;
	}

	// A note on io->duplicate:
	// The physfs.h doc-comment states that duplicate should return a
	// "new value for a stream's (opaque) field", but that's actually
	// not true (according to implementations in the code).
	// In fact you have to construct a new PHYSFS_Io object, with no
	// dependencies on the old one.
	static PHYSFS_Io *cueIoDuplicate(PHYSFS_Io *io)
	{
		CueIO *cio = (CueIO *)io->opaque;
		// Just go ahead and construct a new file stream
		PHYSFS_Io *retval = createIo();
		// Set the appropriate fields
		io->opaque = new CueIO(*cio);
		return retval;
	}

	static int cueIoFlush(PHYSFS_Io *io)
	{
		std::ignore = io;
		return 1;
	}

	static void cueIoDestroy(PHYSFS_Io *io)
	{
		CueIO *cio = (CueIO *)io->opaque;
		delete cio;
		delete io;
	}

	static PHYSFS_Io *getIo(UString fileName, uint32_t lba, int64_t length, CueFileType ftype,
	                        CueTrackMode tmode)
	{
		auto cio = new CueIO(fileName, lba, length, ftype, tmode);
		if (!cio->fileStream)
		{
			delete cio;
			return nullptr;
		}
		PHYSFS_Io *io = createIo();
		io->opaque = cio;
		return io;
	}
};

const int PHYSFS_API_VERSION = 0;
class CueArchiver
{
  private:
	UString imageFile;
	CueFileType fileType;
	CueTrackMode trackMode;

	CueIO *cio;

	struct IsoVolumeDescriptor
	{
		uint8_t type;
		uint8_t identifier[5];
		uint8_t version;
		uint8_t _padding; // This field is here to avoid alignment issues.
		                  // It's only used in the boot volume descriptor, and
		                  // therefore not interesting to us.
		union {
			// Better not even try this one
			/*struct
			{
			    uint8_t bootSystemIdentifier__unused[31];
			    uint8_t bootIdentifier__unused[32];
			    uint8_t bootSystemUse__unused[1977];
			} boot;*/
			struct
			{
				// uint8_t __unused; // Disabled due to ___padding being there
				char sysIdentifier[32];
				char volIdentifier[32];
				uint8_t _unused_8[8];
				Int32LsbMsb volSpaceSize;
				uint8_t _unused_32[32];
				Int16LsbMsb volSetSz;
				Int16LsbMsb volSeqNr;
				Int16LsbMsb lbs;
				Int32LsbMsb pathTblSz;
				uint32_t pathTblLLoc;
				uint32_t optPathTblLLoc;
				uint32_t pathTblMLoc;
				uint32_t optPathTblMLoc;
				uint8_t rootDirEnt[34];
				char volSetIdentifier[128];
				char publisherIdentifier[128];
				char dataPrepIdentifier[128];
				char appIdentifier[128];
				char copyrightIdentifier[38];
				char abstractFileId[36];
				char biblioFileId[37];
				DecDatetime volCreationTime;
				DecDatetime volModificationTime;
				DecDatetime volExpirationTime;
				DecDatetime volEffectiveTime;
				uint8_t fileStructureVersion;
				uint8_t _unused1;
				uint8_t _app_defined__unused[512];
				uint8_t _reserved[653];
			} primary;
			// Supplementary volume descriptor is ignored completely
			struct
			{
				uint8_t _padding[2040];
			} terminator;
		};
	};

	// NOTE: this would have all sorts of alignment issues if used with proper types!
	struct IsoDirRecord_hdr
	{
		uint8_t length;
		uint8_t xarLength;
		uint8_t extentLoc[8];    // cast to int32_lsb_msb
		uint8_t extentLength[8]; // cast to int32_lsb_msb
		DirDatetime recTime;
		uint8_t flags;
		uint8_t fuSize;
		uint8_t gapSize;
		uint8_t volSeqNumber[4]; // cast to int16_lsb_msb
		uint8_t fnLength;
		// That's a bit of a hack, since actual filename might be sized differently
		char fileName[222];
	};

	enum FSEntryFlags
	{
		FSFLAG_HIDDEN = 0x01,
		FSFLAG_DIRENT = 0x02,
		FSFLAG_ASFILE = 0x04,
		FSFLAG_XATTRINFO = 0x08,
		FSFLAG_XATTRPERM = 0x10,
		FSFLAG_RESERVED1 = 0x20,
		FSFLAG_RESERVED2 = 0x40,
		FSFLAG_NOTFINAL = 0x80
	};

	struct FSEntry
	{
		UString name;
		enum
		{
			FS_FILE,
			FS_DIRECTORY
		} type;
		uint32_t offset;
		uint64_t length;
		int64_t timestamp;
		std::map<UString, FSEntry> children;
	} root;

	void readDir(const IsoDirRecord_hdr &dirRecord, FSEntry &parent)
	{
		Int32LsbMsb lm_location;
		Int32LsbMsb lm_length;
		DirDatetime d_datetime;
		std::memcpy(&lm_location, dirRecord.extentLoc, sizeof(Int32LsbMsb));
		std::memcpy(&lm_length, dirRecord.extentLength, sizeof(Int32LsbMsb));
		std::memcpy(&d_datetime, &dirRecord.recTime, sizeof(DirDatetime));
		uint32_t location = lm_location.val;
		int32_t length = lm_length.val;
		int32_t readpos = 0;
		char *semicolonPos = std::strrchr((char *)dirRecord.fileName, ';');
		if (semicolonPos)
		{
			*semicolonPos = '\0';
		} // Ignore the semicolon and everything after it
		parent.name = dirRecord.fileName;
		// As of commit 07a2fe9, we only use lower-case names
		parent.name = to_lower(parent.name);
		parent.length = length;
		parent.offset = location;
		parent.timestamp = d_datetime.toUnixTime();
#if 0 // Stop archiver from being extremely chatty
        LogInfo("Adding entry: %s", parent.name);
        LogInfo("  Location %" PRIu64, parent.offset);
        LogInfo("  Length: %" PRIu64, parent.length);
#endif
		if (!(dirRecord.flags & FSFLAG_DIRENT))
		{
			parent.type = FSEntry::FS_FILE;
			return;
		} // Next portion is directory-specific
		parent.type = FSEntry::FS_DIRECTORY;
		IsoDirRecord_hdr childDirRecord;
#if 0
        LogInfo("Recursing into: %s (location: %d)", parent.name, location);
#endif
		cio->seek(cio->blockSize() * location + readpos);
		do
		{
			// Find a non-empty record
			do
			{
				// Each record starts at an even offset
				if (readpos % 2)
				{
					readpos += 1;
					cio->seek(cio->tell() + 1);
				}
				// Read first 33 bytes containing everything but the name
				readpos += cio->read(&childDirRecord, 33);
				// We can safely bail out if we get over the record length
				if (readpos >= length)
					return;
				// We've read an empty record, that's fine
				if (childDirRecord.length == 0)
				{
					// Check if we crossed the boundary
					if (readpos % cio->blockSize() < 33)
					{
						// Yup, sure did.
						readpos -= readpos % cio->blockSize();
					}
					cio->seek(cio->blockSize() * location + readpos);
				}
			} while (childDirRecord.length == 0);

			if (childDirRecord.fnLength > 0)
			{
				readpos += cio->read(childDirRecord.fileName, childDirRecord.fnLength);
				childDirRecord.fileName[childDirRecord.fnLength] = '\0';
			}
			else
			{
				continue;
			}
			// Each (?) directory on a CD has a "this directory" and "parent directory" entries, we
			// just ignore them
			if (!isalnum(childDirRecord.fileName[0]))
			{
				continue;
			}

			// Now decode what we've read
			FSEntry childEntry;
			int64_t pos = cio->tell();
			readDir(childDirRecord, childEntry);
			// Reset reading position
			cio->seek(pos);
			// cio->seek(cio->blockSize() * location + readpos);
			parent.children[childEntry.name] = childEntry;
		} while ((childDirRecord.length > 0));
	}

	static_assert(sizeof(IsoVolumeDescriptor) == 2048, "Unexpected volume size!");
	static_assert(sizeof(IsoDirRecord_hdr) == 255, "Unexpected direntry size!");
	static_assert(offsetof(IsoDirRecord_hdr, fnLength) == 32, "Unexpected filename offset!");
	CueArchiver(UString fileName, CueFileType ftype, CueTrackMode tmode)
	    : imageFile(fileName), fileType(ftype), trackMode(tmode)
	{
		// "Hey, a .cue-.bin file pair should be really easy to read!" - sfalexrog, 15.04.2016
		fs::path filePath(fileName.c_str());
		// FIXME: This fsize is completely and utterly wrong - unless you're reading an actual iso
		// (mode1_2048)
		uint64_t fsize = fs::file_size(filePath);
		LogInfo("Opening file %s of size %" PRIu64, fileName, fsize);
		cio = new CueIO(fileName, 0, fsize, ftype, tmode);
		if (!cio->fileStream)
		{
			LogError("Could not open file: bad stream!");
		}
		cio->seek(cio->blockSize() * 16);
		LogInfo("Reading ISO volume descriptor");
		IsoVolumeDescriptor descriptor;
		cio->read(&descriptor, sizeof(descriptor));
		LogInfo("CD magic: %c, %c, %c, %c, %c", descriptor.identifier[0], descriptor.identifier[1],
		        descriptor.identifier[2], descriptor.identifier[3], descriptor.identifier[4]);
		const char magic[] = {'C', 'D', '0', '0', '1'};
		if (std::memcmp((void *)magic, (void *)descriptor.identifier, 5))
		{
			LogError("Bad CD magic!");
		}
		LogInfo("Descriptor type: %d", (int)descriptor.type);
		IsoDirRecord_hdr rootRecord;
		std::memcpy(&rootRecord, descriptor.primary.rootDirEnt, 34);
		LogInfo("Volume ID: %s", descriptor.primary.volIdentifier);
		LogInfo("Root dirent length: %d", (int)rootRecord.length);
		readDir(rootRecord, root);
	}
	~CueArchiver() { delete cio; }

	const FSEntry *getFsEntry(const char *name) const
	{
		const FSEntry *current = &root;
		UString dname = name;
		if (dname.length() > 0)
		{
			auto pathParts = split(dname, "/");
			for (auto ppart = pathParts.begin(); ppart != pathParts.end(); ppart++)
			{
				auto subdir = current->children.find(*ppart);
				if (subdir == current->children.end())
				{
					// Not a valid directory, fail fast.
					PHYSFS_setErrorCode(PHYSFS_ERR_NOT_FOUND);
					return nullptr;
				}
				// Go into specified subdirectory
				current = &(subdir->second);
			}
		}
		return current;
	}

	PHYSFS_EnumerateCallbackResult enumerateFiles(const char *dirname, PHYSFS_EnumerateCallback cb,
	                                              const char *origdir, void *callbackdata)
	{
		const FSEntry *current = getFsEntry(dirname);
		if (!current)
			return PHYSFS_ENUM_ERROR;
		if (current->type == FSEntry::FS_DIRECTORY)
		{
			for (auto entry = current->children.begin(); entry != current->children.end(); entry++)
			{
				auto ret = cb(callbackdata, origdir, entry->first.c_str());
				switch (ret)
				{
					case PHYSFS_ENUM_ERROR:
						PHYSFS_setErrorCode(PHYSFS_ERR_APP_CALLBACK);
						return PHYSFS_ENUM_ERROR;
					case PHYSFS_ENUM_STOP:
						return PHYSFS_ENUM_STOP;
					default:
						// Continue enumeration
						break;
				}
			}
		}
		return PHYSFS_ENUM_OK;
	}

	PHYSFS_Io *openRead(const char *fnm)
	{
		const FSEntry *entry = getFsEntry(fnm);
		if (!entry || (entry->type == FSEntry::FS_DIRECTORY))
		{
			return nullptr;
		}
		return CueIO::getIo(imageFile, entry->offset, entry->length, fileType, trackMode);
	}

	int stat(const char *name, PHYSFS_Stat *stat)
	{
		const FSEntry *current = getFsEntry(name);
		if (!current)
		{
			return 0;
		}

		stat->readonly = 1;
		stat->accesstime = current->timestamp;
		stat->createtime = current->timestamp;
		stat->modtime = current->timestamp;
		switch (current->type)
		{
			case FSEntry::FS_FILE:
				stat->filetype = PHYSFS_FILETYPE_REGULAR;
				stat->filesize = current->length;
				break;
			case FSEntry::FS_DIRECTORY:
				stat->filetype = PHYSFS_FILETYPE_DIRECTORY;
				stat->filesize = 0;
				break;
			default:
				// Well, this should never happen?
				LogError("Unexpected FSEntry::type value!");
		}
		return 1;
	}

  public:
	static void *cueOpenArchive(PHYSFS_Io *, const char *filename, int forWriting, int *claimed)
	{
		LogWarning("Opening \"%s\"", filename);
		// FIXME: Here we assume the filename actually points to the actual .cue file,
		// ignoring the PHYSFS_Io (though how would we even read the accompanying file?)
		// TODO: Actually read from PHYSFS_Io to allow mounting non-CUE images?
		if (!filename)
		{
			LogError("FIXME: Cannot operate on purely-PhysFS_Io archives (need a filename)");
			return nullptr;
		}

		if (forWriting)
		{
			LogError("Cue files cannot be written to");
			return nullptr;
		}

		CueParser parser(filename);
		if (!parser.isValid())
		{
			LogError("Could not parse file \"%s\"", filename);
			return nullptr;
		}

		// We know it's a valid CUE file, so claim it
		*claimed = 1;

		fs::path cueFilePath(filename);

		fs::path dataFilePath(cueFilePath.parent_path()); // parser.getDataFileName());
		dataFilePath /= parser.getDataFileName().c_str();

		if (!fs::exists(dataFilePath))
		{
			LogWarning("Could not find binary file \"%s\" referenced in the cuesheet",
			           parser.getDataFileName());
			LogWarning("Trying case-insensitive search...");
			UString ucBin(parser.getDataFileName());
			ucBin = to_lower(ucBin);
			// for (fs::directory_entry &dirent :
			// fs::directory_iterator(cueFilePath.parent_path()))
			for (auto dirent_it = fs::directory_iterator(cueFilePath.parent_path());
			     dirent_it != fs::directory_iterator(); dirent_it++)
			{
				auto dirent = *dirent_it;
				LogInfo("Trying %s", dirent.path().string());
				UString ucDirent(dirent.path().filename().string());
				ucDirent = to_lower(ucDirent);
				if (ucDirent == ucBin)
				{
					dataFilePath = cueFilePath.parent_path();
					dataFilePath /= dirent.path().filename();
				}
			}
			if (!fs::exists(dataFilePath))
			{
				LogError("Binary file does not exist: \"%s\"", dataFilePath.string());
				return nullptr;
			}
			LogWarning("Using \"%s\" as a binary file source", dataFilePath.string());
		}

		return new CueArchiver(dataFilePath.string(), parser.getDataFileType(),
		                       parser.getTrackMode());
	}

	static PHYSFS_EnumerateCallbackResult cueEnumerateFiles(void *opaque, const char *dirname,
	                                                        PHYSFS_EnumerateCallback cb,
	                                                        const char *origdir, void *callbackdata)
	{
		CueArchiver *archiver = (CueArchiver *)opaque;
		return archiver->enumerateFiles(dirname, cb, origdir, callbackdata);
	}

	static PHYSFS_Io *cueOpenRead(void *opaque, const char *fnm)
	{
		CueArchiver *archiver = (CueArchiver *)opaque;
		return archiver->openRead(fnm);
	}

	static PHYSFS_Io *cueOpenWrite(void *opaque, const char *filename)
	{
		std::ignore = opaque;
		std::ignore = filename;
		PHYSFS_setErrorCode(PHYSFS_ERR_READ_ONLY);
		return nullptr;
	}

	static PHYSFS_Io *cueOpenAppend(void *opaque, const char *filename)
	{
		std::ignore = opaque;
		std::ignore = filename;
		PHYSFS_setErrorCode(PHYSFS_ERR_READ_ONLY);
		return nullptr;
	}

	static int cueRemove(void *opaque, const char *filename)
	{
		std::ignore = opaque;
		std::ignore = filename;
		PHYSFS_setErrorCode(PHYSFS_ERR_READ_ONLY);
		return 0;
	}

	static int cueMkdir(void *opaque, const char *filename)
	{
		std::ignore = opaque;
		std::ignore = filename;
		PHYSFS_setErrorCode(PHYSFS_ERR_READ_ONLY);
		return 0;
	}

	static int cueStat(void *opaque, const char *fn, PHYSFS_Stat *stat)
	{
		CueArchiver *archiver = (CueArchiver *)opaque;
		return archiver->stat(fn, stat);
	}

	static void cueCloseArchive(void *opaque)
	{
		CueArchiver *archiver = (CueArchiver *)opaque;
		delete archiver;
	}

	static PHYSFS_Archiver *createArchiver()
	{
		static PHYSFS_Archiver cueArchiver = {PHYSFS_API_VERSION,
		                                      {
		                                          "CUE", "Cuesheet-Backed Image File",
		                                          "Alexey Rogachevsky <sfalexrog@gmail.com>",
		                                          "https://github.com/sfalexeog",
		                                          0 // supportsSymlinks
		                                      },
		                                      cueOpenArchive,
		                                      cueEnumerateFiles,
		                                      cueOpenRead,
		                                      cueOpenWrite,
		                                      cueOpenAppend,
		                                      cueRemove,
		                                      cueMkdir,
		                                      cueStat,
		                                      cueCloseArchive};
		return &cueArchiver;
	}
};

} /* anonymous namespace */

namespace OpenApoc
{
void parseCueFile(UString fileName)
{
	CueParser parser(fileName);
	LogInfo("Parser status: %d", parser.isValid());
	LogInfo("Data file: %s", parser.getDataFileName());
	LogInfo("Track mode: %d", (int)parser.getTrackMode());
	LogInfo("File mode: %d", (int)parser.getDataFileType());
}

PHYSFS_Archiver *getCueArchiver() { return CueArchiver::createArchiver(); }
} // namespace OpenApoc
