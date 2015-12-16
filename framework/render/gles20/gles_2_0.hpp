#ifndef FUNCTION_CPP_GENERATED_HEADEROPENGLES_HPP
#define FUNCTION_CPP_GENERATED_HEADEROPENGLES_HPP



#if defined(__glew_h__) || defined(__GLEW_H__)
#error Attempt to include auto-generated header after including glew.h
#endif
#if defined(__gl_h_) || defined(__GL_H__)
#error Attempt to include auto-generated header after including gl.h
#endif
#if defined(__glext_h_) || defined(__GLEXT_H_) || defined(__gl2ext_h_)
#error Attempt to include auto-generated header after including glext.h
#endif
#if defined(__gltypes_h_)
#error Attempt to include auto-generated header after gltypes.h
#endif
#if defined(__gl_ATI_h_)
#error Attempt to include auto-generated header after including glATI.h
#endif

#define __glew_h__
#define __GLEW_H__
#define __gl_h_
#define __GL_H__
#define __glext_h_
#define __gl2ext_h_
#define __GLEXT_H_
#define __gltypes_h_
#define __gl_ATI_h_

#ifndef APIENTRY
	#if defined(__MINGW32__)
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN 1
		#endif
		#ifndef NOMINMAX
			#define NOMINMAX
		#endif
		#include <windows.h>
	#elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN 1
		#endif
		#ifndef NOMINMAX
			#define NOMINMAX
		#endif
		#include <windows.h>
	#else
		#define APIENTRY
	#endif
#endif /*APIENTRY*/

#ifndef CODEGEN_FUNCPTR
	#define CODEGEN_REMOVE_FUNCPTR
	#if defined(_WIN32)
		#define CODEGEN_FUNCPTR APIENTRY
	#else
		#define CODEGEN_FUNCPTR
	#endif
#endif /*CODEGEN_FUNCPTR*/

#ifndef GLAPI
	#define GLAPI extern
#endif


#ifndef GLES_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GLES_LOAD_GEN_BASIC_OPENGL_TYPEDEFS


#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/

#include <stddef.h>
#ifndef GLEXT_64_TYPES_DEFINED
/* This code block is duplicated in glxext.h, so must be protected */
#define GLEXT_64_TYPES_DEFINED
/* Define int32_t, int64_t, and uint64_t types for UST/MSC */
/* (as used in the GL_EXT_timer_query extension). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <inttypes.h>
#elif defined(__sun__) || defined(__digital__)
#include <inttypes.h>
#if defined(__STDC__)
#if defined(__arch64__) || defined(_LP64)
typedef long int int64_t;
typedef unsigned long int uint64_t;
#else
typedef long long int int64_t;
typedef unsigned long long int uint64_t;
#endif /* __arch64__ */
#endif /* __STDC__ */
#elif defined( __VMS ) || defined(__sgi)
#include <inttypes.h>
#elif defined(__SCO__) || defined(__USLC__)
#include <stdint.h>
#elif defined(__UNIXOS2__) || defined(__SOL64__)
typedef long int int32_t;
typedef long long int int64_t;
typedef unsigned long long int uint64_t;
#elif defined(_WIN32) && defined(__GNUC__)
#include <stdint.h>
#elif defined(_WIN32)
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
/* Fallback if nothing above works */
#include <inttypes.h>
#endif
#endif

#include "khrplatform.h"

typedef signed   char          khronos_int8_t;
typedef unsigned char          khronos_uint8_t;
typedef void             GLvoid;

typedef char             GLchar;

typedef unsigned int     GLenum;

typedef unsigned char    GLboolean;

typedef unsigned int     GLbitfield;

typedef khronos_int8_t   GLbyte;

typedef short            GLshort;

typedef int              GLint;

typedef int              GLsizei;

typedef khronos_uint8_t  GLubyte;

typedef unsigned short   GLushort;

typedef unsigned int     GLuint;

typedef khronos_float_t  GLfloat;

typedef khronos_float_t  GLclampf;

typedef khronos_int32_t  GLfixed;

typedef khronos_intptr_t GLintptr;

typedef khronos_ssize_t  GLsizeiptr;


namespace gl
{


namespace exts
{
    class LoadTest
    {
    public:
        // C++11 safe bool idiom
        explicit operator bool() const
        {
            return m_isLoaded;
        }
        int GetNumMissing() const { return m_numMissing; }
        LoadTest(): m_isLoaded(false), m_numMissing(0) {}
        LoadTest(bool isLoaded, int numMissing) : m_isLoaded(isLoaded), m_numMissing(numMissing) {}
    private:
        bool m_isLoaded;
        int m_numMissing;
    };
} // namespace exts


enum
{
	ACTIVE_ATTRIBUTES                   = 0x8B89,
	ACTIVE_ATTRIBUTE_MAX_LENGTH         = 0x8B8A,
	ACTIVE_TEXTURE                      = 0x84E0,
	ACTIVE_UNIFORMS                     = 0x8B86,
	ACTIVE_UNIFORM_MAX_LENGTH           = 0x8B87,
	ALIASED_LINE_WIDTH_RANGE            = 0x846E,
	ALIASED_POINT_SIZE_RANGE            = 0x846D,
	ALPHA                               = 0x1906,
	ALPHA_BITS                          = 0x0D55,
	ALWAYS                              = 0x0207,
	ARRAY_BUFFER                        = 0x8892,
	ARRAY_BUFFER_BINDING                = 0x8894,
	ATTACHED_SHADERS                    = 0x8B85,
	BACK                                = 0x0405,
	BLEND                               = 0x0BE2,
	BLEND_COLOR                         = 0x8005,
	BLEND_DST_ALPHA                     = 0x80CA,
	BLEND_DST_RGB                       = 0x80C8,
	BLEND_EQUATION                      = 0x8009,
	BLEND_EQUATION_ALPHA                = 0x883D,
	BLEND_EQUATION_RGB                  = 0x8009,
	BLEND_SRC_ALPHA                     = 0x80CB,
	BLEND_SRC_RGB                       = 0x80C9,
	BLUE_BITS                           = 0x0D54,
	BOOL                                = 0x8B56,
	BOOL_VEC2                           = 0x8B57,
	BOOL_VEC3                           = 0x8B58,
	BOOL_VEC4                           = 0x8B59,
	BUFFER_SIZE                         = 0x8764,
	BUFFER_USAGE                        = 0x8765,
	BYTE                                = 0x1400,
	CCW                                 = 0x0901,
	CLAMP_TO_EDGE                       = 0x812F,
	COLOR_ATTACHMENT0                   = 0x8CE0,
	COLOR_BUFFER_BIT                    = 0x00004000,
	COLOR_CLEAR_VALUE                   = 0x0C22,
	COLOR_WRITEMASK                     = 0x0C23,
	COMPILE_STATUS                      = 0x8B81,
	COMPRESSED_TEXTURE_FORMATS          = 0x86A3,
	CONSTANT_ALPHA                      = 0x8003,
	CONSTANT_COLOR                      = 0x8001,
	CULL_FACE                           = 0x0B44,
	CULL_FACE_MODE                      = 0x0B45,
	CURRENT_PROGRAM                     = 0x8B8D,
	CURRENT_VERTEX_ATTRIB               = 0x8626,
	CW                                  = 0x0900,
	DECR                                = 0x1E03,
	DECR_WRAP                           = 0x8508,
	DELETE_STATUS                       = 0x8B80,
	DEPTH_ATTACHMENT                    = 0x8D00,
	DEPTH_BITS                          = 0x0D56,
	DEPTH_BUFFER_BIT                    = 0x00000100,
	DEPTH_CLEAR_VALUE                   = 0x0B73,
	DEPTH_COMPONENT                     = 0x1902,
	DEPTH_COMPONENT16                   = 0x81A5,
	DEPTH_FUNC                          = 0x0B74,
	DEPTH_RANGE                         = 0x0B70,
	DEPTH_TEST                          = 0x0B71,
	DEPTH_WRITEMASK                     = 0x0B72,
	DITHER                              = 0x0BD0,
	DONT_CARE                           = 0x1100,
	DST_ALPHA                           = 0x0304,
	DST_COLOR                           = 0x0306,
	DYNAMIC_DRAW                        = 0x88E8,
	ELEMENT_ARRAY_BUFFER                = 0x8893,
	ELEMENT_ARRAY_BUFFER_BINDING        = 0x8895,
	EQUAL                               = 0x0202,
	ES_VERSION_2_0                      = 1,
	EXTENSIONS                          = 0x1F03,
	FALSE_                              = 0,
	FASTEST                             = 0x1101,
	FIXED                               = 0x140C,
	FLOAT                               = 0x1406,
	FLOAT_MAT2                          = 0x8B5A,
	FLOAT_MAT3                          = 0x8B5B,
	FLOAT_MAT4                          = 0x8B5C,
	FLOAT_VEC2                          = 0x8B50,
	FLOAT_VEC3                          = 0x8B51,
	FLOAT_VEC4                          = 0x8B52,
	FRAGMENT_SHADER                     = 0x8B30,
	FRAMEBUFFER                         = 0x8D40,
	FRAMEBUFFER_ATTACHMENT_OBJECT_NAME  = 0x8CD1,
	FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE  = 0x8CD0,
	FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE = 0x8CD3,
	FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL = 0x8CD2,
	FRAMEBUFFER_BINDING                 = 0x8CA6,
	FRAMEBUFFER_COMPLETE                = 0x8CD5,
	FRAMEBUFFER_INCOMPLETE_ATTACHMENT   = 0x8CD6,
	FRAMEBUFFER_INCOMPLETE_DIMENSIONS   = 0x8CD9,
	FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7,
	FRAMEBUFFER_UNSUPPORTED             = 0x8CDD,
	FRONT                               = 0x0404,
	FRONT_AND_BACK                      = 0x0408,
	FRONT_FACE                          = 0x0B46,
	FUNC_ADD                            = 0x8006,
	FUNC_REVERSE_SUBTRACT               = 0x800B,
	FUNC_SUBTRACT                       = 0x800A,
	GENERATE_MIPMAP_HINT                = 0x8192,
	GEQUAL                              = 0x0206,
	GREATER                             = 0x0204,
	GREEN_BITS                          = 0x0D53,
	HIGH_FLOAT                          = 0x8DF2,
	HIGH_INT                            = 0x8DF5,
	IMPLEMENTATION_COLOR_READ_FORMAT    = 0x8B9B,
	IMPLEMENTATION_COLOR_READ_TYPE      = 0x8B9A,
	INCR                                = 0x1E02,
	INCR_WRAP                           = 0x8507,
	INFO_LOG_LENGTH                     = 0x8B84,
	INT                                 = 0x1404,
	INT_VEC2                            = 0x8B53,
	INT_VEC3                            = 0x8B54,
	INT_VEC4                            = 0x8B55,
	INVALID_ENUM                        = 0x0500,
	INVALID_FRAMEBUFFER_OPERATION       = 0x0506,
	INVALID_OPERATION                   = 0x0502,
	INVALID_VALUE                       = 0x0501,
	INVERT                              = 0x150A,
	KEEP                                = 0x1E00,
	LEQUAL                              = 0x0203,
	LESS                                = 0x0201,
	LINEAR                              = 0x2601,
	LINEAR_MIPMAP_LINEAR                = 0x2703,
	LINEAR_MIPMAP_NEAREST               = 0x2701,
	LINES                               = 0x0001,
	LINE_LOOP                           = 0x0002,
	LINE_STRIP                          = 0x0003,
	LINE_WIDTH                          = 0x0B21,
	LINK_STATUS                         = 0x8B82,
	LOW_FLOAT                           = 0x8DF0,
	LOW_INT                             = 0x8DF3,
	LUMINANCE                           = 0x1909,
	LUMINANCE_ALPHA                     = 0x190A,
	MAX_COMBINED_TEXTURE_IMAGE_UNITS    = 0x8B4D,
	MAX_CUBE_MAP_TEXTURE_SIZE           = 0x851C,
	MAX_FRAGMENT_UNIFORM_VECTORS        = 0x8DFD,
	MAX_RENDERBUFFER_SIZE               = 0x84E8,
	MAX_TEXTURE_IMAGE_UNITS             = 0x8872,
	MAX_TEXTURE_SIZE                    = 0x0D33,
	MAX_VARYING_VECTORS                 = 0x8DFC,
	MAX_VERTEX_ATTRIBS                  = 0x8869,
	MAX_VERTEX_TEXTURE_IMAGE_UNITS      = 0x8B4C,
	MAX_VERTEX_UNIFORM_VECTORS          = 0x8DFB,
	MAX_VIEWPORT_DIMS                   = 0x0D3A,
	MEDIUM_FLOAT                        = 0x8DF1,
	MEDIUM_INT                          = 0x8DF4,
	MIRRORED_REPEAT                     = 0x8370,
	NEAREST                             = 0x2600,
	NEAREST_MIPMAP_LINEAR               = 0x2702,
	NEAREST_MIPMAP_NEAREST              = 0x2700,
	NEVER                               = 0x0200,
	NICEST                              = 0x1102,
	NONE                                = 0,
	NOTEQUAL                            = 0x0205,
	NO_ERROR_                           = 0,
	NUM_COMPRESSED_TEXTURE_FORMATS      = 0x86A2,
	NUM_SHADER_BINARY_FORMATS           = 0x8DF9,
	ONE                                 = 1,
	ONE_MINUS_CONSTANT_ALPHA            = 0x8004,
	ONE_MINUS_CONSTANT_COLOR            = 0x8002,
	ONE_MINUS_DST_ALPHA                 = 0x0305,
	ONE_MINUS_DST_COLOR                 = 0x0307,
	ONE_MINUS_SRC_ALPHA                 = 0x0303,
	ONE_MINUS_SRC_COLOR                 = 0x0301,
	OUT_OF_MEMORY                       = 0x0505,
	PACK_ALIGNMENT                      = 0x0D05,
	POINTS                              = 0x0000,
	POLYGON_OFFSET_FACTOR               = 0x8038,
	POLYGON_OFFSET_FILL                 = 0x8037,
	POLYGON_OFFSET_UNITS                = 0x2A00,
	RED_BITS                            = 0x0D52,
	RENDERBUFFER                        = 0x8D41,
	RENDERBUFFER_ALPHA_SIZE             = 0x8D53,
	RENDERBUFFER_BINDING                = 0x8CA7,
	RENDERBUFFER_BLUE_SIZE              = 0x8D52,
	RENDERBUFFER_DEPTH_SIZE             = 0x8D54,
	RENDERBUFFER_GREEN_SIZE             = 0x8D51,
	RENDERBUFFER_HEIGHT                 = 0x8D43,
	RENDERBUFFER_INTERNAL_FORMAT        = 0x8D44,
	RENDERBUFFER_RED_SIZE               = 0x8D50,
	RENDERBUFFER_STENCIL_SIZE           = 0x8D55,
	RENDERBUFFER_WIDTH                  = 0x8D42,
	RENDERER                            = 0x1F01,
	REPEAT                              = 0x2901,
	REPLACE                             = 0x1E01,
	RGB                                 = 0x1907,
	RGB565                              = 0x8D62,
	RGB5_A1                             = 0x8057,
	RGBA                                = 0x1908,
	RGBA4                               = 0x8056,
	SAMPLER_2D                          = 0x8B5E,
	SAMPLER_CUBE                        = 0x8B60,
	SAMPLES                             = 0x80A9,
	SAMPLE_ALPHA_TO_COVERAGE            = 0x809E,
	SAMPLE_BUFFERS                      = 0x80A8,
	SAMPLE_COVERAGE                     = 0x80A0,
	SAMPLE_COVERAGE_INVERT              = 0x80AB,
	SAMPLE_COVERAGE_VALUE               = 0x80AA,
	SCISSOR_BOX                         = 0x0C10,
	SCISSOR_TEST                        = 0x0C11,
	SHADER_BINARY_FORMATS               = 0x8DF8,
	SHADER_COMPILER                     = 0x8DFA,
	SHADER_SOURCE_LENGTH                = 0x8B88,
	SHADER_TYPE                         = 0x8B4F,
	SHADING_LANGUAGE_VERSION            = 0x8B8C,
	SHORT                               = 0x1402,
	SRC_ALPHA                           = 0x0302,
	SRC_ALPHA_SATURATE                  = 0x0308,
	SRC_COLOR                           = 0x0300,
	STATIC_DRAW                         = 0x88E4,
	STENCIL_ATTACHMENT                  = 0x8D20,
	STENCIL_BACK_FAIL                   = 0x8801,
	STENCIL_BACK_FUNC                   = 0x8800,
	STENCIL_BACK_PASS_DEPTH_FAIL        = 0x8802,
	STENCIL_BACK_PASS_DEPTH_PASS        = 0x8803,
	STENCIL_BACK_REF                    = 0x8CA3,
	STENCIL_BACK_VALUE_MASK             = 0x8CA4,
	STENCIL_BACK_WRITEMASK              = 0x8CA5,
	STENCIL_BITS                        = 0x0D57,
	STENCIL_BUFFER_BIT                  = 0x00000400,
	STENCIL_CLEAR_VALUE                 = 0x0B91,
	STENCIL_FAIL                        = 0x0B94,
	STENCIL_FUNC                        = 0x0B92,
	STENCIL_INDEX8                      = 0x8D48,
	STENCIL_PASS_DEPTH_FAIL             = 0x0B95,
	STENCIL_PASS_DEPTH_PASS             = 0x0B96,
	STENCIL_REF                         = 0x0B97,
	STENCIL_TEST                        = 0x0B90,
	STENCIL_VALUE_MASK                  = 0x0B93,
	STENCIL_WRITEMASK                   = 0x0B98,
	STREAM_DRAW                         = 0x88E0,
	SUBPIXEL_BITS                       = 0x0D50,
	TEXTURE                             = 0x1702,
	TEXTURE0                            = 0x84C0,
	TEXTURE1                            = 0x84C1,
	TEXTURE10                           = 0x84CA,
	TEXTURE11                           = 0x84CB,
	TEXTURE12                           = 0x84CC,
	TEXTURE13                           = 0x84CD,
	TEXTURE14                           = 0x84CE,
	TEXTURE15                           = 0x84CF,
	TEXTURE16                           = 0x84D0,
	TEXTURE17                           = 0x84D1,
	TEXTURE18                           = 0x84D2,
	TEXTURE19                           = 0x84D3,
	TEXTURE2                            = 0x84C2,
	TEXTURE20                           = 0x84D4,
	TEXTURE21                           = 0x84D5,
	TEXTURE22                           = 0x84D6,
	TEXTURE23                           = 0x84D7,
	TEXTURE24                           = 0x84D8,
	TEXTURE25                           = 0x84D9,
	TEXTURE26                           = 0x84DA,
	TEXTURE27                           = 0x84DB,
	TEXTURE28                           = 0x84DC,
	TEXTURE29                           = 0x84DD,
	TEXTURE3                            = 0x84C3,
	TEXTURE30                           = 0x84DE,
	TEXTURE31                           = 0x84DF,
	TEXTURE4                            = 0x84C4,
	TEXTURE5                            = 0x84C5,
	TEXTURE6                            = 0x84C6,
	TEXTURE7                            = 0x84C7,
	TEXTURE8                            = 0x84C8,
	TEXTURE9                            = 0x84C9,
	TEXTURE_2D                          = 0x0DE1,
	TEXTURE_BINDING_2D                  = 0x8069,
	TEXTURE_BINDING_CUBE_MAP            = 0x8514,
	TEXTURE_CUBE_MAP                    = 0x8513,
	TEXTURE_CUBE_MAP_NEGATIVE_X         = 0x8516,
	TEXTURE_CUBE_MAP_NEGATIVE_Y         = 0x8518,
	TEXTURE_CUBE_MAP_NEGATIVE_Z         = 0x851A,
	TEXTURE_CUBE_MAP_POSITIVE_X         = 0x8515,
	TEXTURE_CUBE_MAP_POSITIVE_Y         = 0x8517,
	TEXTURE_CUBE_MAP_POSITIVE_Z         = 0x8519,
	TEXTURE_MAG_FILTER                  = 0x2800,
	TEXTURE_MIN_FILTER                  = 0x2801,
	TEXTURE_WRAP_S                      = 0x2802,
	TEXTURE_WRAP_T                      = 0x2803,
	TRIANGLES                           = 0x0004,
	TRIANGLE_FAN                        = 0x0006,
	TRIANGLE_STRIP                      = 0x0005,
	TRUE_                               = 1,
	UNPACK_ALIGNMENT                    = 0x0CF5,
	UNSIGNED_BYTE                       = 0x1401,
	UNSIGNED_INT                        = 0x1405,
	UNSIGNED_SHORT                      = 0x1403,
	UNSIGNED_SHORT_4_4_4_4              = 0x8033,
	UNSIGNED_SHORT_5_5_5_1              = 0x8034,
	UNSIGNED_SHORT_5_6_5                = 0x8363,
	VALIDATE_STATUS                     = 0x8B83,
	VENDOR                              = 0x1F00,
	VERSION                             = 0x1F02,
	VERTEX_ATTRIB_ARRAY_BUFFER_BINDING  = 0x889F,
	VERTEX_ATTRIB_ARRAY_ENABLED         = 0x8622,
	VERTEX_ATTRIB_ARRAY_NORMALIZED      = 0x886A,
	VERTEX_ATTRIB_ARRAY_POINTER         = 0x8645,
	VERTEX_ATTRIB_ARRAY_SIZE            = 0x8623,
	VERTEX_ATTRIB_ARRAY_STRIDE          = 0x8624,
	VERTEX_ATTRIB_ARRAY_TYPE            = 0x8625,
	VERTEX_SHADER                       = 0x8B31,
	VIEWPORT                            = 0x0BA2,
	ZERO                                = 0,

};


	void ActiveTexture(GLenum texture);
	void AttachShader(GLuint program, GLuint shader);
	void BindAttribLocation(GLuint program, GLuint index, const GLchar* name);
	void BindBuffer(GLenum target, GLuint buffer);
	void BindFramebuffer(GLenum target, GLuint framebuffer);
	void BindRenderbuffer(GLenum target, GLuint renderbuffer);
	void BindTexture(GLenum target, GLuint texture);
	void BlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void BlendEquation(GLenum mode);
	void BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
	void BlendFunc(GLenum sfactor, GLenum dfactor);
	void BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	void BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
	void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
	GLenum CheckFramebufferStatus(GLenum target);
	void Clear(GLbitfield mask);
	void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void ClearDepthf(GLclampf depth);
	void ClearStencil(GLint s);
	void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	void CompileShader(GLuint shader);
	void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
	void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
	void CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	GLuint CreateProgram(void);
	GLuint CreateShader(GLenum type);
	void CullFace(GLenum mode);
	void DeleteBuffers(GLsizei n, const GLuint* buffers);
	void DeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
	void DeleteProgram(GLuint program);
	void DeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
	void DeleteShader(GLuint shader);
	void DeleteTextures(GLsizei n, const GLuint* textures);
	void DepthFunc(GLenum func);
	void DepthMask(GLboolean flag);
	void DepthRangef(GLclampf zNear, GLclampf zFar);
	void DetachShader(GLuint program, GLuint shader);
	void Disable(GLenum cap);
	void DisableVertexAttribArray(GLuint index);
	void DrawArrays(GLenum mode, GLint first, GLsizei count);
	void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
	void Enable(GLenum cap);
	void EnableVertexAttribArray(GLuint index);
	void Finish(void);
	void Flush(void);
	void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void FrontFace(GLenum mode);
	void GenBuffers(GLsizei n, GLuint* buffers);
	void GenerateMipmap(GLenum target);
	void GenFramebuffers(GLsizei n, GLuint* framebuffers);
	void GenRenderbuffers(GLsizei n, GLuint* renderbuffers);
	void GenTextures(GLsizei n, GLuint* textures);
	void GetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
	void GetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
	void GetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
	GLint GetAttribLocation(GLuint program, const GLchar* name);
	void GetBooleanv(GLenum pname, GLboolean* params);
	void GetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
	GLenum GetError(void);
	void GetFloatv(GLenum pname, GLfloat* params);
	void GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params);
	void GetIntegerv(GLenum pname, GLint* params);
	void GetProgramiv(GLuint program, GLenum pname, GLint* params);
	void GetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
	void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);
	void GetShaderiv(GLuint shader, GLenum pname, GLint* params);
	void GetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
	void GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
	void GetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
	const GLubyte* GetString(GLenum name);
	void GetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
	void GetTexParameteriv(GLenum target, GLenum pname, GLint* params);
	void GetUniformfv(GLuint program, GLint location, GLfloat* params);
	void GetUniformiv(GLuint program, GLint location, GLint* params);
	GLint GetUniformLocation(GLuint program, const GLchar* name);
	void GetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
	void GetVertexAttribiv(GLuint index, GLenum pname, GLint* params);
	void GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer);
	void Hint(GLenum target, GLenum mode);
	GLboolean IsBuffer(GLuint buffer);
	GLboolean IsEnabled(GLenum cap);
	GLboolean IsFramebuffer(GLuint framebuffer);
	GLboolean IsProgram(GLuint program);
	GLboolean IsRenderbuffer(GLuint renderbuffer);
	GLboolean IsShader(GLuint shader);
	GLboolean IsTexture(GLuint texture);
	void LineWidth(GLfloat width);
	void LinkProgram(GLuint program);
	void PixelStorei(GLenum pname, GLint param);
	void PolygonOffset(GLfloat factor, GLfloat units);
	void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
	void ReleaseShaderCompiler(void);
	void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void SampleCoverage(GLclampf value, GLboolean invert);
	void Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
	void ShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
	void ShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
	void StencilFunc(GLenum func, GLint ref, GLuint mask);
	void StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
	void StencilMask(GLuint mask);
	void StencilMaskSeparate(GLenum face, GLuint mask);
	void StencilOp(GLenum fail, GLenum zfail, GLenum zpass);
	void StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
	void TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	void TexParameterf(GLenum target, GLenum pname, GLfloat param);
	void TexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
	void TexParameteri(GLenum target, GLenum pname, GLint param);
	void TexParameteriv(GLenum target, GLenum pname, const GLint* params);
	void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
	void Uniform1f(GLint location, GLfloat x);
	void Uniform1fv(GLint location, GLsizei count, const GLfloat* v);
	void Uniform1i(GLint location, GLint x);
	void Uniform1iv(GLint location, GLsizei count, const GLint* v);
	void Uniform2f(GLint location, GLfloat x, GLfloat y);
	void Uniform2fv(GLint location, GLsizei count, const GLfloat* v);
	void Uniform2i(GLint location, GLint x, GLint y);
	void Uniform2iv(GLint location, GLsizei count, const GLint* v);
	void Uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
	void Uniform3fv(GLint location, GLsizei count, const GLfloat* v);
	void Uniform3i(GLint location, GLint x, GLint y, GLint z);
	void Uniform3iv(GLint location, GLsizei count, const GLint* v);
	void Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void Uniform4fv(GLint location, GLsizei count, const GLfloat* v);
	void Uniform4i(GLint location, GLint x, GLint y, GLint z, GLint w);
	void Uniform4iv(GLint location, GLsizei count, const GLint* v);
	void UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void UseProgram(GLuint program);
	void ValidateProgram(GLuint program);
	void VertexAttrib1f(GLuint indx, GLfloat x);
	void VertexAttrib1fv(GLuint indx, const GLfloat* values);
	void VertexAttrib2f(GLuint indx, GLfloat x, GLfloat y);
	void VertexAttrib2fv(GLuint indx, const GLfloat* values);
	void VertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
	void VertexAttrib3fv(GLuint indx, const GLfloat* values);
	void VertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void VertexAttrib4fv(GLuint indx, const GLfloat* values);
	void VertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void Viewport(GLint x, GLint y, GLsizei width, GLsizei height);


namespace sys
{
    exts::LoadTest LoadFunctions();
    
    int getMinorVersion();
    int getMajorVersion();
    bool IsVersionGEQ(int majorVersion, int minorVersion);
    
} // namespace sys



} // namespace gl
#endif // FUNCTION_CPP_GENERATED_HEADEROPENGLES_HPP

