
#pragma once

#include "includes.h"
#include "../library/memory.h"

enum EventTypes
{
	EVENT_WINDOW_ACTIVATE,
	EVENT_WINDOW_DEACTIVATE,
	EVENT_WINDOW_RESIZE,
	EVENT_WINDOW_MANAGER,
	EVENT_WINDOW_CLOSED,
	EVENT_KEY_DOWN,
	EVENT_KEY_UP,
	EVENT_MOUSE_DOWN,
	EVENT_MOUSE_UP,
	EVENT_MOUSE_MOVE,
	EVENT_JOYSTICK_AXIS,
	EVENT_JOYSTICK_HAT,
	EVENT_JOYSTICK_BALL,
	EVENT_JOYSTICK_BUTTON_DOWN,
	EVENT_JOYSTICK_BUTTON_UP,
	EVENT_TIMER_TICK,
#ifdef NETWORK_SUPPORT
	EVENT_NETWORK_CONNECTION_REQUEST,
	EVENT_NETWORK_PACKET_RECEIVED,
	EVENT_NETWORK_DISCONNECTED,
#endif
#ifdef DOWNLOAD_SUPPORT
	EVENT_DOWNLOAD_PROGRESS,
	EVENT_DOWNLOAD_COMPLETE,
#endif
	EVENT_AUDIO_FINISHED,
	EVENT_USER,
	EVENT_UNDEFINED
};

#ifdef DOWNLOAD_SUPPORT
typedef struct FRAMEWORK_DOWNLOAD_EVENT
{
	std::string* URL;
	Memory* Contents;
	double DownloadedBytes;
	double TotalBytesToDownload;
	double UploadedBytes;
	double TotalBytesToUpload;
} FRAMEWORK_DOWNLOAD_EVENT;
#endif

#ifdef NETWORK_SUPPORT
typedef struct FRAMEWORK_NETWORK_EVENT
{
	ENetEvent Traffic;
} FRAMEWORK_NETWORK_EVENT;
#endif

typedef struct FRAMEWORK_DISPLAY_EVENT
{
	bool Active;
	int X;
	int Y;
	int Width;
	int Height;
} FRAMEWORK_DISPLAY_EVENT;

typedef struct FRAMEWORK_JOYSTICK_EVENT
{
	int ID;
	int Stick;
	int Axis;
	float Position;
	int Button;
} FRAMEWORK_JOYSTICK_EVENT;

typedef struct FRAMEWORK_MOUSE_EVENT
{
	int X;
	int Y;
	int WheelVertical;
	int WheelHorizontal;
	int DeltaX;
	int DeltaY;
	int Button;
} FRAMEWORK_MOUSE_EVENT;

typedef struct FRAMEWORK_KEYBOARD_EVENT
{
	int KeyCode;
	unsigned int Modifiers;
} FRAMEWORK_KEYBOARD_EVENT;

typedef struct FRAMEWORK_TIMER_EVENT
{
	void* TimerObject;
} FRAMEWORK_TIMER_EVENT;

typedef union EventData
{
	FRAMEWORK_DISPLAY_EVENT		Display;
	FRAMEWORK_JOYSTICK_EVENT	Joystick;
	FRAMEWORK_KEYBOARD_EVENT	Keyboard;
	FRAMEWORK_MOUSE_EVENT		Mouse;
	Memory*						User;
#ifdef NETWORK_SUPPORT
	FRAMEWORK_NETWORK_EVENT		Network;
#endif
#ifdef DOWNLOAD_SUPPORT
	FRAMEWORK_DOWNLOAD_EVENT	Download;
#endif
	FRAMEWORK_TIMER_EVENT		Timer;
} EventData;

/*
	 Class: Event
	 Provides data regarding events that occur within the system
*/
class Event
{
	public:
		EventTypes Type;
		EventData Data;

		/*
			Constructor: Event
			Defaults the <Type> to Undefined
		*/
		Event();

		/*
			Destructor: ~Event
			For network packets, it calls enet's packet delete.
			For download packets, url and the data are deleted (assumption is that the program will have processed the data)
		*/
		~Event();
};
