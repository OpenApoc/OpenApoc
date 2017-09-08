#pragma once

#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{

class ConfigFileImpl;

class ConfigFile
{
  private:
	up<ConfigFileImpl> pimpl;
	ConfigFile();

  public:
	~ConfigFile();
	bool save();

	// Returns true if the settings have been read
	bool loaded() const;

	int getInt(const UString key);
	bool getBool(const UString key);
	UString getString(const UString key);
	bool get(const UString key); // returns true if the option was specified

	void set(const UString key, const bool value);
	void set(const UString key, const int value);
	void set(const UString key, const UString value);

	void addOptionString(const UString section, const UString longName, const UString shortName,
	                     const UString description, const UString defaultValue);
	void addOptionInt(const UString section, const UString longName, const UString shortName,
	                  const UString description, const int defaultValue);
	void addOptionBool(const UString section, const UString longName, const UString shortName,
	                   const UString description, const bool defaultValue);
	void addOption(const UString section, const UString longName, const UString shortName,
	               const UString description);
	void addPositionalArgument(const UString name, const UString description);

	// returns 'true' if the program should exit (invalid option/'--help' specified)
	bool parseOptions(int argc, char *argv[]);

	// Prints out the help to stdout, used if the running program has decided some argument is
	// invalid
	void showHelp();

	static ConfigFile &getInstance();
};

class ConfigOptionString
{
  private:
	UString section;
	UString name;
	UString description;
	UString defaultValue;

  public:
	ConfigOptionString(const UString section, const UString name, const UString description,
	                   const UString defaultValue = "");
	UString get() const;
};

class ConfigOptionInt
{
  private:
	UString section;
	UString name;
	UString description;
	int defaultValue;

  public:
	ConfigOptionInt(const UString section, const UString name, const UString description,
	                const int defaultValue = 0);
	int get() const;
};

class ConfigOptionBool
{
  private:
	UString section;
	UString name;
	UString description;
	bool defaultValue;

  public:
	ConfigOptionBool(const UString section, const UString name, const UString description,
	                 const bool defaultValue = false);
	bool get() const;
};
static inline ConfigFile &config() { return ConfigFile::getInstance(); }

}; // namespace OpenApoc
