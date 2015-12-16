#include "library/sp.h"
#include "framework/renderer_interface.h"
#include "framework/logger.h"
#include "framework/image.h"
#include "framework/palette.h"
#include <memory>
#include <array>

// namespace
//{
/* Workaround MSVC not liking int64_t being defined here and in allegro */
#define GLEXT_64_TYPES_DEFINED
#include "framework/render/gles20/gles_2_0.hpp"
//#include "framework/render/gles20/gles_2_0.inl"
//} // anonymous namespace

namespace
{

using namespace OpenApoc;

class Program
{
  public:
	GLuint prog;
	static GLuint CreateShader(GLenum type, const UString source)
	{
		GLuint shader = gl::CreateShader(type);
		auto sourceString = source.str();
		const GLchar *string = sourceString.c_str();
		GLint stringLength = sourceString.length();
		gl::ShaderSource(shader, 1, &string, &stringLength);
		gl::CompileShader(shader);
		GLint compileStatus;
		gl::GetShaderiv(shader, gl::COMPILE_STATUS, &compileStatus);
		LogInfo("Shader compilation status: %d", compileStatus);
		if (compileStatus == gl::TRUE_)
			return shader;

		GLint logLength;
		gl::GetShaderiv(shader, gl::INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		gl::GetShaderInfoLog(shader, logLength, NULL, log.get());

		LogError("Shader compile error: %s", log.get());

		gl::DeleteShader(shader);
		return 0;
	}
	Program(const UString vertexSource, const UString fragmentSource) : prog(0)
	{
		GLuint vShader = CreateShader(gl::VERTEX_SHADER, vertexSource);
		if (!vShader)
		{
			LogError("Failed to compile vertex shader");
			return;
		}
		GLuint fShader = CreateShader(gl::FRAGMENT_SHADER, fragmentSource);
		if (!fShader)
		{
			LogError("Failed to compile fragment shader");
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
		LogInfo("Program linking status: %d", linkStatus);
		if (linkStatus == gl::TRUE_)
			return;

		GLint logLength;
		gl::GetProgramiv(prog, gl::INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		gl::GetProgramInfoLog(prog, logLength, NULL, log.get());

		LogError("Program link error: %s", log.get());

		gl::DeleteProgram(prog);
		prog = 0;
		return;
	}

	void Uniform(GLuint loc, Colour c)
	{
		gl::Uniform4f(loc, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	void Uniform(GLuint loc, Vec2<float> v) { gl::Uniform2f(loc, v.x, v.y); }
	void Uniform(GLuint loc, Vec2<int> v)
	{
		// FIXME: Float conversion
		gl::Uniform2f(loc, v.x, v.y);
	}
	void Uniform(GLuint loc, float v) { gl::Uniform1f(loc, v); }
	void Uniform(GLuint loc, int v) { gl::Uniform1i(loc, v); }

	void Uniform(GLuint loc, bool v) { gl::Uniform1f(loc, (v ? 1.0f : 0.0f)); }

	virtual ~Program()
	{
		if (prog)
			gl::DeleteProgram(prog);
	}
};

class SpriteProgram : public Program
{
  protected:
	SpriteProgram(const UString vertexSource, const UString fragmentSource)
	    : Program(vertexSource, fragmentSource)
	{
	}

  public:
	GLint posLoc;
	GLint texcoordLoc;
	GLint screenSizeLoc;
	GLint texLoc;
	GLint flipYLoc;
};
const char *RGBProgram_vertexSource = {
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord_in;\n"
    "varying vec2 texcoord;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2.0), -(tmpPos.y*2.0),0.0,1.0);\n"
    "  else gl_Position = vec4((tmpPos.x*2.0), (tmpPos.y*2.0),0.0,1.0);\n"
    "}\n"};
const char *RGBProgram_fragmentSource = {"#version 100\n"
                                         "precision mediump float;\n"
                                         "varying vec2 texcoord;\n"
                                         "uniform sampler2D tex;\n"
                                         "void main() {\n"
                                         " gl_FragColor = texture2D(tex, texcoord);\n"
                                         "}\n"};
class RGBProgram : public SpriteProgram
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;

  public:
	RGBProgram()
	    : SpriteProgram(RGBProgram_vertexSource, RGBProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(0), currentTexUnit(0)
	{
		this->posLoc = gl::GetAttribLocation(this->prog, "position");
		if (this->posLoc < 0)
			LogError("\"position\" attribute not found in shader");
		this->texcoordLoc = gl::GetAttribLocation(this->prog, "texcoord_in");
		if (this->texcoordLoc < 0)
			LogError("\"texcoord_in\" attribute not found in shader");
		this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
		if (this->screenSizeLoc < 0)
			LogError("\"screenSize\" uniform not found in shader");
		this->texLoc = gl::GetUniformLocation(this->prog, "tex");
		if (this->texLoc < 0)
			LogError("\"tex\" uniform not found in shader");
		this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
		if (this->flipYLoc < 0)
			LogError("\"flipY\" uniform not found in shader");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0)
	{
		if (screenSize != currentScreenSize)
		{
			currentScreenSize = screenSize;
			this->Uniform(this->screenSizeLoc, screenSize);
		}
		if (texUnit != currentTexUnit)
		{
			currentTexUnit = texUnit;
			this->Uniform(this->texLoc, texUnit);
		}
		if (flipY != currentFlipY)
		{
			currentFlipY = flipY;
			this->Uniform(this->flipYLoc, flipY);
		}
	}
};
const char *PaletteProgram_vertexSource = {
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord_in;\n"
    "varying vec2 texcoord;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2.0), -(tmpPos.y*2.0),0.0,1.0);\n"
    "  else gl_Position = vec4((tmpPos.x*2.0), (tmpPos.y*2.0),0.0,1.0);\n"
    "}\n"};
const char *PaletteProgram_fragmentSource = {"#version 100\n"
                                             "precision mediump float;\n"
                                             "varying vec2 texcoord;\n"
                                             "uniform sampler2D tex;\n"
                                             "uniform sampler2D pal;\n"
                                             "void main() {\n"
                                             " float idx = texture2D(tex, texcoord,0.0).r;\n"
                                             " gl_FragColor = texture2D(pal, vec2(idx,0.0),0.0);\n"
                                             "}\n"};
class PaletteProgram : public SpriteProgram
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;
	GLint currentPalUnit;

  public:
	GLint palLoc;
	PaletteProgram()
	    : SpriteProgram(PaletteProgram_vertexSource, PaletteProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(false), currentTexUnit(0), currentPalUnit(0)
	{
		this->posLoc = gl::GetAttribLocation(this->prog, "position");
		this->texcoordLoc = gl::GetAttribLocation(this->prog, "texcoord_in");
		this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
		this->texLoc = gl::GetUniformLocation(this->prog, "tex");
		this->palLoc = gl::GetUniformLocation(this->prog, "pal");
		this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1)
	{
		if (screenSize != currentScreenSize)
		{
			currentScreenSize = screenSize;
			this->Uniform(this->screenSizeLoc, screenSize);
		}
		if (texUnit != currentTexUnit)
		{
			currentTexUnit = texUnit;
			this->Uniform(this->texLoc, texUnit);
		}
		if (palUnit != currentPalUnit)
		{
			currentPalUnit = palUnit;
			this->Uniform(this->palLoc, palUnit);
		}
		if (currentFlipY != flipY)
		{
			currentFlipY = flipY;
			this->Uniform(this->flipYLoc, flipY);
		}
	}
};

const char *SolidColourProgram_vertexSource = {
    "#version 100\n"
    "attribute vec2 position;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2.0), -(tmpPos.y*2.0),0.0,1.0);\n"
    "  else gl_Position = vec4((tmpPos.x*2.0), (tmpPos.y*2.0),0.0,1.0);\n"
    "}\n"};
const char *SolidColourProgram_fragmentSource = {"#version 100\n"
                                                 "precision mediump float;\n"
                                                 "uniform vec4 colour;\n"
                                                 "void main() {\n"
                                                 " gl_FragColor = colour;\n"
                                                 "}\n"};
class SolidColourProgram : public Program
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	Colour currentColour;

  public:
	GLuint posLoc;
	GLuint screenSizeLoc;
	GLuint colourLoc;
	GLuint flipYLoc;
	SolidColourProgram()
	    : Program(SolidColourProgram_vertexSource, SolidColourProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(false), currentColour(0, 0, 0, 0)
	{
		this->posLoc = gl::GetAttribLocation(this->prog, "position");
		this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
		this->colourLoc = gl::GetUniformLocation(this->prog, "colour");
		this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, Colour colour)
	{
		if (currentScreenSize != screenSize)
		{
			currentScreenSize = screenSize;
			this->Uniform(this->screenSizeLoc, screenSize);
		}
		if (currentColour != colour)
		{
			currentColour = colour;
			this->Uniform(this->colourLoc, colour);
		}
		if (currentFlipY != flipY)
		{
			currentFlipY = flipY;
			this->Uniform(this->flipYLoc, flipY);
		}
	}
};
class Quad
{
  public:
	std::array<Vec2<float>, 4> vertices;
	std::array<Vec2<float>, 4> texcoords;
	Quad(const Rect<float> &position, const Rect<float> texCoords = {{0, 0}, {1, 1}},
	     const Vec2<float> &rotationCenter = {0.0f, 0.0f}, float rotationAngleRadians = 0.0f)
	{
		texcoords = {{
		    Vec2<float>{texCoords.p0}, Vec2<float>{texCoords.p1.x, texCoords.p0.y},
		    Vec2<float>{texCoords.p0.x, texCoords.p1.y}, Vec2<float>{texCoords.p1},
		}};

		if (rotationAngleRadians != 0.0f)
		{
			auto rotMatrix = glm::rotate(rotationAngleRadians, Vec3<float>{0.0f, 0.0f, 1.0f});
			Vec2<float> size = position.p1 - position.p0;
			vertices = {{
			    Vec2<float>{0.0f, 0.0f}, Vec2<float>{size.x, 0.0f}, Vec2<float>{0.0f, size.y},
			    Vec2<float>{size},
			}};
			for (auto &p : vertices)
			{
				p -= rotationCenter;
				glm::vec4 transformed = rotMatrix * glm::vec4{p.x, p.y, 0.0f, 1.0f};
				p.x = transformed.x;
				p.y = transformed.y;
				p += rotationCenter;
				p += position.p0;
			}
		}
		else
		{
			vertices = {{
			    Vec2<float>{position.p0}, Vec2<float>{position.p1.x, position.p0.y},
			    Vec2<float>{position.p0.x, position.p1.y}, Vec2<float>{position.p1},
			}};
		}
	}
	void draw(GLuint vertexAttribPos, GLuint texcoordAttribPos)
	{
		gl::EnableVertexAttribArray(vertexAttribPos);
		gl::VertexAttribPointer(vertexAttribPos, 2, gl::FLOAT, gl::FALSE_, 0, &vertices);
		gl::EnableVertexAttribArray(texcoordAttribPos);
		gl::VertexAttribPointer(texcoordAttribPos, 2, gl::FLOAT, gl::FALSE_, 0, &texcoords);
		gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
	}
	void draw(GLuint vertexAttribPos)
	{
		gl::EnableVertexAttribArray(vertexAttribPos);
		gl::VertexAttribPointer(vertexAttribPos, 2, gl::FLOAT, gl::FALSE_, 0, &vertices);
		gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
	}
};
class Line
{
  public:
	std::array<Vec2<float>, 2> vertices;
	float thickness;
	Line(Vec2<float> p0, Vec2<float> p1, float thickness) : thickness(thickness)
	{
		vertices = {{p0, p1}};
	}
	void draw(GLuint vertexAttribPos)
	{
		gl::LineWidth(thickness);
		gl::EnableVertexAttribArray(vertexAttribPos);
		gl::VertexAttribPointer(vertexAttribPos, 2, gl::FLOAT, gl::FALSE_, 0, &vertices);
		gl::DrawArrays(gl::LINES, 0, 2);
	}
};
class ActiveTexture
{
	ActiveTexture(const ActiveTexture &) = delete;

  public:
	static GLenum getUnitEnum(int unit) { return gl::TEXTURE0 + unit; }

	ActiveTexture(int unit)
	{
		GLenum prevUnit;
		gl::GetIntegerv(gl::ACTIVE_TEXTURE, reinterpret_cast<GLint *>(&prevUnit));
		if (prevUnit == getUnitEnum(unit))
		{
			return;
		}
		gl::ActiveTexture(getUnitEnum(unit));
	}
};

class UnpackAlignment
{
	UnpackAlignment(const UnpackAlignment &) = delete;

  public:
	UnpackAlignment(int align)
	{
		GLint prevAlign;
		gl::GetIntegerv(gl::UNPACK_ALIGNMENT, &prevAlign);
		if (prevAlign == align)
		{
			return;
		}
		gl::PixelStorei(gl::UNPACK_ALIGNMENT, align);
	}
};

class BindTexture
{
	BindTexture(const BindTexture &) = delete;

  public:
	GLenum bind;
	int unit;
	bool nop;
	static GLenum getBindEnum(GLenum e)
	{
		switch (e)
		{
			// FIXME: GLES2.0 only has 2D textures
			/*case gl::TEXTURE_1D:
			    return gl::TEXTURE_BINDING_1D;*/
			case gl::TEXTURE_2D:
				return gl::TEXTURE_BINDING_2D;
			/*case gl::TEXTURE_3D:
			    return gl::TEXTURE_BINDING_3D;*/
			default:
				LogError("Unknown texture enum %d", static_cast<int>(e));
				return gl::TEXTURE_BINDING_2D;
		}
	}
	BindTexture(GLuint id, GLint unit = 0, GLenum bind = gl::TEXTURE_2D) : bind(bind), unit(unit)
	{
		ActiveTexture a(unit);
		GLuint prevID;
		gl::GetIntegerv(getBindEnum(bind), reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		gl::BindTexture(bind, id);
	}
};

template <GLenum param> class TexParam
{
	TexParam(const TexParam &) = delete;

  public:
	GLuint id;
	GLenum type;
	bool nop;

	TexParam(GLuint id, GLint value, GLenum type = gl::TEXTURE_2D) : id(id), type(type)
	{
		GLint prevValue;
		BindTexture b(id, 0, type);
		gl::GetTexParameteriv(type, param, &prevValue);
		if (prevValue == value)
		{
			return;
		}
		gl::TexParameteri(type, param, value);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;

  public:
	BindFramebuffer(GLuint id)
	{
		GLuint prevID;
		gl::GetIntegerv(gl::FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		gl::BindFramebuffer(gl::FRAMEBUFFER, id);
	}
};

class FBOData : public RendererImageData
{
  public:
	GLuint fbo;
	GLuint tex;
	Vec2<float> size;
	// Constructor /only/ to be used for default surface (FBO ID == 0)
	FBOData(GLuint fbo)
	    // FIXME: Check FBO == 0
	    // FIXME: Warn if trying to texture from FBO 0
	    : fbo(fbo),
	      tex(-1),
	      size(0, 0)
	{
	}

	FBOData(Vec2<int> size) : size(size.x, size.y)
	{
		gl::GenTextures(1, &this->tex);
		BindTexture b(this->tex);
		gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, size.x, size.y, 0, gl::RGBA, gl::UNSIGNED_BYTE,
		               NULL);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);

		gl::GenFramebuffers(1, &this->fbo);
		BindFramebuffer f(this->fbo);

		gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D, this->tex,
		                         0);
		assert(gl::CheckFramebufferStatus(gl::FRAMEBUFFER) == gl::FRAMEBUFFER_COMPLETE);
	}
	virtual ~FBOData()
	{
		if (tex)
			gl::DeleteTextures(1, &tex);
		if (fbo)
			gl::DeleteFramebuffers(1, &fbo);
	}
};

class GLRGBImage : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<RGBImage> parent;
	GLRGBImage(sp<RGBImage> parent) : size(parent->size), parent(parent)
	{
		RGBImageLock l(parent, ImageLockUse::Read);
		gl::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
		gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, parent->size.x, parent->size.y, 0, gl::RGBA,
		               gl::UNSIGNED_BYTE, l.getData());
	}
	virtual ~GLRGBImage() { gl::DeleteTextures(1, &this->texID); }
};

class GLPalette : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<Palette> parent;
	GLPalette(sp<Palette> parent) : size(Vec2<float>(parent->colours.size(), 1)), parent(parent)
	{
		gl::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
		gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, parent->colours.size(), 1, 0, gl::RGBA,
		               gl::UNSIGNED_BYTE, parent->colours.data());
	}
	virtual ~GLPalette() { gl::DeleteTextures(1, &this->texID); }
};

class GLPaletteImage : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<PaletteImage> parent;
	GLPaletteImage(sp<PaletteImage> parent) : size(parent->size), parent(parent)
	{
		PaletteImageLock l(parent, ImageLockUse::Read);
		gl::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		UnpackAlignment align(1);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
		// FIXME: GLES2.0 doesn't have gl::RED, gl::GREEN or gl::BLUE texture formats,
		// replacing them with gl::LUMINANCE might have side effects.
		gl::TexImage2D(gl::TEXTURE_2D, 0, /*1*/ gl::LUMINANCE, parent->size.x, parent->size.y,
		               0, /*gl::RED,*/
		               gl::LUMINANCE, gl::UNSIGNED_BYTE, l.getData());
	}
	virtual ~GLPaletteImage() { gl::DeleteTextures(1, &this->texID); }
};

class OGLES20Renderer : public Renderer
{
  private:
	sp<RGBProgram> rgbProgram;
	sp<SolidColourProgram> colourProgram;
	sp<PaletteProgram> paletteProgram;
	GLuint currentBoundProgram;
	GLuint currentBoundFBO;

	sp<Surface> currentSurface;
	sp<Palette> currentPalette;

	friend class RendererSurfaceBinding;
	virtual void setSurface(sp<Surface> s) override
	{
		if (this->currentSurface == s)
		{
			return;
		}
		this->flush();
		this->currentSurface = s;
		if (!s->rendererPrivateData)
			s->rendererPrivateData.reset(new FBOData(s->size));

		FBOData *fbo = static_cast<FBOData *>(s->rendererPrivateData.get());
		gl::BindFramebuffer(gl::FRAMEBUFFER, fbo->fbo);
		this->currentBoundFBO = fbo->fbo;
		gl::Viewport(0, 0, s->size.x, s->size.y);
	}
	virtual sp<Surface> getSurface() override { return currentSurface; }
	sp<Surface> defaultSurface;

  public:
	OGLES20Renderer()
	    : rgbProgram(new RGBProgram()), colourProgram(new SolidColourProgram()),
	      paletteProgram(new PaletteProgram()), currentBoundProgram(0), currentBoundFBO(0)
	{
		GLint viewport[4];
		gl::GetIntegerv(gl::VIEWPORT, viewport);
		LogInfo("Viewport {%d,%d,%d,%d}", viewport[0], viewport[1], viewport[2], viewport[3]);
		assert(viewport[0] == 0 && viewport[1] == 0);
		this->defaultSurface = std::make_shared<Surface>(Vec2<int>{viewport[2], viewport[3]});
		this->defaultSurface->rendererPrivateData.reset(new FBOData(0));
		this->currentSurface = this->defaultSurface;

		GLint maxTexUnits;
		gl::GetIntegerv(gl::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
		LogInfo("MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d", maxTexUnits);
		gl::Enable(gl::BLEND);
		gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
	}
	virtual ~OGLES20Renderer(){};
	virtual void clear(Colour c = Colour{0, 0, 0, 0}) override
	{
		this->flush();
		gl::ClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
		gl::Clear(gl::COLOR_BUFFER_BIT);
	}
	virtual void setPalette(sp<Palette> p) override
	{
		if (p == this->currentPalette)
			return;
		this->flush();
		if (!p->rendererPrivateData)
			p->rendererPrivateData.reset(new GLPalette(p));
		this->currentPalette = p;
	}
	virtual sp<Palette> getPalette() override { return this->currentPalette; }
	virtual void draw(sp<Image> image, Vec2<float> position) override
	{
		drawScaled(image, position, image->size, Scaler::Nearest);
	}
	virtual void drawRotated(sp<Image> image, Vec2<float> center, Vec2<float> position,
	                         float angle) override
	{
		auto size = image->size;
		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				image->rendererPrivateData.reset(img);
			}
			this->DrawRGB(*img, position, size, Scaler::Linear, center, angle);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		LogError("Unsupported image type");
	}
	virtual void drawScaled(sp<Image> image, Vec2<float> position, Vec2<float> size,
	                        Scaler scaler = Scaler::Linear) override
	{

		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				image->rendererPrivateData.reset(img);
			}
			this->DrawRGB(*img, position, size, scaler);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		if (paletteImage)
		{
			GLPaletteImage *img =
			    dynamic_cast<GLPaletteImage *>(paletteImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLPaletteImage(paletteImage);
				image->rendererPrivateData.reset(img);
			}
			if (scaler != Scaler::Nearest)
			{
				// blending indices doesn't make sense. You'll have to render
				// it to an RGB surface then scale that
				LogError("Only nearest scaler is supported on paletted images");
			}
			this->DrawPalette(*img, position, size);
			return;
		}

		sp<Surface> surface = std::dynamic_pointer_cast<Surface>(image);
		if (surface)
		{
			FBOData *fbo = dynamic_cast<FBOData *>(surface->rendererPrivateData.get());
			if (!fbo)
			{
				fbo = new FBOData(image->size);
				image->rendererPrivateData.reset(fbo);
			}
			this->DrawSurface(*fbo, position, size, scaler);
			return;
		}
		LogError("Unsupported image type");
	}

	virtual void drawTinted(sp<Image> i, Vec2<float> position, Colour tint) override
	{
		LogError("Unimplemented function");
		std::ignore = i;
		std::ignore = position;
		std::ignore = tint;
	}
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c) override
	{
		this->DrawRect(position, size, c);
	}
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c,
	                      float thickness = 1.0) override
	{
		this->drawLine(position, Vec2<float>{position.x + size.x, position.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x + size.x, position.y},
		               Vec2<float>{position.x + size.x, position.y + size.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x + size.x, position.y + size.y},
		               Vec2<float>{position.x, position.y + size.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x, position.y + size.y}, position, c, thickness);
	}
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0) override
	{
		this->DrawLine(p1, p2, c, thickness);
	}
	virtual void flush() override { /* Nothing to flush */}
	virtual UString getName() override { return "Highly-Experimental GLES2.0 Renderer"; }
	virtual sp<Surface> getDefaultSurface() override { return this->defaultSurface; }

	void BindProgram(sp<Program> p)
	{
		if (this->currentBoundProgram == p->prog)
			return;
		gl::UseProgram(p->prog);
		this->currentBoundProgram = p->prog;
	}
	void DrawRGB(GLRGBImage &img, Vec2<float> offset, Vec2<float> size, Scaler scaler,
	             Vec2<float> rotationCenter = {0, 0}, float rotationAngleRadians = 0)
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl::NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(img.texID);
		TexParam<gl::TEXTURE_MAG_FILTER> mag(img.texID, filter);
		TexParam<gl::TEXTURE_MIN_FILTER> min(img.texID, filter);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}}, rotationCenter, rotationAngleRadians);
		q.draw(rgbProgram->posLoc, rgbProgram->texcoordLoc);
	}
	void DrawPalette(GLPaletteImage &img, Vec2<float> offset, Vec2<float> size)
	{
		BindProgram(paletteProgram);
		Rect<float> pos(offset, offset + size);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(img.texID, 0);

		BindTexture p(
		    static_cast<GLPalette *>(this->currentPalette->rendererPrivateData.get())->texID, 1);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}});
		q.draw(paletteProgram->posLoc, paletteProgram->texcoordLoc);
	}

	void DrawSurface(FBOData &fbo, Vec2<float> offset, Vec2<float> size, Scaler scaler)
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl::NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(fbo.tex);
		TexParam<gl::TEXTURE_MAG_FILTER> mag(fbo.tex, filter);
		TexParam<gl::TEXTURE_MIN_FILTER> min(fbo.tex, filter);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}});
		q.draw(rgbProgram->posLoc, rgbProgram->texcoordLoc);
	}

	void DrawRect(Vec2<float> offset, Vec2<float> size, Colour c)
	{
		BindProgram(colourProgram);
		Rect<float> pos(offset, offset + size);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(this->currentSurface->size, flipY, c);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}});
		q.draw(colourProgram->posLoc);
	}

	void DrawLine(Vec2<float> p0, Vec2<float> p1, Colour c, float thickness)
	{
		BindProgram(colourProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(this->currentSurface->size, flipY, c);
		Line l(p0, p1, thickness);
		l.draw(colourProgram->posLoc);
	}
};

class OGLES20RendererFactory : public OpenApoc::RendererFactory
{
	bool alreadyInitialised;
	bool functionLoadSuccess;

  public:
	OGLES20RendererFactory() : alreadyInitialised(false), functionLoadSuccess(false) {}
	virtual OpenApoc::Renderer *create() override
	{
		if (!alreadyInitialised)
		{
			alreadyInitialised = true;
			auto success = gl::sys::LoadFunctions();
			if (!success)
			{
				LogInfo("failed to load GL implementation functions");
				return nullptr;
			}
			if (success.GetNumMissing())
			{
				LogInfo("GL implementation missing %d functions", success.GetNumMissing());
				return nullptr;
			}
			if (!gl::sys::IsVersionGEQ(2, 0))
			{
				LogInfo("GL version not at least 2.0");
				return nullptr;
			}
			functionLoadSuccess = true;
		}
		if (functionLoadSuccess)
		{
			return new OGLES20Renderer{};
		}
		return nullptr;
	}
};

OpenApoc::RendererRegister<OGLES20RendererFactory> register_at_load_gles_2_0_renderer("GLES_2_0");

}; // anonymous namespace
