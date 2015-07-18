#pragma once

/* A cut down version of the GL2.0 api that we actually use, all dynamically loaded using
 * GetProcAddress*/
#include <functional>
#include <cstdint>
#include <memory>
#include <map>
#include <set>

#include "library/strings.h"

namespace OpenApoc
{

class GLLib;
class GL20
{
private:
	std::unique_ptr<GLLib> lib;
public:
	std::vector<int> version;

	UString vendorString;
	UString rendererString;
	UString versionString;
	UString extensionsString;

	typedef float GLfloat;
	typedef unsigned int GLbitfield;
	typedef unsigned char GLubyte;
	typedef unsigned int GLenum;

	enum GLEnumValues
	{
		VENDOR = 0x1f00,
		RENDERER = 0x1f01,
		VERSION = 0x1f02,
		EXTENSIONS = 0x1f03,
	};

	GL20();
	~GL20();
	bool loadedSuccessfully;


	std::map<UString, bool> loadedExtensions;

	std::set<UString> driverExtensions;

	std::function<void(unsigned int)>Clear;
	std::function<void(float, float, float)> ClearColor;
	std::function<const GLubyte*(GLenum)> GetString;

};

}; //namespace OpenApoc
