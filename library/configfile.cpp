
#include "configfile.h"
#include <ctype.h>
// TODO: Change to use C++ libs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ConfigFile::ConfigFile()
{
	Dirty = false;
}

ConfigFile::ConfigFile( std::wstring Filename )
{
	FILE* fileHnd;
	std::wstring document;
	char buf[1024];

	fileHnd = fopen( Filename.c_str(), "r" );
	if( fileHnd != 0 )
	{
		document.clear();
		while( !feof( fileHnd ) )
		{
			fgets( (char*)&buf, 1024, fileHnd );
			document.append( (char*)&buf );
			memset( (void*)buf, 0, 1024 );
		}
		ParseFile( document );
		fclose( fileHnd );
	}
	Dirty = false;
}

ConfigFile::~ConfigFile()
{
	while( Contents.size() > 0 )
	{
		ConfigData* cd = Contents.back();
		delete cd->Key;
		while( cd->Contents->size() > 0 )
		{
			delete cd->Contents->back();
			cd->Contents->pop_back();
		}
		//free( (void*)Contents.back() );		// <- Should delete cd instead of free?!
		Contents.pop_back();
		delete cd;
	}
}

bool ConfigFile::Save( std::wstring Filename )
{
	return Save( Filename, true );
}

bool ConfigFile::Save( std::wstring Filename, bool OnlyIfChanged )
{
	FILE* fileHnd;
	std::wstring document;
	bool dataNum;
	std::wstring* escstr;

	if( OnlyIfChanged && !Dirty )
	{
		return true;	// Nothing to save
	}

	fileHnd = fopen( Filename.c_str(), "w" );
	if( fileHnd == 0 )
		return false;

	document.clear();
	for( std::list<ConfigData*>::iterator i = Contents.begin(); i != Contents.end(); i++ )
	{
		ConfigData* cd = (ConfigData*)(*i);
		document.append( cd->Key->c_str() );
		if( cd->IsArray )
		{
			document.append( " [ " );

			bool isFirst = true;
			for( std::vector<std::wstring*>::iterator s = cd->Contents->begin(); s != cd->Contents->end(); s++ )
			{
				if( isFirst )
					isFirst = false;
				else
					document.append( ", " );

				std::wstring* cs = (std::wstring*)(*s);
				dataNum = IsNumber( *cs );
				if( !dataNum )
				{
					document.append( "\"" );
					escstr = EscapeString(*cs);
					document.append( escstr->c_str() );
					delete escstr;
					document.append( "\"" );
				} else {
					document.append( cs->c_str() );
				}
			}

			document.append( " ]\n" );
		} else {
			document.append( " = " );
			dataNum = IsNumber( *cd->Contents->front() );
			if( !dataNum )
			{
				document.append( "\"" );
				escstr = EscapeString(*cd->Contents->front());
				document.append( escstr->c_str() );
				delete escstr;
				document.append( "\"" );
			} else {
				document.append( cd->Contents->front()->c_str() );
			}
			document.append( "\n" );
		}

	}

	fputs( document.c_str(), fileHnd );
	fclose( fileHnd );

	Dirty = false;
	return true;
}

bool ConfigFile::KeyExists( std::wstring Key )
{
	if( GetData( Key ) != 0 )
		return true;
	return false;
}

bool ConfigFile::KeyIsArray( std::wstring Key )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
		return false;
	return cd->IsArray;
}

int ConfigFile::GetArraySize( std::wstring Key )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
		return 0;
	return cd->Contents->size();
}

void ConfigFile::RemoveArrayElement( std::wstring Key, int ArrayIndex )
{
	if( !KeyExists( Key ) )
		return;
	ConfigData* cd = GetData( Key );
	delete cd->Contents->at( ArrayIndex );
	cd->Contents->erase( cd->Contents->begin() + ArrayIndex );
}

void ConfigFile::RemoveKey( std::wstring Key )
{
	if( !KeyExists( Key ) )
		return;
	ConfigData* cd = GetData( Key );
	delete cd->Key;
	while( cd->Contents->size() > 0 )
	{
		delete cd->Contents->back();
		cd->Contents->pop_back();
	}
	Contents.remove( cd );
	delete cd;
}

void ConfigFile::ParseFile( std::wstring TextContents )
{
	ConfigData* cd;
	unsigned int charPos = 0;
	std::wstring token;
	bool charQuoted = false;
	bool wasQuoted = false;
	int TokenStep = 0;

	cd = (ConfigData*)malloc( sizeof( ConfigData ) );
	cd->Contents = new std::vector<std::wstring*>();
	cd->IsArray = false;

	while( charPos < TextContents.size() )
	{
		switch( TextContents.at( charPos ) )
		{
			case '#':
				while( charPos < TextContents.size() && TextContents.at( charPos ) != '\n' && TextContents.at( charPos ) != '\r' )
					charPos++;
				break;

			case ' ':
			case '\t':
			case ',':
			case '\n':
			case '\r':
			// case ']':
				if( charQuoted )
				{
					token.append( TextContents.substr( charPos, 1 ) );
				} else if( token.size() > 0 || wasQuoted ) {
					switch( TokenStep )
					{
						case 0:
							cd->Key = new std::wstring( token );
							TokenStep++;
							break;
						case 1:
							if( token == "[" )
								cd->IsArray = true;
							else
								cd->IsArray = false;
							TokenStep++;
							break;
						case 2:
							if( token != "]" )
								cd->Contents->push_back( new std::wstring(token) );
							break;
					}
					if( (!cd->IsArray && (TextContents.at( charPos ) == '\n' || TextContents.at( charPos ) == '\r')) || token == "]" )
					{
						Contents.push_back( cd );
						cd = (ConfigData*)malloc( sizeof( ConfigData ) );
						cd->Contents = new std::vector<std::wstring*>();
						cd->IsArray = false;
						TokenStep = 0;
					}
					token.clear();
					wasQuoted = false;
				}
				break;

			case '\\':
				if( charQuoted )
				{
					token.append( TextContents.substr( charPos + 1, 1 ) );
					charPos++;
				}
				break;

			case '"':
				if( TextContents.at( charPos - 1 ) != '\\' )
				{
					charQuoted = !charQuoted;
					wasQuoted = true;
				}
				break;

			default:
				token.append( TextContents.substr( charPos, 1 ) );
				break;
		}
		charPos++;
	}
	free( cd );

}

bool ConfigFile::IsNumber(std::wstring s)
{
	if( s.empty() )
		return false;
	for( std::wstring::iterator i = s.begin(); i != s.end(); i++ )
	{
		if( !isdigit(*i) && (*i) != '.' && (*i) != '+' && (*i) != '-' )
			return false;
	}
	return true;
}

std::wstring* ConfigFile::EscapeString( std::wstring s )
{
	std::wstring* out = new std::wstring();
	if( !s.empty() )
	{
		for( unsigned int i = 0; i < s.size(); i++ )
		{
			if( s.at(i) == '\\' || s.at(i) == '\"' )
				out->append( "\\" );
			out->append( s.substr( i, 1 ) );
		}
	}
	return out;
}


ConfigData* ConfigFile::GetData( std::wstring Key )
{
	for( std::list<ConfigData*>::iterator i = Contents.begin(); i != Contents.end(); i++ )
	{
		ConfigData* cd = (ConfigData*)(*i);
		if( cd->Key->compare( Key ) == 0 )
			return cd;
	}
	return 0;
}

bool ConfigFile::GetBooleanValue( std::wstring Key, bool* Value )
{
	return GetBooleanValue( Key, 0, Value );
}

bool ConfigFile::GetBooleanValue( std::wstring Key, int ArrayIndex, bool* Value )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
		return false;
	if( Value == 0 )
		return false;
	if( ArrayIndex < 0 || (int)cd->Contents->size() < ArrayIndex )
		return false;

	switch( ((std::wstring*)cd->Contents->at( ArrayIndex ))->at( 0 ) )
	{
		case 'T':
		case 't':
		case 'Y':
		case 'y':
			*Value = true;
			break;
		case 'F':
		case 'f':
		case 'N':
		case 'n':
			*Value = false;
			break;
		default:
			return false;
			break;
	}
	return true;
}

bool ConfigFile::GetIntegerValue( std::wstring Key, int* Value )
{
	return GetIntegerValue( Key, 0, Value );
}
bool ConfigFile::GetIntegerValue( std::wstring Key, int ArrayIndex, int* Value )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
		return false;
	if( Value == 0 )
		return false;
	if( ArrayIndex < 0 || (int)cd->Contents->size() < ArrayIndex )
		return false;

	*Value = atoi( cd->Contents->at( ArrayIndex )->c_str() );
	return true;
}

bool ConfigFile::GetInteger64Value( std::wstring Key, long* Value )
{
	return GetInteger64Value( Key, 0, Value );
}
bool ConfigFile::GetInteger64Value( std::wstring Key, int ArrayIndex, long* Value )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
		return false;
	if( Value == 0 )
		return false;
	if( ArrayIndex < 0 || (int)cd->Contents->size() < ArrayIndex )
		return false;

	*Value = atol( cd->Contents->at( ArrayIndex )->c_str() );
	return true;
}

bool ConfigFile::GetFloatValue( std::wstring Key, float* Value )
{
	return GetFloatValue( Key, 0, Value );
}

bool ConfigFile::GetFloatValue( std::wstring Key, int ArrayIndex, float* Value )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
		return false;
	if( Value == 0 )
		return false;
	if( ArrayIndex < 0 || (int)cd->Contents->size() < ArrayIndex )
		return false;

	*Value = (float)atof( cd->Contents->at( ArrayIndex )->c_str() );
	return true;
}

bool ConfigFile::GetStringValue( std::wstring Key, std::wstring* Value )
{
	return GetStringValue( Key, 0, Value );
}

bool ConfigFile::GetStringValue( std::wstring Key, int ArrayIndex, std::wstring* Value )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
		return false;
	if( Value == 0 )
		return false;
	if( ArrayIndex < 0 || (int)cd->Contents->size() < ArrayIndex )
		return false;

	Value->clear();
	Value->append( *(cd->Contents->at( ArrayIndex )) );
	return true;
}

bool ConfigFile::SetBooleanValue( std::wstring Key, bool Value )
{
	std::wstring* s = new std::wstring(( Value ? "True" : "False" ));
	bool r = SetStringValue( Key, s );
	return r;
}

bool ConfigFile::SetBooleanValue( std::wstring Key, int ArrayIndex, bool Value )
{
	std::wstring* s = new std::wstring(( Value ? "True" : "False" ));
	bool r = SetStringValue( Key, ArrayIndex, s );
	return r;
}

bool ConfigFile::SetIntegerValue( std::wstring Key, int Value )
{
	char val[200];
	sprintf( (char*)&val, "%d", Value );
	std::wstring* s = new std::wstring( val );
	bool r = SetStringValue( Key, s );
	return r;
}

bool ConfigFile::SetIntegerValue( std::wstring Key, int ArrayIndex, int Value )
{
	char val[200];
	sprintf( (char*)&val, "%d", Value );
	std::wstring* s = new std::wstring( val );
	bool r = SetStringValue( Key, ArrayIndex, s );
	return r;
}

bool ConfigFile::SetInteger64Value( std::wstring Key, long Value )
{
	char val[200];
	sprintf( (char*)&val, "%d", Value );
	std::wstring* s = new std::wstring( val );
	bool r = SetStringValue( Key, s );
	return r;
}

bool ConfigFile::SetInteger64Value( std::wstring Key, int ArrayIndex, long Value )
{
	char val[200];
	sprintf( (char*)&val, "%d", Value );
	std::wstring* s = new std::wstring( val );
	bool r = SetStringValue( Key, ArrayIndex, s );
	return r;
}

bool ConfigFile::SetFloatValue( std::wstring Key, float Value )
{
	char val[200];
	sprintf( (char*)&val, "%f", Value );
	std::wstring* s = new std::wstring( val );
	bool r = SetStringValue( Key, s );
	return r;
}

bool ConfigFile::SetFloatValue( std::wstring Key, int ArrayIndex, float Value )
{
	char val[200];
	sprintf( (char*)&val, "%f", Value );
	std::wstring* s = new std::wstring( val );
	bool r = SetStringValue( Key, ArrayIndex, s );
	return r;
}

bool ConfigFile::SetStringValue( std::wstring Key, std::wstring* Value )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
	{
		cd = (ConfigData*)malloc( sizeof(ConfigData) );
		cd->Key = new std::wstring( Key );
		cd->IsArray = false;
		cd->Contents = new std::vector<std::wstring*>();
		Contents.push_back( cd );
	}
	if( cd->Contents->empty() )
	{
		cd->Contents->push_back( Value );
	} else {
		std::wstring* s = cd->Contents->at( 0 );
		s->clear();
		s->append( *Value );
	}
	Dirty = true;
	return true;
}

bool ConfigFile::SetStringValue( std::wstring Key, int ArrayIndex, std::wstring* Value )
{
	ConfigData* cd = GetData( Key );
	if( cd == 0 )
	{
		cd = (ConfigData*)malloc( sizeof(ConfigData) );
		cd->Key = new std::wstring( Key );
		cd->IsArray = true;
		cd->Contents = new std::vector<std::wstring*>();
		Contents.push_back( cd );
	}
	if( (int)cd->Contents->size() <= ArrayIndex )
	{
		while( (long)cd->Contents->size() <= (long)ArrayIndex - 1 )
		{
			cd->Contents->push_back( new std::wstring() );
		}
		cd->Contents->push_back( Value );
	} else {
		std::wstring* s = cd->Contents->at( ArrayIndex );
		s->clear();
		s->append( *Value );
	}
	Dirty = true;
	return true;
}

bool ConfigFile::GetQuickBooleanValue( std::wstring Key, bool Default )
{
  bool res;
  if( !GetBooleanValue( Key, &res ) )
	{
		res = Default;
	}
  return res;
}

bool ConfigFile::GetQuickBooleanValue( std::wstring Key, int ArrayIndex, bool Default )
{
  bool res;
  if( !GetBooleanValue( Key, ArrayIndex, &res ) )
	{
		res = Default;
	}
  return res;
}

int ConfigFile::GetQuickIntegerValue( std::wstring Key, int Default )
{
  int res;
  if( !GetIntegerValue( Key, &res ) )
	{
		res = Default;
	}
  return res;
}

int ConfigFile::GetQuickIntegerValue( std::wstring Key, int ArrayIndex, int Default )
{
  int res;
  if( !GetIntegerValue( Key, ArrayIndex, &res ) )
	{
		res = Default;
	}
  return res;
}

long ConfigFile::GetQuickInteger64Value( std::wstring Key, long Default )
{
  long res;
  if( !GetInteger64Value( Key, &res ) )
	{
		res = Default;
	}
	return res;
}

long ConfigFile::GetQuickInteger64Value( std::wstring Key, int ArrayIndex, long Default )
{
  long res;
  if( !GetInteger64Value( Key, ArrayIndex, &res ) )
	{
		res = Default;
	}
  return res;
}

float ConfigFile::GetQuickFloatValue( std::wstring Key, float Default )
{
  float res;
  if( !GetFloatValue( Key, &res ) )
	{
		res = Default;
	}
  return res;
}

float ConfigFile::GetQuickFloatValue( std::wstring Key, int ArrayIndex, float Default )
{
  float res;
  if( !GetFloatValue( Key, ArrayIndex, &res ) )
	{
		res = Default;
	}
  return res;
}

std::wstring* ConfigFile::GetQuickStringValue( std::wstring Key, std::wstring Default )
{
  std::wstring* res = new std::wstring();
  if( !GetStringValue( Key, res ) )
	{
		res->clear();
		res->append( Default );
	}
  return res;
}

std::wstring* ConfigFile::GetQuickStringValue( std::wstring Key, int ArrayIndex, std::wstring Default )
{
  std::wstring* res = new std::wstring();
  if( !GetStringValue( Key, ArrayIndex, res ) )
	{
		res->clear();
		res->append( Default );
	}
  return res;
}
