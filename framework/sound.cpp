#include "framework/sound.h"
#include "framework/logger.h"
#include "framework/sampleloader_interface.h"
#include "framework/musicloader_interface.h"
#include "framework/sound_interface.h"
#include <map>
#include <memory>
#include <limits>
#include <array>

namespace
{
static float mix(float a, float b, float factor) { return a * (1.0f - factor) + b * factor; }
}; // anonymous namespace

namespace OpenApoc
{
static const std::array<std::pair<float, float>, 3> positionalAudioLUT = {
    std::make_pair(20.0f, 1.0f), // Anything within 20.0f units is at full volume
    std::make_pair(100.0f, 0.25f), // That then scales linearly down to 25% over the next 100 units
    std::make_pair(std::numeric_limits<float>::max(), 0.25f) // Which does not decrease any further
};

// Position is assumed to be in 'map' units
void SoundBackend::playSample(sp<Sample> sample, Vec3<float> position)
{

	float distance = glm::length(position - this->listenerPosition);

	/*FIXME: Quick hack at trying to get scaling based on the 3d location of the sample being
	 * played*/
	float gain = 1.0f;

	for (auto &lutEntry : positionalAudioLUT)
	{
		float lutDistance = lutEntry.first;
		float lutGain = lutEntry.second;
		if (distance <= lutDistance)
		{
			float factor = distance / lutDistance;
			float newGain = mix(gain, lutGain, factor);
			gain = newGain;
			break;
		}
		else
		{
			gain = lutGain;
			distance -= lutDistance;
		}
	}
	LogInfo("Playing sample at {%f,%f,%f} - distance to camera %f, gain %f", position.x, position.y,
	        position.z, distance, gain);
	// Anything within CLOSE_RANGE is at full volume
	this->playSample(sample, gain);
}
void SoundBackend::setListenerPosition(Vec3<float> position) { this->listenerPosition = position; }
}; // namespace OpenApoc
