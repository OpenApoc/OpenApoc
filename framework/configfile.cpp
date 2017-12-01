#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/logger.h"
#include <fstream>
#include <iostream>
#include <list>
#include <physfs.h>
#include <string>

// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace OpenApoc
{

static ConfigFile *configInstance = nullptr;

ConfigFile &ConfigFile::getInstance()
{
	if (!configInstance)
		configInstance = new ConfigFile();
	return *configInstance;
}

class ConfigFileImpl
{
  private:
	std::map<UString, po::options_description> optionSections;
	po::variables_map vm;
	po::positional_options_description posDesc;
	bool parsed;
	std::vector<UString> positionalArgNames;
	UString programName;

	std::map<UString, UString> modifiedOptions;

  public:
	ConfigFileImpl() : parsed(false), programName("program") {}

	void createSection(const UString sectionName)
	{
		if (optionSections.find(sectionName) != optionSections.end())
			return;
		UString sectionDescription = sectionName + " options";
		optionSections.emplace(sectionName, sectionDescription.str());
	}

	void set(const UString key, const UString value) { this->modifiedOptions[key] = value; }
	void set(const UString key, const int value)
	{
		this->modifiedOptions[key] = Strings::fromInteger(value);
	}

	void set(const UString key, const bool value)
	{
		this->modifiedOptions[key] = value ? "1" : "0";
	}

	bool parseOptions(int argc, char *argv[])
	{
		if (this->parsed)
		{
			LogError("Already parsed options");
			return true;
		}
		fs::path programPath(argv[0]);
		// Remove extension (if any) and path
		programName = programPath.filename().string();
		if (programName.endsWith(".exe"))
		{
			programName = programName.substr(0, programName.length() - 4);
		}
		if (!PHYSFS_isInit())
		{
			PHYSFS_init(programName.cStr());
		}
		UString settingsPath;
		// If a file called 'portable.txt' exists in $(PWD), use a local config folder instead of a
		// system one.
		// This can't go through the normal settings system, as it's used by the normal settings
		// system...
		std::ifstream portableFile("./portable.txt");
		if (portableFile)
		{
			LogInfo("portable mode set");
			settingsPath = programName + "_";
		}
		else
		{
			settingsPath = PHYSFS_getPrefDir("OpenApoc", programName.cStr());
		}
		settingsPath += "settings.conf";
		// Setup some config-related options
		this->addOption("", "help", "h", "Show help text and exit");
		this->addOptionString("Config", "File", "", "Path to config file", settingsPath);
		this->addOptionBool("Config", "Read", "", "Read the config file at startup", true);
		this->addOptionBool("Config", "Save", "", "Save the config file at exit", true);

		po::options_description allOptions;
		for (auto &optPair : this->optionSections)
			allOptions.add(optPair.second);

		try
		{
			po::store(
			    po::command_line_parser(argc, argv).positional(posDesc).options(allOptions).run(),
			    vm);
		}
		catch (po::error &err)
		{
			std::cerr << "Failed to parse options: \"" << err.what() << "\"\n";
			this->showHelp();
			return true;
		}
		po::notify(vm);
		this->parsed = true;

		if (this->getBool("Config.Read"))
		{
			// Keep a variable map of 'only' those settings stored in the config file so we can
			// write them back to the config again at save()
			po::variables_map configFileVM;
			auto configPath = this->getString("Config.File");
			std::ifstream inConfig(configPath.str());
			if (inConfig)
			{
				try
				{
					auto parsedConfig = parse_config_file(inConfig, allOptions);
					po::store(parsedConfig, vm);
					for (auto &option : parsedConfig.options)
					{
						// FIXME: Options with multiple values would break this
						this->modifiedOptions[option.string_key] = option.value[0];
					}
				}
				catch (po::error &err)
				{
					std::cerr << "Failed to parse config options: \"" << err.what() << "\"\n";
					this->showHelp();
					return true;
				}
			}
		}

		if (this->get("help"))
		{
			this->showHelp();
			return true;
		}

		return false;
	}

	bool loaded() const { return this->parsed; }

	bool save()
	{
		// Don't try to save if we've not successfully loaded anything
		if (!this->parsed)
			return false;
		auto configPathString = this->getString("Config.File");

		if (this->modifiedOptions.empty())
		{
			// nothing to do
			return true;
		}

		std::map<UString, std::list<UString>> configFileContents;

		for (auto &optionPair : this->modifiedOptions)
		{
			auto splitString = optionPair.first.split(".");
			if (splitString.size() < 1)
			{
				LogError("Invalid option string \"%s\"", optionPair.first.cStr());
				continue;
			}
			UString sectionName;
			for (unsigned i = 0; i < splitString.size() - 1; i++)
			{
				if (i != 0)
					sectionName += ".";
				sectionName += splitString[i];
			}

			UString optionName = splitString[splitString.size() - 1];
			UString configFileLine = optionName + "=" + optionPair.second;
			configFileContents[sectionName].push_back(configFileLine);
		}

		try
		{
			std::string str = configPathString.str();
			fs::path configPath(str);
			auto dir = configPath.parent_path();
			if (!dir.empty())
				fs::create_directories(dir);
			std::ofstream outFile(configPath.string());
			if (!outFile)
			{
				std::cerr << "Failed to open config file \"" << configPath << "\" for writing\n";
				return false;
			}
			for (auto &section : configFileContents)
			{
				auto &sectionName = section.first;
				outFile << "[" << sectionName.str() << "]\n";
				for (auto &line : section.second)
				{
					outFile << line << "\n";
				}
			}
		}
		catch (fs::filesystem_error &e)
		{
			std::cerr << "Failed to write config file \"" << configPathString.str() << "\" : \""
			          << e.what() << "\"\n";
			return false;
		}

		return true;
	}

	bool get(const UString name)
	{
		if (!this->parsed)
		{
			LogError("Not yet parsed options");
			return false;
		}
		auto it = this->modifiedOptions.find(name);
		if (it != this->modifiedOptions.end())
		{
			return true;
		}
		return vm.count(name.str());
	}
	UString getString(const UString key)
	{
		if (!this->parsed)
		{
			LogError("Not yet parsed options");
			return "";
		}
		if (!this->get(key))
		{
			LogError("Option \"%s\" not set", key.cStr());
			return "";
		}
		auto it = this->modifiedOptions.find(key);
		if (it != this->modifiedOptions.end())
		{
			return it->second;
		}
		return vm[key.str()].as<std::string>();
	}
	int getInt(const UString key)
	{
		if (!this->parsed)
		{
			LogError("Not yet parsed options");
			return 0;
		}
		if (!this->get(key))
		{
			LogError("Option \"%s\" not set", key.cStr());
			return 0;
		}
		auto it = this->modifiedOptions.find(key);
		if (it != this->modifiedOptions.end())
		{
			return Strings::toInteger(it->second);
		}
		return vm[key.str()].as<int>();
	}
	bool getBool(const UString key)
	{
		if (!this->parsed)
		{
			LogError("Not yet parsed options");
			return false;
		}
		if (!this->get(key))
		{
			LogError("Option \"%s\" not set", key.cStr());
			return false;
		}
		auto it = this->modifiedOptions.find(key);
		if (it != this->modifiedOptions.end())
		{
			return it->second != "0";
		}
		return vm[key.str()].as<bool>();
	}

	void addOption(const UString section, const UString longName, const UString shortName,
	               const UString description)
	{
		if (this->parsed)
		{
			LogError("Adding option when already parsed");
		}
		this->createSection(section);
		UString combinedOption;
		if (section.empty())
			combinedOption = longName;
		else
			combinedOption = section + "." + longName;
		if (!shortName.empty())
			combinedOption += "," + shortName;
		this->optionSections[section].add_options()(combinedOption.cStr(), description.cStr());
	}
	void addOptionString(const UString section, const UString longName, const UString shortName,
	                     const UString description, const UString defaultValue)
	{
		if (this->parsed)
		{
			LogError("Adding option when already parsed");
		}
		this->createSection(section);
		UString combinedOption;
		if (section.empty())
			combinedOption = longName;
		else
			combinedOption = section + "." + longName;
		if (!shortName.empty())
			combinedOption += "," + shortName;
		this->optionSections[section].add_options()(
		    combinedOption.cStr(), po::value<std::string>()->default_value(defaultValue.str()),
		    description.cStr());
	}
	void addOptionInt(const UString section, const UString longName, const UString shortName,
	                  const UString description, const int defaultValue)
	{
		if (this->parsed)
		{
			LogError("Adding option when already parsed");
		}
		this->createSection(section);
		UString combinedOption;
		if (section.empty())
			combinedOption = longName;
		else
			combinedOption = section + "." + longName;
		if (!shortName.empty())
			combinedOption += "," + shortName;
		this->optionSections[section].add_options()(combinedOption.cStr(),
		                                            po::value<int>()->default_value(defaultValue),
		                                            description.cStr());
	}
	void addOptionBool(const UString section, const UString longName, const UString shortName,
	                   const UString description, const bool defaultValue)
	{
		if (this->parsed)
		{
			LogError("Adding option when already parsed");
		}
		this->createSection(section);
		UString combinedOption;
		if (section.empty())
			combinedOption = longName;
		else
			combinedOption = section + "." + longName;
		if (!shortName.empty())
			combinedOption += "," + shortName;
		this->optionSections[section].add_options()(combinedOption.cStr(),
		                                            po::value<bool>()->default_value(defaultValue),
		                                            description.cStr());
	}
	void addPositionalArgument(const UString name, const UString description)
	{
		if (this->parsed)
		{
			LogError("Adding option when already parsed");
		}
		this->positionalArgNames.push_back(name);
		this->posDesc.add(name.cStr(), 1);
		this->addOptionString("", name, "", description, "");
	}

	void showHelp()
	{
		std::cout << "Usage: " << this->programName << " [options]";
		for (auto &arg : positionalArgNames)
			std::cout << " " << arg;
		std::cout << "\n";
		for (auto &optPair : this->optionSections)
			std::cout << optPair.second << "\n";
	}
};

ConfigFile::ConfigFile() { this->pimpl.reset(new ConfigFileImpl()); }
ConfigFile::~ConfigFile() = default;

bool ConfigFile::save() { return this->pimpl->save(); }

UString ConfigFile::getString(const UString key) { return this->pimpl->getString(key); }

int ConfigFile::getInt(const UString key) { return this->pimpl->getInt(key); }

bool ConfigFile::getBool(const UString key) { return this->pimpl->getBool(key); }
bool ConfigFile::get(const UString key) { return this->pimpl->get(key); }

void ConfigFile::set(const UString key, const UString value) { this->pimpl->set(key, value); }
void ConfigFile::set(const UString key, const int value) { this->pimpl->set(key, value); }
void ConfigFile::set(const UString key, const bool value) { this->pimpl->set(key, value); }

void ConfigFile::addOptionString(const UString section, const UString longName,
                                 const UString shortName, const UString description,
                                 const UString defaultValue)
{
	this->pimpl->addOptionString(section, longName, shortName, description, defaultValue);
}
void ConfigFile::addOptionInt(const UString section, const UString longName,
                              const UString shortName, const UString description,
                              const int defaultValue)
{
	this->pimpl->addOptionInt(section, longName, shortName, description, defaultValue);
}
void ConfigFile::addOptionBool(const UString section, const UString longName,
                               const UString shortName, const UString description,
                               const bool defaultValue)
{
	this->pimpl->addOptionBool(section, longName, shortName, description, defaultValue);
}

void ConfigFile::addOption(const UString section, const UString longName, const UString shortName,
                           const UString description)
{
	this->pimpl->addOption(section, longName, shortName, description);
}

void ConfigFile::addPositionalArgument(const UString name, const UString description)
{
	this->pimpl->addPositionalArgument(name, description);
}

bool ConfigFile::parseOptions(int argc, char *argv[])
{
	return this->pimpl->parseOptions(argc, argv);
}

bool ConfigFile::loaded() const { return this->pimpl->loaded(); }

void ConfigFile::showHelp() { this->pimpl->showHelp(); }

ConfigOptionString::ConfigOptionString(const UString section, const UString name,
                                       const UString description, const UString defaultValue)
    : section(section), name(name), description(description), defaultValue(defaultValue)
{
	config().addOptionString(section, name, "", description, defaultValue);
}

UString ConfigOptionString::get() const
{
	if (section.empty())
		return config().getString(name);
	else
		return config().getString(section + "." + name);
}

ConfigOptionInt::ConfigOptionInt(const UString section, const UString name,
                                 const UString description, const int defaultValue)
    : section(section), name(name), description(description), defaultValue(defaultValue)
{
	config().addOptionInt(section, name, "", description, defaultValue);
}

int ConfigOptionInt::get() const
{

	if (section.empty())
		return config().getInt(name);
	else
		return config().getInt(section + "." + name);
}

ConfigOptionBool::ConfigOptionBool(const UString section, const UString name,
                                   const UString description, const bool defaultValue)
    : section(section), name(name), description(description), defaultValue(defaultValue)
{
	config().addOptionBool(section, name, "", description, defaultValue);
}

bool ConfigOptionBool::get() const
{
	if (section.empty())
		return config().getBool(name);
	else

		return config().getBool(section + "." + name);
}
}; // namespace OpenApoc
