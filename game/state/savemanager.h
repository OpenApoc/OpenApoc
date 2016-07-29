#pragma once
#include "framework/serialization/serialize.h"
#include "game/state/gamestate.h"
#include <future>

class SerializationNode;
class GameState;
namespace OpenApoc
{

enum class SaveType : unsigned
{
	Manual = 0,
	Auto = 1,
	Quick = 2
};

/* Metadata of save archive displayed in savemenu	*/
class SaveMetadata
{
  private:
	UString name;
	UString file;
	UString difficulty;
	time_t creationDate;
	SaveType type;
	unsigned long gameTicks;

  public:
	SaveMetadata();
	~SaveMetadata();
	SaveMetadata(UString name, UString file, time_t creationDate, SaveType type,
	             const sp<GameState> gameState);
	SaveMetadata(const SaveMetadata &metdata, time_t creationDate, const sp<GameState> gameState);

	/* Deserialize given manifest document	*/
	bool deserializeManifest(const sp<SerializationArchive> archive, const UString &saveFileName);

	bool serializeManifest(const sp<SerializationArchive> archive) const;

	/* Creation date of save file (unix epoch)	*/
	time_t getCreationDate() const;

	/* Name of save game	*/
	const UString &getName() const;

	/* Path to file containing (or that will contain) save	*/
	const UString &getFile() const;

	/* Name of difficulty	*/
	const UString &getDifficulty() const;

	/* What kind of save is it */
	const SaveType &getType() const;

	unsigned int getGameTicks() const;
};

/* high level api for managing saved games */
class SaveManager
{
  private:
	UString saveDirectory;
	UString createSavePath(const UString &name) const;
	bool findFreePath(UString &path, const UString &name) const;

	bool saveGame(const SaveMetadata &metadata, const sp<GameState> gameState) const;

  public:
	SaveManager();

	/* load game with given metadata */
	std::future<sp<GameState>> loadGame(const SaveMetadata &metadata) const;

	/* from given file */
	std::future<sp<GameState>> loadGame(const UString &savePath) const;

	/* load from predefined save type, eg Quicksave */
	std::future<sp<GameState>> loadSpecialSave(const SaveType type) const;

	// create new save file with given name
	// WARNING! Name MUST NOT contain invalid filename characters!
	bool newSaveGame(const UString &name, const sp<GameState> gameState) const;

	// saves game to location pointed by metadata, also updates metadata from gamestate
	bool overrideGame(const SaveMetadata &metadata, const UString &newFile,
	                  const sp<GameState> gameState) const;

	// can be used for autosaves, quicksaves etc.
	bool specialSaveGame(SaveType type, const sp<GameState> gameState) const;

	// list all reachable saved games
	std::vector<SaveMetadata> getSaveList() const;
	
	bool deleteGame(const sp<SaveMetadata>& slot) const;
};
}