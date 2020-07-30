#include "game/state/savemanager.h"
#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/framework.h"
#include "framework/options.h"
#include "framework/serialization/serialize.h"
#include "game/state/gamestate.h"
#include "version.h"
#include <algorithm>
#include <sstream>

// boost uuid for generating temporary identifier for new save
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // conversion to string

namespace uuids = boost::uuids;

namespace OpenApoc
{
const UString saveManifestName = "save_manifest";
const UString saveFileExtension = ".save";

SaveManager::SaveManager() : saveDirectory(Options::saveDirOption.get()) {}

UString SaveManager::createSavePath(const UString &name) const
{
	UString result;
	result += saveDirectory;
	result += "/";
	result += name;
	result += saveFileExtension;
	return result;
}

static std::map<SaveType, UString> saveTypeNames{{SaveType::Manual, "New saved game"},
                                                 {SaveType::Quick, "Quicksave"},
                                                 {SaveType::Auto, "Autosave"}};

std::shared_future<void> SaveManager::loadGame(const SaveMetadata &metadata,
                                               sp<GameState> state) const
{
	return loadGame(metadata.getFile(), state);
}

std::shared_future<void> SaveManager::loadGame(const UString &savePath, sp<GameState> state) const
{
	UString saveArchiveLocation = savePath;
	auto loadTask = fw().threadPoolEnqueue([saveArchiveLocation, state]() -> void {
		if (!state->loadGame(saveArchiveLocation))
		{
			LogError("Failed to load '%s'", saveArchiveLocation);
			return;
		}
		state->initState();
		return;
	});

	return loadTask;
}

std::shared_future<void> SaveManager::loadSpecialSave(const SaveType type,
                                                      sp<GameState> state) const
{
	if (type == SaveType::Manual)
	{
		LogError("Cannot load automatic save for type %i", static_cast<int>(type));
		return std::async(std::launch::deferred, []() -> void { return; });
	}

	UString saveName;

	try
	{
		saveName = saveTypeNames.at(type);
	}
	catch (std::out_of_range &)
	{
		LogError("Cannot find name of save type %i", static_cast<int>(type));
		return std::async(std::launch::deferred, []() -> void { return; });
	}

	return loadGame(createSavePath(saveName), state);
}

bool writeArchiveWithBackup(SerializationArchive *archive, const UString &path, bool pack)
{
	fs::path savePath = path;
	fs::path tempPath;
	bool shouldCleanup = false;
	try
	{
		if (!fs::exists(savePath))
		{
			return archive->write(path, pack);
		}

		// WARNING! Dragons live here! Specifically dragon named miniz who hates windows paths
		// (or paths not starting with dot)
		// therefore I'm doing gymnastics here to backup and still pass original path string to
		// archive write
		// that is really bad, because if user clicks exit, save will be renamed to some random
		// junk
		// however it will still function as regular save file, so maybe not that bad?
		fs::path saveDirectory = savePath.parent_path();
		bool haveNewName = false;
		for (int retries = 5; retries > 0; retries--)
		{
			{
				tempPath = saveDirectory /
				           (boost::uuids::to_string(uuids::random_generator()()) + ".save");
				if (!fs::exists(tempPath))
				{
					haveNewName = true;
					break;
				}
			}
		}

		if (!haveNewName)
		{
			LogError("Unable to create temporary file at \"%s\"", tempPath.string());
			return false;
		}

		fs::rename(savePath, tempPath);
		shouldCleanup = true;
		bool saveSuccess = archive->write(path, pack);
		shouldCleanup = false;

		if (saveSuccess)
		{
			fs::remove_all(tempPath);
			return true;
		}
		else
		{
			if (fs::exists(savePath))
			{
				fs::remove_all(savePath);
			}

			fs::rename(tempPath, savePath);
		}
	}
	catch (fs::filesystem_error &exception)
	{
		if (shouldCleanup)
		{
			if (fs::exists(savePath))
			{
				fs::remove_all(savePath);
			}
			fs::rename(tempPath, savePath);
		}

		LogError("Unable to save game: \"%s\"", exception.what());
	}

	return false;
}

bool SaveManager::findFreePath(UString &path, const UString &name) const
{
	path = createSavePath("save_" + name);
	if (fs::exists(path))
	{
		for (int retries = 5; retries > 0; retries--)
		{
			path = createSavePath("save_" + name + std::to_string(rand()));
			if (!fs::exists(path))
			{
				return true;
			}
		}

		LogError("Unable to generate filename for save %s", name);
		return false;
	}

	return true;
}

bool SaveManager::newSaveGame(const UString &name, const sp<GameState> gameState) const
{
	UString path;
	if (!findFreePath(path, name))
	{
		return false;
	}

	SaveMetadata manifest(name, path, time(nullptr), SaveType::Manual, gameState);
	return saveGame(manifest, gameState);
}

bool SaveManager::overrideGame(const SaveMetadata &metadata, const UString &newName,
                               const sp<GameState> gameState) const
{
	SaveMetadata updatedMetadata(newName, metadata.getFile(), time(nullptr), metadata.getType(),
	                             gameState);
	bool result = saveGame(updatedMetadata, gameState);
	if (result && newName != metadata.getName())
	{
		// if renamed file move to path with new name
		UString newFile;
		if (findFreePath(newFile, newName))
		{
			try
			{
				fs::rename(metadata.getFile(), newFile);
			}
			catch (fs::filesystem_error &error)
			{
				LogWarning("Error while removing renamed save: \"%s\"", error.what());
			}
		}
	}

	return result;
}

bool SaveManager::saveGame(const SaveMetadata &metadata, const sp<GameState> gameState) const
{
	bool pack = Options::packSaveOption.get();
	const UString path = metadata.getFile();
	auto archive = SerializationArchive::createArchive();
	if (gameState->serialize(archive.get()) && metadata.serializeManifest(archive.get()))
	{
		return writeArchiveWithBackup(archive.get(), path, pack);
	}

	return false;
}

bool SaveManager::specialSaveGame(SaveType type, const sp<GameState> gameState) const
{
	if (type == SaveType::Manual)
	{
		LogError("Cannot create automatic save for type %i", static_cast<int>(type));
		return false;
	}

	UString saveName;
	try
	{
		saveName = saveTypeNames.at(type);
	}
	catch (std::out_of_range &)
	{
		LogError("Cannot find name of save type %i", static_cast<int>(type));
		return false;
	}

	SaveMetadata manifest(saveName, createSavePath(saveName), time(nullptr), type, gameState);
	return saveGame(manifest, gameState);
}

std::vector<SaveMetadata> SaveManager::getSaveList() const
{
	auto dirString = Options::saveDirOption.get();
	fs::path saveDirectory = dirString;
	std::vector<SaveMetadata> saveList;
	try
	{
		if (!fs::exists(saveDirectory) && !fs::create_directories(saveDirectory))
		{
			LogWarning("Save directory \"%s\" not found, and could not be created!", saveDirectory);
			return saveList;
		}

		for (auto i = fs::directory_iterator(saveDirectory); i != fs::directory_iterator(); ++i)
		{
			if (i->path().extension().string() != saveFileExtension)
			{
				continue;
			}

			std::string saveFileName = i->path().filename().string();
			// miniz can't read paths not starting with dir or with windows slashes
			UString savePath = saveDirectory.string() + "/" + saveFileName;
			if (auto archive = SerializationArchive::readArchive(savePath))
			{
				SaveMetadata metadata;
				if (metadata.deserializeManifest(archive.get(), savePath))
				{
					saveList.push_back(metadata);
				}
				else // accept saves with missing manifest if extension is correct
				{
					saveList.push_back(SaveMetadata("Unknown(Missing manifest)", savePath, 0,
					                                SaveType::Manual, nullptr));
				}
			}
		}
	}
	catch (fs::filesystem_error &er)
	{
		LogError("Error while enumerating directory: \"%s\"", er.what());
	}

	sort(saveList.begin(), saveList.end(), [](const SaveMetadata &lhs, const SaveMetadata &rhs) {
		return lhs.getCreationDate() > rhs.getCreationDate();
	});

	return saveList;
}

bool SaveManager::deleteGame(const sp<SaveMetadata> &slot) const
{
	try
	{
		if (!fs::exists(slot->getFile()))
		{
			LogWarning("Attempt to delete not existing file");
			return false;
		}

		fs::remove_all(slot->getFile());
		return true;
	}
	catch (fs::filesystem_error &exception)
	{
		LogError("Unable to delete saved gane: \"%s\"", exception.what());
		return false;
	}
}

bool SaveMetadata::deserializeManifest(SerializationArchive *archive, const UString &saveFileName)
{
	auto root = archive->getRoot("", saveManifestName.c_str());
	if (!root)
	{
		return false;
	}

	auto nameNode = root->getNodeOpt("name");
	if (!nameNode)
	{
		return false;
	}
	this->name = nameNode->getValue();

	auto difficultyNode = root->getNodeOpt("difficulty");
	if (difficultyNode)
	{
		this->difficulty = difficultyNode->getValue();
	}

	auto saveDateNode = root->getNodeOpt("save_date");
	if (saveDateNode)
	{
		std::istringstream stream(saveDateNode->getValue());
		time_t timestamp;
		stream >> timestamp;
		this->creationDate = timestamp;
	}

	auto gameTicksNode = root->getNodeOpt("game_ticks");
	if (gameTicksNode)
	{
		gameTicks = gameTicksNode->getValueUInt();
	}

	auto typeNode = root->getNodeOpt("type");
	if (typeNode)
	{
		this->type = static_cast<SaveType>(Strings::toInteger(typeNode->getValue()));
	}
	else
	{
		this->type = SaveType::Manual;
	}

	this->file = saveFileName;
	return true;
}

bool SaveMetadata::serializeManifest(SerializationArchive *archive) const
{
	auto root = archive->newRoot("", saveManifestName.c_str());
	if (!root)
	{
		return false;
	}
	auto nameNode = root->addNode("name");
	nameNode->setValue(this->getName());

	auto difficultyNode = root->addNode("difficulty");
	difficultyNode->setValue(this->getDifficulty());

	auto savedateNode = root->addNode("save_date");
	savedateNode->setValue(std::to_string(time(nullptr)));

	auto gameTicksNode = root->addNode("game_ticks");
	gameTicksNode->setValue(std::to_string(getGameTicks()));

	auto versionNode = root->addNode("game_version");
	versionNode->setValue(OPENAPOC_VERSION);

	if (this->type != SaveType::Manual)
	{
		auto typeNode = root->addNode("type");
		typeNode->setValue(Strings::fromInteger(static_cast<unsigned>(this->type)));
	}

	return true;
}

time_t SaveMetadata::getCreationDate() const { return creationDate; }

SaveMetadata::SaveMetadata() : creationDate(0), type(), gameTicks(0){};
SaveMetadata::~SaveMetadata() = default;
;
SaveMetadata::SaveMetadata(UString name, UString file, time_t creationDate, SaveType type,
                           const sp<GameState> gameState)
    : name(name), file(file), creationDate(creationDate), type(type)
{
	if (gameState)
	{
		gameTicks = gameState->gameTime.getTicks();
		// this->difficulty = gameState->difficulty; ?
	}
}
SaveMetadata::SaveMetadata(const SaveMetadata &metdata, time_t creationDate,
                           const sp<GameState> gameState)
    : name(metdata.name), file(metdata.file), creationDate(creationDate), type(metdata.type)
{
	if (gameState)
	{
		gameTicks = gameState->gameTime.getTicks();
		// this->difficulty = gameState->difficulty; ?
	}
}
const UString &SaveMetadata::getName() const { return name; }
const UString &SaveMetadata::getFile() const { return file; }
const UString &SaveMetadata::getDifficulty() const { return difficulty; }
const SaveType &SaveMetadata::getType() const { return type; }
uint64_t SaveMetadata::getGameTicks() const { return gameTicks; }
} // namespace OpenApoc
