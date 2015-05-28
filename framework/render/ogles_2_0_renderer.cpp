#include <GLES2/gl2.h>

#include "framework/renderer_interface.h"
#include "framework/logger.h"
#include "framework/image.h"
#include "framework/palette.h"
#include <memory>

namespace {

using namespace OpenApoc;

class OGLES20RendererFactory : public OpenApoc::RendererFactory
{
public:
	virtual OpenApoc::Renderer *create()
	{
		//Clear any accumulated errors
		while (glGetError() != GL_NO_ERROR)
			{}

		const char *versionStr = (const char*)glGetString(GL_VERSION);
		if (glGetError() != GL_NO_ERROR)
		{
			LogInfo("Failed to get GL_VERSION string");
			return nullptr;
		}
		LogInfo("GL_VERSION = \"%s\"", versionStr);
		UString version(versionStr);
		//A GLES version string goes:
		//"OpenGL ES x.y whatever"
		auto splitVersion = version.split(' ');
		if (splitVersion.size() >= 3 &&
			splitVersion[0] == "OpenGL" &&
			splitVersion[1] == "ES" &&
			Strings::IsNumeric(splitVersion[2]) &&
			Strings::ToInteger(splitVersion[2]) >= 2)
		{
			LogInfo("Valid OpenGLES2+ context found");
			//return new GLES20Renderer();
			return nullptr;
		}
		LogInfo("Not a GLESv2+ context - trying to find GLES2 compatibility extension");

		auto extensions = UString((const char*)glGetString(GL_EXTENSIONS)).split(' ');
		for (auto &e : extensions)
		{
			if (e == "GL_ARB_ES2_compatibility")
			{
				LogInfo("Found GL_ARB_ES2_compatibility extension");
				//return new GLES20Renderer();
				return nullptr;
			}
		}
		return nullptr;
	}
};

OpenApoc::RendererRegister<OGLES20RendererFactory> register_at_load_gles_2_0_renderer("GLES_2_0");



}; //anonymous namespace
