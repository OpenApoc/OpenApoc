
#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "framework/includes.h"

namespace OpenApoc {

class ConfigFile
{
	private:
		std::map<std::string, std::string> values;
		std::map<std::string, std::string> defaults;

	public:
		ConfigFile(const std::string fileName, std::map<std::string, std::string> defaults);
		void save(const std::string fileName);

		int getInt(const std::string key);
		bool getBool(const std::string key);
		std::string getString(const std::string key);
		void set(const std::string key, bool value);
		void set(const std::string key, int value);
		void set(const std::string key, const std::string value);

};

}; //namespace OpenApoc
