
#pragma once

#include <string>
#include <sstream>
#include <vector>

class Strings
{

	public:

		static std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems)
		{
			std::stringstream ss(s);
			std::string item;
			while( std::getline(ss, item, delim) )
			{
				elems.push_back(item);
			}
			return elems;
		}


		static std::vector<std::string> Split(const std::string &s, char delim)
		{
			std::vector<std::string> elems;
			Split(s, delim, elems);
			return elems;
		}

		static bool IsNumeric(const std::string &s)
		{
			bool isnumeric = true;

			for( int i = 0; i < s.length(); i++ )
			{
				switch( s[i] )
				{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case '+':
					case '-':
					case '.':
					case ',':
						break;
					default:
						return false;
				}
			}
			return true;
		}

		static int ToInteger(const std::string &s)
		{
			if( !IsNumeric( s ) )
			{
				return 0;
			}
			return atoi( s.c_str() );
		}

		static float ToFloat(const std::string &s)
		{
			if( !IsNumeric( s ) )
			{
				return 0;
			}
			return atof( s.c_str() );
		}

};