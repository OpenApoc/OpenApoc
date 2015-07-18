#include "framework/renderer_interface.h"
#include "framework/render/gl20/gl20.h"
#include "framework/render/gl20/gl20_helpers.h"
#include "framework/image.h"

#include "framework/render/gl20/gl20_shaders.inl"

#include <cassert>

namespace
{

using namespace OpenApoc;

class FBOData : public RendererImageData
{
private:
	const GL20 &gl;
public:
	GL20::GLuint fbo;
	GL20::GLuint tex;
	Vec2<float> size;
	//Constructor /only/ to be used for default surface (FBO ID == 0)
	FBOData(const GL20 &gl, GL20::GLuint fbo)
		//FIXME: Check FBO == 0
		//FIXME: Warn if trying to texture from FBO 0
		: gl(gl), fbo(fbo), tex(-1), size(0,0){}

	FBOData(const GL20 &gl, Vec2<int> size)
		: gl(gl), size(size.x, size.y)
	{
		gl.GenTextures(1, &this->tex);
		BindTexture b(gl, this->tex);
		gl.TexImage2D(GL20::TEXTURE_2D, 0, GL20::RGBA8, size.x, size.y, 0, GL20::RGBA, GL20::UNSIGNED_BYTE, NULL);
		gl.TexParameteri(GL20::TEXTURE_2D, GL20::TEXTURE_MIN_FILTER, GL20::NEAREST);
		gl.TexParameteri(GL20::TEXTURE_2D, GL20::TEXTURE_MAG_FILTER, GL20::NEAREST);
		gl.GenFramebuffers(1, &this->fbo);
		BindFramebuffer f(gl, this->fbo);

		gl.FramebufferTexture2D(GL20::DRAW_FRAMEBUFFER, GL20::COLOR_ATTACHMENT0, GL20::TEXTURE_2D, this->tex, 0);
		assert(gl.CheckFramebufferStatus(GL20::DRAW_FRAMEBUFFER) == GL20::FRAMEBUFFER_COMPLETE);


	}
	virtual ~FBOData()
	{
		if (tex)
			gl.DeleteTextures(1, &tex);
		if (fbo)
			gl.DeleteFramebuffers(1, &fbo);
	}
};

class GL20Renderer : public OpenApoc::Renderer
{
private:
	bool has_texture_array;
	std::unique_ptr<OpenApoc::GL20> gl;
	std::shared_ptr<Surface> defaultSurface;
	std::shared_ptr<Surface> currentSurface;
	GL20::GLuint currentBoundFBO;

	virtual void setSurface(std::shared_ptr<Surface> s)
	{
		this->flush();
		this->currentSurface = s;
		if (!s->rendererPrivateData)
			s->rendererPrivateData.reset(new FBOData(*gl, s->size));

		FBOData *fbo = static_cast<FBOData*>(s->rendererPrivateData.get());
		gl->BindFramebuffer(GL20::FRAMEBUFFER, fbo->fbo);
		this->currentBoundFBO = fbo->fbo;
		gl->Viewport(0, 0, s->size.x, s->size.y);
	}
	virtual std::shared_ptr<Surface> getSurface()
	{
		return this->currentSurface;
	}
public:
	GL20Renderer(std::unique_ptr<OpenApoc::GL20> ingl)
		: gl(std::move(ingl))
	{
		GL20::GLint viewport[4];
		gl->GetIntegerv(GL20::VIEWPORT, viewport);
		LogInfo("Viewport {%d,%d,%d,%d}", viewport[0], viewport[1], viewport[2], viewport[3]);
		assert(viewport[0] == 0 && viewport[1] == 0);
		this->defaultSurface = std::make_shared<Surface>(Vec2<int>{viewport[2], viewport[3]});
		this->defaultSurface->rendererPrivateData.reset(new FBOData(*gl, 0));
		currentBoundFBO = 0;
		this->currentSurface = this->defaultSurface;

		this->has_texture_array = (gl->driverExtensions.find("GL_EXT_texture_array") != gl->driverExtensions.end());

	}
	virtual void clear(Colour c = Colour{0,0,0,0})
	{
		LogError("Unimplemented");
	}
	virtual void setPalette(std::shared_ptr<Palette> p)
	{
		LogError("Unimplemented");
	}
	virtual void draw(std::shared_ptr<Image> i, Vec2<float> position)
	{
		LogError("Unimplemented");
	}
	virtual void drawRotated(std::shared_ptr<Image> i, Vec2<float> center, Vec2<float> position, float angle)
	{
		LogError("Unimplemented");
	}
	virtual void drawScaled(std::shared_ptr<Image> i, Vec2<float> position, Vec2<float> size, Scaler scaler = Scaler::Linear)
	{
		LogError("Unimplemented");
	}
	virtual void drawTinted(std::shared_ptr<Image> i, Vec2<float> position, Colour tint)
	{
		LogError("Unimplemented");
	}
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c)
	{
		LogError("Unimplemented");
	}
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0)
	{
		LogError("Unimplemented");
	}
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0)
	{
		LogError("Unimplemented");
	}
	virtual void flush()
	{
		LogError("Unimplemented");
	}
	virtual UString getName()
	{
		UString str = "GL20 renderer w/extensions:";
		if (has_texture_array)
			str += " texture_array";
		return str;
	}
	virtual std::shared_ptr<Surface> getDefaultSurface()
	{
		return this->defaultSurface;
	}

};

class OGL20RendererFactory : public OpenApoc::RendererFactory
{
bool alreadyInitialised;
bool functionLoadSuccess;
public:
	OGL20RendererFactory()
		: alreadyInitialised(false), functionLoadSuccess(false){}
	virtual OpenApoc::Renderer *create()
	{
		if (!alreadyInitialised)
		{
			alreadyInitialised = true;
			std::unique_ptr<OpenApoc::GL20> gl20(new OpenApoc::GL20());
			if (!gl20->loadedSuccessfully)
				return nullptr;
			return new GL20Renderer(std::move(gl20));
		}
		return nullptr;
	}
};

OpenApoc::RendererRegister<OGL20RendererFactory> register_at_load_gl_2_0_renderer("GL_2_0");

}; //anonymous namespace
