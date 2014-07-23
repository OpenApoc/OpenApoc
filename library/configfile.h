
#pragma once

#define _CRT_SECURE_NO_WARNINGS

//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
#include <string>
#include <vector>
#include <list>

typedef struct ConfigData
{
	std::wstring* Key;
	bool IsArray;
	std::vector<std::wstring*>* Contents;
} ConfigData;


class ConfigFile
{
	private:
		std::list<ConfigData*> Contents;

		void ParseFile( std::wstring TextContents );
		bool IsNumber( std::wstring s );
		std::wstring* EscapeString( std::wstring s );

		ConfigData* GetData( std::wstring Key );

		bool Dirty;

	public:
		ConfigFile();
		ConfigFile( std::wstring Filename );
		~ConfigFile();
		bool Save( std::wstring Filename );
		bool Save( std::wstring Filename, bool OnlyIfChanged );

		bool KeyExists( std::wstring Key );
		bool KeyIsArray( std::wstring Key );

		int GetArraySize( std::wstring Key );
		void RemoveArrayElement( std::wstring Key, int ArrayIndex );
		void RemoveKey( std::wstring Key );

		bool GetBooleanValue( std::wstring Key, bool* Value );
		bool GetBooleanValue( std::wstring Key, int ArrayIndex, bool* Value );
		bool GetIntegerValue( std::wstring Key, int* Value );
		bool GetIntegerValue( std::wstring Key, int ArrayIndex, int* Value );
		bool GetInteger64Value( std::wstring Key, long* Value );
		bool GetInteger64Value( std::wstring Key, int ArrayIndex, long* Value );
		bool GetFloatValue( std::wstring Key, float* Value );
		bool GetFloatValue( std::wstring Key, int ArrayIndex, float* Value );
		bool GetStringValue( std::wstring Key, std::wstring* Value );
		bool GetStringValue( std::wstring Key, int ArrayIndex, std::wstring* Value );

		bool GetQuickBooleanValue( std::wstring Key, bool Default );
		bool GetQuickBooleanValue( std::wstring Key, int ArrayIndex, bool Default );
		int GetQuickIntegerValue( std::wstring Key, int Default );
		int GetQuickIntegerValue( std::wstring Key, int ArrayIndex, int Default );
		long GetQuickInteger64Value( std::wstring Key, long Default );
		long GetQuickInteger64Value( std::wstring Key, int ArrayIndex, long Default );
		float GetQuickFloatValue( std::wstring Key, float Default );
		float GetQuickFloatValue( std::wstring Key, int ArrayIndex, float Default );
		std::wstring* GetQuickStringValue( std::wstring Key, std::wstring Default );
		std::wstring* GetQuickStringValue( std::wstring Key, int ArrayIndex, std::wstring Default );

		bool SetBooleanValue( std::wstring Key, bool Value );
		bool SetBooleanValue( std::wstring Key, int ArrayIndex, bool Value );
		bool SetIntegerValue( std::wstring Key, int Value );
		bool SetIntegerValue( std::wstring Key, int ArrayIndex, int Value );
		bool SetInteger64Value( std::wstring Key, long Value );
		bool SetInteger64Value( std::wstring Key, int ArrayIndex, long Value );
		bool SetFloatValue( std::wstring Key, float Value );
		bool SetFloatValue( std::wstring Key, int ArrayIndex, float Value );
		bool SetStringValue( std::wstring Key, std::wstring* Value );
		bool SetStringValue( std::wstring Key, int ArrayIndex, std::wstring* Value );


};
