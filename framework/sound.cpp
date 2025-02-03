#include "framework/sound.h"
#include "framework/logger.h"
#include <array>
#include <glm/glm.hpp>
#include <limits>
#include <memory>
#include <utility>

namespace OpenApoc
{
static const std::array<std::pair<float, float>, 4> positionalAudioLUT = {{
    std::make_pair(3.0f, 1.0f),   // Anything within 3.0f units is at full volume
    std::make_pair(30.0f, 0.25f), // That then scales linearly down to 25% over the next 30 units
    std::make_pair(60.0f, 0.10f), // That then scales linearly down to 10% over the next 30 units
    std::make_pair(std::numeric_limits<float>::max(), 0.10f) // Which does not decrease any further
}};

// Position is assumed to be in 'map' units, gainMultiplier within 0 and 1
void SoundBackend::playSample(sp<Sample> sample, Vec3<float> position, float gainMultiplier)
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
	LogInfo("Playing sample at {{{:f},{:f},{:f}}} - distance to camera {:f}, gain {:f}", position.x,
	        position.y, position.z, distance, gain);
	// Anything within CLOSE_RANGE is at full volume
	this->playSample(sample, gain * gainMultiplier);
}
void SoundBackend::setListenerPosition(Vec3<float> position) { this->listenerPosition = position; }
}; // namespace OpenApoc
