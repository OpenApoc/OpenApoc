#include "framework/renderer.h"
#include "framework/image.h"

#include "gl_3_0.cpp"

namespace OpenApoc {

namespace {

class Program
{
	public:
		GLuint prog;
		static GLuint CreateShader(GLenum type, const std::string source)
		{
			GLuint shader = gl::CreateShader(type);
			const GLchar *string = source.c_str();
			GLint stringLength = source.length();
			gl::ShaderSource(shader, 1, &string, &stringLength);
			gl::CompileShader(shader);
			GLint compileStatus;
			gl::GetShaderiv(shader, gl::COMPILE_STATUS, &compileStatus);
			if (compileStatus == gl::TRUE_)
				return shader;

			GLint logLength;
			gl::GetShaderiv(shader, gl::INFO_LOG_LENGTH, &logLength);

			std::unique_ptr<char[]> log(new char[logLength]);
			gl::GetShaderInfoLog(shader, logLength, NULL, log.get());

			std::cerr << "Shader compile error:\n\"" << std::string(log.get()) << "\"\n";

			gl::DeleteShader(shader);
			return 0;
		}
		Program(const std::string vertexSource, const std::string fragmentSource)
			: prog(0)
		{
			GLuint vShader = CreateShader(gl::VERTEX_SHADER, vertexSource);
			if (!vShader)
			{
				std::cerr << "Failed to compile vertex shader\n";
				return;
			}
			GLuint fShader = CreateShader(gl::FRAGMENT_SHADER, fragmentSource);
			if (!fShader)
			{
				std::cerr << "Failed to compile fragment shader\n";
				gl::DeleteShader(vShader);
				return;
			}

			prog = gl::CreateProgram();
			gl::AttachShader(prog, vShader);
			gl::AttachShader(prog, fShader);

			gl::DeleteShader(vShader);
			gl::DeleteShader(fShader);

			gl::LinkProgram(prog);

			GLint linkStatus;
			gl::GetProgramiv(prog, gl::LINK_STATUS, &linkStatus);
			if (linkStatus == gl::TRUE_)
				return;

			GLint logLength;
			gl::GetProgramiv(prog, gl::INFO_LOG_LENGTH, &logLength);

			std::unique_ptr<char[]> log(new char[logLength]);
			gl::GetProgramInfoLog(prog, logLength, NULL, log.get());

			std::cerr << "Program link error:\n\"" << std::string(log.get()) << "\"\n";

			gl::DeleteProgram(prog);
			prog = 0;
			return;

		}
		virtual ~Program()
		{
			if (prog)
				gl::DeleteProgram(prog);
		};
};

class SpriteProgram : public Program
{
	protected:
		SpriteProgram(const std::string vertexSource, const std::string fragmentSource)
			: Program(vertexSource, fragmentSource)
			{
			}
	public:
		GLuint positionAttribLocation;
		GLuint sizeAttribLocation;
		GLuint offsetAttribLocation;
		GLuint screenSizeUniformLocation;
		GLuint textureUniformLocation;
};

class RGBProgram : public SpriteProgram
{
	private:
		constexpr static const char* vertexSource = {
                "#version 130\n"
                "in vec2 position;\n"
                "in vec2 size;\n"
                "in vec2 offset;\n"
				"out vec2 texcoord;\n"
                "uniform vec2 screenSize;\n"
                "void main() {\n"
                "  texcoord = position;\n"
                "  vec2 tmpPos = position;\n"
                "  tmpPos *= size;\n"
                "  tmpPos += offset;\n"
                "  tmpPos /= screenSize;\n"
                "  tmpPos -= vec2(0.5,0.5);\n"
                "  gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
                "}\n"
		};
		constexpr static const char* fragmentSource  = {
                "#version 130\n"
                "in vec2 texcoord;\n"
                "uniform sampler2D tex;\n"
				"out vec4 out_colour;\n"
                "void main() {\n"
                "  out_colour = texture2D(tex, texcoord);\n"
                "}\n"
		};
	public:
		RGBProgram()
			: SpriteProgram(vertexSource, fragmentSource)
			{
			}
};

class BindTexture
{
	BindTexture(const BindTexture &) = delete;
public:
	GLenum bind;
	GLuint prevID;
	GLenum getBindEnum(GLenum e)
	{
		switch (e) {
			case gl::TEXTURE_1D: return gl::TEXTURE_BINDING_1D;
			case gl::TEXTURE_2D: return gl::TEXTURE_BINDING_2D;
			case gl::TEXTURE_3D: return gl::TEXTURE_BINDING_3D;
			default: assert(0);
		}
	}
	BindTexture(GLuint id, GLenum bind = gl::TEXTURE_2D)
		: bind(bind)
	{
		gl::GetIntegerv(getBindEnum(bind), (GLint*)&prevID);
		gl::BindTexture(bind, id);
	}
	~BindTexture()
	{
		gl::BindTexture(bind, prevID);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;
public:
	GLuint prevID;
	BindFramebuffer(GLuint id)
	{
		gl::GetIntegerv(gl::DRAW_FRAMEBUFFER_BINDING, (GLint*)&prevID);
		gl::BindFramebuffer(gl::DRAW_FRAMEBUFFER, id);

	}
	~BindFramebuffer()
	{
		gl::BindFramebuffer(gl::DRAW_FRAMEBUFFER, prevID);
	}
};

class FBOData : public RendererImageData
{
public:
	GLuint fbo;
	GLuint tex;
	FBOData(GLuint fbo)
		: fbo(fbo){}

	FBOData(Vec2<int> size)
	{
		gl::GenTextures(1, &this->tex);
		BindTexture b(this->tex);
		gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA8, size.x, size.y, 0, gl::RGBA, gl::UNSIGNED_BYTE, NULL);
		gl::GenFramebuffers(1, &this->fbo);
		BindFramebuffer f(this->fbo);

		gl::FramebufferTexture2D(gl::DRAW_FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D, this->tex, 0);
		assert(gl::CheckFramebufferStatus(gl::DRAW_FRAMEBUFFER) == gl::FRAMEBUFFER_COMPLETE);


	}
	virtual ~FBOData()
	{
		if (tex)
			gl::DeleteTextures(1, &tex);
		if (fbo)
			gl::DeleteFramebuffers(1, &fbo);
	}
};

class OGL30Renderer : public Renderer
{
private:
	std::unique_ptr<RGBProgram> rgbProgram;
	GLuint currentBountProgram;

	std::shared_ptr<Surface> currentSurface;
	friend class RendererSurfaceBinding;
	virtual void setSurface(std::shared_ptr<Surface> s)
	{
		this->flush();
		this->currentSurface = s;
		if (!s->rendererPrivateData)
			s->rendererPrivateData.reset(new FBOData(s->size));

		FBOData *fbo = static_cast<FBOData*>(s->rendererPrivateData.get());
		gl::BindFramebuffer(gl::FRAMEBUFFER, fbo->fbo);
		gl::Viewport(0, 0, s->size.x, s->size.y);
	};
	virtual std::shared_ptr<Surface> getSurface()
	{
		return currentSurface;
	};
	std::shared_ptr<Surface> defaultSurface;
public:
	OGL30Renderer();
	virtual ~OGL30Renderer();
	virtual void clear(Colour c = Colour{0,0,0,0});
	virtual void setPalette(std::shared_ptr<Palette> p){};
	virtual void draw(Image &i, Vec2<float> position){};
	virtual void drawRotated(Image &i, Vec2<float> center, Vec2<float> position, float angle){};
	virtual void drawScaled(Image &i, Vec2<float> position, Vec2<float> size, Scaler scaler = Scaler::Linear){};
	virtual void drawTinted(Image &i, Vec2<float> position, Colour tint){};
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c){};
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0){};
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0){};
	virtual void flush();
	virtual std::string getName();
	virtual std::shared_ptr<Surface>getDefaultSurface()
	{
		return this->defaultSurface;
	};

};

OGL30Renderer::OGL30Renderer()
	: rgbProgram(new RGBProgram())
{
	GLint viewport[4];
	gl::GetIntegerv(gl::VIEWPORT, viewport);
	std::cerr << "Viewport {" << viewport[0] << "," << viewport[1] << "," << viewport[2] << "," << viewport[3] << "}\n";
	assert(viewport[0] == 0 && viewport[1] == 0);
	this->defaultSurface = std::make_shared<Surface>(Vec2<int>{viewport[2], viewport[3]});
	this->defaultSurface->rendererPrivateData.reset(new FBOData(0));
	this->currentSurface = this->defaultSurface;

	GLint maxTexArrayLayers;
	gl::GetIntegerv(gl::MAX_ARRAY_TEXTURE_LAYERS, &maxTexArrayLayers);
	std::cerr << "MAX_ARRAY_TEXTURE_LAYERS: \"" << maxTexArrayLayers << "\"\n";
}

OGL30Renderer::~OGL30Renderer()
{

}

void
OGL30Renderer::clear(Colour c)
{
	this->flush();
	gl::ClearColor(c.r, c.g, c.b, c.a);
	gl::Clear(gl::COLOR_BUFFER_BIT);
}

void
OGL30Renderer::flush()
{
}

std::string
OGL30Renderer::getName()
{
	return "OGL3.0 Renderer";
}

}; //anonymouse namespace

OpenApoc::Renderer *
OpenApoc::Renderer::createRenderer()
{
	auto success = gl::sys::LoadFunctions();
	if (!success)
	{
		std::cerr << "Failed to load GL3.0\n";
		return nullptr;
	}
	if (success.GetNumMissing())
	{
		std::cerr << "Failed to load " << success.GetNumMissing() << " GL3.0 functions\n";
		return nullptr;
	}
	return new OGL30Renderer();
}
}; //namesapce OpenApoc
