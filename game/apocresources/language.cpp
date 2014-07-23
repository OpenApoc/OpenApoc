
#include "language.h"

Language::Language( std::wstring Language )
{
	std::wstring path( "data/LANGUAGES/" );
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

void Language::LoadAdditionalPack( std::wstring Filename )
{
	std::wstring path( "data/LANGUAGES/" );
	path.append( Filename );
	path.append( ".TXT" );
	languagepacks.push_back( new ConfigFile( path ) );
}

std::wstring* Language::GetText( std::wstring Key )
{
	for( std::vector<ConfigFile*>::const_iterator i = languagepacks.begin(); i != languagepacks.end(); i++ )
	{
		ConfigFile* c = (ConfigFile*)(*i);
		if( c->KeyExists( Key ) )
		{
			return c->GetQuickStringValue( Key, "" );
		}
	}
	return new std::wstring(Key);
}

