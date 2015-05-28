#include <GLES2/gl2.h>

#include "framework/renderer_interface.h"
#include "framework/logger.h"
#include "framework/image.h"
#include "framework/palette.h"
#include <memory>

namespace {

using namespace OpenApoc;

class OGLES20Renderer : public Renderer
{
private:
public:
	virtual ~OGLES20Renderer()
	{
	}
	virtual void setSurface(std::shared_ptr<Surface> s)
	{
		std::ignore = s;
		LogError("UNIMPLEMENTED");
	}
	virtual std::shared_ptr<Surface> getSurface()
	{
		LogError("UNIMPLEMENTED");
		return nullptr;
	}
	virtual void clear(Colour c = Colour{0,0,0,0})
	{
		std::ignore = c;
		LogError("UNIMPLEMENTED");
	}
	virtual void setPalette(std::shared_ptr<Palette> p)
	{
		std::ignore = p;
		LogError("UNIMPLEMENTED");
	}
	virtual void draw(std::shared_ptr<Image> i, Vec2<float> position)
	{
		std::ignore = i;
		std::ignore = position;
		LogError("UNIMPLEMENTED");
	}
	virtual void drawRotated(std::shared_ptr<Image> i, Vec2<float> center, Vec2<float> position, float angle)
	{
		std::ignore = i;
		std::ignore = center;
		std::ignore = position;
		std::ignore = angle;
		LogError("UNIMPLEMENTED");
	}
	virtual void drawScaled(std::shared_ptr<Image> i, Vec2<float> position, Vec2<float> size, Scaler scaler = Scaler::Linear)
	{
		std::ignore = i;
		std::ignore = position;
		std::ignore = size;
		std::ignore = scaler;
		LogError("UNIMPLEMENTED");
	}
	virtual void drawTinted(std::shared_ptr<Image> i, Vec2<float> position, Colour tint)
	{
		std::ignore = i;
		std::ignore = position;
		std::ignore = tint;
		LogError("UNIMPLEMENTED");
	}
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c)
	{
		std::ignore = position;
		std::ignore = size;
		std::ignore = c;
		LogError("UNIMPLEMENTED");
	}
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0)
	{
		std::ignore = position;
		std::ignore = size;
		std::ignore = c;
		std::ignore = thickness;
		LogError("UNIMPLEMENTED");
	}
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0)
	{
		std::ignore = p1;
		std::ignore = p2;
		std::ignore = c;
		std::ignore = thickness;
		LogError("UNIMPLEMENTED");
	}
	virtual void flush()
	{
		LogError("UNIMPLEMENTED");
	}
	virtual UString getName()
	{
		return "OpenGL|ES 2.0 renderer";
	}
	virtual std::shared_ptr<Surface> getDefaultSurface()
	{
		LogError("UNIMPLEMENTED");
		return nullptr;
	}
};

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
			return new OGLES20Renderer();
		}
		LogInfo("Not a GLESv2+ context - trying to find GLES2 compatibility extension");

		auto extensions = UString((const char*)glGetString(GL_EXTENSIONS)).split(' ');
		for (auto &e : extensions)
		{
			if (e == "GL_ARB_ES2_compatibility")
			{
				LogInfo("Found GL_ARB_ES2_compatibility extension");
				return new OGLES20Renderer();
			}
		}
		return nullptr;
	}
};

OpenApoc::RendererRegister<OGLES20RendererFactory> register_at_load_gles_2_0_renderer("GLES_2_0");



}; //anonymous namespace
