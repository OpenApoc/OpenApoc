#include "framework/jukebox.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "library/xorshift.h"
#include <array>

namespace OpenApoc
{
namespace
{

class JukeBoxImpl : public JukeBox
{
	Framework &fw;
	unsigned int position;
	std::vector<sp<MusicTrack>> trackList;
	PlayMode mode;
	PlayList list;
	Xorshift128Plus<uint64_t> rng;
	bool playlistsLoaded = false;

	std::map<PlayList, std::vector<UString>> playlists;

  public:
	JukeBoxImpl(Framework &fw) : fw(fw), position(0), mode(PlayMode::Shuffle), list(PlayList::None)
	{
		// Use the time to give a little initial randomness to the shuffle rng
		auto time_now = std::chrono::system_clock::now();
		uint64_t time_seconds =
		    std::chrono::duration_cast<std::chrono::seconds>(time_now.time_since_epoch()).count();
		rng.seed(time_seconds);
	}
	~JukeBoxImpl() override { this->stop(); }

	void shuffle() { std::shuffle(trackList.begin(), trackList.end(), rng); }

	void play(PlayList list, PlayMode mode) override
	{
		if (!playlistsLoaded)
		{
			LogWarning("JukeBox::play() called without any playlists loaded");
			return;
		}
		if (this->list == list)
			return;
		this->list = list;
		if (this->list == PlayList::None)
		{
			this->stop();
		}
		else
		{
			this->play(playlists[list], mode);
		}
	}

	void play(const std::vector<UString> &tracks, PlayMode mode) override
	{
		this->trackList.clear();
		this->position = 0;
		this->mode = mode;
		for (auto &track : tracks)
		{
			auto musicTrack = fw.data->loadMusic(track);
			if (!musicTrack)
				LogError("Failed to load music track \"%s\" - skipping", track);
			else
				this->trackList.push_back(musicTrack);
		}
		if (mode == PlayMode::Shuffle)
			shuffle();
		this->progressTrack(this);
		this->fw.soundBackend->playMusic(progressTrack, this);
	}

	static void progressTrack(void *data)
	{
		JukeBoxImpl *jukebox = static_cast<JukeBoxImpl *>(data);
		if (jukebox->trackList.empty())
		{
			LogWarning("Trying to play empty jukebox");
			return;
		}
		if (jukebox->position >= jukebox->trackList.size())
		{
			LogInfo("End of jukebox playlist");
			return;
		}
		LogInfo("Playing track %u (%s)", jukebox->position,
		        jukebox->trackList[jukebox->position]->getName());
		jukebox->fw.soundBackend->setTrack(jukebox->trackList[jukebox->position]);

		jukebox->position++;
		if (jukebox->position >= jukebox->trackList.size())
		{
			if (jukebox->mode == PlayMode::Loop)
			{
				jukebox->position = 0;
			}
			else if (jukebox->mode == PlayMode::Shuffle)
			{
				jukebox->position = 0;
				jukebox->shuffle();
			}
		}
	}

	void stop() override
	{
		this->list = PlayList::None;
		fw.soundBackend->stopMusic();
	}

	void loadPlaylists() override
	{
		const std::map<PlayList, UString> playlistPaths = {
		    {PlayList::City, "playlists/city.xml"},
		    {PlayList::Tactical, "playlists/tactical.xml"},
		    {PlayList::Action, "playlists/action.xml"},
		    {PlayList::Alien, "playlists/alien.xml"},

		};

		for (const auto &[playlist, path] : playlistPaths)
		{
			auto file = fw.data->fs.open(path);
			if (!file)
			{
				LogWarning("Failed to open playlist file \"%s\"", path);
				continue;
			}
			auto data = file.readAll();
			if (!data)
			{
				LogWarning("Failed to read playlist file \"%s\"", path);
				continue;
			}

			pugi::xml_document doc;
			auto result = doc.load_buffer(data.get(), file.size());
			if (!result)
			{
				LogWarning("Failed to parse playlist \"%s\" - \"%s\" at \"%llu\"", path,
				           result.description(), result.offset);
				continue;
			}
			auto node = doc.child("openapoc_playlist");
			if (!node)
			{
				LogWarning("No root \"openapoc_playlist\" element in playlist file \"%s\"", path);
				continue;
			}

			for (auto child = node.first_child(); child; child = child.next_sibling())
			{
				UString nodename = child.name();
				if (nodename == "playlist_entry")
				{
					UString entry = child.text().as_string();
					this->playlists[playlist].push_back(entry);
				}
			}
		}
		this->playlistsLoaded = true;
	}
};

} // namespace

up<JukeBox> createJukebox(Framework &fw) { return mkup<JukeBoxImpl>(fw); }
} // namespace OpenApoc
