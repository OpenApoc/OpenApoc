#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include <cstdint>
#include <future>
#include <vector>

namespace OpenApoc
{
class SerializationArchive;
class GameState;

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
	uint64_t gameTicks;

  public:
	SaveMetadata();
	~SaveMetadata();
	SaveMetadata(UString name, UString file, time_t creationDate, SaveType type,
	             const sp<GameState> gameState);
	SaveMetadata(const SaveMetadata &metdata, time_t creationDate, const sp<GameState> gameState);

	/* Deserialize given manifest document	*/
	bool deserializeManifest(SerializationArchive *archive, const UString &saveFileName);

	bool serializeManifest(SerializationArchive *archive) const;

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

	uint64_t getGameTicks() const;
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
	std::shared_future<void> loadGame(const SaveMetadata &metadata, sp<GameState> state) const;

	/* from given file */
	std::shared_future<void> loadGame(const UString &savePath, sp<GameState> state) const;

	/* load from predefined save type, eg Quicksave */
	std::shared_future<void> loadSpecialSave(const SaveType type, sp<GameState> state) const;

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

	bool deleteGame(const sp<SaveMetadata> &slot) const;
};
} // namespace OpenApoc
