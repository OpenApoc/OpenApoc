#include <array>
#include <vorbis/vorbisfile.h>

#include "framework/data.h"
#include "framework/logger.h"
#include "framework/musicloader_interface.h"
#include "framework/sound.h"

namespace OpenApoc
{

namespace detail
{

struct VorbisMusicTrack : public MusicTrack
{

	static MusicTrack::MusicCallbackReturn fillMusicData(sp<MusicTrack> thisTrack,
	                                                     unsigned int maxSamples,
	                                                     void *sampleBuffer,
	                                                     unsigned int *returnedSamples)
	{
		auto music = std::dynamic_pointer_cast<VorbisMusicTrack>(thisTrack);
		*returnedSamples = 0;
		if (!music)
		{
			LogError("Trying to play non-vorbis music");
			return MusicTrack::MusicCallbackReturn::End;
		}
		if (!music->_valid)
		{
			LogError("VorbisMusic: Trying to play non-valid track");
			return MusicTrack::MusicCallbackReturn::End;
		}
		constexpr int bytes_per_sample = 2; // Always use S16
		constexpr int samples_are_signed = 1;
		constexpr int big_endian = 0; // Little endian only
		const auto channels = music->format.channels;

		auto buffer_bytes = maxSamples * bytes_per_sample * channels;

		unsigned int total_read_bytes = 0;

		char *output_buffer_position = static_cast<char *>(sampleBuffer);

		while (buffer_bytes)
		{

			auto read_bytes =
			    ov_read(&music->_vorbis_file, output_buffer_position, buffer_bytes, big_endian,
			            bytes_per_sample, samples_are_signed, &music->_bitstream);
			if (read_bytes < 0)
			{
				LogError("VorbisMusic: Error %d decoding music", read_bytes);
				return MusicTrack::MusicCallbackReturn::End;
			}
			if (read_bytes == 0)
			{
				// EOF
				*returnedSamples = total_read_bytes / bytes_per_sample;
				return MusicTrack::MusicCallbackReturn::End;
			}
			LogAssert(read_bytes <= buffer_bytes);
			buffer_bytes -= read_bytes;
			total_read_bytes += read_bytes;
			output_buffer_position += read_bytes;
		}

		*returnedSamples = total_read_bytes / (bytes_per_sample * channels);
		return MusicTrack::MusicCallbackReturn::Continue;
	}

  public:
	UString _name;
	IFile _file;
	OggVorbis_File _vorbis_file;
	bool _valid = false;
	int _bitstream = 0;
	VorbisMusicTrack(const UString &name, IFile file) : _name(name), _file(std::move(file))
	{
		this->callback = fillMusicData;
		// FIXME: Arbitrary buffer size?
		this->requestedSampleBufferSize = 4096;
	}

	static size_t vorbis_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
	{
		// FIXME: May overflow on 32bit machines trying to read a 4gb+ file?
		// But then there's no way that *ptr can point to that much memory anyway.....
		size_t byte_size = size * nmemb;
		auto *file = static_cast<IFile *>(datasource);

		file->read(static_cast<char *>(ptr), byte_size);
		auto read_bytes = file->gcount();
		return read_bytes;
	}
	~VorbisMusicTrack() override
	{
		if (_valid)
		{
			ov_clear(&_vorbis_file);
		}
	}
	const UString &getName() const override { return _name; }

	static constexpr ov_callbacks callbacks = {
	    /*read_func = */ vorbis_read_func,
	    /*seek_func = */ nullptr,
	    /*close_func = */ nullptr,
	    /*tell_func = */ nullptr,
	};
};

static constexpr std::array<int, 2> allowed_sample_rates = {22050, 44100};
static constexpr std::array<int, 2> allowed_channel_counts = {1, 2};

class VorbisMusicLoader : public MusicLoader
{
  private:
	Data &_data;

  public:
	VorbisMusicLoader(Data &data) : _data(data) {}
	~VorbisMusicLoader() override = default;

	sp<MusicTrack> loadMusic(UString path) override
	{
		auto strings = split(path, ":");
		// expected format: "ogg:file.ogg"
		if (strings.size() != 2 || strings[0] != "ogg")
		{
			LogInfo("VorbisFile: Not valid vorbis string \"%s\"", path);
			return nullptr;
		}
		auto file = _data.fs.open(strings[1]);
		if (!file)
		{
			LogInfo("VorbisMusic: Failed to open \"%s\"", strings[1]);
			return nullptr;
		}

		auto music = mksp<VorbisMusicTrack>(path, std::move(file));

		auto ret = ov_open_callbacks(&music->_file, &music->_vorbis_file, nullptr, 0,
		                             VorbisMusicTrack::callbacks);

		if (ret < 0)
		{
			LogWarning("VorbisMusic: Error %d opening file \"%s\"", ret, path);
			return nullptr;
		}

		music->_valid = true;

		auto *info = ov_info(&music->_vorbis_file, -1);
		if (!info)
		{
			LogWarning("VorbisMusic: Failed to read info for \"%s\"", path);
			return nullptr;
		}

		bool valid_sample_rate = false;
		for (const auto allowed_sample_rate : allowed_sample_rates)
		{
			if (allowed_sample_rate == info->rate)
			{
				valid_sample_rate = true;
				break;
			}
		}
		if (!valid_sample_rate)
		{
			LogWarning("VorbisMusic: \"%s\" has unsupported sample rate \"%d\"", path, info->rate);
			return nullptr;
		}

		bool valid_channel_count = false;
		for (const auto allowed_channel_count : allowed_channel_counts)
		{
			if (allowed_channel_count == info->channels)
			{
				valid_channel_count = true;
				break;
			}
		}
		if (!valid_channel_count)
		{
			LogWarning("VorbisMusic: \"%s\" has unsupported channel count \"%d\"", path,
			           info->channels);
			return nullptr;
		}

		LogInfo("VorbisMusic: Successfully opened \"%s\" - channels: %d, samplerate: %d", path,
		        info->channels, info->rate);
		music->format.channels = info->channels;
		music->format.frequency = info->rate;
		// Always assume sint16 - it'll convert it otherwise
		music->format.format = OpenApoc::AudioFormat::SampleFormat::PCM_SINT16;

		return music;
	}
};

class VorbisMusicLoaderFactory : public MusicLoaderFactory
{
  public:
	MusicLoader *create(Data &data) override { return new VorbisMusicLoader(data); }
};

}; // namespace detail

MusicLoaderFactory *getVorbisMusicLoaderFactory() { return new detail::VorbisMusicLoaderFactory; }

} // namespace OpenApoc
