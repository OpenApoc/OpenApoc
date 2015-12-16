#ifndef FUNCTION_CPP_GENERATED_HEADEREGL_HPP
#define FUNCTION_CPP_GENERATED_HEADEREGL_HPP



#if defined(__glew_h__) || defined(__GLEW_H__)
#error Attempt to include auto-generated header after including glew.h
#endif
#if defined(__gl_h_) || defined(__GL_H__) || defined(__gl2_h_)
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

#include "eglplatform.h"

namespace egl
{
	typedef unsigned int Boolean;
	typedef void *Display;
	typedef void *Config;
	typedef void *Surface;
	typedef void *Context;
	typedef void(*__eglMustCastToProperFunctionPointerType)(void);
	typedef void *Sync;
	typedef intptr_t Attrib;
	typedef khronos_utime_nanoseconds_t EGLTime;
	typedef void *Image;
	typedef signed   char          khronos_int8_t;
	typedef unsigned char          khronos_uint8_t;
	typedef EGLNativeDisplayType NativeDisplayType;
	typedef EGLNativeWindowType NativeWindowType;
	typedef unsigned int Enum;
	typedef void *ClientBuffer;

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


const NativeDisplayType DEFAULT_DISPLAY = ((NativeDisplayType)0);
const Context NO_CONTEXT = ((Context)0);
const Display NO_DISPLAY = ((Display)0);
const Surface NO_SURFACE = ((Surface)0);
const EGLint DONT_CARE = ((EGLint)-1);
const EGLint UNKNOWN = ((EGLint)-1);
enum
{
	VERSION_1_0                              = 1,
	VERSION_1_1                              = 1,
	VERSION_1_2                              = 1,
	VERSION_1_3                              = 1,
	VERSION_1_4                              = 1,
	SUCCESS                                  = 0x3000,
	NOT_INITIALIZED                          = 0x3001,
	BAD_ACCESS                               = 0x3002,
	BAD_ALLOC                                = 0x3003,
	BAD_ATTRIBUTE                            = 0x3004,
	BAD_CONFIG                               = 0x3005,
	BAD_CONTEXT                              = 0x3006,
	BAD_CURRENT_SURFACE                      = 0x3007,
	BAD_DISPLAY                              = 0x3008,
	BAD_MATCH                                = 0x3009,
	BAD_NATIVE_PIXMAP                        = 0x300A,
	BAD_NATIVE_WINDOW                        = 0x300B,
	BAD_PARAMETER                            = 0x300C,
	BAD_SURFACE                              = 0x300D,
	CONTEXT_LOST                             = 0x300E,
	MAX_PBUFFER_HEIGHT                       = 0x302A,
	MAX_PBUFFER_PIXELS                       = 0x302B,
	MAX_PBUFFER_WIDTH                        = 0x302C,
	NATIVE_VISUAL_ID                         = 0x302E,
	NATIVE_VISUAL_TYPE                       = 0x302F,
	NONE                                     = 0x3038,
	BIND_TO_TEXTURE_RGB                      = 0x3039,
	RGB_BUFFER                               = 0x308E,
	LUMINANCE_BUFFER                         = 0x308F,
	NO_TEXTURE                               = 0x305C,
	TEXTURE_RGB                              = 0x305D,
	TEXTURE_RGBA                             = 0x305E,
	TEXTURE_2D                               = 0x305F,
	VENDOR                                   = 0x3053,
	VERSION                                  = 0x3054,
	EXTENSIONS                               = 0x3055,
	CLIENT_APIS                              = 0x308D,
	HEIGHT                                   = 0x3056,
	WIDTH                                    = 0x3057,
	LARGEST_PBUFFER                          = 0x3058,
	TEXTURE_FORMAT                           = 0x3080,
	TEXTURE_TARGET                           = 0x3081,
	MIPMAP_TEXTURE                           = 0x3082,
	MIPMAP_LEVEL                             = 0x3083,
	RENDER_BUFFER                            = 0x3086,
	VG_COLORSPACE                            = 0x3087,
	VG_ALPHA_FORMAT                          = 0x3088,
	HORIZONTAL_RESOLUTION                    = 0x3090,
	VERTICAL_RESOLUTION                      = 0x3091,
	PIXEL_ASPECT_RATIO                       = 0x3092,
	SWAP_BEHAVIOR                            = 0x3093,
	MULTISAMPLE_RESOLVE                      = 0x3099,
	BACK_BUFFER                              = 0x3084,
	SINGLE_BUFFER                            = 0x3085,
	VG_COLORSPACE_sRGB                       = 0x3089,
	VG_COLORSPACE_LINEAR                     = 0x308A,
	VG_ALPHA_FORMAT_NONPRE                   = 0x308B,
	VG_ALPHA_FORMAT_PRE                      = 0x308C,
	DISPLAY_SCALING                          = 10000,
	BUFFER_PRESERVED                         = 0x3094,
	BUFFER_DESTROYED                         = 0x3095,
	OPENVG_IMAGE                             = 0x3096,
	CONTEXT_CLIENT_TYPE                      = 0x3097,
	CONTEXT_CLIENT_VERSION                   = 0x3098,
	MULTISAMPLE_RESOLVE_DEFAULT              = 0x309A,
	MULTISAMPLE_RESOLVE_BOX                  = 0x309B,
	OPENGL_ES_API                            = 0x30A0,
	OPENVG_API                               = 0x30A1,
	OPENGL_API                               = 0x30A2,
	DRAW                                     = 0x3059,
	READ                                     = 0x305A,
	CORE_NATIVE_ENGINE                       = 0x305B,
};

namespace conf
{
	enum
	{
		FALSE_                                   = 0,
		TRUE_                                    = 1,
		BUFFER_SIZE                              = 0x3020,
		ALPHA_SIZE                               = 0x3021,
		BLUE_SIZE                                = 0x3022,
		GREEN_SIZE                               = 0x3023,
		RED_SIZE                                 = 0x3024,
		DEPTH_SIZE                               = 0x3025,
		STENCIL_SIZE                             = 0x3026,
		CONFIG_CAVEAT                            = 0x3027,
		CONFIG_ID                                = 0x3028,
		LEVEL                                    = 0x3029,
		NATIVE_RENDERABLE                        = 0x302D,
		SAMPLES                                  = 0x3031,
		SAMPLE_BUFFERS                           = 0x3032,
		SURFACE_TYPE                             = 0x3033,
		TRANSPARENT_TYPE                         = 0x3034,
		TRANSPARENT_BLUE_VALUE                   = 0x3035,
		TRANSPARENT_GREEN_VALUE                  = 0x3036,
		TRANSPARENT_RED_VALUE                    = 0x3037,
		BIND_TO_TEXTURE_RGBA                     = 0x303A,
		MIN_SWAP_INTERVAL                        = 0x303B,
		MAX_SWAP_INTERVAL                        = 0x303C,
		LUMINANCE_SIZE                           = 0x303D,
		ALPHA_MASK_SIZE                          = 0x303E,
		COLOR_BUFFER_TYPE                        = 0x303F,
		RENDERABLE_TYPE                          = 0x3040,
		MATCH_NATIVE_PIXMAP                      = 0x3041,
		CONFORMANT                               = 0x3042,
		SLOW_CONFIG                              = 0x3050,
		NON_CONFORMANT_CONFIG                    = 0x3051,
		TRANSPARENT_RGB                          = 0x3052,
		PBUFFER_BIT                              = 0x0001,
		PIXMAP_BIT                               = 0x0002,
		WINDOW_BIT                               = 0x0004,
		VG_COLORSPACE_LINEAR_BIT                 = 0x0020,
		VG_ALPHA_FORMAT_PRE_BIT                  = 0x0040,
		MULTISAMPLE_RESOLVE_BOX_BIT              = 0x0200,
		SWAP_BEHAVIOR_PRESERVED_BIT              = 0x0400,
		OPENGL_ES_BIT                            = 0x0001,
		OPENVG_BIT                               = 0x0002,
		OPENGL_ES2_BIT                           = 0x0004,
		OPENGL_BIT                               = 0x0008
	}; //enum 
} // namespace conf

	extern int (CODEGEN_FUNCPTR *GetError)(void);
	extern Display (CODEGEN_FUNCPTR *GetDisplay)(NativeDisplayType display_id);
	extern Boolean (CODEGEN_FUNCPTR *Initialize)(Display dpy, int *major, int *minor);
	extern Boolean (CODEGEN_FUNCPTR *Terminate)(Display dpy);
	extern const char * (CODEGEN_FUNCPTR *QueryString)(Display dpy, int name);
	extern Boolean (CODEGEN_FUNCPTR *GetConfigs)(Display dpy, Config *configs, int config_size, int *num_config);
	extern Boolean (CODEGEN_FUNCPTR *ChooseConfig)(Display dpy, const int *attrib_list, Config *configs, int config_size, int *num_config);
	extern Boolean (CODEGEN_FUNCPTR *GetConfigAttrib)(Display dpy, Config config, int attribute, int *value);
	extern Surface (CODEGEN_FUNCPTR *CreateWindowSurface)(Display dpy, Config config, NativeWindowType win, const int *attrib_list);
	extern Surface (CODEGEN_FUNCPTR *CreatePbufferSurface)(Display dpy, Config config, const int *attrib_list);
	extern Surface (CODEGEN_FUNCPTR *CreatePixmapSurface)(Display dpy, Config config, NativePixmapType pixmap, const int *attrib_list);
	extern Boolean (CODEGEN_FUNCPTR *DestroySurface)(Display dpy, Surface surface);
	extern Boolean (CODEGEN_FUNCPTR *QuerySurface)(Display dpy, Surface surface, int attribute, int *value);
	extern Boolean (CODEGEN_FUNCPTR *BindAPI)(Enum api);
	extern Enum (CODEGEN_FUNCPTR *QueryAPI)(void);
	extern Boolean (CODEGEN_FUNCPTR *WaitClient)(void);
	extern Boolean (CODEGEN_FUNCPTR *ReleaseThread)(void);
	extern Surface (CODEGEN_FUNCPTR *CreatePbufferFromClientBuffer)(Display dpy, Enum buftype, ClientBuffer buffer, Config config, const int *attrib_list);
	extern Boolean (CODEGEN_FUNCPTR *SurfaceAttrib)(Display dpy, Surface surface, int attribute, int value);
	extern Boolean (CODEGEN_FUNCPTR *BindTexImage)(Display dpy, Surface surface, int buffer);
	extern Boolean (CODEGEN_FUNCPTR *ReleaseTexImage)(Display dpy, Surface surface, int buffer);
	extern Boolean (CODEGEN_FUNCPTR *SwapInterval)(Display dpy, int interval);
	extern Context (CODEGEN_FUNCPTR *CreateContext)(Display dpy, Config config, Context share_context, const int *attrib_list);
	extern Boolean (CODEGEN_FUNCPTR *DestroyContext)(Display dpy, Context ctx);
	extern Boolean (CODEGEN_FUNCPTR *MakeCurrent)(Display dpy, Surface draw, Surface read, Context ctx);
	extern Context (CODEGEN_FUNCPTR *GetCurrentContext)(void);
	extern Surface (CODEGEN_FUNCPTR *GetCurrentSurface)(int readdraw);
	extern Display (CODEGEN_FUNCPTR *GetCurrentDisplay)(void);
	extern Boolean (CODEGEN_FUNCPTR *QueryContext)(Display dpy, Context ctx, int attribute, int *value);
	extern Boolean (CODEGEN_FUNCPTR *WaitGL)(void);
	extern Boolean (CODEGEN_FUNCPTR *WaitNative)(int engine);
	extern Boolean (CODEGEN_FUNCPTR *SwapBuffers)(Display dpy, Surface surface);
	extern Boolean (CODEGEN_FUNCPTR *CopyBuffers)(Display dpy, Surface surface, NativePixmapType target);
	extern __eglMustCastToProperFunctionPointerType (CODEGEN_FUNCPTR *GetProcAddress)(const char *procname);

namespace sys
{
    exts::LoadTest LoadFunctions();
    
    int getMinorVersion();
    int getMajorVersion();
    bool IsVersionGEQ(int majorVersion, int minorVersion);
    
} // namespace sys



} // namespace egl
#endif // FUNCTION_CPP_GENERATED_HEADEREGL_HPP

