#include "framework/fs.h"
#include "framework/image.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "framework/sound.h"
#include "framework/video.h"
#include <cstring>
#include <mutex>
#include <queue>

// libsmacker.h doesn't set C abi by default, so wrap
extern "C"
{
#include "dependencies/libsmacker/smacker.h"
}

namespace OpenApoc
{

class SMKVideo;
MusicTrack::MusicCallbackReturn fillSMKMusicData(sp<MusicTrack> thisTrack, unsigned int maxSamples,
                                                 void *sampleBuffer, unsigned int *returnedSamples);

class SMKMusicTrack : public MusicTrack
{
  private:
	sp<SMKVideo> video;
	sp<FrameAudio> current_frame;
	unsigned current_frame_sample_position;

  public:
	SMKMusicTrack(sp<SMKVideo> video);
	const UString &getName() const override;
	MusicCallbackReturn fillData(unsigned int maxSamples, void *sampleBuffer,
	                             unsigned int *returnedSamples);
};

class SMKVideo : public Video, public std::enable_shared_from_this<SMKVideo>
{
  public:
	smk smk_ctx;
	std::chrono::duration<unsigned int, std::nano> frame_time;
	unsigned long frame_count;
	unsigned int current_frame_video;
	unsigned int current_frame_audio;

	unsigned current_frame_read;

	Vec2<int> frame_size;

	size_t video_data_size;
	up<char[]> video_data;

	bool stopped;

	std::recursive_mutex frame_queue_lock;
	std::queue<sp<FrameImage>> image_queue;
	std::queue<sp<FrameAudio>> audio_queue;

	AudioFormat audio_format;
	unsigned audio_bytes_per_sample;
	UString file_path;

	wp<SMKMusicTrack> music_track;

	SMKVideo()
	    : smk_ctx(nullptr), frame_time(0), frame_count(0), current_frame_video(0),
	      current_frame_audio(0), current_frame_read(0), frame_size(0, 0), video_data_size(0),
	      stopped(false)
	{
	}

	std::chrono::duration<unsigned int, std::nano> getFrameTime() const override
	{
		return this->frame_time;
	}
	unsigned getFrameCount() const override { return this->frame_count; }
	Vec2<int> getVideoSize() const override { return this->frame_size; }

	sp<FrameImage> popImage() override
	{
		std::lock_guard<std::recursive_mutex> l(this->frame_queue_lock);
		if (this->image_queue.empty())
		{
			if (this->readNextFrame() == false)
			{
				return nullptr;
			}
		}
		auto frame = this->image_queue.front();
		this->image_queue.pop();
		this->current_frame_video++;
		return frame;
	}

	sp<FrameAudio> popAudio() override
	{
		std::lock_guard<std::recursive_mutex> l(this->frame_queue_lock);
		if (this->audio_queue.empty())
		{
			if (this->readNextFrame() == false)
			{
				return nullptr;
			}
		}
		auto frame = this->audio_queue.front();
		this->audio_queue.pop();
		this->current_frame_audio++;
		return frame;
	}

	bool readNextFrame()
	{
		std::lock_guard<std::recursive_mutex> l(this->frame_queue_lock);
		if (this->stopped)
			return false;

		char ret;
		if (this->current_frame_read == 0)
			ret = smk_first(this->smk_ctx);
		else
			ret = smk_next(this->smk_ctx);

		if (ret == SMK_ERROR)
		{
			LogWarning("Error decoding frame %u", this->current_frame_read);
			return false;
		}

		if (ret == SMK_LAST)
		{
			LogInfo("Last frame %u", this->current_frame_read);
			this->stopped = true;
		}

		const unsigned char *palette_data = smk_get_palette(this->smk_ctx);
		if (!palette_data)
		{
			LogWarning("Failed to get palette data for frame %u", this->current_frame_read);
			return false;
		}
		const unsigned char *image_data = smk_get_video(this->smk_ctx);
		if (!image_data)
		{
			LogWarning("Failed to get image data for frame %u", this->current_frame_read);
			return false;
		}

		auto frame = mksp<FrameImage>();
		frame->frame = this->current_frame_read;

		auto img = mksp<PaletteImage>(this->frame_size);
		frame->image = img;
		frame->palette = mksp<Palette>(256);

		PaletteImageLock img_lock(img);
		memcpy(img_lock.getData(), image_data, this->frame_size.x * this->frame_size.y);

		for (unsigned int i = 0; i < 256; i++)
		{
			auto red = *palette_data++;
			auto green = *palette_data++;
			auto blue = *palette_data++;
			frame->palette->setColour(i, {red, green, blue});
		}

		auto audio_frame = mksp<FrameAudio>();
		audio_frame->frame = current_frame_read;
		audio_frame->format = this->audio_format;
		unsigned long audio_bytes = smk_get_audio_size(this->smk_ctx, 0);
		if (audio_bytes == 0)
		{
			LogWarning("Error reading audio size for frame %u", this->current_frame_read);
			return false;
		}

		auto sample_count =
		    audio_bytes / (this->audio_bytes_per_sample * this->audio_format.channels);
		audio_frame->samples.reset(new char[audio_bytes]);
		audio_frame->sample_count = sample_count;
		auto sample_pointer = smk_get_audio(this->smk_ctx, 0);
		if (!sample_pointer)
		{
			LogWarning("Error reading audio data for frame %u", this->current_frame_read);
			return false;
		}
		memcpy(audio_frame->samples.get(), sample_pointer, audio_bytes);

		LogInfo("Read %lu samples bytes, %u samples", audio_bytes, audio_frame->sample_count);

		this->image_queue.push(frame);
		this->audio_queue.push(audio_frame);

		LogInfo("read frame %u", this->current_frame_read);
		this->current_frame_read++;
		return true;
	}

	void stop() override { this->stopped = true; }

	bool load(IFile &file)
	{
		double usf; // uSeconds per frame
		this->video_data = file.readAll();
		this->video_data_size = file.size();

		auto video_path = file.systemPath();
		this->file_path = video_path;

		LogInfo("Read %llu bytes from video",
		        static_cast<unsigned long long>(this->video_data_size));

		this->smk_ctx = smk_open_memory(reinterpret_cast<unsigned char *>(this->video_data.get()),
		                                static_cast<unsigned long>(this->video_data_size));
		if (!this->smk_ctx)
		{
			LogWarning("Failed to read SMK file \"%s\"", video_path);
			this->video_data.reset();
			return false;
		}
		LogInfo("Successfully created SMK context");

		if (smk_info_all(this->smk_ctx, nullptr, &this->frame_count, &usf))
		{
			LogWarning("Failed to read SMK file info from \"%s\"", video_path);
			this->video_data.reset();
			smk_close(this->smk_ctx);
			this->smk_ctx = nullptr;
			return false;
		}

		this->frame_time = std::chrono::nanoseconds((unsigned int)(usf * 1000));

		LogInfo("Video frame count %lu, ns per frame = %u (USF: %f)", this->frame_count,
		        this->frame_time.count(), usf);

		unsigned long height, width;
		if (smk_info_video(this->smk_ctx, &width, &height, nullptr))
		{
			LogWarning("Failed to read SMK video info from \"%s\"", video_path);
			this->video_data.reset();
			smk_close(this->smk_ctx);
			this->smk_ctx = nullptr;
			return false;
		}

		this->frame_size = {width, height};
		LogInfo("Video frame size {%u,%u}", this->frame_size.x, this->frame_size.y);

		auto ret = smk_enable_video(this->smk_ctx, 1);
		if (ret == SMK_ERROR)
		{
			LogWarning("Error enabling video for \"%s\"", video_path);
			return false;
		}

		unsigned char audio_track_mask;
		unsigned char channels[7];
		unsigned char bitdepth[7];
		unsigned long audio_rate[7];

		ret = smk_info_audio(this->smk_ctx, &audio_track_mask, channels, bitdepth, audio_rate);
		if (ret == SMK_ERROR)
		{
			LogWarning("Error reading audio info for \"%s\"", video_path);
			return false;
		}

		if (audio_track_mask & SMK_AUDIO_TRACK_0)
		{
			// WE only support a single track
			LogInfo("Audio track: channels %u depth %u rate %lu", (unsigned)channels[0],
			        (unsigned)bitdepth[0], audio_rate[0]);
		}
		else
		{
			LogWarning("Unsupported audio track mask 0x%02x for \"%s\"", (unsigned)audio_track_mask,
			           video_path);
			return false;
		}
		switch (channels[0])
		{
			case 1:
				// Mono
				this->audio_format.channels = 1;
				break;

			case 2:
				// Stereo
				this->audio_format.channels = 2;
				break;
			default:
				LogWarning("Unsupported audio channel count %u for \"%s\"", (unsigned)channels[0],
				           video_path);
				return false;
		}
		switch (bitdepth[0])
		{
			case 8:
				this->audio_format.format = AudioFormat::SampleFormat::PCM_UINT8;
				this->audio_bytes_per_sample = 1;
				break;
			case 16:
				this->audio_format.format = AudioFormat::SampleFormat::PCM_SINT16;
				this->audio_bytes_per_sample = 2;
				break;
			default:
				LogWarning("Unsupported audio bit depth %u for \"%s\"", (unsigned)bitdepth[0],
				           video_path);
				return false;
		}
		this->audio_format.frequency = audio_rate[0];

		ret = smk_enable_audio(this->smk_ctx, 0, 1);

		if (ret == SMK_ERROR)
		{
			LogWarning("Error enabling audio track 0 for \"%s\"", video_path);
		}

		// Everything looks  good
		return true;
	}

	sp<MusicTrack> getMusicTrack() override
	{
		auto ptr = this->music_track.lock();
		if (!ptr)
		{
			ptr = mksp<SMKMusicTrack>(shared_from_this());
			this->music_track = ptr;
		}
		return ptr;
	}

	~SMKVideo() override
	{
		if (this->smk_ctx)
			smk_close(this->smk_ctx);
	}
};

MusicTrack::MusicCallbackReturn fillSMKMusicData(sp<MusicTrack> thisTrack, unsigned int maxSamples,
                                                 void *sampleBuffer, unsigned int *returnedSamples)
{
	auto track = std::dynamic_pointer_cast<SMKMusicTrack>(thisTrack);
	LogAssert(track);
	return track->fillData(maxSamples, sampleBuffer, returnedSamples);
}
SMKMusicTrack::SMKMusicTrack(sp<SMKVideo> video)
    : video(video), current_frame(video->popAudio()), current_frame_sample_position(0)
{
	this->format = video->audio_format;
	this->requestedSampleBufferSize = this->format.frequency / 10;
	this->callback = fillSMKMusicData;
}

const UString &SMKMusicTrack::getName() const { return this->video->file_path; }

MusicTrack::MusicCallbackReturn SMKMusicTrack::fillData(unsigned int maxSamples, void *sampleBuffer,
                                                        unsigned int *returnedSamples)
{
	if (!this->current_frame)
	{
		LogWarning("Playing beyond end of video");
		*returnedSamples = 0;
		return MusicCallbackReturn::End;
	}

	unsigned int samples_in_this_frame = std::min(
	    maxSamples, this->current_frame->sample_count - this->current_frame_sample_position);

	unsigned int audio_size_bytes = samples_in_this_frame *
	                                this->current_frame->format.getSampleSize() *
	                                this->current_frame->format.channels;

	unsigned int audio_offset_bytes = this->current_frame_sample_position *
	                                  this->current_frame->format.getSampleSize() *
	                                  this->current_frame->format.channels;

	memcpy(sampleBuffer, this->current_frame->samples.get() + audio_offset_bytes, audio_size_bytes);

	*returnedSamples = samples_in_this_frame;

	this->current_frame_sample_position += samples_in_this_frame;
	LogAssert(this->current_frame_sample_position <= this->current_frame->sample_count);

	if (this->current_frame_sample_position == this->current_frame->sample_count)
	{
		this->current_frame = video->popAudio();
		this->current_frame_sample_position = 0;
	}

	if (this->current_frame)
	{
		return MusicCallbackReturn::Continue;
	}
	else
	{
		return MusicCallbackReturn::End;
	}
}
sp<Video> loadSMKVideo(IFile &file)
{
	auto vid = mksp<SMKVideo>();
	if (vid->load(file))
		return vid;
	else
		return nullptr;
}

} // namespace OpenApoc
