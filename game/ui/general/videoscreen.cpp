#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/ui/general/videoscreen.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "framework/video.h"
#include <chrono>
#include <cmath>

namespace OpenApoc
{

VideoScreen::VideoScreen(const UString &videoPath, sp<Stage> nextScreen)
    : Stage(), nextScreen(nextScreen)
{
	if (videoPath != "")
	{
		this->video = fw().data->loadVideo(videoPath);
		if (!this->video)
		{
			LogWarning("Failed to load video \"%s\"", videoPath);
		}
		else
		{
			// Scale keeping aspect ratio to the max of the screen size
			Vec2<float> unscaled_frame_size = this->video->getVideoSize();
			Vec2<float> display_size = fw().displayGetSize();
			Vec2<float> scale_factors = display_size / unscaled_frame_size;
			float scale = std::min(scale_factors.x, scale_factors.y);
			this->frame_size = unscaled_frame_size * scale;
			LogInfo("Scaling video from %s to %s", this->video->getVideoSize(), this->frame_size);
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
		fw().stageQueueCommand({StageCmd::Command::REPLACE, this->nextScreen});
	}
}

void VideoScreen::render()
{
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
}

bool VideoScreen::isTransition() { return false; }

}; // namespace OpenApoc
