
#define CODEGEN_FUNCPTR APIENTRY

#if defined(__APPLE__)
#include <mach-o/dyld.h>

static void *AppleGLGetProcAddress(const GLubyte *name)
{
	static const struct mach_header *image = NULL;
	NSSymbol symbol;
	char *symbolName;
	if (NULL == image)
	{
		image = NSAddImage("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL",
			NSADDIMAGE_OPTION_RETURN_ON_ERROR);
	}
	/* prepend a '_' for the Unix C symbol mangling convention */
	symbolName = malloc(strlen((const char *)name) + 2);
	strcpy(symbolName + 1, (const char *)name);
	symbolName[0] = '_';
	symbol = NULL;
	/* if (NSIsSymbolNameDefined(symbolName))
	symbol = NSLookupAndBindSymbol(symbolName); */
	symbol = image ? NSLookupSymbolInImage(image, symbolName,
		NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
		NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR)
		: NULL;
	free(symbolName);
	return symbol ? NSAddressOfSymbol(symbol) : NULL;
}
#endif /* __APPLE__ */

#if defined(__sgi) || defined(__sun)
#include <dlfcn.h>
#include <stdio.h>

static void *SunGetProcAddress(const GLubyte *name)
{
	static void *h = NULL;
	static void *gpa;

	if (h == NULL)
	{
		if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL)
			return NULL;
		gpa = dlsym(h, "glXGetProcAddress");
	}

	if (gpa != NULL)
		return ((void *(*)(const GLubyte *))gpa)(name);
	else
		return dlsym(h, (const char *)name);
}
#endif /* __sgi || __sun */

#if defined(_WIN32)

#ifdef _MSC_VER
#pragma warning(disable : 4055)
#pragma warning(disable : 4054)
#endif

#define WIN_GLES2_LIBRARY_NAME "libEGL.dll"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static HMODULE eglLib = NULL;

static HMODULE WinLoadEglLib()
{
	eglLib = LoadLibraryA(WIN_GLES2_LIBRARY_NAME);
	return eglLib;
}

static int TestPointer(const PROC pTest)
{
	ptrdiff_t iTest;
	if (!pTest)
		return 0;
	iTest = (ptrdiff_t)pTest;

	if (iTest == 1 || iTest == 2 || iTest == 3 || iTest == -1)
		return 0;

	return 1;
}

static PROC WinGetProcAddress(const char *name)
{
	// GLES2.0: Don't try to use wglGetProcAddress, just try libEGL.dll
	/*
	PROC pFunc = wglGetProcAddress((LPCSTR)name);
	if (TestPointer(pFunc))
	{
	return pFunc;
	}
	*/
	//glMod = GetModuleHandleA(WIN_GLES2_LIBRARY_NAME);
	if (!eglLib)
		return nullptr;
	return (PROC)GetProcAddress(eglLib, (LPCSTR)name);
}

#define IntLoadLibrary()		WinLoadEglLib()
#define IntGetProcAddress(name) WinGetProcAddress(name)

#undef WIN_GLES2_LIBRARY_NAME 
#else
#if defined(__APPLE__)
#define IntGetProcAddress(name) AppleGLGetProcAddress(name)
#else
#if defined(__sgi) || defined(__sun)
#define IntGetProcAddress(name) SunGetProcAddress(name)
#else
#if defined(__ANDROID__)
#include <dlfcn.h>

static void* eglLib = NULL;

static void* AndroidLoadEglLib()
{
	eglLib = dlopen("libEGL.so", RTLD_LAZY);
	return eglLib;
}

static void* AndroidGetProcAddress(const char* name)
{
	if (!eglLib)
	{
		AndroidLoadEglLib();
	}
	return dlsym(eglLib, name);
}

#define IntLoadLibrary()		AndroidLoadEglLib()
#define IntGetProcAddress(name) AndroidGetProcAddress(name)

#else /* GLX */
#include <GL/glx.h>
#define IntGetProcAddress(name) (*glXGetProcAddressARB)(reinterpret_cast<const GLubyte *>(name))
#endif
#endif
#endif
#endif

namespace egl
{

	typedef int (CODEGEN_FUNCPTR *PFNGETERROR)(void);
	PFNGETERROR GetError = 0;
	typedef Display(CODEGEN_FUNCPTR *PFNGETDISPLAY)(NativeDisplayType display_id);
	PFNGETDISPLAY GetDisplay = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNINITIALIZE)(Display dpy, int *major, int *minor);
	PFNINITIALIZE Initialize = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNTERMINATE)(Display dpy);
	PFNTERMINATE Terminate = 0;
	typedef const char * (CODEGEN_FUNCPTR *PFNQUERYSTRING)(Display dpy, int name);
	PFNQUERYSTRING QueryString = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNGETCONFIGS)(Display dpy, Config *configs, int config_size, int *num_config);
	PFNGETCONFIGS GetConfigs = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNCHOOSECONFIG)(Display dpy, const int *attrib_list, Config *configs, int config_size, int *num_config);
	PFNCHOOSECONFIG ChooseConfig = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNGETCONFIGATTRIB)(Display dpy, Config config, int attribute, int *value);
	PFNGETCONFIGATTRIB GetConfigAttrib = 0;
	typedef Surface(CODEGEN_FUNCPTR *PFNCREATEWINDOWSURFACE)(Display dpy, Config config, NativeWindowType win, const int *attrib_list);
	PFNCREATEWINDOWSURFACE CreateWindowSurface = 0;
	typedef Surface(CODEGEN_FUNCPTR *PFNCREATEPBUFFERSURFACE)(Display dpy, Config config, const int *attrib_list);
	PFNCREATEPBUFFERSURFACE CreatePbufferSurface = 0;
	typedef Surface(CODEGEN_FUNCPTR *PFNCREATEPIXMAPSURFACE)(Display dpy, Config config, NativePixmapType pixmap, const int *attrib_list);
	PFNCREATEPIXMAPSURFACE CreatePixmapSurface = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNDESTROYSURFACE)(Display dpy, Surface surface);
	PFNDESTROYSURFACE DestroySurface = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNQUERYSURFACE)(Display dpy, Surface surface, int attribute, int *value);
	PFNQUERYSURFACE QuerySurface = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNBINDAPI)(Enum api);
	PFNBINDAPI BindAPI = 0;
	typedef Enum (CODEGEN_FUNCPTR *PFNQUERYAPI)(void);
	PFNQUERYAPI QueryAPI = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNWAITCLIENT)(void);
	PFNWAITCLIENT WaitClient = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNRELEASETHREAD)(void);
	PFNRELEASETHREAD ReleaseThread = 0;
	typedef Surface(CODEGEN_FUNCPTR *PFNCREATEPBUFFERFROMCLIENTBUFFER)(Display dpy, Enum buftype, ClientBuffer buffer, Config config, const int *attrib_list);
	PFNCREATEPBUFFERFROMCLIENTBUFFER CreatePbufferFromClientBuffer = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNSURFACEATTRIB)(Display dpy, Surface surface, int attribute, int value);
	PFNSURFACEATTRIB SurfaceAttrib = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNBINDTEXIMAGE)(Display dpy, Surface surface, int buffer);
	PFNBINDTEXIMAGE BindTexImage = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNRELEASETEXIMAGE)(Display dpy, Surface surface, int buffer);
	PFNRELEASETEXIMAGE ReleaseTexImage = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNSWAPINTERVAL)(Display dpy, int interval);
	PFNSWAPINTERVAL SwapInterval = 0;
	typedef Context(CODEGEN_FUNCPTR *PFNCREATECONTEXT)(Display dpy, Config config, Context share_context, const int *attrib_list);
	PFNCREATECONTEXT CreateContext = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNDESTROYCONTEXT)(Display dpy, Context ctx);
	PFNDESTROYCONTEXT DestroyContext = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNMAKECURRENT)(Display dpy, Surface draw, Surface read, Context ctx);
	PFNMAKECURRENT MakeCurrent = 0;
	typedef Context(CODEGEN_FUNCPTR *PFNGETCURRENTCONTEXT)(void);
	PFNGETCURRENTCONTEXT GetCurrentContext = 0;
	typedef Surface(CODEGEN_FUNCPTR *PFNGETCURRENTSURFACE)(int readdraw);
	PFNGETCURRENTSURFACE GetCurrentSurface = 0;
	typedef Display(CODEGEN_FUNCPTR *PFNGETCURRENTDISPLAY)(void);
	PFNGETCURRENTDISPLAY GetCurrentDisplay = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNQUERYCONTEXT)(Display dpy, Context ctx, int attribute, int *value);
	PFNQUERYCONTEXT QueryContext = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNWAITGL)(void);
	PFNWAITGL WaitGL = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNWAITNATIVE)(int engine);
	PFNWAITNATIVE WaitNative = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNSWAPBUFFERS)(Display dpy, Surface surface);
	PFNSWAPBUFFERS SwapBuffers = 0;
	typedef Boolean(CODEGEN_FUNCPTR *PFNCOPYBUFFERS)(Display dpy, Surface surface, NativePixmapType target);
	PFNCOPYBUFFERS CopyBuffers = 0;
	typedef __eglMustCastToProperFunctionPointerType(CODEGEN_FUNCPTR *PFNGETPROCADDRESS)(const char *procname);
	PFNGETPROCADDRESS GetProcAddress = 0;

	namespace sys
	{

		exts::LoadTest LoadFunctions()
		{
			if (!IntLoadLibrary())
			{
				return exts::LoadTest(); // instant failure
			}
			int numFailed = 0;
			GetError = reinterpret_cast<PFNGETERROR>(IntGetProcAddress("eglGetError"));
			if (!GetError)
				++numFailed;
			GetDisplay = reinterpret_cast<PFNGETDISPLAY>(IntGetProcAddress("eglGetDisplay"));
			if (!GetDisplay)
				++numFailed;
			Initialize = reinterpret_cast<PFNINITIALIZE>(IntGetProcAddress("eglInitialize"));
			if (!Initialize)
				++numFailed;
			Terminate = reinterpret_cast<PFNTERMINATE>(IntGetProcAddress("eglTerminate"));
			if (!Terminate)
				++numFailed;
			QueryString = reinterpret_cast<PFNQUERYSTRING>(IntGetProcAddress("eglQueryString"));
			if (!QueryString)
				++numFailed;
			GetConfigs = reinterpret_cast<PFNGETCONFIGS>(IntGetProcAddress("eglGetConfigs"));
			if (!GetConfigs)
				++numFailed;
			ChooseConfig = reinterpret_cast<PFNCHOOSECONFIG>(IntGetProcAddress("eglChooseConfig"));
			if (!ChooseConfig)
				++numFailed;
			GetConfigAttrib = reinterpret_cast<PFNGETCONFIGATTRIB>(IntGetProcAddress("eglGetConfigAttrib"));
			if (!GetConfigAttrib)
				++numFailed;
			CreateWindowSurface = reinterpret_cast<PFNCREATEWINDOWSURFACE>(IntGetProcAddress("eglCreateWindowSurface"));
			if (!CreateWindowSurface)
				++numFailed;
			CreatePbufferSurface = reinterpret_cast<PFNCREATEPBUFFERSURFACE>(IntGetProcAddress("eglCreatePbufferSurface"));
			if (!CreatePbufferSurface)
				++numFailed;
			CreatePixmapSurface = reinterpret_cast<PFNCREATEPIXMAPSURFACE>(IntGetProcAddress("eglCreatePixmapSurface"));
			if (!CreatePixmapSurface)
				++numFailed;
			DestroySurface = reinterpret_cast<PFNDESTROYSURFACE>(IntGetProcAddress("eglDestroySurface"));
			if (!DestroySurface)
				++numFailed;
			QuerySurface = reinterpret_cast<PFNQUERYSURFACE>(IntGetProcAddress("eglQuerySurface"));
			if (!QuerySurface)
				++numFailed;
			BindAPI = reinterpret_cast<PFNBINDAPI>(IntGetProcAddress("eglBindAPI"));
			if (!BindAPI)
				++numFailed;
			QueryAPI = reinterpret_cast<PFNQUERYAPI>(IntGetProcAddress("eglQueryAPI"));
			if (!QueryAPI)
				++numFailed;
			WaitClient = reinterpret_cast<PFNWAITCLIENT>(IntGetProcAddress("eglWaitClient"));
			if (!WaitClient)
				++numFailed;
			ReleaseThread = reinterpret_cast<PFNRELEASETHREAD>(IntGetProcAddress("eglReleaseThread"));
			if (!ReleaseThread)
				++numFailed;
			CreatePbufferFromClientBuffer = reinterpret_cast<PFNCREATEPBUFFERFROMCLIENTBUFFER>(IntGetProcAddress("eglCreatePbufferFromClientBuffer"));
			if (!CreatePbufferFromClientBuffer)
				++numFailed;
			SurfaceAttrib = reinterpret_cast<PFNSURFACEATTRIB>(IntGetProcAddress("eglSurfaceAttrib"));
			if (!SurfaceAttrib)
				++numFailed;
			BindTexImage = reinterpret_cast<PFNBINDTEXIMAGE>(IntGetProcAddress("eglBindTexImage"));
			if (!BindTexImage)
				++numFailed;
			ReleaseTexImage = reinterpret_cast<PFNRELEASETEXIMAGE>(IntGetProcAddress("eglReleaseTexImage"));
			if (!ReleaseTexImage)
				++numFailed;
			SwapInterval = reinterpret_cast<PFNSWAPINTERVAL>(IntGetProcAddress("eglSwapInterval"));
			if (!SwapInterval)
				++numFailed;
			CreateContext = reinterpret_cast<PFNCREATECONTEXT>(IntGetProcAddress("eglCreateContext"));
			if (!CreateContext)
				++numFailed;
			DestroyContext = reinterpret_cast<PFNDESTROYCONTEXT>(IntGetProcAddress("eglDestroyContext"));
			if (!DestroyContext)
				++numFailed;
			MakeCurrent = reinterpret_cast<PFNMAKECURRENT>(IntGetProcAddress("eglMakeCurrent"));
			if (!MakeCurrent)
				++numFailed;
			GetCurrentContext = reinterpret_cast<PFNGETCURRENTCONTEXT>(IntGetProcAddress("eglGetCurrentContext"));
			if (!GetCurrentContext)
				++numFailed;
			GetCurrentSurface = reinterpret_cast<PFNGETCURRENTSURFACE>(IntGetProcAddress("eglGetCurrentSurface"));
			if (!GetCurrentSurface)
				++numFailed;
			GetCurrentDisplay = reinterpret_cast<PFNGETCURRENTDISPLAY>(IntGetProcAddress("eglGetCurrentDisplay"));
			if (!GetCurrentDisplay)
				++numFailed;
			QueryContext = reinterpret_cast<PFNQUERYCONTEXT>(IntGetProcAddress("eglQueryContext"));
			if (!QueryContext)
				++numFailed;
			WaitGL = reinterpret_cast<PFNWAITGL>(IntGetProcAddress("eglWaitGL"));
			if (!WaitGL)
				++numFailed;
			WaitNative = reinterpret_cast<PFNWAITNATIVE>(IntGetProcAddress("eglWaitNative"));
			if (!WaitNative)
				++numFailed;
			SwapBuffers = reinterpret_cast<PFNSWAPBUFFERS>(IntGetProcAddress("eglSwapBuffers"));
			if (!SwapBuffers)
				++numFailed;
			CopyBuffers = reinterpret_cast<PFNCOPYBUFFERS>(IntGetProcAddress("eglCopyBuffers"));
			if (!CopyBuffers)
				++numFailed;
			GetProcAddress = reinterpret_cast<PFNGETPROCADDRESS>(IntGetProcAddress("eglGetProcAddress"));
			if (!GetProcAddress)
				++numFailed;
			return exts::LoadTest(true, numFailed);
		}

		static int e_major_version = 0;
		static int e_minor_version = 0;

		static void GetEGLVersion()
		{
			// FIXME: Add a proper EGL 
		}


	}
}