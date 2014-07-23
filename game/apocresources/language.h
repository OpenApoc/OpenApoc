
#pragma once

#include "../../framework/includes.h"
#include "../../library/configfile.h"


class Language
{

	private:
		std::vector<ConfigFile*> languagepacks;


	public:
		Language( std::wstring Language );
		~Language();

		void LoadAdditionalPack( std::wstring Filename );
		std::wstring* GetText( std::wstring Key );

};
