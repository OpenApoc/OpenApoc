#pragma once

#include "framework/sound.h"
#include "library/sp.h"
#include <chrono>

namespace OpenApoc
{

class Image;
class Palette;
class AudioFormat;
class IFile;

class FrameImage
{
  public:
	unsigned frame;
	sp<Image> image;
	sp<Palette> palette;
};

class FrameAudio
{
  public:
	unsigned frame;
	up<char[]> samples;
	unsigned sample_count;
	AudioFormat format;
};

class Video
{
  private:
  public:
	virtual std::chrono::duration<unsigned int, std::nano> getFrameTime() const = 0;
	virtual unsigned getFrameCount() const = 0;
	virtual Vec2<int> getVideoSize() const = 0;

	virtual sp<FrameImage> popImage() = 0;
	virtual sp<FrameAudio> popAudio() = 0;
	virtual ~Video() = default;
	virtual void stop() = 0;
	virtual sp<MusicTrack> getMusicTrack() = 0;
};

sp<Video> loadSMKVideo(IFile &file);

} // namespace OpenApoc
