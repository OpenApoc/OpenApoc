#include "framework/image.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "framework/renderer.h"
#include "framework/renderer_interface.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <glm/gtx/rotate_vector.hpp>
#include <list>
#include <mutex>
#include <thread>

#define GLESWRAP_GLES3
#include "framework/render/gles30_v2/gleswrap.h"

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT LogAssert
#include "framework/render/gles30_v2/stb_rect_pack.h"

namespace OpenApoc
{
namespace
{

static std::atomic<bool> renderer_dead = true;

// Forward declaration needed for RendererImageData
class OGLES30Renderer;

using GL = gles_wrap::Gles3;

static up<GL> gl;

static const auto RGB_IMAGE_TEX_SLOT = GL::TEXTURE0;
static const int RGB_IMAGE_TEX_IDX = 0;
static const auto PALETTE_IMAGE_TEX_SLOT = GL::TEXTURE1;
static const int PALETTE_IMAGE_TEX_IDX = 1;
static const auto PALETTE_TEX_SLOT = GL::TEXTURE2;
static const int PALETTE_TEX_IDX = 2;

static const auto SCRATCH_TEX_SLOT = GL::TEXTURE3;

GL::GLuint CreateShader(GL::GLenum type, const UString &source)
{
	GL::GLuint shader = gl->CreateShader(type);
	auto sourceString = source;
	const GL::GLchar *string = sourceString.c_str();
	GL::GLint stringLength = sourceString.length();
	gl->ShaderSource(shader, 1, &string, &stringLength);
	gl->CompileShader(shader);
	GL::GLint compileStatus;
	gl->GetShaderiv(shader, GL::COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL::TRUE)
		return shader;

	GL::GLint logLength;
	gl->GetShaderiv(shader, GL::INFO_LOG_LENGTH, &logLength);

	std::unique_ptr<char[]> log(new char[logLength]);
	gl->GetShaderInfoLog(shader, logLength, NULL, log.get());

	LogError("Shader compile error: %s", log.get());

	gl->DeleteShader(shader);
	return 0;
}

GL::GLuint CompileProgram(const UString &vertexSource, const UString &fragmentSource)
{
	GL::GLuint prog = 0;
	GL::GLuint vShader = CreateShader(GL::VERTEX_SHADER, vertexSource);
	if (!vShader)
	{
		LogError("Failed to compile vertex shader");
		return 0;
	}
	GL::GLuint fShader = CreateShader(GL::FRAGMENT_SHADER, fragmentSource);
	if (!fShader)
	{
		LogError("Failed to compile fragment shader");
		gl->DeleteShader(vShader);
		return 0;
	}

	prog = gl->CreateProgram();
	gl->AttachShader(prog, vShader);
	gl->AttachShader(prog, fShader);

	gl->DeleteShader(vShader);
	gl->DeleteShader(fShader);

	gl->LinkProgram(prog);

	GL::GLint linkStatus;
	gl->GetProgramiv(prog, GL::LINK_STATUS, &linkStatus);
	if (linkStatus == GL::TRUE)
		return prog;

	GL::GLint logLength;
	gl->GetProgramiv(prog, GL::INFO_LOG_LENGTH, &logLength);

	std::unique_ptr<char[]> log(new char[logLength]);
	gl->GetProgramInfoLog(prog, logLength, NULL, log.get());

	LogError("Program link error: %s", log.get());

	gl->DeleteProgram(prog);
	return 0;
}

struct SpriteDescription
{
	int32_t uses_palette; // Used as a bool
	int32_t page;
	Vec2<float> spritesheet_position;
	Vec2<float> spritesheet_size;
	Vec2<float> screen_position;
	Vec2<float> screen_size;
	Colour tint;
};

class SpritesheetEntry final : public RendererImageData
{
  public:
	SpritesheetEntry(Vec2<int> size, sp<Image> parent)
	    : parent(parent), position({-1, -1}), size(size), page(-1)
	{
	}
	wp<Image> parent;
	Vec2<int> position;
	Vec2<int> size;
	// page == -1 means it's not yet packed into a spritesheet
	int page;
	~SpritesheetEntry() override = default;
};

class SpritesheetPage
{
  private:
	up<stbrp_node[]> pack_nodes;
	stbrp_context pack_context;
	int page_no;

  public:
	SpritesheetPage(int page_no, Vec2<int> size, int node_count) : page_no(page_no)
	{
		LogAssert(node_count > 0);
		LogAssert(size.x > 0);
		LogAssert(size.y > 0);
		pack_nodes.reset(new stbrp_node[node_count]);
		stbrp_init_target(&pack_context, size.x, size.y, pack_nodes.get(), node_count);
		stbrp_setup_heuristic(&pack_context, STBRP__INIT_skyline);
	}

	void addMultiple(std::vector<sp<SpritesheetEntry>> &entries)
	{
		up<stbrp_rect[]> rects(new stbrp_rect[entries.size()]);
		for (unsigned int i = 0; i < entries.size(); i++)
		{
			LogAssert(entries[i]->page == -1);
			rects[i].id = i;
			rects[i].w = entries[i]->size.x;
			rects[i].h = entries[i]->size.y;
		}

		stbrp_pack_rects(&pack_context, rects.get(), entries.size());

		for (unsigned int i = 0; i < entries.size(); i++)
		{
			if (rects[i].was_packed)
			{
				// It might re-order the input array, so use the 'id' not the index to get the
				// entry[]
				int idx = rects[i].id;
				entries[idx]->page = this->page_no;
				entries[idx]->position = {rects[i].x, rects[i].y};
			}
		}
	}

	bool addEntry(sp<SpritesheetEntry> entry)
	{
		LogAssert(entry->page == -1);
		stbrp_rect r;
		r.w = entry->size.x;
		r.h = entry->size.y;
		stbrp_pack_rects(&pack_context, &r, 1);
		if (r.was_packed)
		{
			entry->position = {r.x, r.y};
			entry->page = this->page_no;
			this->entries.push_back(entry);
			return true;
		}
		return false;
	}

	std::list<wp<SpritesheetEntry>> entries;
};

class Spritesheet
{
	// FIXME: Magic number? I guess it's the max num rects we can fit in a spritesheet (used by
	// stb_rect_pack)
	const int node_count = 4096;

  public:
	std::vector<sp<SpritesheetPage>> pages;
	Vec2<int> page_size;
	GL::GLenum format;
	GL::GLuint tex_id;
	Spritesheet(Vec2<int> page_size, GL::GLenum format)
	    : page_size(page_size), format(format), tex_id(0)
	{
	}
	~Spritesheet()
	{
		if (tex_id)
			gl->DeleteTextures(1, &tex_id);
	}
	void reuploadTextures()
	{
		if (tex_id)
			gl->DeleteTextures(1, &this->tex_id);
		gl->GenTextures(1, &this->tex_id);
		gl->ActiveTexture(SCRATCH_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D_ARRAY, this->tex_id);
		gl->TexParameteri(GL::TEXTURE_2D_ARRAY, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D_ARRAY, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D_ARRAY, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
		gl->TexParameteri(GL::TEXTURE_2D_ARRAY, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

		GL::GLenum data_format = GL::RGBA;
		if (format == GL::RGBA8)
			data_format = GL::RGBA;
		else if (format == GL::R8UI)
			data_format = GL::RED_INTEGER;
		else
			LogError("Unknown GL internal format 0x%x", format);

		gl->TexImage3D(GL::TEXTURE_2D_ARRAY, 0, this->format, this->page_size.x, this->page_size.y,
		               this->pages.size(), 0, data_format, GL::UNSIGNED_BYTE, nullptr);

		for (auto &page : this->pages)
		{
			for (auto &entry : page->entries)
			{
				auto entryPtr = entry.lock();
				// It's possible there are some images that have been dropped since packing - as
				// there doesn't seem to be a good way of 'removing' stb_rect_pack nodes to re-use
				// the area, don't upload it but leave the entry (can be removed at the next 'full'
				// repack)
				if (!entryPtr)
					continue;
				this->upload(entryPtr);
			}
		}
	}
	void upload(sp<SpritesheetEntry> entry)
	{
		LogAssert(entry->page >= 0);
		LogAssert(entry->page < (int)this->pages.size());
		auto image = entry->parent.lock();
		if (!image)
		{
			LogError("Spritesheet entry has no parent");
		}
		auto rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			LogAssert(format == GL::RGBA8);
			RGBImageLock l(rgbImage);
			gl->ActiveTexture(SCRATCH_TEX_SLOT);
			gl->BindTexture(GL::TEXTURE_2D_ARRAY, this->tex_id);
			gl->TexSubImage3D(GL::TEXTURE_2D_ARRAY, 0, entry->position.x, entry->position.y,
			                  entry->page, entry->size.x, entry->size.y, 1, GL::RGBA,
			                  GL::UNSIGNED_BYTE, l.getData());
			return;
		}
		auto palImage = std::dynamic_pointer_cast<PaletteImage>(image);
		if (palImage)
		{
			LogAssert(format == GL::R8UI);
			PaletteImageLock l(palImage);
			gl->ActiveTexture(SCRATCH_TEX_SLOT);
			gl->BindTexture(GL::TEXTURE_2D_ARRAY, this->tex_id);
			gl->TexSubImage3D(GL::TEXTURE_2D_ARRAY, 0, entry->position.x, entry->position.y,
			                  entry->page, entry->size.x, entry->size.y, 1, GL::RED_INTEGER,
			                  GL::UNSIGNED_BYTE, l.getData());
			return;
		}
		LogError("Unknown image type");
	}
	void repack()
	{
		std::vector<sp<SpritesheetEntry>> validEntries;
		for (auto &page : this->pages)
		{
			for (auto &entryPtr : page->entries)
			{
				auto entry = entryPtr.lock();
				if (entry)
				{
					entry->page = -1;
					entry->position = {-1, -1};
					validEntries.push_back(entry);
				}
			}
		}
		pages.clear();
		while (!validEntries.empty())
		{
			LogInfo("Repack: creating sheet %d", (int)pages.size());
			auto page = mksp<SpritesheetPage>((int)pages.size(), page_size, node_count);
			pages.push_back(page);
			page->addMultiple(validEntries);

			std::vector<sp<SpritesheetEntry>> unpackedEntries;
			for (auto &entry : validEntries)
			{
				if (entry->page == -1)
				{
					unpackedEntries.push_back(entry);
				}
			}
			validEntries = std::move(unpackedEntries);
		}
		this->reuploadTextures();
	}
	void addSprite(sp<SpritesheetEntry> entry)
	{
		LogAssert(entry->page == -1);
		LogAssert(entry->size.x < page_size.x);
		LogAssert(entry->size.y < page_size.y);
		for (auto &page : this->pages)
		{
			if (page->addEntry(entry))
			{
				this->upload(entry);
				return;
			}
		}
		// Required a new page
		LogInfo("Creating spritesheet page %d", (int)pages.size());
		auto page = mksp<SpritesheetPage>((int)pages.size(), page_size, node_count);
		auto ret = page->addEntry(entry);
		if (!ret)
		{
			LogError("Failed to pack a %s sized sprite in a new page of size %s?", entry->size,
			         page_size);
		}
		this->pages.push_back(page);
		// Because of the way texStorage sets the array length at creation time
		// we have to re-upload /everything/...
		this->reuploadTextures();
	}
};

class SpriteBuffer
{
  private:
	GL::GLuint sprite_buffer_id;
	GL::GLuint vertex_buffer_id;
	GL::GLuint vao_id;

	unsigned int buffer_contents;
	std::vector<SpriteDescription> buffer;

  public:
	SpriteBuffer(int buffer_size, GL::GLuint position_attr = 0, GL::GLuint uses_palette_attr = 1,
	             GL::GLuint page_attr = 2, GL::GLuint spritesheet_position_attr = 3,
	             GL::GLuint spritesheet_size_attr = 4, GL::GLuint screen_position_attr = 5,
	             GL::GLuint screen_size_attr = 6, GL::GLuint tint_attr = 7)
	    : sprite_buffer_id(0), vertex_buffer_id(0), vao_id(0), buffer_contents(0),
	      buffer(buffer_size)
	{
		LogAssert(buffer_size > 0);
		gl->GenVertexArrays(1, &this->vao_id);
		gl->BindVertexArray(vao_id);
		gl->GenBuffers(1, &this->sprite_buffer_id);
		gl->BindBuffer(GL::ARRAY_BUFFER, this->sprite_buffer_id);
		gl->BufferData(GL::ARRAY_BUFFER, sizeof(SpriteDescription) * buffer_size, NULL,
		               GL::STREAM_DRAW);

		// Mark all SpriteDescription values as having an attrib divisor of 1 (so they are
		// progressed for each instance, not each vertex)
		gl->EnableVertexAttribArray(uses_palette_attr);
		gl->VertexAttribDivisor(uses_palette_attr, 1);
		gl->VertexAttribIPointer(uses_palette_attr, 1, GL::INT, sizeof(SpriteDescription),
		                         (GL::GLvoid *)offsetof(SpriteDescription, uses_palette));

		gl->EnableVertexAttribArray(page_attr);
		gl->VertexAttribDivisor(page_attr, 1);
		gl->VertexAttribIPointer(page_attr, 1, GL::INT, sizeof(SpriteDescription),
		                         (GL::GLvoid *)offsetof(SpriteDescription, page));

		gl->EnableVertexAttribArray(spritesheet_position_attr);
		gl->VertexAttribDivisor(spritesheet_position_attr, 1);
		gl->VertexAttribPointer(spritesheet_position_attr, 2, GL::FLOAT, GL::FALSE,
		                        sizeof(SpriteDescription),
		                        (GL::GLvoid *)offsetof(SpriteDescription, spritesheet_position));

		gl->EnableVertexAttribArray(spritesheet_size_attr);
		gl->VertexAttribDivisor(spritesheet_size_attr, 1);
		gl->VertexAttribPointer(spritesheet_size_attr, 2, GL::FLOAT, GL::FALSE,
		                        sizeof(SpriteDescription),
		                        (GL::GLvoid *)offsetof(SpriteDescription, spritesheet_size));

		gl->EnableVertexAttribArray(screen_position_attr);
		gl->VertexAttribDivisor(screen_position_attr, 1);
		gl->VertexAttribPointer(screen_position_attr, 2, GL::FLOAT, GL::FALSE,
		                        sizeof(SpriteDescription),
		                        (GL::GLvoid *)offsetof(SpriteDescription, screen_position));

		gl->EnableVertexAttribArray(screen_size_attr);
		gl->VertexAttribDivisor(screen_size_attr, 1);
		gl->VertexAttribPointer(screen_size_attr, 2, GL::FLOAT, GL::FALSE,
		                        sizeof(SpriteDescription),
		                        (GL::GLvoid *)offsetof(SpriteDescription, screen_size));

		gl->EnableVertexAttribArray(tint_attr);
		gl->VertexAttribDivisor(tint_attr, 1);
		gl->VertexAttribPointer(tint_attr, 4, GL::UNSIGNED_BYTE, GL::TRUE,
		                        sizeof(SpriteDescription),
		                        (GL::GLvoid *)offsetof(SpriteDescription, tint));

		// Setup the base vertices for the sprite quad - these have the default attrib divisor 0 so
		// are progressed per vertex within each instance
		Vec2<float> position_values[4] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
		static_assert(sizeof(position_values) == 2 * 4 * sizeof(float),
		              "Unexpected position_values size");
		gl->GenBuffers(1, &this->vertex_buffer_id);
		gl->BindBuffer(GL::ARRAY_BUFFER, this->vertex_buffer_id);
		gl->BufferData(GL::ARRAY_BUFFER, sizeof(position_values), &position_values[0],
		               GL::STATIC_DRAW);

		gl->EnableVertexAttribArray(position_attr);
		gl->VertexAttribPointer(position_attr, 2, GL::FLOAT, GL::FALSE, sizeof(Vec2<float>), 0);
	}
	~SpriteBuffer()
	{
		if (vao_id)
			gl->DeleteVertexArrays(1, &vao_id);
		if (sprite_buffer_id)
			gl->DeleteBuffers(1, &sprite_buffer_id);
		if (vertex_buffer_id)
			gl->DeleteBuffers(1, &vertex_buffer_id);
	}
	bool isFull() const { return buffer_contents >= this->buffer.size(); }
	bool isEmpty() const { return this->buffer.empty(); }
	void reset() { this->buffer_contents = 0; }
	void pushRGB(sp<SpritesheetEntry> e, Vec2<float> screenPos, Vec2<float> screenSize, Colour tint)
	{
		LogAssert(!this->isFull());
		LogAssert(e->page != -1);

		auto &d = this->buffer[this->buffer_contents];
		this->buffer_contents++;

		d.uses_palette = 0;
		d.page = e->page;
		d.spritesheet_position = e->position;
		d.spritesheet_size = e->size;
		d.screen_position = screenPos;
		d.screen_size = screenSize;
		d.tint = tint;
	}
	void pushPalette(sp<SpritesheetEntry> e, Vec2<float> screenPos, Vec2<float> screenSize,
	                 Colour tint)
	{
		LogAssert(!this->isFull());
		LogAssert(e->page != -1);

		auto &d = this->buffer[this->buffer_contents];
		this->buffer_contents++;

		d.uses_palette = 1;
		d.page = e->page;
		d.spritesheet_position = e->position;
		d.spritesheet_size = e->size;
		d.screen_position = screenPos;
		d.screen_size = screenSize;
		d.tint = tint;
	}
	void draw()
	{
		if (this->buffer_contents == 0)
		{
			LogWarning("Calling draw with no sprites stored?");
			return;
		}
		gl->BindBuffer(GL::ARRAY_BUFFER, this->sprite_buffer_id);
		gl->BufferSubData(GL::ARRAY_BUFFER, 0, this->buffer_contents * sizeof(SpriteDescription),
		                  this->buffer.data());
		gl->BindVertexArray(vao_id);
		gl->DrawArraysInstanced(GL::TRIANGLE_STRIP, 0, 4, this->buffer_contents);
	}
};

class SpriteDrawMachine
{
  private:
	const char *SpriteProgram_vertexSource = {
	    "#version 300 es\n"
	    // Position is expected to be vertices of a {0,0},{1,1} pair of triangles
	    "layout (location = 0) in vec2 in_position;\n"
	    "layout (location = 1) in int in_uses_palette;\n"
	    "layout (location = 2) in int in_page;\n"
	    "layout (location = 3) in vec2 in_spritesheet_position;\n"
	    "layout (location = 4) in vec2 in_spritesheet_size;\n"
	    "layout (location = 5) in vec2 in_screen_position;\n"
	    "layout (location = 6) in vec2 in_screen_size;\n"
	    "layout (location = 7) in vec4 in_tint;\n"
	    "uniform vec2 viewport_size;\n"
	    "uniform bool flipY;\n"
	    "out vec2 texcoord;\n"
	    "out vec4 tint;\n"
	    "flat out int page;\n"
	    "flat out int uses_palette;\n"
	    "void main() {\n"
	    "  uses_palette = in_uses_palette;\n"
	    "  page = in_page;\n"
	    "  tint = in_tint;\n"
	    "  texcoord = in_spritesheet_position + in_position * in_spritesheet_size;\n"
	    // This calculates the screen position from (0..viewport_size)
	    "  vec2 tmp_pos = in_screen_position + in_position * in_screen_size;\n"
	    // Then scales that to (-1..1)
	    "  tmp_pos = ((tmp_pos / viewport_size) - vec2(0.5,0.5)) * vec2(2,2);\n"
	    "  if (flipY) gl_Position = vec4(tmp_pos.x, -tmp_pos.y, 0, 1);\n"
	    "  else  gl_Position = vec4(tmp_pos.x, tmp_pos.y, 0, 1);\n"
	    "}\n"};

	const char *SpriteProgram_fragmentSource = {
	    "#version 300 es\n"
	    // FIXME: highp is rather excessive...
	    "precision highp float;\n"
	    "precision highp int;\n"
	    "in vec2 texcoord;\n"
	    "in vec4 tint;\n"
	    "flat in int page;\n"
	    "flat in int uses_palette;\n"
	    "uniform highp isampler2DArray paletted_spritesheets;\n"
	    "uniform mediump sampler2DArray rgb_spritesheets;\n"
	    "uniform sampler2D palette;\n"
	    "out vec4 out_colour;\n"
	    "void main() {\n"
	    "  if (uses_palette == 1) {\n"
	    "    int idx = texelFetch(paletted_spritesheets, ivec3(texcoord.x, texcoord.y, page), "
	    "0).r;\n"
	    "    if (idx == 0) discard;\n"
	    "    out_colour = texelFetch(palette, ivec2(idx, 0), 0) * tint;\n"
	    "  } else {\n"
	    "    out_colour = texelFetch(rgb_spritesheets, ivec3(texcoord.x, texcoord.y, page), 0) * "
	    "tint;\n"
	    "  }\n"
	    "}\n"};
	std::vector<up<SpriteBuffer>> buffers;
	unsigned int current_buffer;

	GL::GLuint sprite_program_id;
	Spritesheet palette_spritesheet;
	Spritesheet rgb_spritesheet;

	GL::GLint viewport_size_location;
	GL::GLint flip_y_location;
	GL::GLint paletted_spritesheets_location;
	GL::GLint rgb_spritesheets_location;
	GL::GLint palette_location;

	sp<SpritesheetEntry> createSpritesheetEntry(sp<RGBImage> i)
	{
		auto entry = mksp<SpritesheetEntry>(i->size, i);
		rgb_spritesheet.addSprite(entry);
		return entry;
	}

	sp<SpritesheetEntry> createSpritesheetEntry(sp<PaletteImage> i)
	{
		auto entry = mksp<SpritesheetEntry>(i->size, i);
		palette_spritesheet.addSprite(entry);
		return entry;
	}

  public:
	unsigned int used_buffers = 0;
	SpriteDrawMachine(unsigned int bufferSize, unsigned int bufferCount,
	                  Vec2<unsigned int> spritesheet_page_size)
	    : current_buffer(0), palette_spritesheet(spritesheet_page_size, GL::R8UI),
	      rgb_spritesheet(spritesheet_page_size, GL::RGBA8)
	{
		LogAssert(bufferSize > 0);
		LogAssert(bufferCount > 0);
		this->sprite_program_id =
		    CompileProgram(SpriteProgram_vertexSource, SpriteProgram_fragmentSource);

		gl->UseProgram(this->sprite_program_id);

		this->viewport_size_location =
		    gl->GetUniformLocation(this->sprite_program_id, "viewport_size");
		LogAssert(this->viewport_size_location >= 0);
		this->flip_y_location = gl->GetUniformLocation(this->sprite_program_id, "flipY");
		LogAssert(this->flip_y_location >= 0);
		this->paletted_spritesheets_location =
		    gl->GetUniformLocation(this->sprite_program_id, "paletted_spritesheets");
		LogAssert(this->paletted_spritesheets_location >= 0);
		gl->Uniform1i(this->paletted_spritesheets_location, PALETTE_IMAGE_TEX_IDX);
		this->rgb_spritesheets_location =
		    gl->GetUniformLocation(this->sprite_program_id, "rgb_spritesheets");
		LogAssert(this->rgb_spritesheets_location >= 0);
		gl->Uniform1i(this->rgb_spritesheets_location, RGB_IMAGE_TEX_IDX);
		this->palette_location = gl->GetUniformLocation(this->sprite_program_id, "palette");
		LogAssert(this->palette_location >= 0);
		gl->Uniform1i(this->palette_location, PALETTE_TEX_IDX);

		for (unsigned int i = 0; i < bufferCount; i++)
			this->buffers.emplace_back(new SpriteBuffer(bufferSize));
	}
	~SpriteDrawMachine()
	{
		if (sprite_program_id)
			gl->DeleteProgram(sprite_program_id);
	}
	void flush(Vec2<unsigned int> viewport_size, bool flip_y)
	{
		if (this->buffers[this->current_buffer]->isEmpty())
			return;
		gl->UseProgram(this->sprite_program_id);
		gl->ActiveTexture(PALETTE_IMAGE_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D_ARRAY, this->palette_spritesheet.tex_id);
		gl->ActiveTexture(RGB_IMAGE_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D_ARRAY, this->rgb_spritesheet.tex_id);

		gl->Uniform1i(this->flip_y_location, flip_y ? 1 : 0);
		gl->Uniform2f(this->viewport_size_location, (float)viewport_size.x, (float)viewport_size.y);

		this->buffers[this->current_buffer]->draw();
		this->buffers[this->current_buffer]->reset();
		this->current_buffer = (this->current_buffer + 1) % this->buffers.size();
		this->used_buffers++;
	}
	void draw(sp<RGBImage> i, Vec2<float> screenPos, Vec2<float> screenSize,
	          Vec2<unsigned int> viewport_size, bool flip_y, Colour tint = {255, 255, 255, 255})
	{
		auto sprite = std::dynamic_pointer_cast<SpritesheetEntry>(i->rendererPrivateData);
		if (!sprite)
		{
			sprite = this->createSpritesheetEntry(i);
			i->rendererPrivateData = sprite;
		}
		if (this->buffers[this->current_buffer]->isFull())
		{
			this->flush(viewport_size, flip_y);
		}
		this->buffers[this->current_buffer]->pushRGB(sprite, screenPos, screenSize, tint);
	}
	void draw(sp<PaletteImage> i, Vec2<float> screenPos, Vec2<float> screenSize,
	          Vec2<unsigned int> viewport_size, bool flip_y, Colour tint = {255, 255, 255, 255})
	{
		auto sprite = std::dynamic_pointer_cast<SpritesheetEntry>(i->rendererPrivateData);
		if (!sprite)
		{
			sprite = this->createSpritesheetEntry(i);
			i->rendererPrivateData = sprite;
		}
		if (this->buffers[this->current_buffer]->isFull())
		{
			this->flush(viewport_size, flip_y);
		}
		this->buffers[this->current_buffer]->pushPalette(sprite, screenPos, screenSize, tint);
	}
};

class GLRGBTexture final : public RendererImageData
{
  public:
	GL::GLuint tex_id = 0;
	Vec2<unsigned int> size;
	OGLES30Renderer *owner;
	GLRGBTexture(sp<RGBImage> i, OGLES30Renderer *owner) : size(i->size), owner(owner)
	{
		RGBImageLock l(i);
		gl->GenTextures(1, &this->tex_id);
		gl->ActiveTexture(SCRATCH_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, this->tex_id);
		gl->TexImage2D(GL::TEXTURE_2D, 0, GL::RGBA8, i->size.x, i->size.y, 0, GL::RGBA,
		               GL::UNSIGNED_BYTE, l.getData());
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);
	}
	~GLRGBTexture() override;
};

class GLPaletteTexture final : public RendererImageData
{
  public:
	GL::GLuint tex_id = 0;
	Vec2<unsigned int> size;
	OGLES30Renderer *owner;
	GLPaletteTexture(sp<PaletteImage> i, OGLES30Renderer *owner) : size(i->size), owner(owner)
	{
		PaletteImageLock l(i);
		gl->GenTextures(1, &this->tex_id);
		gl->ActiveTexture(SCRATCH_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, this->tex_id);
		gl->TexImage2D(GL::TEXTURE_2D, 0, GL::R8UI, i->size.x, i->size.y, 0, GL::RED_INTEGER,
		               GL::UNSIGNED_BYTE, l.getData());
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);
	}
	~GLPaletteTexture() override;
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;
	GL::GLuint prevID;
	GL::GLuint id;

  public:
	BindFramebuffer(GL::GLuint id) : id(id)
	{
		gl->GetIntegerv(GL::DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GL::GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		gl->BindFramebuffer(GL::DRAW_FRAMEBUFFER, id);
	}
	~BindFramebuffer()
	{
		if (prevID != id)
		{
			gl->BindFramebuffer(GL::DRAW_FRAMEBUFFER, prevID);
		}
	}
};

class GLSurface final : public RendererImageData
{
  public:
	GL::GLuint fbo_id;
	GL::GLuint tex_id;
	Vec2<unsigned int> size;
	OGLES30Renderer *owner;
	GLSurface(GL::GLuint fbo, Vec2<unsigned int> size, OGLES30Renderer *owner)
	    : fbo_id(fbo), tex_id(0), size(size), owner(owner)
	{
	}
	GLSurface(Vec2<unsigned int> size, OGLES30Renderer *owner) : size(size), owner(owner)
	{
		LogAssert(size.x > 0 && size.y > 0);
		gl->GenTextures(1, &this->tex_id);
		gl->ActiveTexture(SCRATCH_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, this->tex_id);
		gl->TexImage2D(GL::TEXTURE_2D, 0, GL::RGBA8, size.x, size.y, 0, GL::RGBA, GL::UNSIGNED_BYTE,
		               NULL);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);

		gl->GenFramebuffers(1, &this->fbo_id);
		gl->BindFramebuffer(GL::DRAW_FRAMEBUFFER, this->fbo_id);
		gl->FramebufferTexture2D(GL::DRAW_FRAMEBUFFER, GL::COLOR_ATTACHMENT0, GL::TEXTURE_2D,
		                         this->tex_id, 0);
		if (gl->CheckFramebufferStatus(GL::DRAW_FRAMEBUFFER) != GL::FRAMEBUFFER_COMPLETE)
		{
			LogError("Surface framebuffer not complete");
		}
	}
	~GLSurface() override;
	sp<Image> readBack() override
	{
		BindFramebuffer b(this->fbo_id);
		auto img = mksp<RGBImage>(this->size);

		// Reset and pixelStorei args to zero padding to match RGBImage.
		// The assumption here is this is a 'slow path' anyway, so doesn't really matter if there
		// are unnecessary state changes.
		// Also, I suspect many of these values will never change from their default during the
		// lifetime of this context, but better safe than sorry.
		gl->PixelStorei(GL::PACK_ROW_LENGTH, 0);
		gl->PixelStorei(GL::PACK_SKIP_PIXELS, 0);
		gl->PixelStorei(GL::PACK_SKIP_ROWS, 0);
		gl->PixelStorei(GL::PACK_ALIGNMENT, 1);

		RGBImageLock l(img);

		// Copy the image a row at a time, as OpenGL specifies framebuffer coords to start at the
		// bottom, but RGBImage assumes it starts at the top
		for (unsigned int y = 0; y < this->size.y; y++)
		{
			unsigned int row = this->size.y - y - 1;
			// Assume 4bpp RGBA8888
			unsigned int stride = this->size.x * 4;
			char *linePtr = reinterpret_cast<char *>(l.getData());
			linePtr += (y * stride);
			gl->ReadPixels(0, row, size.x, row + 1, GL::RGBA, GL::UNSIGNED_BYTE, linePtr);
		}
		return img;
	}
};

class TexturedDrawMachine
{
  private:
	const char *TexProgram_vertexSource = {
	    "#version 300 es\n"
	    "layout (location = 0) in vec2 in_position;\n"
	    "layout (location = 1) in vec2 in_texcoords;\n"
	    "layout (location = 2) in vec4 in_tint;\n"
	    "uniform vec2 tex_size;\n"
	    "uniform bool flipY;\n"
	    "out vec2 texcoord;\n"
	    "out vec4 tint;\n"
	    "void main() {\n"
	    "  texcoord = in_texcoords * tex_size;\n"
	    "  tint = in_tint;\n"
	    "  if (flipY) gl_Position = vec4(in_position.x, -in_position.y, 0, 1);\n"
	    "  else  gl_Position = vec4(in_position.x, in_position.y, 0, 1);\n"
	    "}\n"};
	const char *TexProgram_fragmentSource = {
	    "#version 300 es\n"
	    // FIXME: highp is rather excessive...
	    "precision highp float;\n"
	    "precision highp int;\n"
	    "in vec2 texcoord;\n"
	    "in vec4 tint;\n"
	    "uniform int uses_palette;\n"
	    "uniform highp isampler2D palette_texture;\n"
	    "uniform mediump sampler2D rgb_texture;\n"
	    "uniform sampler2D palette;\n"
	    "out vec4 out_colour;\n"
	    "void main() {\n"
	    "  vec4 tmp_colour;\n"
	    "  if (uses_palette == 1) {\n"
	    "    int idx = texelFetch(palette_texture, ivec2(texcoord.x, texcoord.y), 0).r;\n"
	    "    if (idx == 0) discard;\n"
	    "    tmp_colour = texelFetch(palette, ivec2(idx, 0), 0);\n"
	    "  } else {\n"
	    "    vec2 normalised_texture_coords = texcoord / vec2(textureSize(rgb_texture, 0));\n"
	    "    tmp_colour = texture(rgb_texture, normalised_texture_coords);\n"
	    "  }\n"
	    "  out_colour = tmp_colour * tint;\n"
	    "}\n"};

	struct TexturedQuadBuffers
	{
		GL::GLuint position_buffer;
		GL::GLuint vao_id;
	};
	std::vector<TexturedQuadBuffers> buffers;
	unsigned int current_buffer;

	GL::GLuint tex_program_id;

	GL::GLint flip_y_location;
	GL::GLint palette_texture_location;
	GL::GLint rgb_texture_location;
	GL::GLint palette_location;
	GL::GLint tex_size_location;
	GL::GLint uses_palette_location;

	struct VertexDesc
	{
		Vec2<float> position;
		Vec2<float> texcoord;
		Colour tint;
	};

	struct PositionVertices
	{
		VertexDesc vertices[4];
	};
	OGLES30Renderer *owner;

  public:
	unsigned int used_buffers = 0;
	TexturedDrawMachine(unsigned int bufferCount, OGLES30Renderer *owner,
	                    GL::GLuint position_attr = 0, GL::GLuint texcoord_attr = 1,
	                    GL::GLuint tint_attr = 2)
	    : current_buffer(0), tex_program_id(0), owner(owner)
	{
		LogAssert(bufferCount > 0);
		this->tex_program_id = CompileProgram(TexProgram_vertexSource, TexProgram_fragmentSource);

		gl->UseProgram(this->tex_program_id);

		this->flip_y_location = gl->GetUniformLocation(this->tex_program_id, "flipY");
		LogAssert(this->flip_y_location >= 0);

		this->palette_texture_location =
		    gl->GetUniformLocation(this->tex_program_id, "palette_texture");
		LogAssert(this->palette_texture_location >= 0);

		gl->Uniform1i(this->palette_texture_location, PALETTE_IMAGE_TEX_IDX);
		this->rgb_texture_location = gl->GetUniformLocation(this->tex_program_id, "rgb_texture");
		LogAssert(this->rgb_texture_location >= 0);

		gl->Uniform1i(this->rgb_texture_location, RGB_IMAGE_TEX_IDX);
		this->palette_location = gl->GetUniformLocation(this->tex_program_id, "palette");
		LogAssert(this->palette_location >= 0);

		gl->Uniform1i(this->palette_location, PALETTE_TEX_IDX);

		this->tex_size_location = gl->GetUniformLocation(this->tex_program_id, "tex_size");
		LogAssert(this->tex_size_location >= 0);

		this->uses_palette_location = gl->GetUniformLocation(this->tex_program_id, "uses_palette");
		LogAssert(this->uses_palette_location >= 0);

		for (unsigned int i = 0; i < bufferCount; i++)
		{
			TexturedQuadBuffers b;

			gl->GenVertexArrays(1, &b.vao_id);
			gl->BindVertexArray(b.vao_id);

			gl->GenBuffers(1, &b.position_buffer);
			gl->BindBuffer(GL::ARRAY_BUFFER, b.position_buffer);
			gl->BufferData(GL::ARRAY_BUFFER, sizeof(PositionVertices), NULL, GL::STREAM_DRAW);

			gl->EnableVertexAttribArray(position_attr);
			gl->VertexAttribPointer(position_attr, 2, GL::FLOAT, GL::FALSE, sizeof(VertexDesc),
			                        (GL::GLvoid *)offsetof(VertexDesc, position));
			gl->EnableVertexAttribArray(texcoord_attr);
			gl->VertexAttribPointer(texcoord_attr, 2, GL::FLOAT, GL::FALSE, sizeof(VertexDesc),
			                        (GL::GLvoid *)offsetof(VertexDesc, texcoord));

			gl->EnableVertexAttribArray(tint_attr);
			gl->VertexAttribPointer(tint_attr, 4, GL::UNSIGNED_BYTE, GL::TRUE, sizeof(VertexDesc),
			                        (GL::GLvoid *)offsetof(VertexDesc, tint));

			this->buffers.push_back(b);
		}
	}
	~TexturedDrawMachine()
	{
		if (tex_program_id)
			gl->DeleteProgram(tex_program_id);
		for (auto buf : this->buffers)
		{
			gl->DeleteVertexArrays(1, &buf.vao_id);
			gl->DeleteBuffers(1, &buf.position_buffer);
		}
	}

	void draw(bool paletted, Vec2<float> tex_size, Vec2<float> screenPos, Vec2<float> screenSize,
	          Vec2<float> rotationCenter, float rotationAngleRadians,
	          Vec2<unsigned int> viewport_size, bool flip_y, Colour tint)
	{
		static const Vec2<float> identity_quad[4] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};

		PositionVertices v;

		auto rotMatrix = glm::rotate(rotationAngleRadians, Vec3<float>{0.0f, 0.0f, 1.0f});

		for (unsigned int i = 0; i < 4; i++)
		{
			auto p = identity_quad[i];
			p *= screenSize;
			p -= rotationCenter;
			glm::vec4 transformed = rotMatrix * glm::vec4{p.x, p.y, 0.0f, 1.0f};
			p.x = transformed.x;
			p.y = transformed.y;
			p += rotationCenter;
			p += screenPos;
			// p is now in 0..viewport_size coords - scale to (-1..1)
			p /= viewport_size;
			p -= Vec2<float>{0.5f, 0.5f};
			p *= Vec2<float>{2.0f, 2.0f};

			v.vertices[i].position = p;
			v.vertices[i].texcoord = identity_quad[i];
			v.vertices[i].tint = tint;
		}

		gl->UseProgram(this->tex_program_id);

		gl->Uniform1i(this->flip_y_location, flip_y ? 1 : 0);

		gl->Uniform1i(this->uses_palette_location, paletted ? 1 : 0);
		gl->Uniform2f(this->tex_size_location, tex_size.x, tex_size.y);

		auto &buf = this->buffers[this->current_buffer];
		this->current_buffer = (this->current_buffer + 1) % this->buffers.size();
		this->used_buffers++;

		gl->BindBuffer(GL::ARRAY_BUFFER, buf.position_buffer);
		gl->BufferSubData(GL::ARRAY_BUFFER, 0, sizeof(PositionVertices), &v);

		gl->BindVertexArray(buf.vao_id);
		gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);
	}

	void draw(sp<RGBImage> i, Vec2<float> screenPos, Vec2<float> screenSize,
	          Vec2<float> rotationCenter, float rotationAngleRadians,
	          Vec2<unsigned int> viewport_size, bool flip_y, Renderer::Scaler scaler,
	          Colour tint = {255, 255, 255, 255})
	{
		auto tex = std::dynamic_pointer_cast<GLRGBTexture>(i->rendererPrivateData);
		if (!tex)
		{
			tex = mksp<GLRGBTexture>(i, owner);
			i->rendererPrivateData = tex;
		}
		gl->ActiveTexture(RGB_IMAGE_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, tex->tex_id);
		if (scaler == Renderer::Scaler::Linear)
		{
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR);
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR);
		}
		else
		{
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
		}
		this->draw(false, i->size, screenPos, screenSize, rotationCenter, rotationAngleRadians,
		           viewport_size, flip_y, tint);
	}

	void draw(sp<Surface> i, Vec2<float> screenPos, Vec2<float> screenSize,
	          Vec2<float> rotationCenter, float rotationAngleRadians,
	          Vec2<unsigned int> viewport_size, bool flip_y, Renderer::Scaler scaler,
	          Colour tint = {255, 255, 255, 255})
	{
		auto tex = std::dynamic_pointer_cast<GLSurface>(i->rendererPrivateData);
		if (!tex)
		{
			LogWarning("Drawing using undefined surface contents");
			tex = mksp<GLSurface>(i->size, owner);
			i->rendererPrivateData = tex;
		}
		gl->ActiveTexture(RGB_IMAGE_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, tex->tex_id);
		if (scaler == Renderer::Scaler::Linear)
		{
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR);
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR);
		}
		else
		{
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
			gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
		}
		this->draw(false, i->size, screenPos, screenSize, rotationCenter, rotationAngleRadians,
		           viewport_size, flip_y, tint);
	}

	void draw(sp<PaletteImage> i, Vec2<float> screenPos, Vec2<float> screenSize,
	          Vec2<float> rotationCenter, float rotationAngleRadians,
	          Vec2<unsigned int> viewport_size, bool flip_y, Colour tint = {255, 255, 255, 255})
	{
		auto tex = std::dynamic_pointer_cast<GLPaletteTexture>(i->rendererPrivateData);
		if (!tex)
		{
			tex = mksp<GLPaletteTexture>(i, owner);
			i->rendererPrivateData = tex;
		}
		gl->ActiveTexture(PALETTE_IMAGE_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, tex->tex_id);
		this->draw(true, i->size, screenPos, screenSize, rotationCenter, rotationAngleRadians,
		           viewport_size, flip_y, tint);
	}
};

class ColouredDrawMachine
{
  private:
	const char *ColourProgram_vertexSource = {
	    "#version 300 es\n"
	    "layout (location = 0) in vec2 in_position;\n"
	    "layout (location = 1) in vec4 in_colour;\n"
	    "out vec4 colour;\n"
	    "uniform bool flipY;\n"
	    "uniform vec2 viewport_size;\n"
	    "void main() {\n"
	    "  colour = in_colour;\n"
	    "  vec2 tmpPos = in_position;\n"
	    "  tmpPos /= viewport_size;\n"
	    "  tmpPos -= vec2(0.5,0.5);\n"
	    "  tmpPos *= vec2(2,2);\n"
	    "  if (flipY) gl_Position = vec4(tmpPos.x, -tmpPos.y, 0, 1);\n"
	    "  else  gl_Position = vec4(tmpPos.x, tmpPos.y, 0, 1);\n"
	    "}\n"};
	const char *ColourProgram_fragmentSource = {"#version 300 es\n"
	                                            // FIXME: highp is rather excessive...
	                                            "precision highp float;\n"
	                                            "in vec4 colour;\n"
	                                            "out vec4 out_colour;\n"
	                                            "void main() {\n"
	                                            "  out_colour = colour;\n"
	                                            "}\n"};

	struct ColouredVertex
	{
		Vec2<float> position;
		Colour colour;
	};
	// We assume Colour classes are 4 packed bytes
	static_assert(sizeof(Colour) == 4, "Unexpected Colour size");
	struct ColouredDescription
	{
		ColouredVertex vertices[4];
	};
	struct ColouredColouredBuffer
	{
		GL::GLuint vertex_buffer;
		GL::GLuint vao_id;
		ColouredDescription data;
	};
	std::vector<ColouredColouredBuffer> buffers;
	unsigned int current_buffer;

	GL::GLuint colour_program_id;

	GL::GLint flip_y_location;
	GL::GLint viewport_size_location;

  public:
	unsigned int used_buffers = 0;
	ColouredDrawMachine(unsigned int bufferCount, GL::GLuint position_attr = 0,
	                    GL::GLuint colour_attr = 1)
	    : current_buffer(0), colour_program_id(0)
	{
		LogAssert(bufferCount > 0);

		this->colour_program_id =
		    CompileProgram(ColourProgram_vertexSource, ColourProgram_fragmentSource);

		gl->UseProgram(this->colour_program_id);

		this->flip_y_location = gl->GetUniformLocation(this->colour_program_id, "flipY");
		LogAssert(this->flip_y_location >= 0);

		this->viewport_size_location =
		    gl->GetUniformLocation(this->colour_program_id, "viewport_size");
		LogAssert(this->viewport_size_location >= 0);

		for (unsigned int i = 0; i < bufferCount; i++)
		{
			ColouredColouredBuffer b;

			gl->GenVertexArrays(1, &b.vao_id);
			gl->BindVertexArray(b.vao_id);

			gl->GenBuffers(1, &b.vertex_buffer);
			gl->BindBuffer(GL::ARRAY_BUFFER, b.vertex_buffer);
			gl->BufferData(GL::ARRAY_BUFFER, sizeof(ColouredDescription), NULL, GL::STREAM_DRAW);

			gl->EnableVertexAttribArray(position_attr);
			gl->VertexAttribPointer(position_attr, 2, GL::FLOAT, GL::FALSE, sizeof(ColouredVertex),
			                        (GL::GLvoid *)offsetof(ColouredVertex, position));

			gl->EnableVertexAttribArray(colour_attr);
			gl->VertexAttribPointer(colour_attr, 4, GL::UNSIGNED_BYTE, GL::TRUE,
			                        sizeof(ColouredVertex),
			                        (GL::GLvoid *)offsetof(ColouredVertex, colour));

			this->buffers.push_back(b);
		}
	}

	// This expects screen coordinates as position values
	void drawQuad(Vec2<float> positions[4], Colour colours[4], Vec2<unsigned int> viewport_size,
	              bool flip_y)
	{
		auto &buf = this->buffers[this->current_buffer];

		for (int i = 0; i < 4; i++)
		{
			buf.data.vertices[i].position = positions[i];
			buf.data.vertices[i].colour = colours[i];
		}

		gl->BindVertexArray(buf.vao_id);
		gl->UseProgram(this->colour_program_id);
		gl->Uniform1i(this->flip_y_location, flip_y);
		gl->Uniform2f(this->viewport_size_location, viewport_size.x, viewport_size.y);

		gl->BindBuffer(GL::ARRAY_BUFFER, buf.vertex_buffer);
		gl->BufferSubData(GL::ARRAY_BUFFER, 0, sizeof(ColouredDescription), &buf.data);
		gl->DrawArrays(GL::TRIANGLE_STRIP, 0, 4);

		this->current_buffer = (this->current_buffer + 1) % this->buffers.size();
		this->used_buffers++;
	}

	void drawLine(Vec2<float> positions[2], Colour colours[2], Vec2<float> viewport_size,
	              bool flip_y, float thickness)
	{
		auto &buf = this->buffers[this->current_buffer];

		for (unsigned int i = 0; i < 2; i++)
		{
			buf.data.vertices[i].position = positions[i];
			buf.data.vertices[i].colour = colours[i];
		}

		gl->BindVertexArray(buf.vao_id);
		gl->UseProgram(this->colour_program_id);
		gl->Uniform1i(this->flip_y_location, flip_y);
		gl->Uniform2f(this->viewport_size_location, viewport_size.x, viewport_size.y);
		gl->LineWidth(thickness);

		gl->BindBuffer(GL::ARRAY_BUFFER, buf.vertex_buffer);
		gl->BufferSubData(GL::ARRAY_BUFFER, 0, sizeof(ColouredDescription), &buf.data);
		gl->DrawArrays(GL::LINE_STRIP, 0, 2);

		this->current_buffer = (this->current_buffer + 1) % this->buffers.size();
		this->used_buffers++;
	}

	~ColouredDrawMachine()
	{
		if (this->colour_program_id)
			gl->DeleteProgram(this->colour_program_id);
		for (auto &buffer : this->buffers)
		{
			gl->DeleteVertexArrays(1, &buffer.vao_id);
			gl->DeleteBuffers(1, &buffer.vertex_buffer);
		}
	}
};

class GLPalette final : public RendererImageData
{
  public:
	GL::GLuint tex_id;
	GLPalette(sp<Palette> parent) : tex_id(0)
	{
		gl->GenTextures(1, &this->tex_id);
		gl->ActiveTexture(SCRATCH_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, this->tex_id);
		gl->TexImage2D(GL::TEXTURE_2D, 0, GL::RGBA8, parent->colours.size(), 1, 0, GL::RGBA,
		               GL::UNSIGNED_BYTE, parent->colours.data());
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::NEAREST);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE);
		gl->TexParameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE);
	}
	~GLPalette() override
	{
		if (this->tex_id)
			gl->DeleteTextures(1, &this->tex_id);
	}
};

class OGLES30Renderer final : public Renderer
{
  private:
	void setSurface(sp<Surface> s) override
	{
		this->flush();
		this->current_surface = s;
		auto fbo = std::dynamic_pointer_cast<GLSurface>(s->rendererPrivateData);
		if (!fbo)
		{
			fbo = mksp<GLSurface>(s->size, this);
			s->rendererPrivateData = fbo;
		}
		gl->BindFramebuffer(GL::DRAW_FRAMEBUFFER, fbo->fbo_id);
		gl->Viewport(0, 0, s->size.x, s->size.y);
	}
	sp<Surface> getSurface() override { return this->current_surface; }
	Vec2<unsigned int> spritesheetPageSize = {4096, 4096};
	Vec2<unsigned int> maxSpriteSizeToPack{256, 256};
	unsigned int spriteBufferSize = 16384;
	unsigned int spriteBufferCount = 40;

	unsigned int texturedBufferCount = 100;

	unsigned int quadBufferCount = 100;

	up<SpriteDrawMachine> spriteMachine;
	up<TexturedDrawMachine> texturedMachine;
	up<ColouredDrawMachine> colouredDrawMachine;

	enum State
	{
		Idle,
		BatchingSprites,
	};

	State state;

	sp<Surface> default_surface;
	sp<Surface> current_surface;
	sp<Palette> current_palette;

	std::thread::id bound_thread;
	std::mutex destroyed_texture_list_mutex;
	std::list<GL::GLuint> destroyed_texture_list;
	std::mutex destroyed_framebuffer_list_mutex;
	std::list<GL::GLuint> destroyed_framebuffer_list;

  public:
	OGLES30Renderer();

	unsigned int maxSpriteBuffers = 0;
	unsigned int maxTexturedBuffers = 0;
	unsigned int maxColouredBuffers = 0;
	~OGLES30Renderer() override
	{
		LogInfo("Max %u sprite buffers %u textured buffers %u coloured buffers",
		        this->maxSpriteBuffers, this->maxTexturedBuffers, this->maxColouredBuffers);
		renderer_dead = true;
	}

	void newFrame() override
	{
		if (this->spriteMachine->used_buffers > this->maxSpriteBuffers)
		{
			LogInfo("New max sprite buffers: %u", this->spriteMachine->used_buffers);
			this->maxSpriteBuffers = this->spriteMachine->used_buffers;
		}
		if (this->texturedMachine->used_buffers > this->maxTexturedBuffers)
		{
			LogInfo("New max textured buffers: %u", this->texturedMachine->used_buffers);
			this->maxTexturedBuffers = this->texturedMachine->used_buffers;
		}
		if (this->colouredDrawMachine->used_buffers > this->maxColouredBuffers)
		{
			LogInfo("New max coloured buffers: %u", this->colouredDrawMachine->used_buffers);
			this->maxColouredBuffers = this->colouredDrawMachine->used_buffers;
		}
		this->spriteMachine->used_buffers = 0;
		this->texturedMachine->used_buffers = 0;
		this->colouredDrawMachine->used_buffers = 0;
	}

	void clear(Colour c) override
	{
		this->flush();
		gl->ClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
		gl->Clear(GL::COLOR_BUFFER_BIT);
	}
	void setPalette(sp<Palette> p) override
	{
		this->flush();
		this->current_palette = p;
		if (p == nullptr)
		{
			gl->ActiveTexture(PALETTE_TEX_SLOT);
			gl->BindTexture(GL::TEXTURE_2D, 0);
			return;
		}
		auto pal = std::dynamic_pointer_cast<GLPalette>(p->rendererPrivateData);
		if (!pal)
		{
			pal = mksp<GLPalette>(p);
			p->rendererPrivateData = pal;
		}
		gl->ActiveTexture(PALETTE_TEX_SLOT);
		gl->BindTexture(GL::TEXTURE_2D, pal->tex_id);
	}
	sp<Palette> getPalette() override { return this->current_palette; }
	void draw(sp<Image> i, Vec2<float> position) override
	{
		this->drawScaled(i, position, i->size, Scaler::Nearest);
	}
	void drawRotated(sp<Image> i, Vec2<float> center, Vec2<float> position, float angle) override
	{
		this->flush();
		auto viewport_size = this->current_surface->size;
		bool flip_y = (this->current_surface == this->default_surface);
		auto size = i->size;
		auto paletteImage = std::dynamic_pointer_cast<PaletteImage>(i);
		if (paletteImage)
		{
			this->texturedMachine->draw(paletteImage, position, size, center, angle, viewport_size,
			                            flip_y);
			return;
		}
		auto rgbImage = std::dynamic_pointer_cast<RGBImage>(i);
		if (rgbImage)
		{
			this->texturedMachine->draw(rgbImage, position, size, center, angle, viewport_size,
			                            flip_y, Renderer::Scaler::Linear);
			return;
		}
		auto surface = std::dynamic_pointer_cast<Surface>(i);
		if (surface)
		{
			this->flush();
			this->texturedMachine->draw(surface, position, size, center, angle, viewport_size,
			                            flip_y, Renderer::Scaler::Linear);
			return;
		}
		LogError("Unknown image type");
	}
	void drawScaled(sp<Image> i, Vec2<float> position, Vec2<float> size, Scaler scaler) override
	{
		auto viewport_size = this->current_surface->size;
		bool flip_y = (this->current_surface == this->default_surface);
		auto paletteImage = std::dynamic_pointer_cast<PaletteImage>(i);
		if (paletteImage)
		{
			if (paletteImage->size.x <= this->maxSpriteSizeToPack.x &&
			    paletteImage->size.y <= this->maxSpriteSizeToPack.y)
			{
				if (this->state != State::BatchingSprites)
				{
					this->flush();
					this->state = State::BatchingSprites;
				}
				this->spriteMachine->draw(paletteImage, position, size, viewport_size, flip_y);
			}
			else
			{
				this->flush();
				this->texturedMachine->draw(paletteImage, position, size, {0, 0}, 0, viewport_size,
				                            flip_y);
			}
			return;
		}
		auto rgbImage = std::dynamic_pointer_cast<RGBImage>(i);
		if (rgbImage)
		{
			if (rgbImage->size.x <= this->maxSpriteSizeToPack.x &&
			    rgbImage->size.y <= this->maxSpriteSizeToPack.y && scaler == Scaler::Nearest)
			{
				if (this->state != State::BatchingSprites)
				{
					this->flush();
					this->state = State::BatchingSprites;
				}
				this->spriteMachine->draw(rgbImage, position, size, viewport_size, flip_y);
			}
			else
			{
				this->flush();
				this->texturedMachine->draw(rgbImage, position, size, {0, 0}, 0, viewport_size,
				                            flip_y, scaler);
			}
			return;
		}
		auto surface = std::dynamic_pointer_cast<Surface>(i);
		if (surface)
		{
			this->flush();
			this->texturedMachine->draw(surface, position, size, {0, 0}, 0, viewport_size, flip_y,
			                            scaler);
			return;
		}
		LogError("Unknown image type");
	}
	void drawTinted(sp<Image> i, Vec2<float> position, Colour tint) override
	{
		auto viewport_size = this->current_surface->size;
		bool flip_y = (this->current_surface == this->default_surface);
		auto size = i->size;
		auto paletteImage = std::dynamic_pointer_cast<PaletteImage>(i);
		if (paletteImage)
		{
			if (paletteImage->size.x <= this->maxSpriteSizeToPack.x &&
			    paletteImage->size.y <= this->maxSpriteSizeToPack.y)
			{
				if (this->state != State::BatchingSprites)
				{
					this->flush();
					this->state = State::BatchingSprites;
				}
				this->spriteMachine->draw(paletteImage, position, size, viewport_size, flip_y,
				                          tint);
			}
			else
			{
				this->flush();
				this->texturedMachine->draw(paletteImage, position, size, {0, 0}, 0, viewport_size,
				                            flip_y, tint);
			}
			return;
		}
		auto rgbImage = std::dynamic_pointer_cast<RGBImage>(i);
		if (rgbImage)
		{
			if (rgbImage->size.x <= this->maxSpriteSizeToPack.x &&
			    rgbImage->size.y <= this->maxSpriteSizeToPack.y)
			{
				if (this->state != State::BatchingSprites)
				{
					this->flush();
					this->state = State::BatchingSprites;
				}
				this->spriteMachine->draw(rgbImage, position, size, viewport_size, flip_y, tint);
			}
			else
			{

				this->flush();
				this->texturedMachine->draw(rgbImage, position, size, {0, 0}, 0, viewport_size,
				                            flip_y, Renderer::Scaler::Nearest, tint);
			}
			return;
		}
		auto surface = std::dynamic_pointer_cast<Surface>(i);
		if (surface)
		{
			this->flush();
			this->texturedMachine->draw(surface, position, size, {0, 0}, 0, viewport_size, flip_y,
			                            Renderer::Scaler::Nearest, tint);
			return;
		}
	}
	void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c) override
	{
		this->flush();
		Vec2<float> positions[4];
		Colour colours[4];
		static const Vec2<float> identity_quad[4] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
		auto viewport_size = this->current_surface->size;
		bool flip_y = (this->current_surface == this->default_surface);

		for (int i = 0; i < 4; i++)
		{
			// Position is currently in 0..current_surface.size coords, we want them in normalized
			// device coords (-1..1)
			positions[i] = position + size * identity_quad[i];
			colours[i] = c;
		}
		this->colouredDrawMachine->drawQuad(positions, colours, viewport_size, flip_y);
	}
	void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness) override
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
	void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness) override
	{
		this->flush();
		Vec2<float> positions[2];
		Colour colours[2];
		bool flip_y = (this->current_surface == this->default_surface);
		auto viewport_size = this->current_surface->size;

		colours[0] = c;
		colours[1] = c;
		positions[0] = p1 + Vec2<float>{0.5, 0.5};
		positions[1] = p2 + Vec2<float>{0.5, 0.5};

		this->colouredDrawMachine->drawLine(positions, colours, viewport_size, flip_y, thickness);
	}
	void flush() override
	{
		auto viewport_size = this->current_surface->size;
		bool flip_y = (this->current_surface == this->default_surface);
		if (this->state == State::BatchingSprites)
		{
			this->spriteMachine->flush(viewport_size, flip_y);
			this->state = State::Idle;
		}
		// Cleanup any outstanding destroyed texture or framebuffer objects
		{
			std::lock_guard<std::mutex> lock(this->destroyed_texture_list_mutex);

			for (auto &id : this->destroyed_texture_list)
			{
				gl->DeleteTextures(1, &id);
			}
			this->destroyed_texture_list.clear();
		}
		{
			std::lock_guard<std::mutex> lock(this->destroyed_framebuffer_list_mutex);

			for (auto &id : this->destroyed_framebuffer_list)
			{
				gl->DeleteFramebuffers(1, &id);
			}
			this->destroyed_framebuffer_list.clear();
		}
	}
	UString getName() override { return "GLES30 Renderer"; }
	sp<Surface> getDefaultSurface() override { return this->default_surface; }
	// These can be called from any thread - e.g. from the Image destructors
	void delete_texture_object(GL::GLuint id)
	{
		// If we're already on the bound thread, just immediately destroy
		if (this->bound_thread == std::this_thread::get_id())
		{
			gl->DeleteTextures(1, &id);
			return;
		}
		// Otherwise add it to a list for future destruction
		{
			std::lock_guard<std::mutex> lock(this->destroyed_texture_list_mutex);
			this->destroyed_texture_list.push_back(id);
		}
	}
	void delete_framebuffer_object(GL::GLuint id)
	{
		// If we're already on the bound thread, just immediately destroy
		if (this->bound_thread == std::this_thread::get_id())
		{
			gl->DeleteFramebuffers(1, &id);
			return;
		}
		// Otherwise add it to a list for future destruction
		{
			std::lock_guard<std::mutex> lock(this->destroyed_framebuffer_list_mutex);
			this->destroyed_framebuffer_list.push_back(id);
		}
	}
};

void GLESWRAP_APIENTRY debug_message_proc(GL::KhrDebug::GLenum, GL::KhrDebug::GLenum, GL::GLuint,
                                          GL::KhrDebug::GLenum severity, GL::GLsizei,
                                          const GL::GLchar *message, const void *)
{
	switch (severity)
	{
		case GL::KhrDebug::DEBUG_SEVERITY_HIGH:
		case GL::KhrDebug::DEBUG_SEVERITY_MEDIUM:
		{
			LogWarning("Debug message: \"%s\"", message);
			break;
		}
		default:
		{
			LogInfo("Debug message: \"%s\"", message);
			break;
		}
	}
}

OGLES30Renderer::OGLES30Renderer() : state(State::Idle)
{
	this->bound_thread = std::this_thread::get_id();
	this->spriteMachine.reset(
	    new SpriteDrawMachine{spriteBufferSize, spriteBufferCount, spritesheetPageSize});
	this->texturedMachine.reset(new TexturedDrawMachine{texturedBufferCount, this});
	this->colouredDrawMachine.reset(new ColouredDrawMachine{quadBufferCount});
	GL::GLint viewport[4];
	gl->GetIntegerv(GL::VIEWPORT, viewport);
	LogInfo("Viewport {%d,%d,%d,%d}", viewport[0], viewport[1], viewport[2], viewport[3]);
	this->default_surface = mksp<Surface>(Vec2<int>{viewport[2], viewport[3]});
	this->default_surface->rendererPrivateData =
	    mksp<GLSurface>(0, Vec2<int>{viewport[2], viewport[3]}, this);
	this->current_surface = default_surface;
	gl->Enable(GL::BLEND);
	gl->BlendFuncSeparate(GL::SRC_ALPHA, GL::ONE_MINUS_SRC_ALPHA, GL::SRC_ALPHA, GL::DST_ALPHA);
	gl->PixelStorei(GL::UNPACK_ALIGNMENT, 1);

	GL::GLint max_texture_size;
	gl->GetIntegerv(GL::MAX_TEXTURE_SIZE, &max_texture_size);
	LogAssert(max_texture_size > 0);
	if (spritesheetPageSize.x > (unsigned int)max_texture_size ||
	    spritesheetPageSize.y > (unsigned int)max_texture_size)
	{
		LogWarning("Default spritesheet size %s larger than HW limit %d - clamping...",
		           spritesheetPageSize, max_texture_size);
		spritesheetPageSize.x = std::min(spritesheetPageSize.x, (unsigned int)max_texture_size);
		spritesheetPageSize.y = std::min(spritesheetPageSize.y, (unsigned int)max_texture_size);
	}
	LogInfo("Set spritesheet size to %s", spritesheetPageSize);

	bool use_debug = false;

#ifndef NDEBUG
	use_debug = gl->KHR_debug.supported;
#endif

	if (use_debug)
	{
		LogInfo("Enabling KHR_debug output");
		gl->Enable(static_cast<GL::GLenum>(GL::KhrDebug::DEBUG_OUTPUT_SYNCHRONOUS));
		gl->Enable(static_cast<GL::GLenum>(GL::KhrDebug::DEBUG_OUTPUT));
		gl->KHR_debug.DebugMessageCallback(debug_message_proc, NULL);
	}
	renderer_dead = false;
}

class OGLES30RendererFactory : public RendererFactory
{
	bool alreadyInitialised;

  public:
	OGLES30RendererFactory() : alreadyInitialised(false) {}
	OpenApoc::Renderer *create() override
	{
		if (!alreadyInitialised)
		{
			LogAssert(gl == nullptr);
			alreadyInitialised = true;
			// First see if we're a direct OpenGL|ES context
			if (GL::supported(true))
			{
				LogInfo("Using OpenGL ES3 compatibility");
				gl.reset(new GL(true));
			}
			// Then check for ES3 compatibility extension on desktop OpenGL
			else if (GL::supported(false))
			{
				LogInfo("Using OpenGL|ES context");
				gl.reset(new GL(false));
			}
			else
			{
				LogInfo("Failed to find ES3-compatible device");
				return nullptr;
			}
			return new OGLES30Renderer();
		}
		else
		{
			LogWarning("Initialisation already attempted");
			return nullptr;
		}
	}
};

GLRGBTexture::~GLRGBTexture()
{
	if (renderer_dead)
	{
		LogWarning("GLRGBTexture being destroyed after renderer");
		return;
	}

	owner->delete_texture_object(this->tex_id);
}
GLPaletteTexture::~GLPaletteTexture()
{
	if (renderer_dead)
	{
		LogWarning("GLPaletteTexture being destroyed after renderer");
		return;
	}
	owner->delete_texture_object(this->tex_id);
}
GLSurface::~GLSurface()
{
	if (renderer_dead)
	{
		LogWarning("GLSurface being destroyed after renderer");
		return;
	}
	if (this->fbo_id)
		owner->delete_framebuffer_object(this->fbo_id);
	if (this->tex_id)
		owner->delete_texture_object(this->tex_id);
}

} // anonymous namespace

RendererFactory *getGLES30RendererFactory() { return new OGLES30RendererFactory(); }

} // namespace OpenApoc
