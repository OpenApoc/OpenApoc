#include "framework/fs.h"
#include "framework/image.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "framework/video.h"

// libsmacker.h doesn't set C abi by default, so wrap
extern "C" {
#include "dependencies/libsmacker/smacker.h"
}

namespace OpenApoc
{

class SMKVideo : public Video
{
  public:
	smk smk_ctx;
	std::chrono::duration<unsigned int, std::nano> frame_time;
	unsigned long frame_count;
	unsigned int current_frame;
	Vec2<int> frame_size;

	size_t video_data_size;
	up<char[]> video_data;

	bool stopped;

	SMKVideo()
	    : smk_ctx(nullptr), frame_time(0), frame_count(0), current_frame(0), frame_size(0, 0),
	      video_data_size(0), stopped(false)
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
		TRACE_FN;
		char ret;
		if (this->stopped)
			return nullptr;
		if (this->current_frame == 0)
			ret = smk_first(this->smk_ctx);
		else
			ret = smk_next(this->smk_ctx);

		if (ret == SMK_ERROR)
		{
			LogWarning("Error decoding frame %u", this->current_frame);
			return nullptr;
		}

		if (ret == SMK_LAST)
		{
			LogInfo("Last frame %u", this->current_frame);
			this->stopped = true;
		}

		unsigned char *palette_data = smk_get_palette(this->smk_ctx);
		if (!palette_data)
		{
			LogWarning("Failed to get palette data for frame %u", this->current_frame);
			return nullptr;
		}
		unsigned char *image_data = smk_get_video(this->smk_ctx);
		if (!image_data)
		{
			LogWarning("Failed to get image data for frame %u", this->current_frame);
			return nullptr;
		}

		auto frame = mksp<FrameImage>();

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
			frame->palette->SetColour(i, {red, green, blue});
		}

		LogInfo("Popping frame %u", this->current_frame);
		this->current_frame++;
		return frame;
	}
	// FIXME: implement audio
	sp<FrameAudio> popAudio() override { return nullptr; }
	void stop() override { this->stopped = true; }

	bool load(IFile &file)
	{
		TRACE_FN;
		double usf; // uSeconds per frame
		this->video_data = file.readAll();
		this->video_data_size = file.size();

		auto videoPath = file.systemPath();

		LogInfo("Read %llu bytes from video",
		        static_cast<unsigned long long>(this->video_data_size));

		this->smk_ctx = smk_open_memory(reinterpret_cast<unsigned char *>(this->video_data.get()),
		                                static_cast<unsigned long>(this->video_data_size));
		if (!this->smk_ctx)
		{
			LogWarning("Failed to read SMK file \"%s\"", videoPath.c_str());
			this->video_data.reset();
			return false;
		}
		LogInfo("Successfully created SMK context",
		        static_cast<unsigned long long>(this->video_data_size));

		if (smk_info_all(this->smk_ctx, nullptr, &this->frame_count, &usf))
		{
			LogWarning("Failed to read SMK file info from \"%s\"", videoPath.c_str());
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
			LogWarning("Failed to read SMK video info from \"%s\"", videoPath.c_str());
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
			LogWarning("Error enabling video for \"%s\"", videoPath.c_str());
			return false;
		}

		// Everything looks  good
		return true;
	}

	~SMKVideo() override
	{
		if (this->smk_ctx)
			smk_close(this->smk_ctx);
	}
};

sp<Video> loadSMKVideo(IFile &file)
{
	auto vid = mksp<SMKVideo>();
	if (vid->load(file))
		return vid;
	else
		return nullptr;
}

} // namespace OpenApoc
