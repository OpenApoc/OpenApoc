#define _USE_MATH_DEFINES
#include "game/ui/general/videoscreen.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "framework/sound.h"
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
		this->video = fw().data->loadVideo(videoPath);
		if (!this->video)
		{
			LogWarning("Failed to load video \"%s\"", videoPath.cStr());
		}
		else
		{
			// Scale keeping aspect ratio to the max of the screen size
			Vec2<float> unscaled_frame_size = this->video->getVideoSize();
			Vec2<float> display_size = fw().displayGetSize();
			Vec2<float> scale_factors = display_size / unscaled_frame_size;
			float scale = std::min(scale_factors.x, scale_factors.y);
			this->frame_size = unscaled_frame_size * scale;
			LogInfo("Scaling video from {%d,%d} to {%d,%d}", this->video->getVideoSize().x,
			        this->video->getVideoSize().y, this->frame_size.x, this->frame_size.y);
			this->frame_position = (fw().displayGetSize() / 2) - (this->frame_size / 2);
		}
	}
	else
	{
		LogInfo("No video");
	}
}

void VideoScreen::begin()
{
	// FIXME: This is now useless, as it doesn't actually load anything interesting here
	loadingimage = fw().data->loadImage("ui/loading.png");
	if (!backgroundimage)
	{
		backgroundimage = fw().data->loadImage("ui/logo.png");
	}
	fw().displaySetIcon();
	loadingimageangle = 0;
	last_frame_time = std::chrono::high_resolution_clock::now();
	if (this->video)
	{
		this->current_frame = this->video->popImage();

		fw().soundBackend->setTrack(video->getMusicTrack());
		fw().soundBackend->playMusic([](void *) {}, nullptr);
	}
}

void VideoScreen::pause() {}

void VideoScreen::resume() {}

void VideoScreen::finish() {}

void VideoScreen::eventOccurred(Event *e)
{
	if ((e->type() == EVENT_KEY_DOWN && e->keyboard().KeyCode == SDLK_ESCAPE) ||
	    (e->type() == EVENT_MOUSE_DOWN || e->type() == EVENT_FINGER_DOWN))
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

void VideoScreen::update()
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
				fw().stageQueueCommand({StageCmd::Command::REPLACE, this->nextScreenFn()});
				return;
			default:
				// Not yet finished
				return;
		}
	}
}

void VideoScreen::render()
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
		                          this->frame_size, Renderer::Scaler::Nearest);
	}
	else
	{

		int logow = fw().displayGetWidth() / 3;
		float logosc = logow / static_cast<float>(backgroundimage->size.x);

		Vec2<float> logoPosition{
		    fw().displayGetWidth() / 2 - (backgroundimage->size.x * logosc / 2),
		    fw().displayGetHeight() / 2 - (backgroundimage->size.y * logosc / 2)};
		Vec2<float> logoSize{backgroundimage->size.x * logosc, backgroundimage->size.y * logosc};

		fw().renderer->drawScaled(backgroundimage, logoPosition, logoSize);

		fw().renderer->drawRotated(
		    loadingimage, Vec2<float>{24, 24},
		    Vec2<float>{fw().displayGetWidth() - 50, fw().displayGetHeight() - 50},
		    loadingimageangle);
	}
}

bool VideoScreen::isTransition() { return false; }

}; // namespace OpenApoc
