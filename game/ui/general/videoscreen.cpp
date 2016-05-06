#define _USE_MATH_DEFINES
#include "game/ui/general/videoscreen.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "framework/video.h"
#include <chrono>
#include <cmath>

namespace OpenApoc
{

VideoScreen::VideoScreen(const UString &videoPath, std::future<void> task,
                         std::function<sp<Stage>()> nextScreenFn, sp<Image> background)
    : Stage(), loading_task(std::move(task)), nextScreenFn(nextScreenFn),
      backgroundimage(background)
{
	if (videoPath != "")
	{
		this->video = fw().data->load_video(videoPath);
		if (!this->video)
		{
			LogWarning("Failed to load video \"%s\"", videoPath.c_str());
		}
		else
		{
			this->frame_size = this->video->getVideoSize();
			this->frame_position = (fw().Display_GetSize() / 2) - (this->frame_size / 2);
		}
	}
	else
	{
		LogInfo("No video");
	}
}

void VideoScreen::Begin()
{
	// FIXME: This is now useless, as it doesn't actually load anything interesting here
	loadingimage = fw().data->load_image("UI/LOADING.PNG");
	if (!backgroundimage)
	{
		backgroundimage = fw().data->load_image("UI/LOGO.PNG");
	}
	fw().Display_SetIcon();
	loadingimageangle = 0;
	last_frame_time = std::chrono::high_resolution_clock::now();
	this->current_frame = this->video->popImage();

	fw().soundBackend->setTrack(video->getMusicTrack());
	fw().soundBackend->playMusic([](void *) {}, nullptr);
}

void VideoScreen::Pause() {}

void VideoScreen::Resume() {}

void VideoScreen::Finish() {}

void VideoScreen::EventOccurred(Event *e)
{
	if (e->Type() == EVENT_KEY_DOWN && e->Keyboard().KeyCode == SDLK_ESCAPE)
	{
		// Magically skip the rest of the video
		if (this->video)
		{
			this->video->stop();
			this->video = nullptr;
		}
		this->current_frame = nullptr;
	}
}

void VideoScreen::Update(StageCmd *const cmd)
{
	if (!this->current_frame)
	{
		loadingimageangle += (M_PI + 0.05f);
		if (loadingimageangle >= M_PI * 2.0f)
			loadingimageangle -= M_PI * 2.0f;

		auto status = this->loading_task.wait_for(std::chrono::seconds(0));
		switch (status)
		{
			case std::future_status::ready:
				cmd->cmd = StageCmd::Command::REPLACE;
				cmd->nextStage = this->nextScreenFn();
				return;
			default:
				// Not yet finished
				return;
		}
	}
}

void VideoScreen::Render()
{
	TRACE_FN;
	if (this->video)
	{
		auto time_now = std::chrono::high_resolution_clock::now();
		auto time_since_last_frame = time_now - this->last_frame_time;
		while (time_since_last_frame >= this->video->getFrameTime())
		{
			this->last_frame_time += video->getFrameTime();
			this->current_frame = this->video->popImage();

			if (!this->current_frame)
			{
				// End of video
				this->video = nullptr;
				break;
			}
			time_since_last_frame = time_now - this->last_frame_time;
		}
	}
	if (this->current_frame)
	{
		if (this->current_frame->palette)
			fw().renderer->setPalette(this->current_frame->palette);
		fw().renderer->drawScaled(this->current_frame->image, this->frame_position,
		                          this->frame_size);
	}
	else
	{

		int logow = fw().Display_GetWidth() / 3;
		float logosc = logow / static_cast<float>(backgroundimage->size.x);

		Vec2<float> logoPosition{
		    fw().Display_GetWidth() / 2 - (backgroundimage->size.x * logosc / 2),
		    fw().Display_GetHeight() / 2 - (backgroundimage->size.y * logosc / 2)};
		Vec2<float> logoSize{backgroundimage->size.x * logosc, backgroundimage->size.y * logosc};

		fw().renderer->drawScaled(backgroundimage, logoPosition, logoSize);

		fw().renderer->drawRotated(
		    loadingimage, Vec2<float>{24, 24},
		    Vec2<float>{fw().Display_GetWidth() - 50, fw().Display_GetHeight() - 50},
		    loadingimageangle);
	}
}

bool VideoScreen::IsTransition() { return false; }

}; // namespace OpenApoc
