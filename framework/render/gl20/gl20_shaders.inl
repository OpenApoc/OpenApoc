#ifdef GL20_SHADERS_INL
#error gl20_shaders.inl should only be included onse
#endif

#define GL20_SHADERS_INL

#include <string>

namespace
{

static const std::string RGBProgram_vertexSource =
	"uniform vec2 screen_size;\n"
	"uniform bool flipY;\n"
	"attribute vec2 texcoord;\n"
	"attribute vec2 position;\n"
	"varying vec2 texcoord_out;\n"
	"void main() {\n"
	"  texcoord_out = texcoord;\n"
	"  vec2 screenPosition = position / screen_size;\n"
	"  screenPosition -= vec2(0.5,0.5);\n"
	"  screenPosition *= vec2(2,2);\n"
	"  if (flipY) screenPosition.y = -screenPosition.y;\n"
	"  gl_Position = vec4(screenPosition.x, screenPosition.y, 0, 1);\n"
	"}\n";


static const std::string RGBProgram_fragmentSource =
	"uniform sampler2D tex;\n"
	"varying vec2 texcoord_out;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(tex, texcoord_out);\n"
	"}\n";

static const std::string PalProgram_fragmentSource =
	"uniform sampler2D tex;\n"
	"uniform sampler1D pal;\n"
	"varying vec2 texcoord_out;\n"
	"void main() {\n"
	"  gl_FragColor = texture1D(pal, texture2D(tex, texcoord_out).r);\n"
	"}\n";
}; //anonymous namespace
