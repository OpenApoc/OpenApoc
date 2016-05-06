#pragma once
#include "framework/includes.h"
#include "framework/stage.h"
#include "library/sp.h"
#include <chrono>
#include <future>

namespace OpenApoc
{

class Video;
class Image;
class FrameImage;

class VideoScreen : public Stage
{
  private:
	std::future<void> loading_task;
	sp<Image> loadingimage;
	std::function<sp<Stage>()> nextScreenFn;
	sp<Image> backgroundimage;
	float loadingimageangle;

	Vec2<int> frame_position;
	Vec2<int> frame_size;
	std::chrono::high_resolution_clock::time_point last_frame_time;

	sp<Video> video;
	sp<FrameImage> current_frame;

  public:
	VideoScreen(const UString &videoPath, std::future<void> task,
	            std::function<sp<Stage>()> nextScreenFn, sp<Image> background = nullptr);
	// Stage control
	void Begin() override;
	void Pause() override;
	void Resume() override;
	void Finish() override;
	void EventOccurred(Event *e) override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	bool IsTransition() override;
};

}; // namespace OpenApoc
