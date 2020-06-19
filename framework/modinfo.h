#pragma once

#include "library/strings.h"
#include <optional>

namespace OpenApoc
{
class ModInfo
{
  private:
	UString name;
	UString author;
	UString version;
	UString description;
	UString link;
	UString ID;
	std::list<UString> _requirements;
	std::list<UString> _conflicts;
	UString dataPath;
	UString statePath;
	UString minVersion;
	UString modLoadScript;

  public:
	// The user-visible name of the mod
	const UString &getName() const { return name; }
	void setName(const UString &newName) { name = newName; }
	// The author(s)
	const UString &getAuthor() const { return author; }
	void setAuthor(const UString &newAuthor) { author = newAuthor; }
	// The mod version (Using a string to allow any version scheme - this is just user info, we
	// don't enfore any versioning)
	const UString &getVersion() const { return version; }
	void setVersion(const UString &newVersion) { version = newVersion; }
	// A "short" description
	const UString &getDescription() const { return description; }
	void setDescription(const UString &newDescription) { description = newDescription; }
	// A link - likely to a website?
	const UString &getLink() const { return link; }
	void setLink(const UString &newLink) { link = newLink; }
	// A /UNIQUE/ ID - how this mod is referenced internally.
	// SUGGESTION: Use a reverse DNS-like format: e.g."
	// org.openapoc.mod.MY_USERNAME.MY_MOD
	// as your username is unique on the site, you know it'll never confict (unless someone does
	// something dumb)
	const UString &getID() const { return ID; }
	void setID(const UString &newID) { ID = newID; }
	// A list of IDs this mod depends on
	const std::list<UString> &requirements() const { return _requirements; }
	std::list<UString> requirements() { return _requirements; }
	// A list of IDs this mod is known to not work with
	const std::list<UString> &conflicts() const { return _conflicts; }
	std::list<UString> conflicts() { return _conflicts; }

	// A path (relative to this mod's directory) to load a GameState from
	const UString &getStatePath() const { return statePath; }
	void setStatePath(const UString &newPath) { statePath = newPath; }

	// A path (relative to this mod's directory) to append to the data list (note, files here
	// override any with the same name earlier in the mod load order - so it's a good idea to put
	// everything under a folder named the same as the ID to avoid conflicts)
	const UString &getDataPath() const { return dataPath; }
	void setDataPath(const UString &newPath) { dataPath = newPath; }

	// The minimum version of OpenApoc this mod needs to run
	const UString &getMinVersion() const { return minVersion; }
	void setMinVersion(const UString &newVersion) { minVersion = newVersion; }

	static std::optional<ModInfo> getInfo(const UString &path);
	bool writeInfo(const UString &path);

	const UString &getModLoadScript() const { return modLoadScript; }
	void setModLoadScript(const UString &newScript) { modLoadScript = newScript; }
};
} // namespace OpenApoc
