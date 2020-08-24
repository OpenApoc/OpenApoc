#include "framework/jukebox.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "library/xorshift.h"

namespace OpenApoc
{
namespace
{
// TODO: Make this moddable
const std::vector<std::vector<UString>> playlists = {
    // None
    {},
    // Cityscape Ambient
    {"rawmusic:music:0:8413184", "rawmusic:music:8413184:11223040",
     "rawmusic:music:19636224:16293888", "rawmusic:music:35930112:10682368",
     "rawmusic:music:46612480:11309056", "rawmusic:music:57921536:14200832",
     "rawmusic:music:72122368:11974656", "rawmusic:music:84097024:8519680",
     "rawmusic:music:92616704:11448320", "rawmusic:music:104065024:3692544"},
    // Tactical Ambient (also Cityscape Action)
    {"rawmusic:music:107757568:10749952", "rawmusic:music:118507520:12140544",
     "rawmusic:music:130648064:11474944", "rawmusic:music:142123008:11921408",
     "rawmusic:music:154044416:11878400", "rawmusic:music:165922816:10727424",
     "rawmusic:music:176650240:10563584", "rawmusic:music:187213824:9541632",
     "rawmusic:music:196755456:10473472", "rawmusic:music:207228928:11307008"},
    // Tactical Action
    {"rawmusic:music:218535936:12933120", "rawmusic:music:231469056:2646016",
     "rawmusic:music:234115072:3076096", "rawmusic:music:237191168:2646016",
     "rawmusic:music:239837184:2639872", "rawmusic:music:242477056:2650112",
     "rawmusic:music:245127168:2732032", "rawmusic:music:247859200:2646016"},

    // Alien Dimension
    {"rawmusic:music:250505216:12849152", "rawmusic:music:263354368:12408832",
     "rawmusic:music:275763200:12054528", "rawmusic:music:287817728:11036672",
     "rawmusic:music:298854400:12834816"}};

class JukeBoxImpl : public JukeBox
{
	Framework &fw;
	unsigned int position;
	std::vector<sp<MusicTrack>> trackList;
	PlayMode mode;
	PlayList list;
	Xorshift128Plus<uint64_t> rng;

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
		if (this->list == list)
			return;
		this->list = list;
		if (this->list == PlayList::None)
		{
			this->stop();
		}
		else
		{
			this->play(playlists[(int)list], mode);
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
};

} // namespace

up<JukeBox> createJukebox(Framework &fw) { return mkup<JukeBoxImpl>(fw); }
} // namespace OpenApoc
