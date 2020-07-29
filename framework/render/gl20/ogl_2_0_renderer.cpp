#include "framework/image.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "framework/renderer.h"
#include "framework/renderer_interface.h"
#include "library/sp.h"
#include <array>
#include <atomic>
#include <glm/gtx/rotate_vector.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

/* Workaround MSVC not liking int64_t being defined here and in allegro */
#define GLEXT_64_TYPES_DEFINED
#include "framework/render/gl20/gl_2_0.hpp"
#include "framework/render/gl20/gl_2_0.inl"

namespace
{

using namespace OpenApoc;

std::atomic<bool> renderer_dead = true;

// Forward declaration needed for RendererImageData
class OGL20Renderer;

class Program
{
  public:
	GLuint prog;
	static GLuint createShader(GLenum type, const UString source)
	{
		GLuint shader = gl20::CreateShader(type);
		auto sourceString = source;
		const GLchar *string = sourceString.c_str();
		GLint stringLength = sourceString.length();
		gl20::ShaderSource(shader, 1, &string, &stringLength);
		gl20::CompileShader(shader);
		GLint compileStatus;
		gl20::GetShaderiv(shader, gl20::COMPILE_STATUS, &compileStatus);
		if (compileStatus == gl20::TRUE_)
			return shader;

		GLint logLength;
		gl20::GetShaderiv(shader, gl20::INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		gl20::GetShaderInfoLog(shader, logLength, NULL, log.get());

		LogError("Shader compile error: %s", log.get());

		gl20::DeleteShader(shader);
		return 0;
	}
	Program(const UString vertexSource, const UString fragmentSource) : prog(0)
	{
		GLuint vShader = createShader(gl20::VERTEX_SHADER, vertexSource);
		if (!vShader)
		{
			LogError("Failed to compile vertex shader");
			return;
		}
		GLuint fShader = createShader(gl20::FRAGMENT_SHADER, fragmentSource);
		if (!fShader)
		{
			LogError("Failed to compile fragment shader");
			gl20::DeleteShader(vShader);
			return;
		}

		prog = gl20::CreateProgram();
		gl20::AttachShader(prog, vShader);
		gl20::AttachShader(prog, fShader);

		gl20::DeleteShader(vShader);
		gl20::DeleteShader(fShader);

		gl20::LinkProgram(prog);

		GLint linkStatus;
		gl20::GetProgramiv(prog, gl20::LINK_STATUS, &linkStatus);
		if (linkStatus == gl20::TRUE_)
			return;

		GLint logLength;
		gl20::GetProgramiv(prog, gl20::INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		gl20::GetProgramInfoLog(prog, logLength, NULL, log.get());

		LogError("Program link error: %s", log.get());

		gl20::DeleteProgram(prog);
		prog = 0;
		return;
	}

	void uniform(GLuint loc, Colour c)
	{
		gl20::Uniform4f(loc, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	void uniform(GLuint loc, Vec2<float> v) { gl20::Uniform2f(loc, v.x, v.y); }
	void uniform(GLuint loc, Vec2<int> v)
	{
		// FIXME: Float conversion
		gl20::Uniform2f(loc, v.x, v.y);
	}
	void uniform(GLuint loc, float v) { gl20::Uniform1f(loc, v); }
	void uniform(GLuint loc, int v) { gl20::Uniform1i(loc, v); }

	void uniform(GLuint loc, bool v) { gl20::Uniform1f(loc, (v ? 1.0f : 0.0f)); }

	virtual ~Program()
	{
		if (prog)
			gl20::DeleteProgram(prog);
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
	GLint posLoc = -1;
	GLint texcoordLoc = -1;
	GLint screenSizeLoc = -1;
	GLint texLoc = -1;
	GLint flipYLoc = -1;
	GLint tintLoc = -1;
};
const char *RGBProgram_vertexSource = {
    "#version 110\n"
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
const char *RGBProgram_fragmentSource = {"#version 110\n"
                                         "varying vec2 texcoord;\n"
                                         "uniform sampler2D tex;\n"
                                         "uniform vec4 tint;\n"
                                         "void main() {\n"
                                         " gl_FragColor = tint * texture2D(tex, texcoord);\n"
                                         "}\n"};
class RGBProgram : public SpriteProgram
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;
	Colour currentTint;

  public:
	RGBProgram()
	    : SpriteProgram(RGBProgram_vertexSource, RGBProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(0), currentTexUnit(0), currentTint(0, 0, 0, 0)
	{
		this->posLoc = gl20::GetAttribLocation(this->prog, "position");
		if (this->posLoc < 0)
			LogError("\"position\" attribute not found in shader");
		this->texcoordLoc = gl20::GetAttribLocation(this->prog, "texcoord_in");
		if (this->texcoordLoc < 0)
			LogError("\"texcoord_in\" attribute not found in shader");
		this->screenSizeLoc = gl20::GetUniformLocation(this->prog, "screenSize");
		if (this->screenSizeLoc < 0)
			LogError("\"screenSize\" uniform not found in shader");
		this->texLoc = gl20::GetUniformLocation(this->prog, "tex");
		if (this->texLoc < 0)
			LogError("\"tex\" uniform not found in shader");
		this->flipYLoc = gl20::GetUniformLocation(this->prog, "flipY");
		if (this->flipYLoc < 0)
			LogError("\"flipY\" uniform not found in shader");
		this->tintLoc = gl20::GetUniformLocation(this->prog, "tint");
		if (this->tintLoc < 0)
			LogError("\"tint\" uniform not found in shader");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, Colour tint = {255, 255, 255, 255},
	                 GLint texUnit = 0)
	{
		if (screenSize != currentScreenSize)
		{
			currentScreenSize = screenSize;
			this->uniform(this->screenSizeLoc, screenSize);
		}
		if (texUnit != currentTexUnit)
		{
			currentTexUnit = texUnit;
			this->uniform(this->texLoc, texUnit);
		}
		if (flipY != currentFlipY)
		{
			currentFlipY = flipY;
			this->uniform(this->flipYLoc, flipY);
		}
		if (tint != currentTint)
		{
			currentTint = tint;
			this->uniform(this->tintLoc, tint);
		}
	}
};
const char *PaletteProgram_vertexSource = {
    "#version 110\n"
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
const char *PaletteProgram_fragmentSource = {
    "#version 110\n"
    "varying vec2 texcoord;\n"
    "uniform sampler2D tex;\n"
    "uniform sampler2D pal;\n"
    "uniform vec4 tint;\n"
    "void main() {\n"
    " float idx = texture2D(tex, texcoord,0.0).r;\n"
    " gl_FragColor = tint * texture2D(pal, vec2(idx,0.0),0.0);\n"
    "}\n"};
class PaletteProgram : public SpriteProgram
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;
	GLint currentPalUnit;
	Colour currentTint;

  public:
	GLint palLoc;
	PaletteProgram()
	    : SpriteProgram(PaletteProgram_vertexSource, PaletteProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(false), currentTexUnit(0), currentPalUnit(0),
	      currentTint(0, 0, 0, 0)
	{
		this->posLoc = gl20::GetAttribLocation(this->prog, "position");
		this->texcoordLoc = gl20::GetAttribLocation(this->prog, "texcoord_in");
		this->screenSizeLoc = gl20::GetUniformLocation(this->prog, "screenSize");
		this->texLoc = gl20::GetUniformLocation(this->prog, "tex");
		this->palLoc = gl20::GetUniformLocation(this->prog, "pal");
		this->flipYLoc = gl20::GetUniformLocation(this->prog, "flipY");
		this->tintLoc = gl20::GetUniformLocation(this->prog, "tint");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, Colour tint = {255, 255, 255, 255},
	                 GLint texUnit = 0, GLint palUnit = 1)
	{
		if (screenSize != currentScreenSize)
		{
			currentScreenSize = screenSize;
			this->uniform(this->screenSizeLoc, screenSize);
		}
		if (texUnit != currentTexUnit)
		{
			currentTexUnit = texUnit;
			this->uniform(this->texLoc, texUnit);
		}
		if (palUnit != currentPalUnit)
		{
			currentPalUnit = palUnit;
			this->uniform(this->palLoc, palUnit);
		}
		if (currentFlipY != flipY)
		{
			currentFlipY = flipY;
			this->uniform(this->flipYLoc, flipY);
		}
		if (tint != currentTint)
		{
			currentTint = tint;
			this->uniform(this->tintLoc, tint);
		}
	}
};

const char *SolidColourProgram_vertexSource = {
    "#version 110\n"
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
const char *SolidColourProgram_fragmentSource = {"#version 110\n"
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
		this->posLoc = gl20::GetAttribLocation(this->prog, "position");
		this->screenSizeLoc = gl20::GetUniformLocation(this->prog, "screenSize");
		this->colourLoc = gl20::GetUniformLocation(this->prog, "colour");
		this->flipYLoc = gl20::GetUniformLocation(this->prog, "flipY");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, Colour colour)
	{
		if (currentScreenSize != screenSize)
		{
			currentScreenSize = screenSize;
			this->uniform(this->screenSizeLoc, screenSize);
		}
		if (currentColour != colour)
		{
			currentColour = colour;
			this->uniform(this->colourLoc, colour);
		}
		if (currentFlipY != flipY)
		{
			currentFlipY = flipY;
			this->uniform(this->flipYLoc, flipY);
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
		    Vec2<float>{texCoords.p0},
		    Vec2<float>{texCoords.p1.x, texCoords.p0.y},
		    Vec2<float>{texCoords.p0.x, texCoords.p1.y},
		    Vec2<float>{texCoords.p1},
		}};

		if (rotationAngleRadians != 0.0f)
		{
			auto rotMatrix = glm::rotate(rotationAngleRadians, Vec3<float>{0.0f, 0.0f, 1.0f});
			Vec2<float> size = position.p1 - position.p0;
			vertices = {{
			    Vec2<float>{0.0f, 0.0f},
			    Vec2<float>{size.x, 0.0f},
			    Vec2<float>{0.0f, size.y},
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
			    Vec2<float>{position.p0},
			    Vec2<float>{position.p1.x, position.p0.y},
			    Vec2<float>{position.p0.x, position.p1.y},
			    Vec2<float>{position.p1},
			}};
		}
	}
	void draw(GLuint vertexAttribPos, GLuint texcoordAttribPos)
	{
		gl20::EnableVertexAttribArray(vertexAttribPos);
		gl20::VertexAttribPointer(vertexAttribPos, 2, gl20::FLOAT, gl20::FALSE_, 0, &vertices);
		gl20::EnableVertexAttribArray(texcoordAttribPos);
		gl20::VertexAttribPointer(texcoordAttribPos, 2, gl20::FLOAT, gl20::FALSE_, 0, &texcoords);
		gl20::DrawArrays(gl20::TRIANGLE_STRIP, 0, 4);
	}
	void draw(GLuint vertexAttribPos)
	{
		gl20::EnableVertexAttribArray(vertexAttribPos);
		gl20::VertexAttribPointer(vertexAttribPos, 2, gl20::FLOAT, gl20::FALSE_, 0, &vertices);
		gl20::DrawArrays(gl20::TRIANGLE_STRIP, 0, 4);
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
		gl20::LineWidth(thickness);
		gl20::EnableVertexAttribArray(vertexAttribPos);
		gl20::VertexAttribPointer(vertexAttribPos, 2, gl20::FLOAT, gl20::FALSE_, 0, &vertices);
		gl20::DrawArrays(gl20::LINES, 0, 2);
	}
};
class ActiveTexture
{
	ActiveTexture(const ActiveTexture &) = delete;

  public:
	static GLenum getUnitEnum(int unit) { return gl20::TEXTURE0 + unit; }

	ActiveTexture(int unit)
	{
		GLenum prevUnit;
		gl20::GetIntegerv(gl20::ACTIVE_TEXTURE, reinterpret_cast<GLint *>(&prevUnit));
		if (prevUnit == getUnitEnum(unit))
		{
			return;
		}
		gl20::ActiveTexture(getUnitEnum(unit));
	}
};

class UnpackAlignment
{
	UnpackAlignment(const UnpackAlignment &) = delete;

  public:
	UnpackAlignment(int align)
	{
		GLint prevAlign;
		gl20::GetIntegerv(gl20::UNPACK_ALIGNMENT, &prevAlign);
		if (prevAlign == align)
		{
			return;
		}
		gl20::PixelStorei(gl20::UNPACK_ALIGNMENT, align);
	}
};

class BindTexture
{
	BindTexture(const BindTexture &) = delete;

  public:
	GLenum bind;
	int unit;
	static GLenum getBindEnum(GLenum e)
	{
		switch (e)
		{
			case gl20::TEXTURE_1D:
				return gl20::TEXTURE_BINDING_1D;
			case gl20::TEXTURE_2D:
				return gl20::TEXTURE_BINDING_2D;
			case gl20::TEXTURE_3D:
				return gl20::TEXTURE_BINDING_3D;
			default:
				LogError("Unknown texture enum %d", static_cast<int>(e));
				return gl20::TEXTURE_BINDING_2D;
		}
	}
	BindTexture(GLuint id, GLint unit = 0, GLenum bind = gl20::TEXTURE_2D) : bind(bind), unit(unit)
	{
		ActiveTexture a(unit);
		GLuint prevID;
		gl20::GetIntegerv(getBindEnum(bind), reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		gl20::BindTexture(bind, id);
	}
};

template <GLenum param> class TexParam
{
	TexParam(const TexParam &) = delete;

  public:
	GLuint id;
	GLenum type;

	TexParam(GLuint id, GLint value, GLenum type = gl20::TEXTURE_2D) : id(id), type(type)
	{
		GLint prevValue;
		BindTexture b(id, 0, type);
		gl20::GetTexParameteriv(type, param, &prevValue);
		if (prevValue == value)
		{
			return;
		}
		gl20::TexParameteri(type, param, value);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;

  public:
	BindFramebuffer(GLuint id)
	{
		GLuint prevID;
		gl20::GetIntegerv(gl20::FRAMEBUFFER_BINDING_EXT, reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		gl20::BindFramebufferEXT(gl20::FRAMEBUFFER_EXT, id);
	}
};

class FBOData : public RendererImageData
{
  public:
	GLuint fbo;
	GLuint tex;
	Vec2<float> size;
	OGL20Renderer *owner;
	// Constructor /only/ to be used for default surface (FBO ID == 0)
	FBOData(GLuint fbo, Vec2<int> size, OGL20Renderer *owner)
	    // FIXME: Check FBO == 0
	    // FIXME: Warn if trying to texture from FBO 0
	    : fbo(fbo), tex(-1), size(size), owner(owner)
	{
	}

	sp<Image> readBack() override
	{
		auto img = mksp<RGBImage>(size);
		BindFramebuffer f(this->fbo);

		RGBImageLock l(img);
		// Foiled once again by inverted y! Read in each line bottom->top writing top->bottom in the
		// image
		uint8_t *imgPos = reinterpret_cast<uint8_t *>(l.getData());
		unsigned imgStride = size.x * 4;
		for (int y = 0; y < size.y; y++)
		{
			gl20::ReadPixels(0, size.y - 1 - y, size.x, size.y - y, gl20::RGBA, gl20::UNSIGNED_BYTE,
			                 imgPos);
			imgPos += imgStride;
		}

		return img;
	}

	FBOData(Vec2<int> size, OGL20Renderer *owner) : size(size.x, size.y), owner(owner)
	{
		gl20::GenTextures(1, &this->tex);
		BindTexture b(this->tex);
		gl20::TexImage2D(gl20::TEXTURE_2D, 0, gl20::RGBA8, size.x, size.y, 0, gl20::RGBA,
		                 gl20::UNSIGNED_BYTE, NULL);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MIN_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MAG_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_S, gl20::CLAMP_TO_EDGE);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_T, gl20::CLAMP_TO_EDGE);

		gl20::GenFramebuffersEXT(1, &this->fbo);
		BindFramebuffer f(this->fbo);

		gl20::FramebufferTexture2DEXT(gl20::FRAMEBUFFER_EXT, gl20::COLOR_ATTACHMENT0_EXT,
		                              gl20::TEXTURE_2D, this->tex, 0);
		LogAssert(gl20::CheckFramebufferStatusEXT(gl20::FRAMEBUFFER_EXT) ==
		          gl20::FRAMEBUFFER_COMPLETE_EXT);
	}
	~FBOData() override;
};

class GLRGBImage : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<RGBImage> parent;
	OGL20Renderer *owner;
	GLRGBImage(sp<RGBImage> parent, OGL20Renderer *owner)
	    : size(parent->size), parent(parent), owner(owner)
	{
		RGBImageLock l(parent, ImageLockUse::Read);
		gl20::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MIN_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MAG_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_S, gl20::CLAMP_TO_EDGE);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_T, gl20::CLAMP_TO_EDGE);
		gl20::TexImage2D(gl20::TEXTURE_2D, 0, gl20::RGBA, parent->size.x, parent->size.y, 0,
		                 gl20::RGBA, gl20::UNSIGNED_BYTE, l.getData());
	}
	~GLRGBImage() override;
};

class GLPalette : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<Palette> parent;
	OGL20Renderer *owner;
	GLPalette(sp<Palette> parent, OGL20Renderer *owner)
	    : size(Vec2<float>(parent->colours.size(), 1)), parent(parent), owner(owner)
	{
		gl20::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MIN_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MAG_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_S, gl20::CLAMP_TO_EDGE);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_T, gl20::CLAMP_TO_EDGE);
		gl20::TexImage2D(gl20::TEXTURE_2D, 0, gl20::RGBA, parent->colours.size(), 1, 0, gl20::RGBA,
		                 gl20::UNSIGNED_BYTE, parent->colours.data());
	}
	~GLPalette() override;
};

class GLPaletteImage : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<PaletteImage> parent;
	OGL20Renderer *owner;
	GLPaletteImage(sp<PaletteImage> parent, OGL20Renderer *owner)
	    : size(parent->size), parent(parent), owner(owner)
	{
		PaletteImageLock l(parent, ImageLockUse::Read);
		gl20::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		UnpackAlignment align(1);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MIN_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_MAG_FILTER, gl20::NEAREST);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_S, gl20::CLAMP_TO_EDGE);
		gl20::TexParameteri(gl20::TEXTURE_2D, gl20::TEXTURE_WRAP_T, gl20::CLAMP_TO_EDGE);
		gl20::TexImage2D(gl20::TEXTURE_2D, 0, 1, parent->size.x, parent->size.y, 0, gl20::RED,
		                 gl20::UNSIGNED_BYTE, l.getData());
	}
	~GLPaletteImage() override;
};

class OGL20Renderer : public Renderer
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
	void setSurface(sp<Surface> s) override
	{
		if (this->currentSurface == s)
		{
			return;
		}
		this->flush();
		this->currentSurface = s;
		if (!s->rendererPrivateData)
			s->rendererPrivateData.reset(new FBOData(s->size, this));

		FBOData *fbo = static_cast<FBOData *>(s->rendererPrivateData.get());
		gl20::BindFramebufferEXT(gl20::FRAMEBUFFER_EXT, fbo->fbo);
		this->currentBoundFBO = fbo->fbo;
		gl20::Viewport(0, 0, s->size.x, s->size.y);
	}
	sp<Surface> getSurface() override { return currentSurface; }
	sp<Surface> defaultSurface;

	std::thread::id bound_thread;
	std::mutex destroyed_texture_list_mutex;
	std::list<GLuint> destroyed_texture_list;
	std::mutex destroyed_framebuffer_list_mutex;
	std::list<GLuint> destroyed_framebuffer_list;

  public:
	OGL20Renderer()
	    : rgbProgram(new RGBProgram()), colourProgram(new SolidColourProgram()),
	      paletteProgram(new PaletteProgram()), currentBoundProgram(0), currentBoundFBO(0)
	{
		this->bound_thread = std::this_thread::get_id();
		GLint viewport[4];
		gl20::GetIntegerv(gl20::VIEWPORT, viewport);
		LogInfo("Viewport {%d,%d,%d,%d}", viewport[0], viewport[1], viewport[2], viewport[3]);
		LogAssert(viewport[0] == 0 && viewport[1] == 0);
		this->defaultSurface = mksp<Surface>(Vec2<int>{viewport[2], viewport[3]});
		this->defaultSurface->rendererPrivateData.reset(
		    new FBOData(0, {viewport[2], viewport[3]}, this));
		this->currentSurface = this->defaultSurface;

		GLint maxTexUnits;
		gl20::GetIntegerv(gl20::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
		LogInfo("MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d", maxTexUnits);
		gl20::Enable(gl20::BLEND);
		gl20::BlendFuncSeparate(gl20::SRC_ALPHA, gl20::ONE_MINUS_SRC_ALPHA, gl20::SRC_ALPHA,
		                        gl20::DST_ALPHA);
		renderer_dead = false;
	}
	~OGL20Renderer() override { renderer_dead = true; };
	void clear(Colour c = Colour{0, 0, 0, 0}) override
	{
		this->flush();
		gl20::ClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
		gl20::Clear(gl20::COLOR_BUFFER_BIT);
	}
	void setPalette(sp<Palette> p) override
	{
		if (p == this->currentPalette)
			return;
		this->flush();
		if (!p->rendererPrivateData)
			p->rendererPrivateData.reset(new GLPalette(p, this));
		this->currentPalette = p;
	}
	sp<Palette> getPalette() override { return this->currentPalette; }
	void draw(sp<Image> image, Vec2<float> position) override
	{
		drawScaled(image, position, image->size, Scaler::Nearest);
	}
	void drawRotated(sp<Image> image, Vec2<float> center, Vec2<float> position,
	                 float angle) override
	{
		auto size = image->size;
		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage, this);
				image->rendererPrivateData.reset(img);
			}
			this->drawRgb(*img, position, size, Scaler::Linear, center, angle);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		LogError("Unsupported image type");
	}
	void drawScaled(sp<Image> image, Vec2<float> position, Vec2<float> size,
	                Scaler scaler = Scaler::Linear) override
	{
		drawScaledImage(image, position, size, scaler);
	}
	void drawScaledImage(sp<Image> image, Vec2<float> position, Vec2<float> size,
	                     Scaler scaler = Scaler::Linear, Colour tint = {255, 255, 255, 255})
	{

		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage, this);
				image->rendererPrivateData.reset(img);
			}
			this->drawRgb(*img, position, size, scaler, {0, 0}, 0, tint);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		if (paletteImage)
		{
			GLPaletteImage *img =
			    dynamic_cast<GLPaletteImage *>(paletteImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLPaletteImage(paletteImage, this);
				image->rendererPrivateData.reset(img);
			}
			if (scaler != Scaler::Nearest)
			{
				// blending indices doesn't make sense. You'll have to render
				// it to an RGB surface then scale that
				LogError("Only nearest scaler is supported on paletted images");
			}
			this->drawPalette(*img, position, size, tint);
			return;
		}

		sp<Surface> surface = std::dynamic_pointer_cast<Surface>(image);
		if (surface)
		{
			FBOData *fbo = dynamic_cast<FBOData *>(surface->rendererPrivateData.get());
			if (!fbo)
			{
				fbo = new FBOData(image->size, this);
				image->rendererPrivateData.reset(fbo);
			}
			this->drawSurface(*fbo, position, size, scaler, tint);
			return;
		}
		LogError("Unsupported image type");
	}

	void drawTinted(sp<Image> i, Vec2<float> position, Colour tint) override
	{
		drawScaledImage(i, position, i->size, Scaler::Nearest, tint);
	}
	void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c) override
	{
		bindProgram(colourProgram);
		Rect<float> pos(position, position + size);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(this->currentSurface->size, flipY, c);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}});
		q.draw(colourProgram->posLoc);
	}
	void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0) override
	{

		// The lines are all shifted in x/y by {capsize} to ensure the corners are correctly covered
		// and don't overlap (which may be an issue if alpha != 1.0f:
		//
		// The cap 'ownership' for lines 1,2,3,4 is shifted by (x-1), (y-1), (x+1), (y+1)
		// In picture form:
		//
		// 4333333
		// 4     2
		// 4     2
		// 1111112
		//
		// At the corners we have a bit of complexity to correctly cap & avoid overlap:
		//
		// p0 = position
		// p1 = position + size
		//
		//  p1.y|----+-------+---------------------------+
		//      |    |       |                           |
		//      v    |       |   Line 3                  |
		//      Y    |       |                           |
		//      ^    |       C-------------------+-------+
		//      |    | Line 4|                   |       |
		//      |    |       |                   |       |
		//      |    |       |                   |       |
		//      |    |       |                   | Line 2|
		//      |    |       |                   |       |
		//      |    D-------+-------------------+       |
		//      |    |            ^              |       |
		//      |    |  Line 1    | thickness    |       |
		//      |    |            v              |       |
		//  p0.y|----A---------------------------B-------+
		//      |    |                                   |
		//     0+----------------> X <-----------------------
		//      0   p0.x                                 p1.x
		//
		// As wide lines are apparently a massive ballache in opengl to stick to any kind of raster
		// standard, this is actually implemented using rects for each line.
		// Assuming that wide lines are centered around {+0.5, +0.5} A.y (for example) would be
		//
		// Line1 goes from origin A (p0) to with size (size.x - thickness, thickness)
		// Line2 goes from origin B (p1.x - thickness, p0.y) with size (thickness, size.y -
		// thickness)
		// Line3 goes from origin C (p0.x + thickness, p1.y - thickness) with size (size.x -
		// thickness, thickness)
		// Line4 goes from origin D(p0.x, p0.y + thickness) with size (thickness, size.y -
		// thickness)
		Vec2<float> p0 = position;
		Vec2<float> p1 = position + size;

		Vec2<float> A = {p0};
		Vec2<float> sizeA = {size.x - thickness, thickness};

		Vec2<float> B = {p1.x - thickness, p0.y};
		Vec2<float> sizeB = {thickness, size.y - thickness};

		Vec2<float> C = {p0.x + thickness, p1.y - thickness};
		Vec2<float> sizeC = {size.x - thickness, thickness};

		Vec2<float> D = {p0.x, p0.y + thickness};
		Vec2<float> sizeD = {thickness, size.y - thickness};

		this->drawFilledRect(A, sizeA, c);
		this->drawFilledRect(B, sizeB, c);
		this->drawFilledRect(C, sizeC, c);
		this->drawFilledRect(D, sizeD, c);
	}
	void flush() override
	{
		// Cleanup any outstanding destroyed texture or framebuffer objects
		{
			std::lock_guard<std::mutex> lock(this->destroyed_texture_list_mutex);

			for (auto &id : this->destroyed_texture_list)
			{
				gl20::DeleteTextures(1, &id);
			}
			this->destroyed_texture_list.clear();
		}
		{
			std::lock_guard<std::mutex> lock(this->destroyed_framebuffer_list_mutex);

			for (auto &id : this->destroyed_framebuffer_list)
			{
				gl20::DeleteFramebuffersEXT(1, &id);
			}
			this->destroyed_framebuffer_list.clear();
		}
	}
	UString getName() override { return "OGL2.0 Renderer"; }
	sp<Surface> getDefaultSurface() override { return this->defaultSurface; }

	void bindProgram(sp<Program> p)
	{
		if (this->currentBoundProgram == p->prog)
			return;
		gl20::UseProgram(p->prog);
		this->currentBoundProgram = p->prog;
	}
	void drawRgb(GLRGBImage &img, Vec2<float> offset, Vec2<float> size, Scaler scaler,
	             Vec2<float> rotationCenter = {0, 0}, float rotationAngleRadians = 0,
	             Colour tint = {255, 255, 255, 255})
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl20::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl20::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl20::NEAREST;
				break;
		}
		bindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY, tint);
		BindTexture t(img.texID);
		TexParam<gl20::TEXTURE_MAG_FILTER> mag(img.texID, filter);
		TexParam<gl20::TEXTURE_MIN_FILTER> min(img.texID, filter);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}}, rotationCenter, rotationAngleRadians);
		q.draw(rgbProgram->posLoc, rgbProgram->texcoordLoc);
	}
	void drawPalette(GLPaletteImage &img, Vec2<float> offset, Vec2<float> size,
	                 Colour tint = {255, 255, 255, 255})
	{
		bindProgram(paletteProgram);
		Rect<float> pos(offset, offset + size);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteProgram->setUniforms(this->currentSurface->size, flipY, tint);
		BindTexture t(img.texID, 0);

		BindTexture p(
		    static_cast<GLPalette *>(this->currentPalette->rendererPrivateData.get())->texID, 1);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}});
		q.draw(paletteProgram->posLoc, paletteProgram->texcoordLoc);
	}

	void drawSurface(FBOData &fbo, Vec2<float> offset, Vec2<float> size, Scaler scaler,
	                 Colour tint = {255, 255, 255, 255})
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl20::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl20::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl20::NEAREST;
				break;
		}
		bindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY, tint);
		BindTexture t(fbo.tex);
		TexParam<gl20::TEXTURE_MAG_FILTER> mag(fbo.tex, filter);
		TexParam<gl20::TEXTURE_MIN_FILTER> min(fbo.tex, filter);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}});
		q.draw(rgbProgram->posLoc, rgbProgram->texcoordLoc);
	}

	void drawLine(Vec2<float> p0, Vec2<float> p1, Colour c, float thickness) override
	{
		bindProgram(colourProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(this->currentSurface->size, flipY, c);
		Line l(p0, p1, thickness);
		l.draw(colourProgram->posLoc);
	}
	// These can be called from any thread - e.g. from the Image destructors
	void delete_texture_object(GLuint id)
	{
		// If we're already on the bound thread, just immediately destroy
		if (this->bound_thread == std::this_thread::get_id())
		{
			gl20::DeleteTextures(1, &id);
			return;
		}
		// Otherwise add it to a list for future destruction
		{
			std::lock_guard<std::mutex> lock(this->destroyed_texture_list_mutex);
			this->destroyed_texture_list.push_back(id);
		}
	}
	void delete_framebuffer_object(GLuint id)
	{
		// If we're already on the bound thread, just immediately destroy
		if (this->bound_thread == std::this_thread::get_id())
		{
			gl20::DeleteFramebuffersEXT(1, &id);
			return;
		}
		// Otherwise add it to a list for future destruction
		{
			std::lock_guard<std::mutex> lock(this->destroyed_framebuffer_list_mutex);
			this->destroyed_framebuffer_list.push_back(id);
		}
	}
};

class OGL20RendererFactory : public OpenApoc::RendererFactory
{
	bool alreadyInitialised;
	bool functionLoadSuccess;

  public:
	OGL20RendererFactory() : alreadyInitialised(false), functionLoadSuccess(false) {}
	OpenApoc::Renderer *create() override
	{
		if (!alreadyInitialised)
		{
			alreadyInitialised = true;
			auto success = gl20::sys::LoadFunctions();
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
			if (!gl20::sys::IsVersionGEQ(2, 0))
			{
				LogInfo("GL version not at least 2.0, got %d.%d", gl20::sys::GetMajorVersion(),
				        gl20::sys::GetMinorVersion());
				return nullptr;
			}
			functionLoadSuccess = true;
		}
		if (functionLoadSuccess)
		{
			return new OGL20Renderer{};
		}
		return nullptr;
	}
};

FBOData::~FBOData()
{
	if (renderer_dead)
	{
		LogWarning("FBOData being destroyed after renderer");
		return;
	}
	if (tex)
		owner->delete_texture_object(tex);
	if (fbo)
		owner->delete_framebuffer_object(fbo);
}
GLRGBImage::~GLRGBImage()
{
	if (renderer_dead)
	{
		LogWarning("GLRGBImage being destroyed after renderer");
		return;
	}
	owner->delete_texture_object(this->texID);
}
GLPalette::~GLPalette()
{
	if (renderer_dead)
	{
		LogWarning("GLPalette being destroyed after renderer");
		return;
	}
	owner->delete_texture_object(this->texID);
}
GLPaletteImage::~GLPaletteImage()
{
	if (renderer_dead)
	{
		LogWarning("GLPaletteImage being destroyed after renderer");
		return;
	}
	owner->delete_texture_object(this->texID);
}

} // anonymous namespace

namespace OpenApoc
{
RendererFactory *getGL20RendererFactory() { return new OGL20RendererFactory(); }
} // namespace OpenApoc
