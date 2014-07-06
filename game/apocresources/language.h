
#pragma once

#include "../../framework/includes.h"
#include "../../library/configfile.h"


class Language
{

	private:
		std::vector<ConfigFile*> languagepacks;


	public:
		Language( std::string Language );
		~Language();

		void LoadAdditionalPack( std::string Filename );
		std::string* GetText( std::string Key );

};
