
#include "framework/data.h"
#include "framework/logger.h"
#include "framework/sampleloader_interface.h"
#include "framework/sound.h"
#include "library/sp.h"
#include <SDL.h>
#include <optional>

namespace
{

using namespace OpenApoc;

static std::optional<AudioFormat::SampleFormat> sdlFormatToSampleFormat(SDL_AudioFormat format)
{
	switch (format)
	{
		case AUDIO_S16LSB:
			return AudioFormat::SampleFormat::PCM_SINT16;
		case AUDIO_U8:
			return AudioFormat::SampleFormat::PCM_UINT8;
		default:
			return {};
	}
}

class WavSampleLoader : public SampleLoader
{
	const Data &data;

  public:
	constexpr explicit WavSampleLoader(const Data &data) : data(data) {}
	sp<Sample> loadSample(UString path) override
	{
		// Raw sound format strings come in the form:
		// WAV:FILENAME
		// They are all assumed to be PCK_UINT8 1channel

		const auto splitString = split(path, ":");
		if (splitString.size() != 2)
		{
			LogInfo("String \"%s\" doesn't look like a rawsample - need 2 elements (got %zu)", path,
			        splitString.size());
			return nullptr;
		}
		if (splitString[0] != "WAV")
		{
			LogInfo("String \"%s\" doesn't look like a wav - no WAV prefix", path);
			return nullptr;
		}
		const auto file = data.fs.open(splitString[1]);
		if (!file)
		{
			LogWarning("wav \"%s\" failed to open file \"%s\"", path, splitString[1]);
			return nullptr;
		}

		const auto fullPath = file.systemPath();

		auto sample = mksp<Sample>();
		SDL_AudioSpec spec;
		uint8_t *buf;
		uint32_t bufSize;

		const auto *returnedSpec = SDL_LoadWAV(fullPath.c_str(), &spec, &buf, &bufSize);
		if (returnedSpec == nullptr)
		{
			LogWarning("Failed to open WAV file at \"%s\" - \"%s\"", fullPath, SDL_GetError());
			return nullptr;
		}

		if (spec.channels != 1)
		{
			LogWarning("Failed to open WAV file at \"%s\" - only single channel samples supported",
			           fullPath);
			SDL_FreeWAV(buf);
			return nullptr;
		}

		const auto format = sdlFormatToSampleFormat(spec.format);
		if (!format)
		{
			LogWarning("Failed to open WAV file at \"%s\" - unsupported SDL format 0x%08x",
			           fullPath, spec.format);
			SDL_FreeWAV(buf);
			return nullptr;
		}

		sample->format.frequency = spec.freq;
		sample->format.channels = 1;
		sample->format.format = *format;
		sample->sampleCount = bufSize / sample->format.getSampleSize();
		sample->data.reset(new uint8_t[bufSize]);
		memcpy(sample->data.get(), buf, bufSize);

		SDL_FreeWAV(buf);
		return sample;
	}
};

class WavSampleLoaderFactory : public SampleLoaderFactory
{
  public:
	SampleLoader *create(Data &data) override { return new WavSampleLoader(data); }
};

}; // anonymous namespace

namespace OpenApoc
{
SampleLoaderFactory *getWAVSampleLoaderFactory() { return new WavSampleLoaderFactory(); }
} // namespace OpenApoc
