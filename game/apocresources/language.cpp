
#include "language.h"

Language::Language( std::string Language )
{
	std::string path( "data/LANGUAGES/" );
	path.append( Language );
	path.append( ".TXT" );
	languagepacks.push_back( new ConfigFile( path ) );
}

Language::~Language()
{
	while( languagepacks.size() > 0 )
	{
		ConfigFile* c = languagepacks.back();
		languagepacks.pop_back();
		delete c;
	}
}

void Language::LoadAdditionalPack( std::string Filename )
{
	std::string path( "data/LANGUAGES/" );
	path.append( Filename );
	path.append( ".TXT" );
	languagepacks.push_back( new ConfigFile( path ) );
}

std::string* Language::GetText( std::string Key )
{
	for( std::vector<ConfigFile*>::const_iterator i = languagepacks.begin(); i != languagepacks.end(); i++ )
	{
		ConfigFile* c = (ConfigFile*)(*i);
		if( c->KeyExists( Key ) )
		{
			return c->GetQuickStringValue( Key, "" );
		}
	}
	return new std::string(Key);
}

