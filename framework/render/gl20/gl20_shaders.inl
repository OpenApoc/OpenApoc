#ifdef GL20_SHADERS_INL
#error gl20_shaders.inl should only be included onse
#endif

#define GL20_SHADERS_INL

#include <string>

namespace
{

static const std::string RGBProgram_vertexSource =
	"uniform vec2 screen_size;\n"
	"uniform vec2 sprite_size;\n"
	"uniform bool flipY;\n"
	"attribute vec2 texcoord;\n"
	"attribute vec2 position;\n"
	"varying vec2 texcoord_out;\n"
	"void main() {\n"
	"  texcoord_out = texcoord;\n"
	"  position /= screen_size;\n"
	"  position -= vec2(0.5,0.5);\n"
	"  position *= 2;\n"
	"  if (flipY) position.y = -position.y;\n"
	"  gl_position = vec4(position.x, position.y, 0, 1);\n"
	"}\n";


static const std::string RGBProgram_fragmentSource =
	"uniform sampler2D tex;\n"
	"varying vec2 texcoord_out;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(tex, texcoord_out);\n"
	"}\n";
}; //anonymous namespace
