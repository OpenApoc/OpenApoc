#include "framework/image.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "framework/renderer_interface.h"
#include "framework/trace.h"
#include "library/sp.h"
#include <array>
#include <memory>

/* Workaround MSVC not liking int64_t being defined here and in allegro */
#define GLEXT_64_TYPES_DEFINED
#include "framework/render/gl30/gl_3_0.hpp"
#include "framework/render/gl30/gl_3_0.inl"

namespace
{

using namespace OpenApoc;

class Program
{
  public:
	GLuint prog;
	static GLuint CreateShader(GLenum type, const UString source)
	{
		GLuint shader = gl30::CreateShader(type);
		auto sourceString = source.str();
		const GLchar *string = sourceString.c_str();
		GLint stringLength = sourceString.length();
		gl30::ShaderSource(shader, 1, &string, &stringLength);
		gl30::CompileShader(shader);
		GLint compileStatus;
		gl30::GetShaderiv(shader, gl30::COMPILE_STATUS, &compileStatus);
		if (compileStatus == gl30::TRUE_)
			return shader;

		GLint logLength;
		gl30::GetShaderiv(shader, gl30::INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		gl30::GetShaderInfoLog(shader, logLength, NULL, log.get());

		LogError("Shader compile error: %s", log.get());

		gl30::DeleteShader(shader);
		return 0;
	}
	Program(const UString vertexSource, const UString fragmentSource) : prog(0)
	{
		GLuint vShader = CreateShader(gl30::VERTEX_SHADER, vertexSource);
		if (!vShader)
		{
			LogError("Failed to compile vertex shader");
			return;
		}
		GLuint fShader = CreateShader(gl30::FRAGMENT_SHADER, fragmentSource);
		if (!fShader)
		{
			LogError("Failed to compile fragment shader");
			gl30::DeleteShader(vShader);
			return;
		}

		prog = gl30::CreateProgram();
		gl30::AttachShader(prog, vShader);
		gl30::AttachShader(prog, fShader);

		gl30::DeleteShader(vShader);
		gl30::DeleteShader(fShader);

		gl30::LinkProgram(prog);

		GLint linkStatus;
		gl30::GetProgramiv(prog, gl30::LINK_STATUS, &linkStatus);
		if (linkStatus == gl30::TRUE_)
			return;

		GLint logLength;
		gl30::GetProgramiv(prog, gl30::INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		gl30::GetProgramInfoLog(prog, logLength, NULL, log.get());

		LogError("Program link error: %s", log.get());

		gl30::DeleteProgram(prog);
		prog = 0;
		return;
	}

	void Uniform(GLuint loc, Colour c)
	{
		gl30::Uniform4f(loc, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	void Uniform(GLuint loc, Vec2<float> v) { gl30::Uniform2f(loc, v.x, v.y); }
	void Uniform(GLuint loc, Vec2<int> v)
	{
		// FIXME: Float conversion
		gl30::Uniform2f(loc, v.x, v.y);
	}
	void Uniform(GLuint loc, float v) { gl30::Uniform1f(loc, v); }
	void Uniform(GLuint loc, int v) { gl30::Uniform1i(loc, v); }

	void Uniform(GLuint loc, bool v) { gl30::Uniform1f(loc, (v ? 1.0f : 0.0f)); }

	virtual ~Program()
	{
		if (prog)
			gl30::DeleteProgram(prog);
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
	GLint tintLoc;
};
const char *RGBProgram_vertexSource = {
    "#version 130\n"
    "in vec2 position;\n"
    "in vec2 texcoord_in;\n"
    "out vec2 texcoord;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
    "  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
    "}\n"};
const char *RGBProgram_fragmentSource = {"#version 130\n"
                                         "in vec2 texcoord;\n"
                                         "uniform sampler2D tex;\n"
                                         "uniform vec4 tint;\n"
                                         "out vec4 out_colour;\n"
                                         "void main() {\n"
                                         " out_colour = tint * texture(tex, texcoord);\n"
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
		this->posLoc = gl30::GetAttribLocation(this->prog, "position");
		if (this->posLoc < 0)
			LogError("\"position\" attribute not found in shader");
		this->texcoordLoc = gl30::GetAttribLocation(this->prog, "texcoord_in");
		if (this->texcoordLoc < 0)
			LogError("\"texcoord_in\" attribute not found in shader");
		this->screenSizeLoc = gl30::GetUniformLocation(this->prog, "screenSize");
		if (this->screenSizeLoc < 0)
			LogError("\"screenSize\" uniform not found in shader");
		this->texLoc = gl30::GetUniformLocation(this->prog, "tex");
		if (this->texLoc < 0)
			LogError("\"tex\" uniform not found in shader");
		this->flipYLoc = gl30::GetUniformLocation(this->prog, "flipY");
		if (this->flipYLoc < 0)
			LogError("\"flipY\" uniform not found in shader");
		this->tintLoc = gl30::GetUniformLocation(this->prog, "tint");
		if (this->tintLoc < 0)
			LogError("\"tint\" uniform not found in shader");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0,
	                 Colour tint = {255, 255, 255, 255})
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
		if (tint != currentTint)
		{
			currentTint = tint;
			this->Uniform(this->tintLoc, tint);
		}
	}
};
const char *PaletteProgram_vertexSource = {
    "#version 130\n"
    "in vec2 position;\n"
    "in vec2 texcoord_in;\n"
    "out vec2 texcoord;\n"
    "flat out int sprite;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
    "  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
    "}\n"};
const char *PaletteProgram_fragmentSource = {
    "#version 130\n"
    "in vec2 texcoord;\n"
    "uniform isampler2D tex;\n"
    "uniform sampler2D pal;\n"
    "uniform vec4 tint;\n"
    "out vec4 out_colour;\n"
    "void main() {\n"
    " int idx = texelFetch(tex, ivec2(texcoord.x, texcoord.y),0).r;\n"
    " if (idx == 0) discard;\n"
    " out_colour = tint * texelFetch(pal, ivec2(idx,0),0);\n"
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
		this->posLoc = gl30::GetAttribLocation(this->prog, "position");
		this->texcoordLoc = gl30::GetAttribLocation(this->prog, "texcoord_in");
		this->screenSizeLoc = gl30::GetUniformLocation(this->prog, "screenSize");
		this->texLoc = gl30::GetUniformLocation(this->prog, "tex");
		this->palLoc = gl30::GetUniformLocation(this->prog, "pal");
		this->flipYLoc = gl30::GetUniformLocation(this->prog, "flipY");
		this->tintLoc = gl30::GetUniformLocation(this->prog, "tint");
		if (this->tintLoc < 0)
			LogError("\"tint\" uniform not found in shader");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1,
	                 Colour tint = {255, 255, 255, 255})
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
		if (tint != currentTint)
		{
			currentTint = tint;
			this->Uniform(this->tintLoc, tint);
		}
	}
};

const char *PaletteSetProgram_vertexSource = {
    "#version 130\n"
    "in vec2 position;\n"
    "in vec2 texcoord_in;\n"
    "in int sprite_in;\n"
    "out vec2 texcoord;\n"
    "flat out int sprite;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  sprite = sprite_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
    "  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
    "}\n"};
const char *PaletteSetProgram_fragmentSource = {
    "#version 130\n"
    "in vec2 texcoord;\n"
    "flat in int sprite;\n"
    "uniform isampler2DArray tex;\n"
    "uniform sampler2D pal;\n"
    "uniform vec4 tint;\n"
    "out vec4 out_colour;\n"
    "void main() {\n"
    " int idx = texelFetch(tex, ivec3(texcoord.x, texcoord.y, sprite), 0).r;\n"
    " if (idx == 0) discard;\n"
    " out_colour = tint * texelFetch(pal, ivec2(idx,0), 0);\n"
    "}\n"};
class PaletteSetProgram : public Program
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;
	GLint currentPalUnit;
	Colour currentTint;

  public:
	GLint posLoc;
	GLint texcoordLoc;
	GLint spriteLoc;
	GLint screenSizeLoc;
	GLint texLoc;
	GLint palLoc;
	GLint flipYLoc;
	GLint tintLoc;
	PaletteSetProgram()
	    : Program(PaletteSetProgram_vertexSource, PaletteSetProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(false), currentTexUnit(0), currentPalUnit(0),
	      currentTint(0, 0, 0, 0)
	{
		this->posLoc = gl30::GetAttribLocation(this->prog, "position");
		this->texcoordLoc = gl30::GetAttribLocation(this->prog, "texcoord_in");
		this->spriteLoc = gl30::GetAttribLocation(this->prog, "sprite_in");

		this->screenSizeLoc = gl30::GetUniformLocation(this->prog, "screenSize");
		this->texLoc = gl30::GetUniformLocation(this->prog, "tex");
		this->palLoc = gl30::GetUniformLocation(this->prog, "pal");
		this->flipYLoc = gl30::GetUniformLocation(this->prog, "flipY");
		this->tintLoc = gl30::GetUniformLocation(this->prog, "tint");
		if (this->tintLoc < 0)
			LogError("\"tint\" uniform not found in shader");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1,
	                 Colour tint = {255, 255, 255, 255})
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
		if (tint != currentTint)
		{
			currentTint = tint;
			this->Uniform(this->tintLoc, tint);
		}
	}
	class VertexDef
	{
	  public:
		VertexDef(Vec2<float> pos, Vec2<float> texcoords, int sprite)
		    : pos(pos), texcoords(texcoords), sprite(sprite)
		{
		}
		VertexDef() {}

		Vec2<float> pos;
		Vec2<float> texcoords;
		int sprite;
	};
	static_assert(sizeof(VertexDef) == 20, "VertexDef should be tightly packed");
	class SpriteDef
	{
	  public:
		SpriteDef(Vec2<float> offset, Vec2<float> size, int sprite)
		{
			v[0] = {offset, Vec2<float>{0, 0}, sprite};
			v[1] = {offset + size * Vec2<float>{0, 1}, Vec2<float>{0, 1}, sprite};
			v[2] = {offset + size * Vec2<float>{1, 0}, Vec2<float>{1, 0}, sprite};
			v[3] = {offset + size * Vec2<float>{1, 0}, Vec2<float>{1, 0}, sprite};
		}

		VertexDef v[4];
	};
	static_assert(sizeof(SpriteDef) == sizeof(VertexDef) * 4, "SpriteDef should be tightly packed");
};

const char *SolidColourProgram_vertexSource = {
    "#version 130\n"
    "in vec2 position;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
    "  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
    "}\n"};
const char *SolidColourProgram_fragmentSource = {"#version 130\n"
                                                 "uniform vec4 colour;\n"
                                                 "out vec4 out_colour;\n"
                                                 "void main() {\n"
                                                 " out_colour = colour;\n"
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
		this->posLoc = gl30::GetAttribLocation(this->prog, "position");
		this->screenSizeLoc = gl30::GetUniformLocation(this->prog, "screenSize");
		this->colourLoc = gl30::GetUniformLocation(this->prog, "colour");
		this->flipYLoc = gl30::GetUniformLocation(this->prog, "flipY");
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
	Quad(const Rect<float> &position, const Rect<float> texCoords,
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
		gl30::EnableVertexAttribArray(vertexAttribPos);
		gl30::VertexAttribPointer(vertexAttribPos, 2, gl30::FLOAT, gl30::FALSE_, 0, &vertices);
		gl30::EnableVertexAttribArray(texcoordAttribPos);
		gl30::VertexAttribPointer(texcoordAttribPos, 2, gl30::FLOAT, gl30::FALSE_, 0, &texcoords);
		gl30::DrawArrays(gl30::TRIANGLE_STRIP, 0, 4);
	}
	void draw(GLuint vertexAttribPos)
	{
		gl30::EnableVertexAttribArray(vertexAttribPos);
		gl30::VertexAttribPointer(vertexAttribPos, 2, gl30::FLOAT, gl30::FALSE_, 0, &vertices);
		gl30::DrawArrays(gl30::TRIANGLE_STRIP, 0, 4);
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
		gl30::LineWidth(thickness);
		gl30::EnableVertexAttribArray(vertexAttribPos);
		gl30::VertexAttribPointer(vertexAttribPos, 2, gl30::FLOAT, gl30::FALSE_, 0, &vertices);
		gl30::DrawArrays(gl30::LINES, 0, 2);
	}
};
class ActiveTexture
{
	ActiveTexture(const ActiveTexture &) = delete;

  public:
	static GLenum getUnitEnum(int unit) { return gl30::TEXTURE0 + unit; }

	ActiveTexture(int unit)
	{
		GLenum prevUnit;
		gl30::GetIntegerv(gl30::ACTIVE_TEXTURE, reinterpret_cast<GLint *>(&prevUnit));
		if (prevUnit == getUnitEnum(unit))
		{
			return;
		}
		gl30::ActiveTexture(getUnitEnum(unit));
	}
};

class UnpackAlignment
{
	UnpackAlignment(const UnpackAlignment &) = delete;

  public:
	UnpackAlignment(int align)
	{
		GLint prevAlign;
		gl30::GetIntegerv(gl30::UNPACK_ALIGNMENT, &prevAlign);
		if (prevAlign == align)
		{
			return;
		}
		gl30::PixelStorei(gl30::UNPACK_ALIGNMENT, align);
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
			case gl30::TEXTURE_1D:
				return gl30::TEXTURE_BINDING_1D;
			case gl30::TEXTURE_2D:
				return gl30::TEXTURE_BINDING_2D;
			case gl30::TEXTURE_3D:
				return gl30::TEXTURE_BINDING_3D;
			case gl30::TEXTURE_2D_ARRAY:
				return gl30::TEXTURE_BINDING_2D_ARRAY;
			default:
				LogError("Unknown texture enum %d", static_cast<int>(e));
				return gl30::TEXTURE_BINDING_2D;
		}
	}
	BindTexture(GLuint id, GLint unit = 0, GLenum bind = gl30::TEXTURE_2D) : bind(bind), unit(unit)
	{
		ActiveTexture a(unit);
		GLuint prevID;
		gl30::GetIntegerv(getBindEnum(bind), reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		gl30::BindTexture(bind, id);
	}
};

template <GLenum param> class TexParam
{
	TexParam(const TexParam &) = delete;

  public:
	GLuint id;
	GLenum type;
	bool nop;

	TexParam(GLuint id, GLint value, GLenum type = gl30::TEXTURE_2D) : id(id), type(type)
	{
		GLint prevValue;
		BindTexture b(id, 0, type);
		gl30::GetTexParameteriv(type, param, &prevValue);
		if (prevValue == value)
		{
			return;
		}
		gl30::TexParameteri(type, param, value);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;

  public:
	BindFramebuffer(GLuint id)
	{
		GLuint prevID;
		gl30::GetIntegerv(gl30::DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		gl30::BindFramebuffer(gl30::DRAW_FRAMEBUFFER, id);
	}
};

class FBOData : public RendererImageData
{
  public:
	GLuint fbo;
	GLuint tex;
	GLuint depthBuffer;
	Vec2<float> size;
	// Constructor /only/ to be used for default surface (FBO ID == 0)
	FBOData(GLuint fbo, Vec2<int> size)
	    // FIXME: Check FBO == 0
	    // FIXME: Warn if trying to texture from FBO 0
	    : fbo(fbo),
	      tex(-1),
	      size(size)
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
			gl30::ReadPixels(0, size.y - 1 - y, size.x, size.y - y, gl30::RGBA, gl30::UNSIGNED_BYTE,
			                 imgPos);
			imgPos += imgStride;
		}

		return img;
	}

	FBOData(Vec2<int> size) : size(size.x, size.y)
	{
		TRACE_FN;
		gl30::GenTextures(1, &this->tex);
		BindTexture b(this->tex);
		gl30::TexImage2D(gl30::TEXTURE_2D, 0, gl30::RGBA8, size.x, size.y, 0, gl30::RGBA,
		                 gl30::UNSIGNED_BYTE, NULL);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MIN_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MAG_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_S, gl30::CLAMP_TO_EDGE);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_T, gl30::CLAMP_TO_EDGE);

		gl30::GenRenderbuffers(1, &depthBuffer);
		gl30::BindRenderbuffer(gl30::RENDERBUFFER, depthBuffer);
		gl30::RenderbufferStorage(gl30::RENDERBUFFER, gl30::DEPTH_COMPONENT24, size.x, size.y);

		gl30::GenFramebuffers(1, &this->fbo);
		BindFramebuffer f(this->fbo);

		gl30::FramebufferTexture2D(gl30::DRAW_FRAMEBUFFER, gl30::COLOR_ATTACHMENT0,
		                           gl30::TEXTURE_2D, this->tex, 0);
		gl30::FramebufferRenderbuffer(gl30::DRAW_FRAMEBUFFER, gl30::DEPTH_ATTACHMENT,
		                              gl30::RENDERBUFFER, depthBuffer);
		LogAssert(gl30::CheckFramebufferStatus(gl30::DRAW_FRAMEBUFFER) ==
		          gl30::FRAMEBUFFER_COMPLETE);
	}
	virtual ~FBOData()
	{
		if (tex)
			gl30::DeleteTextures(1, &tex);
		if (depthBuffer)
			gl30::DeleteRenderbuffers(1, &depthBuffer);
		if (fbo)
			gl30::DeleteFramebuffers(1, &fbo);
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
		TRACE_FN;
		RGBImageLock l(parent, ImageLockUse::Read);
		gl30::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MIN_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MAG_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_S, gl30::CLAMP_TO_EDGE);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_T, gl30::CLAMP_TO_EDGE);
		gl30::TexImage2D(gl30::TEXTURE_2D, 0, gl30::RGBA, parent->size.x, parent->size.y, 0,
		                 gl30::RGBA, gl30::UNSIGNED_BYTE, l.getData());
	}
	virtual ~GLRGBImage() { gl30::DeleteTextures(1, &this->texID); }
};

class GLPalette : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<Palette> parent;
	GLPalette(sp<Palette> parent) : size(Vec2<float>(parent->colours.size(), 1)), parent(parent)
	{
		TRACE_FN;
		gl30::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MIN_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MAG_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_S, gl30::CLAMP_TO_EDGE);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_T, gl30::CLAMP_TO_EDGE);
		gl30::TexImage2D(gl30::TEXTURE_2D, 0, gl30::RGBA, parent->colours.size(), 1, 0, gl30::RGBA,
		                 gl30::UNSIGNED_BYTE, parent->colours.data());
	}
	virtual ~GLPalette() { gl30::DeleteTextures(1, &this->texID); }
};

class GLPaletteImage : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<PaletteImage> parent;
	GLPaletteImage(sp<PaletteImage> parent) : size(parent->size), parent(parent)
	{
		TRACE_FN;
		PaletteImageLock l(parent, ImageLockUse::Read);
		gl30::GenTextures(1, &this->texID);
		BindTexture b(this->texID);
		UnpackAlignment align(1);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MIN_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_MAG_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_S, gl30::CLAMP_TO_EDGE);
		gl30::TexParameteri(gl30::TEXTURE_2D, gl30::TEXTURE_WRAP_T, gl30::CLAMP_TO_EDGE);
		gl30::TexImage2D(gl30::TEXTURE_2D, 0, gl30::R8UI, parent->size.x, parent->size.y, 0,
		                 gl30::RED_INTEGER, gl30::UNSIGNED_BYTE, l.getData());
	}
	virtual ~GLPaletteImage() { gl30::DeleteTextures(1, &this->texID); }
};

class GLPaletteSpritesheet : public RendererImageData
{
  public:
	std::weak_ptr<ImageSet> parent;
	Vec2<int> maxSize;
	unsigned numSprites;
	GLuint texID;
	GLPaletteSpritesheet(sp<ImageSet> parent)
	    : parent(parent), maxSize(parent->maxSize), numSprites(parent->images.size())
	{
		TRACE_FN;
		gl30::GenTextures(1, &this->texID);
		BindTexture b(this->texID, 0, gl30::TEXTURE_2D_ARRAY);
		UnpackAlignment align(1);
		gl30::TexParameteri(gl30::TEXTURE_2D_ARRAY, gl30::TEXTURE_MIN_FILTER, gl30::NEAREST);
		gl30::TexParameteri(gl30::TEXTURE_2D_ARRAY, gl30::TEXTURE_MAG_FILTER, gl30::NEAREST);
		gl30::TexImage3D(gl30::TEXTURE_2D_ARRAY, 0, gl30::R8UI, maxSize.x, maxSize.y, numSprites, 0,
		                 gl30::RED_INTEGER, gl30::UNSIGNED_BYTE, NULL);

		std::unique_ptr<char[]> zeros(new char[maxSize.x * maxSize.y]);
		memset(zeros.get(), 1, maxSize.x * maxSize.y);

		LogInfo("Uploading %d sprites in {%d,%d} spritesheet", numSprites, maxSize.x, maxSize.y);

		for (unsigned int i = 0; i < numSprites; i++)
		{
			sp<PaletteImage> img = std::dynamic_pointer_cast<PaletteImage>(parent->images[i]);
			// FIXME: HACK - better way of clearing undefined portions to '0'?
			gl30::TexSubImage3D(gl30::TEXTURE_2D_ARRAY, 0, 0, 0, i, maxSize.x, maxSize.y, 1,
			                    gl30::RED_INTEGER, gl30::UNSIGNED_BYTE, zeros.get());

			PaletteImageLock l(img, ImageLockUse::Read);

			gl30::TexSubImage3D(gl30::TEXTURE_2D_ARRAY, 0, 0, 0, i, img->size.x, img->size.y, 1,
			                    gl30::RED_INTEGER, gl30::UNSIGNED_BYTE, l.getData());
		}
		LogInfo("Uploading spritesheet complete");
	}
	virtual ~GLPaletteSpritesheet() { gl30::DeleteTextures(1, &this->texID); }
};

class OGL30Renderer : public Renderer
{
  private:
	enum class RendererState
	{
		Idle,
		BatchingSpritesheet,
	};
	RendererState state;
	sp<RGBProgram> rgbProgram;
	sp<SolidColourProgram> colourProgram;
	sp<PaletteProgram> paletteProgram;
	sp<PaletteSetProgram> paletteSetProgram;
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
			s->rendererPrivateData.reset(new FBOData(s->size));

		FBOData *fbo = static_cast<FBOData *>(s->rendererPrivateData.get());
		gl30::BindFramebuffer(gl30::FRAMEBUFFER, fbo->fbo);
		this->currentBoundFBO = fbo->fbo;
		gl30::Viewport(0, 0, s->size.x, s->size.y);
	}
	sp<Surface> getSurface() override { return currentSurface; }
	sp<Surface> defaultSurface;

  public:
	OGL30Renderer();
	virtual ~OGL30Renderer();
	void clear(Colour c = Colour{0, 0, 0, 0}) override;
	void setPalette(sp<Palette> p) override
	{
		if (p == this->currentPalette)
			return;
		this->flush();
		if (!p->rendererPrivateData)
			p->rendererPrivateData.reset(new GLPalette(p));
		this->currentPalette = p;
	}
	sp<Palette> getPalette() override { return this->currentPalette; }

	void draw(sp<Image> i, Vec2<float> position) override;
	void drawRotated(sp<Image> image, Vec2<float> center, Vec2<float> position,
	                 float angle) override
	{
		auto size = image->size;
		if (this->state != RendererState::Idle)
			this->flush();
		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				image->rendererPrivateData.reset(img);
			}
			DrawRGB(*img, position, size, Scaler::Linear, center, angle);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		LogError("Unsupported image type");
	}
	void drawScaled(sp<Image> image, Vec2<float> position, Vec2<float> size,
	                Scaler scaler = Scaler::Linear) override
	{

		if (this->state != RendererState::Idle)
			this->flush();
		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				image->rendererPrivateData.reset(img);
			}
			DrawRGB(*img, position, size, scaler);
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
			DrawPalette(*img, position, size);
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
			DrawSurface(*fbo, position, size, scaler);
			return;
		}
		LogError("Unsupported image type");
	}
	void drawTinted(sp<Image> i, Vec2<float> position, Colour tint) override
	{
		auto scaler = Scaler::Linear;
		auto size = i->size;
		// Tint is program state (uniform) so can't be batched
		if (this->state != RendererState::Idle)
			this->flush();
		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(i);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				i->rendererPrivateData.reset(img);
			}
			DrawRGB(*img, position, size, scaler, {0, 0}, 0, tint);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(i);
		if (paletteImage)
		{
			GLPaletteImage *img =
			    dynamic_cast<GLPaletteImage *>(paletteImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLPaletteImage(paletteImage);
				i->rendererPrivateData.reset(img);
			}
			if (scaler != Scaler::Nearest)
			{
				// blending indices doesn't make sense. You'll have to render
				// it to an RGB surface then scale that
				LogError("Only nearest scaler is supported on paletted images");
			}
			DrawPalette(*img, position, size, tint);
			return;
		}

		sp<Surface> surface = std::dynamic_pointer_cast<Surface>(i);
		if (surface)
		{
			FBOData *fbo = dynamic_cast<FBOData *>(surface->rendererPrivateData.get());
			if (!fbo)
			{
				fbo = new FBOData(i->size);
				i->rendererPrivateData.reset(fbo);
			}
			DrawSurface(*fbo, position, size, scaler, tint);
			return;
		}
		LogError("Unsupported image type");
	}
	void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c) override;
	void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0) override
	{

		// The lines are all shifted in x/y by {capsize} to ensure the corners are correcly covered
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
	void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0) override
	{
		if (this->state != RendererState::Idle)
			this->flush();
		this->DrawLine(p1, p2, c, thickness);
	}
	void flush() override;
	UString getName() override;
	sp<Surface> getDefaultSurface() override { return this->defaultSurface; }

	void BindProgram(sp<Program> p)
	{
		if (this->currentBoundProgram == p->prog)
			return;
		gl30::UseProgram(p->prog);
		this->currentBoundProgram = p->prog;
	}

	void DrawRGB(GLRGBImage &img, Vec2<float> offset, Vec2<float> size, Scaler scaler,
	             Vec2<float> rotationCenter = {0, 0}, float rotationAngleRadians = 0,
	             Colour tint = {255, 255, 255, 255})
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl30::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl30::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl30::NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY, 0, tint);
		BindTexture t(img.texID);
		TexParam<gl30::TEXTURE_MAG_FILTER> mag(img.texID, filter);
		TexParam<gl30::TEXTURE_MIN_FILTER> min(img.texID, filter);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}}, rotationCenter, rotationAngleRadians);
		q.draw(rgbProgram->posLoc, rgbProgram->texcoordLoc);
	}

	void DrawPalette(GLPaletteImage &img, Vec2<float> offset, Vec2<float> size,
	                 Colour tint = {255, 255, 255, 255})
	{
		BindProgram(paletteProgram);
		Rect<float> pos(offset, offset + size);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteProgram->setUniforms(this->currentSurface->size, flipY, 0, 1, tint);
		BindTexture t(img.texID, 0);

		BindTexture p(
		    static_cast<GLPalette *>(this->currentPalette->rendererPrivateData.get())->texID, 1);
		Quad q(pos, Rect<float>{{0, 0}, {img.size}});
		q.draw(paletteProgram->posLoc, paletteProgram->texcoordLoc);
	}

	void DrawSurface(FBOData &fbo, Vec2<float> offset, Vec2<float> size, Scaler scaler,
	                 Colour tint = {255, 255, 255, 255})
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl30::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl30::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl30::NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY, 0, tint);
		BindTexture t(fbo.tex);
		TexParam<gl30::TEXTURE_MAG_FILTER> mag(fbo.tex, filter);
		TexParam<gl30::TEXTURE_MIN_FILTER> min(fbo.tex, filter);
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
		Quad q(pos, Rect<float>{{0, 0}, {0, 0}});
		q.draw(colourProgram->posLoc);
	}

	void DrawLine(Vec2<float> p0, Vec2<float> p1, Colour c, float thickness)
	{
		BindProgram(colourProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(this->currentSurface->size, flipY, c);
		Line l(p0 + Vec2<float>{0.5, 0.5}, p1 + Vec2<float>{0.5, 0.5}, thickness);
		l.draw(colourProgram->posLoc);
	}

	class BatchedVertex
	{
	  public:
		Vec2<float> position;
		Vec2<float> texCoord;
		int spriteIdx;
		BatchedVertex() {}
		BatchedVertex(Vec2<float> p, Vec2<float> tc, int i)
		    : position(p), texCoord(tc), spriteIdx(i)
		{
		}
	};
	static_assert(sizeof(BatchedVertex) == 20, "BatchedVertex unexpected size");

	class BatchedSprite
	{
	  public:
		std::array<BatchedVertex, 4> vertices;
		BatchedSprite(Vec2<float> screenPosition, Vec2<float> spriteSize, int spriteIdx)
		{
			Vec2<float> maxTexCoords = spriteSize;
			Vec2<float> maxPosition = screenPosition + spriteSize;
			vertices[0] = BatchedVertex{screenPosition, Vec2<float>{0, 0}, spriteIdx};
			vertices[1] = BatchedVertex{Vec2<float>{screenPosition.x, maxPosition.y},
			                            Vec2<float>{0, maxTexCoords.y}, spriteIdx};
			vertices[2] = BatchedVertex{Vec2<float>{maxPosition.x, screenPosition.y},
			                            Vec2<float>{maxTexCoords.x, 0}, spriteIdx};
			vertices[3] = BatchedVertex{maxPosition, maxTexCoords, spriteIdx};
		}
	};
	static_assert(sizeof(BatchedSprite) == sizeof(BatchedVertex) * 4,
	              "BatchedSprite unexpected size");

	std::vector<BatchedSprite> batchedSprites;
	unsigned maxBatchedSprites;
	unsigned maxSpritesheetSize;
	sp<GLPaletteSpritesheet> boundSpritesheet;

	std::unique_ptr<GLint[]> firstList;
	std::unique_ptr<GLsizei[]> countList;

	void DrawBatchedSpritesheet()
	{
		BindProgram(paletteSetProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteSetProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(this->boundSpritesheet->texID, 0, gl30::TEXTURE_2D_ARRAY);
		BindTexture p(
		    static_cast<GLPalette *>(this->currentPalette->rendererPrivateData.get())->texID, 1);

		gl30::EnableVertexAttribArray(paletteSetProgram->posLoc);
		gl30::EnableVertexAttribArray(paletteSetProgram->texcoordLoc);
		gl30::EnableVertexAttribArray(paletteSetProgram->spriteLoc);

		const char *vertexPtr = reinterpret_cast<const char *>(this->batchedSprites.data());

		gl30::VertexAttribPointer(paletteSetProgram->posLoc, 2, gl30::FLOAT, gl30::FALSE_,
		                          sizeof(BatchedVertex),
		                          vertexPtr + offsetof(BatchedVertex, position));
		gl30::VertexAttribPointer(paletteSetProgram->texcoordLoc, 2, gl30::FLOAT, gl30::FALSE_,
		                          sizeof(BatchedVertex),
		                          vertexPtr + offsetof(BatchedVertex, texCoord));
		gl30::VertexAttribIPointer(paletteSetProgram->spriteLoc, 1, gl30::INT,
		                           sizeof(BatchedVertex),
		                           vertexPtr + offsetof(BatchedVertex, spriteIdx));

		gl30::MultiDrawArrays(gl30::TRIANGLE_STRIP, this->firstList.get(), this->countList.get(),
		                      this->batchedSprites.size());

		this->batchedSprites.clear();
		this->state = RendererState::Idle;
	}
};

OGL30Renderer::OGL30Renderer()
    : state(RendererState::Idle), rgbProgram(new RGBProgram()),
      colourProgram(new SolidColourProgram()), paletteProgram(new PaletteProgram()),
      paletteSetProgram(new PaletteSetProgram()), currentBoundProgram(0), currentBoundFBO(0)
{
	GLint viewport[4];
	gl30::GetIntegerv(gl30::VIEWPORT, viewport);
	LogInfo("Viewport {%d,%d,%d,%d}", viewport[0], viewport[1], viewport[2], viewport[3]);
	LogAssert(viewport[0] == 0 && viewport[1] == 0);
	this->defaultSurface = mksp<Surface>(Vec2<int>{viewport[2], viewport[3]});
	this->defaultSurface->rendererPrivateData.reset(new FBOData(0, {viewport[2], viewport[3]}));
	this->currentSurface = this->defaultSurface;

	GLint maxTexArrayLayers;
	gl30::GetIntegerv(gl30::MAX_ARRAY_TEXTURE_LAYERS, &maxTexArrayLayers);
	LogInfo("MAX_ARRAY_TEXTURE_LAYERS: %d", maxTexArrayLayers);
	this->maxBatchedSprites = 2048;
	this->maxSpritesheetSize = maxTexArrayLayers;

	this->firstList.reset(new GLint[this->maxBatchedSprites]);
	this->countList.reset(new GLsizei[this->maxBatchedSprites]);

	for (unsigned int i = 0; i < this->maxBatchedSprites; i++)
	{
		this->firstList[i] = 4 * i;
		this->countList[i] = 4;
	}

	GLint maxTexUnits;
	gl30::GetIntegerv(gl30::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
	LogInfo("MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d", maxTexUnits);
	gl30::Enable(gl30::BLEND);
	gl30::BlendFunc(gl30::SRC_ALPHA, gl30::ONE_MINUS_SRC_ALPHA);
}

OGL30Renderer::~OGL30Renderer() {}

void OGL30Renderer::clear(Colour c)
{
	this->flush();
	gl30::ClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	gl30::Clear(gl30::COLOR_BUFFER_BIT);
}

void OGL30Renderer::draw(sp<Image> image, Vec2<float> position)
{
	sp<ImageSet> owningSet = image->owningSet.lock();
	if (owningSet)
	{
		if (owningSet->images.size() > maxSpritesheetSize)
		{
			static bool warnonce = false;
			if (!warnonce)
			{
				warnonce = true;
				LogWarning("Spritesheet size %zu would be over max array size %d - falling back to "
				           "'slow' path",
				           owningSet->images.size(), maxSpritesheetSize);
			}
		}
		else
		{
			sp<GLPaletteSpritesheet> ss =
			    std::dynamic_pointer_cast<GLPaletteSpritesheet>(owningSet->rendererPrivateData);
			if (!ss)
			{
				ss = mksp<GLPaletteSpritesheet>(owningSet);
				owningSet->rendererPrivateData = ss;
			}
			switch (this->state)
			{
				default:
					this->flush();
				case RendererState::BatchingSpritesheet:
					if (ss != this->boundSpritesheet ||
					    this->batchedSprites.size() >= this->maxBatchedSprites)
					{
						this->flush();
					}
				case RendererState::Idle:
					break;
			}
			this->boundSpritesheet = ss;
			this->state = RendererState::BatchingSpritesheet;
			this->batchedSprites.emplace_back(position, Vec2<float>(image->size.x, image->size.y),
			                                  image->indexInSet);
			return;
		}
	}
	drawScaled(image, position, image->size, Scaler::Nearest);
}
void OGL30Renderer::drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c)
{
	this->flush();
	DrawRect(position, size, c);
}

void OGL30Renderer::flush()
{
	switch (this->state)
	{
		case RendererState::Idle:
			break;
		case RendererState::BatchingSpritesheet:
			this->DrawBatchedSpritesheet();
			break;
	}
	this->state = RendererState::Idle;
}

UString OGL30Renderer::getName() { return "OGL3.0 Renderer"; }

class OGL30RendererFactory : public OpenApoc::RendererFactory
{
	bool alreadyInitialised;
	bool functionLoadSuccess;

  public:
	OGL30RendererFactory() : alreadyInitialised(false), functionLoadSuccess(false) {}
	virtual OpenApoc::Renderer *create() override
	{
		if (!alreadyInitialised)
		{
			alreadyInitialised = true;
			auto success = gl30::sys::LoadFunctions();
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
			if (!gl30::sys::IsVersionGEQ(3, 0))
			{
				LogInfo("GL version not at least 3.0, got %d.%d", gl30::sys::GetMajorVersion(),
				        gl30::sys::GetMinorVersion());
				return nullptr;
			}
			functionLoadSuccess = true;
		}
		if (functionLoadSuccess)
			return new OGL30Renderer();
		return nullptr;
	}
};

}; // anonymous namespace

namespace OpenApoc
{
RendererFactory *getGL30RendererFactory() { return new OGL30RendererFactory(); }
} // namespace OpenApoc
