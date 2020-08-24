#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include <vector>

namespace OpenApoc
{

class Framework;

class JukeBox
{
  public:
	enum class PlayMode
	{
		Once,
		Loop,
		Shuffle
	};
	enum class PlayList
	{
		None,
		City,
		Tactical,
		Action,
		Alien
	};
	virtual ~JukeBox() = default;
	virtual void play(PlayList list, PlayMode mode = PlayMode::Shuffle) = 0;
	virtual void play(const std::vector<UString> &tracks, PlayMode mode = PlayMode::Shuffle) = 0;
	virtual void stop() = 0;
	virtual void loadPlaylists() = 0;
};

up<JukeBox> createJukebox(Framework &fw);

} // namespace OpenApoc
