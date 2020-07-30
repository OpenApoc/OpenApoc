#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/logger.h"
#include "framework/options.h"
#include <fstream>
#include <iostream>
#include <list>
#include <physfs.h>
#include <string>
#include <variant>

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

namespace
{

template <typename Variant, size_t I = std::variant_size<Variant>::value - 1> struct any_to_variant
{
	static Variant convert(const boost::any &opt)
	{
		if (opt.type() == typeid(typename std::variant_alternative<I, Variant>::type))
			return boost::any_cast<typename std::variant_alternative<I, Variant>::type>(opt);
		else
			return any_to_variant<Variant, I - 1>::convert(opt);
	}
};
template <typename Variant> struct any_to_variant<Variant, 0>
{
	static Variant convert(const boost::any &opt)
	{
		return boost::any_cast<typename std::variant_alternative<0, Variant>::type>(opt);
	}
};
} // namespace

class ConfigFileImpl
{
  private:
	std::map<UString, po::options_description> optionSections;
	po::variables_map vm;
	po::positional_options_description posDesc;
	bool parsed;
	std::vector<UString> positionalArgNames;
	UString programName;

	using ModifiedOptionT = std::variant<int, float, bool, UString>;
	std::map<UString, ModifiedOptionT> modifiedOptions;

	struct ToStringVisitor
	{
		using result_type = UString;
		UString operator()(const int &operand) const { return Strings::fromInteger(operand); }
		UString operator()(const float &operand) const { return Strings::fromFloat(operand); }
		UString operator()(const bool &operand) const { return operand ? "1" : "0"; }
		UString operator()(const UString &str) const { return str; }
	};

  public:
	ConfigFileImpl() : parsed(false), programName("program") {}

	void createSection(const UString sectionName)
	{
		if (optionSections.find(sectionName) != optionSections.end())
			return;
		UString sectionDescription = sectionName + " options";
		optionSections.emplace(sectionName, sectionDescription);
	}

	template <typename T> void set(const UString &key, const T value)
	{
		this->modifiedOptions[key] = value;
	}

	bool parseOptions(int argc, const char *const argv[])
	{
		if (this->parsed)
		{
			LogError("Already parsed options");
			return true;
		}
		fs::path programPath(argv[0]);
		// Remove extension (if any) and path
		programName = programPath.filename().string();
		if (ends_with(programName, ".exe"))
		{
			programName = programName.substr(0, programName.length() - 4);
		}
		if (!PHYSFS_isInit())
		{
			PHYSFS_init(programName.c_str());
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
			settingsPath = PHYSFS_getPrefDir("OpenApoc", programName.c_str());
		}
		settingsPath += "settings.conf";
		// Setup some config-related options
		this->addOption("", "help", "h", "Show help text and exit");
		this->addOptionTyped<UString>("Config", "File", "", "Path to config file", settingsPath);
		this->addOptionTyped<bool>("Config", "Read", "", "Read the config file at startup", true);
		this->addOptionTyped<bool>("Config", "Save", "", "Save the config file at exit", true);

		po::options_description allOptions;
		for (auto &optPair : this->optionSections)
			allOptions.add(optPair.second);

		try
		{
			auto parsed = po::command_line_parser(argc, argv)
			                  .positional(posDesc)
			                  .options(allOptions)
			                  .allow_unregistered()
			                  .run();
			po::store(parsed, vm);
			auto unknown_options = po::collect_unrecognized(parsed.options, po::include_positional);
			for (const auto &unknown : unknown_options)
			{
				LogWarning("Ignoring option \"%s\"", unknown);
			}
		}
		catch (po::error &err)
		{
			std::cerr << "Failed to parse options: \"" << err.what() << "\"\n";
			this->showHelp();
			return true;
		}
		po::notify(vm);
		this->parsed = true;

		if (this->getTyped<bool>("Config.Read"))
		{
			// Keep a variable map of 'only' those settings stored in the config file so we can
			// write them back to the config again at save()
			po::variables_map configFileVM;
			auto configPath = this->getTyped<UString>("Config.File");
			std::ifstream inConfig(configPath);
			if (inConfig)
			{
				try
				{
					auto parsedConfig = parse_config_file(inConfig, allOptions);
					po::store(parsedConfig, vm);
					for (auto &option : parsedConfig.options)
					{
						// FIXME: Options with multiple values would break this
						this->modifiedOptions[option.string_key] =
						    any_to_variant<ModifiedOptionT>::convert(vm[option.string_key].value());
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
		auto configPathString = this->getTyped<UString>("Config.File");

		if (this->modifiedOptions.empty())
		{
			// nothing to do
			return true;
		}

		std::map<UString, std::list<UString>> configFileContents;

		for (auto &optionPair : this->modifiedOptions)
		{
			auto splitString = split(optionPair.first, ".");
			if (splitString.size() < 1)
			{
				LogError("Invalid option string \"%s\"", optionPair.first);
				continue;
			}
			UString sectionName;
			for (unsigned i = 0; i < splitString.size() - 1; i++)
			{
				if (i != 0)
					sectionName += ".";
				sectionName += splitString[i];
			}

			auto optionName = splitString[splitString.size() - 1];
			UString configFileLine =
			    format("%s=%s", optionName, std::visit(ToStringVisitor(), optionPair.second));
			configFileContents[sectionName].push_back(configFileLine);
		}

		try
		{
			fs::path configPath(configPathString);
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
				outFile << "[" << sectionName << "]\n";
				for (auto &line : section.second)
				{
					outFile << line << "\n";
				}
			}
		}
		catch (fs::filesystem_error &e)
		{
			std::cerr << "Failed to write config file \"" << configPathString << "\" : \""
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
		return vm.count(name);
	}
	template <typename T> const T &getTyped(const UString &key)
	{
		if (!this->parsed)
		{
			LogError("Not yet parsed options");
			throw std::exception();
		}
		if (!this->get(key))
		{
			LogError("Option \"%s\" not set", key);
			throw std::exception();
		}
		auto it = this->modifiedOptions.find(key);
		if (it != this->modifiedOptions.end())
		{
			return std::get<T>(it->second);
		}
		return vm[key].as<T>();
	}

	UString describe(const UString section, const UString name)
	{
		if (!this->parsed)
		{
			LogError("Not yet parsed options");
			return "";
		}
		UString combinedOption;
		if (section.empty())
			combinedOption = name;
		else
			combinedOption = section + "." + name;
		return optionSections[section].find(combinedOption, true).description();
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
		this->optionSections[section].add_options()(combinedOption.c_str(), description.c_str());
	}
	template <typename T>
	void addOptionTyped(const UString section, const UString longName, const UString shortName,
	                    const UString description, const T defaultValue)
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
		this->optionSections[section].add_options()(combinedOption.c_str(),
		                                            po::value<T>()->default_value(defaultValue),
		                                            description.c_str());
	}
	void addPositionalArgument(const UString name, const UString description)
	{
		if (this->parsed)
		{
			LogError("Adding option when already parsed");
		}
		this->positionalArgNames.push_back(name);
		this->posDesc.add(name.c_str(), 1);
		this->addOptionTyped<UString>("", name, "", description, "");
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

	std::map<UString, std::vector<ConfigOption>> getOptions()
	{
		std::map<UString, std::vector<ConfigOption>> options;
		for (auto &optPair : this->optionSections)
		{
			std::vector<ConfigOption> vec;
			for (auto &opt : optPair.second.options())
			{
				std::string short_name = opt->long_name();
				size_t dot = short_name.find_last_of('.');
				if (dot != std::string::npos)
				{
					short_name = short_name.substr(dot + 1);
				}
				vec.emplace_back(ConfigOption(optPair.first, short_name, opt->description()));
			}
			options[optPair.first] = vec;
		}
		return options;
	}
};

ConfigFile::ConfigFile() { this->pimpl.reset(new ConfigFileImpl()); }
ConfigFile::~ConfigFile() = default;

bool ConfigFile::save() { return this->pimpl->save(); }

UString ConfigFile::getString(const UString &key) { return this->pimpl->getTyped<UString>(key); }
int ConfigFile::getInt(const UString &key) { return this->pimpl->getTyped<int>(key); }
bool ConfigFile::getBool(const UString &key) { return this->pimpl->getTyped<bool>(key); }
float ConfigFile::getFloat(const UString &key) { return this->pimpl->getTyped<float>(key); }
bool ConfigFile::get(const UString &key) { return this->pimpl->get(key); }

UString ConfigFile::describe(const UString &section, const UString &name)
{
	return this->pimpl->describe(section, name);
}

void ConfigFile::set(const UString &key, const UString value) { this->pimpl->set(key, value); }
void ConfigFile::set(const UString &key, const int value) { this->pimpl->set(key, value); }
void ConfigFile::set(const UString &key, const bool value) { this->pimpl->set(key, value); }
void ConfigFile::set(const UString &key, const float value) { this->pimpl->set(key, value); }

void ConfigFile::addOptionString(const UString section, const UString longName,
                                 const UString shortName, const UString description,
                                 const UString defaultValue)
{
	this->pimpl->addOptionTyped<UString>(section, longName, shortName, description, defaultValue);
}
void ConfigFile::addOptionInt(const UString section, const UString longName,
                              const UString shortName, const UString description,
                              const int defaultValue)
{
	this->pimpl->addOptionTyped<int>(section, longName, shortName, description, defaultValue);
}
void ConfigFile::addOptionBool(const UString section, const UString longName,
                               const UString shortName, const UString description,
                               const bool defaultValue)
{
	this->pimpl->addOptionTyped<bool>(section, longName, shortName, description, defaultValue);
}
void ConfigFile::addOptionFloat(const UString section, const UString longName,
                                const UString shortName, const UString description,
                                const float defaultValue)
{
	this->pimpl->addOptionTyped<float>(section, longName, shortName, description, defaultValue);
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

bool ConfigFile::parseOptions(int argc, const char *const argv[])
{
	return this->pimpl->parseOptions(argc, argv);
}

bool ConfigFile::loaded() const { return this->pimpl->loaded(); }

void ConfigFile::showHelp() { this->pimpl->showHelp(); }

std::map<UString, std::vector<ConfigOption>> ConfigFile::getOptions()
{
	return this->pimpl->getOptions();
}

ConfigOption::ConfigOption(const UString section, const UString name, const UString description)
    : section(section), name(name), description(description)
{
}

UString ConfigOption::getKey() const
{
	if (section.empty())
		return name;
	else
		return section + "." + name;
}

ConfigOptionString::ConfigOptionString(const UString section, const UString name,
                                       const UString description, const UString defaultValue)
    : ConfigOption(section, name, description)
{
	config().addOptionString(section, name, "", description, defaultValue);
}

UString ConfigOptionString::get() const { return config().getString(getKey()); }

void ConfigOptionString::set(const UString &newValue) { config().set(getKey(), newValue); }

ConfigOptionInt::ConfigOptionInt(const UString section, const UString name,
                                 const UString description, const int defaultValue)
    : ConfigOption(section, name, description)
{
	config().addOptionInt(section, name, "", description, defaultValue);
}

int ConfigOptionInt::get() const { return config().getInt(getKey()); }

void ConfigOptionInt::set(int newValue) { config().set(getKey(), newValue); }

ConfigOptionBool::ConfigOptionBool(const UString section, const UString name,
                                   const UString description, const bool defaultValue)
    : ConfigOption(section, name, description)
{
	config().addOptionBool(section, name, "", description, defaultValue);
}

bool ConfigOptionBool::get() const { return config().getBool(getKey()); }
void ConfigOptionBool::set(bool newValue) { config().set(getKey(), newValue); }

ConfigOptionFloat::ConfigOptionFloat(const UString section, const UString name,
                                     const UString description, const float defaultValue)
    : ConfigOption(section, name, description)
{
	config().addOptionFloat(section, name, "", description, defaultValue);
}

float ConfigOptionFloat::get() const { return config().getFloat(getKey()); }

void validate(boost::any &v, const std::vector<std::string> &values, UString *, int)
{
	if (values.size() == 1)
		v = boost::any(UString(values[0]));
	else
		throw po::validation_error(po::validation_error::invalid_option_value);
}

}; // namespace OpenApoc
