#include "framework/harness.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/options.h"
#include "framework/stage.h"
#include "library/strings_format.h"
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_video.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <typeinfo>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using HarnessSocket = SOCKET;
static const HarnessSocket HARNESS_INVALID = INVALID_SOCKET;
#define harnessCloseSocket closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using HarnessSocket = int;
static const HarnessSocket HARNESS_INVALID = -1;
#define harnessCloseSocket close
#endif

#ifdef __GNUG__
#include <cxxabi.h>
#include <memory>
#endif

namespace OpenApoc
{
namespace
{

UString demangleStage(const Stage *stage)
{
	if (!stage)
	{
		return "none";
	}
	const char *raw = typeid(*stage).name();
#ifdef __GNUG__
	int status = 0;
	std::unique_ptr<char, void (*)(void *)> demangled(
	    abi::__cxa_demangle(raw, nullptr, nullptr, &status), std::free);
	if (status == 0 && demangled)
	{
		UString name = demangled.get();
		const UString prefix = "OpenApoc::";
		if (name.rfind(prefix, 0) == 0)
		{
			name = name.substr(prefix.size());
		}
		return name;
	}
#endif
	return raw;
}

int sdlButtonFromName(const UString &name)
{
	const auto n = to_lower(name);
	if (n == "right")
	{
		return SDL_BUTTON_RIGHT;
	}
	if (n == "middle")
	{
		return SDL_BUTTON_MIDDLE;
	}
	return SDL_BUTTON_LEFT;
}

bool parseIntToken(const UString &tok, int &out)
{
	if (tok.empty())
	{
		return false;
	}
	char *end = nullptr;
	const long v = std::strtol(tok.c_str(), &end, 10);
	if (!end || *end != '\0')
	{
		return false;
	}
	out = static_cast<int>(v);
	return true;
}

bool parseKeyName(const UString &name, int &keyCode, int &scanCode)
{
	const SDL_Keycode key = SDL_GetKeyFromName(name.c_str());
	if (key == SDLK_UNKNOWN)
	{
		return false;
	}
	keyCode = static_cast<int>(key);
	scanCode = static_cast<int>(SDL_GetScancodeFromKey(key));
	return true;
}

void setNonBlocking(HarnessSocket fd)
{
#ifdef _WIN32
	u_long mode = 1;
	ioctlsocket(fd, FIONBIO, &mode);
#else
	const int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

void sendAll(HarnessSocket fd, const UString &text)
{
	const char *p = text.c_str();
	size_t left = text.size();
	while (left > 0)
	{
#ifdef _WIN32
		const int n = send(fd, p, static_cast<int>(left), 0);
#elif defined(MSG_NOSIGNAL)
		const ssize_t n = send(fd, p, left, MSG_NOSIGNAL);
#else
		const ssize_t n = send(fd, p, left, 0);
#endif
		if (n <= 0)
		{
			return;
		}
		p += n;
		left -= static_cast<size_t>(n);
	}
}

HarnessQueryFunction harnessQueryHandler;
HarnessActionFunction harnessActionHandler;
HarnessUIFunction harnessUIHandler;

} // namespace

void setHarnessQueryHandler(HarnessQueryFunction function)
{
	harnessQueryHandler = std::move(function);
}

HarnessQueryFunction getHarnessQueryHandler() { return harnessQueryHandler; }

void setHarnessActionHandler(HarnessActionFunction function)
{
	harnessActionHandler = std::move(function);
}

HarnessActionFunction getHarnessActionHandler() { return harnessActionHandler; }

void setHarnessUIHandler(HarnessUIFunction function) { harnessUIHandler = std::move(function); }

HarnessUIFunction getHarnessUIHandler() { return harnessUIHandler; }

bool parseHarnessCommand(const UString &line, HarnessCommand &out)
{
	out = {};
	UString trimmed = line;
	while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n' ||
	                            trimmed.back() == ' ' || trimmed.back() == '\t'))
	{
		trimmed.pop_back();
	}
	size_t start = 0;
	while (start < trimmed.size() && (trimmed[start] == ' ' || trimmed[start] == '\t'))
	{
		start++;
	}
	if (start > 0)
	{
		trimmed = trimmed.substr(start);
	}
	if (trimmed.empty())
	{
		out.type = HarnessCommand::Type::Unknown;
		out.error = "empty";
		return false;
	}

	auto parts = split(trimmed, " \t");
	if (parts.empty())
	{
		out.type = HarnessCommand::Type::Unknown;
		out.error = "empty";
		return false;
	}
	const auto verb = to_upper(parts[0]);

	auto restAfter = [&](size_t index) -> UString
	{
		size_t pos = 0;
		for (size_t i = 0; i <= index && pos < trimmed.size(); i++)
		{
			while (pos < trimmed.size() && (trimmed[pos] == ' ' || trimmed[pos] == '\t'))
			{
				pos++;
			}
			while (pos < trimmed.size() && trimmed[pos] != ' ' && trimmed[pos] != '\t')
			{
				pos++;
			}
		}
		while (pos < trimmed.size() && (trimmed[pos] == ' ' || trimmed[pos] == '\t'))
		{
			pos++;
		}
		return pos < trimmed.size() ? trimmed.substr(pos) : "";
	};

	if (verb == "STATUS")
	{
		out.type = HarnessCommand::Type::Status;
		return true;
	}
	if (verb == "QUIT")
	{
		out.type = HarnessCommand::Type::Quit;
		return true;
	}
	if (verb == "TEXT")
	{
		out.type = HarnessCommand::Type::Text;
		out.text = restAfter(0);
		if (out.text.empty())
		{
			out.type = HarnessCommand::Type::Unknown;
			out.error = "TEXT needs a string";
			return false;
		}
		return true;
	}
	if (verb == "GS")
	{
		out.type = HarnessCommand::Type::Query;
		out.text = restAfter(0);
		if (out.text.empty())
		{
			out.type = HarnessCommand::Type::Unknown;
			out.error = "GS needs a query";
			return false;
		}
		return true;
	}
	if (verb == "UI")
	{
		out.type = HarnessCommand::Type::UiDump;
		out.text = restAfter(0);
		return true;
	}
	if (verb == "SAVE")
	{
		out.type = HarnessCommand::Type::Save;
		out.path = restAfter(0);
		if (out.path.empty())
		{
			out.type = HarnessCommand::Type::Unknown;
			out.error = "SAVE needs a path";
			return false;
		}
		return true;
	}
	if (verb == "SCREENSHOT")
	{
		out.type = HarnessCommand::Type::Screenshot;
		out.path = restAfter(0);
		if (out.path.empty())
		{
			out.type = HarnessCommand::Type::Unknown;
			out.error = "SCREENSHOT needs a path";
			return false;
		}
		return true;
	}
	if (verb == "KEY" || verb == "KEYDOWN" || verb == "KEYUP")
	{
		if (parts.size() < 2)
		{
			out.error = "KEY needs a name";
			return false;
		}
		// SDL key names can contain spaces ("Left Shift", "Page Down"), so take the whole
		// remainder of the line rather than just the next token.
		const UString keyName = restAfter(0);
		if (!parseKeyName(keyName, out.keyCode, out.scanCode))
		{
			out.error = format("unknown key \"{0}\"", keyName);
			return false;
		}
		if (verb == "KEY")
		{
			out.type = HarnessCommand::Type::Key;
		}
		else if (verb == "KEYDOWN")
		{
			out.type = HarnessCommand::Type::KeyDown;
		}
		else
		{
			out.type = HarnessCommand::Type::KeyUp;
		}
		return true;
	}
	if (verb == "RESIZE")
	{
		if (parts.size() < 3 || !parseIntToken(parts[1], out.x) || !parseIntToken(parts[2], out.y))
		{
			out.error = "RESIZE width height";
			return false;
		}
		out.type = HarnessCommand::Type::Resize;
		return true;
	}
	if (verb == "MOVE")
	{
		if (parts.size() < 3 || !parseIntToken(parts[1], out.x) || !parseIntToken(parts[2], out.y))
		{
			out.error = "MOVE x y";
			return false;
		}
		out.type = HarnessCommand::Type::Move;
		return true;
	}
	if (verb == "CLICK" || verb == "DOWN" || verb == "UP")
	{
		if (parts.size() < 3 || !parseIntToken(parts[1], out.x) || !parseIntToken(parts[2], out.y))
		{
			out.error = format("{0} x y [button]", parts[0]);
			return false;
		}
		if (parts.size() >= 4)
		{
			out.sdlButton = sdlButtonFromName(parts[3]);
		}
		if (verb == "CLICK")
		{
			out.type = HarnessCommand::Type::Click;
		}
		else if (verb == "DOWN")
		{
			out.type = HarnessCommand::Type::Down;
		}
		else
		{
			out.type = HarnessCommand::Type::Up;
		}
		return true;
	}
	if (verb == "SCROLL")
	{
		if (parts.size() < 4 || !parseIntToken(parts[1], out.x) ||
		    !parseIntToken(parts[2], out.y) || !parseIntToken(parts[3], out.wheelVertical))
		{
			out.error = "SCROLL x y dy [dx]";
			return false;
		}
		if (parts.size() >= 5)
		{
			parseIntToken(parts[4], out.wheelHorizontal);
		}
		out.type = HarnessCommand::Type::Scroll;
		return true;
	}

	if (verb == "CONTROLS" || verb == "HELP")
	{
		out.type = HarnessCommand::Type::Action;
		out.text = to_lower(verb);
		// CONTROLS takes an optional control id, to enumerate that control's children by
		// position -- the only way to address runtime list rows, which have no names.
		for (size_t i = 1; i < parts.size(); i++)
		{
			out.args.push_back(parts[i]);
		}
		return true;
	}
	if (verb == "CONTROL")
	{
		if (parts.size() < 2)
		{
			out.error = "CONTROL needs a control id";
			return false;
		}
		out.type = HarnessCommand::Type::Action;
		out.text = "control";
		for (size_t i = 1; i < parts.size(); i++)
		{
			out.args.push_back(parts[i]);
		}
		return true;
	}
	if (verb == "ACTION")
	{
		if (parts.size() < 2)
		{
			out.error = "ACTION needs a verb";
			return false;
		}
		out.type = HarnessCommand::Type::Action;
		out.text = to_lower(parts[1]);
		for (size_t i = 2; i < parts.size(); i++)
		{
			out.args.push_back(parts[i]);
		}
		return true;
	}

	out.error = format("unknown command \"{0}\"", parts[0]);
	return false;
}

Harness::Harness(int port)
{
#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LogError("Harness WSAStartup failed");
		return;
	}
#endif
	if (!bindListen(port))
	{
		LogError("Harness failed to listen on 127.0.0.1:{0}", port);
		return;
	}
	LogInfo("Harness listening on 127.0.0.1:{0}", port);
}

Harness::~Harness()
{
	for (auto &c : clients)
	{
		if (c.fd != static_cast<int>(HARNESS_INVALID))
		{
			harnessCloseSocket(static_cast<HarnessSocket>(c.fd));
		}
	}
	if (listenFd != static_cast<int>(HARNESS_INVALID) && listenFd >= 0)
	{
		harnessCloseSocket(static_cast<HarnessSocket>(listenFd));
	}
#ifdef _WIN32
	WSACleanup();
#endif
}

bool Harness::listening() const { return listenFd >= 0; }

bool Harness::bindListen(int port)
{
	const HarnessSocket fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == HARNESS_INVALID)
	{
		return false;
	}
	int yes = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(static_cast<uint16_t>(port));
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
	{
		harnessCloseSocket(fd);
		return false;
	}
	if (listen(fd, 4) != 0)
	{
		harnessCloseSocket(fd);
		return false;
	}
	setNonBlocking(fd);
	listenFd = static_cast<int>(fd);
	listenPort = port;
	return true;
}

void Harness::poll(Framework &fw)
{
	if (listenFd < 0)
	{
		return;
	}
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(static_cast<HarnessSocket>(listenFd), &rfds);
	int maxFd = listenFd;
	for (auto &c : clients)
	{
		if (c.fd >= 0)
		{
			FD_SET(static_cast<HarnessSocket>(c.fd), &rfds);
			maxFd = std::max(maxFd, c.fd);
		}
	}
	timeval tv{};
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	const int n = select(maxFd + 1, &rfds, nullptr, nullptr, &tv);
	if (n <= 0)
	{
		return;
	}
	if (FD_ISSET(static_cast<HarnessSocket>(listenFd), &rfds))
	{
		acceptReady();
	}
	for (auto &c : clients)
	{
		if (c.fd >= 0 && FD_ISSET(static_cast<HarnessSocket>(c.fd), &rfds))
		{
			readClient(c, fw);
		}
	}
	clients.erase(
	    std::remove_if(clients.begin(), clients.end(), [](const Client &c) { return c.fd < 0; }),
	    clients.end());
}

void Harness::acceptReady()
{
	sockaddr_in addr{};
	socklen_t len = sizeof(addr);
	const HarnessSocket fd =
	    accept(static_cast<HarnessSocket>(listenFd), reinterpret_cast<sockaddr *>(&addr), &len);
	if (fd == HARNESS_INVALID)
	{
		return;
	}
	if (addr.sin_addr.s_addr != htonl(INADDR_LOOPBACK))
	{
		harnessCloseSocket(fd);
		return;
	}
	setNonBlocking(fd);
#ifdef SO_NOSIGPIPE
	int nosig = 1;
	setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<const char *>(&nosig), sizeof(nosig));
#endif
	clients.push_back(Client{static_cast<int>(fd), ""});
}

void Harness::readClient(Client &c, Framework &fw)
{
	char buf[1024];
#ifdef _WIN32
	const int n = recv(static_cast<HarnessSocket>(c.fd), buf, sizeof(buf), 0);
#else
	const ssize_t n = recv(static_cast<HarnessSocket>(c.fd), buf, sizeof(buf), 0);
#endif
	if (n <= 0)
	{
		closeClient(c);
		return;
	}
	c.buffer.append(buf, static_cast<size_t>(n));
	size_t nl;
	while ((nl = c.buffer.find('\n')) != UString::npos)
	{
		UString line = c.buffer.substr(0, nl);
		c.buffer.erase(0, nl + 1);
		HarnessCommand cmd;
		UString reply;
		if (!parseHarnessCommand(line, cmd))
		{
			reply = format("ERR {0}\n", cmd.error.empty() ? "parse" : cmd.error);
		}
		else
		{
			reply = execute(cmd, fw);
			if (reply.empty() || reply.back() != '\n')
			{
				reply += "\n";
			}
		}
		sendAll(static_cast<HarnessSocket>(c.fd), reply);
	}
}

void Harness::closeClient(Client &c)
{
	if (c.fd >= 0)
	{
		harnessCloseSocket(static_cast<HarnessSocket>(c.fd));
		c.fd = -1;
	}
}

void Harness::injectMove(Framework &fw, int x, int y, int buttonMask)
{
	auto *e = new MouseEvent(EVENT_MOUSE_MOVE);
	e->mouse().X = x;
	e->mouse().Y = y;
	e->mouse().DeltaX = x - lastX;
	e->mouse().DeltaY = y - lastY;
	e->mouse().Button = buttonMask;
	e->mouse().WheelVertical = 0;
	e->mouse().WheelHorizontal = 0;
	lastX = x;
	lastY = y;
	fw.pushEvent(e);
	warpCursor(fw, x, y);
}

void Harness::injectButton(Framework &fw, int x, int y, int sdlButton, bool down)
{
	auto *e = new MouseEvent(down ? EVENT_MOUSE_DOWN : EVENT_MOUSE_UP);
	e->mouse().X = x;
	e->mouse().Y = y;
	e->mouse().DeltaX = 0;
	e->mouse().DeltaY = 0;
	e->mouse().WheelVertical = 0;
	e->mouse().WheelHorizontal = 0;
	e->mouse().Button = SDL_BUTTON(sdlButton);
	lastX = x;
	lastY = y;
	fw.pushEvent(e);
}

void Harness::injectKey(Framework &fw, int keyCode, int scanCode, bool down)
{
	auto *e = new KeyboardEvent(down ? EVENT_KEY_DOWN : EVENT_KEY_UP);
	e->keyboard().KeyCode = keyCode;
	e->keyboard().ScanCode = scanCode;
	e->keyboard().Modifiers = 0;
	fw.pushEvent(e);
}

void Harness::warpCursor(Framework &fw, int x, int y)
{
	// The engine tracks the cursor from the injected event itself (ApocCursor::eventOccured), so
	// warping the OS pointer is purely cosmetic -- and it steals the physical mouse from whoever is
	// using the machine during a long automated run. Opt-in only.
	if (!Options::harnessWarpCursor.get())
	{
		return;
	}
	auto *window = static_cast<SDL_Window *>(fw.getWindowHandle());
	if (!window)
	{
		return;
	}
	int ww = 0;
	int wh = 0;
	SDL_GetWindowSize(window, &ww, &wh);
	const auto size = fw.displayGetSize();
	int wx = x;
	int wy = y;
	if (size.x > 0 && size.y > 0 && ww > 0 && wh > 0)
	{
		wx = x * ww / size.x;
		wy = y * wh / size.y;
	}
	SDL_WarpMouseInWindow(window, wx, wy);
}

UString Harness::execute(const HarnessCommand &cmd, Framework &fw)
{
	switch (cmd.type)
	{
		case HarnessCommand::Type::Move:
			injectMove(fw, cmd.x, cmd.y, 0);
			return "OK";
		case HarnessCommand::Type::Down:
			injectMove(fw, cmd.x, cmd.y, 0);
			injectButton(fw, cmd.x, cmd.y, cmd.sdlButton, true);
			return "OK";
		case HarnessCommand::Type::Up:
			injectButton(fw, cmd.x, cmd.y, cmd.sdlButton, false);
			return "OK";
		case HarnessCommand::Type::Click:
			injectMove(fw, cmd.x, cmd.y, 0);
			injectButton(fw, cmd.x, cmd.y, cmd.sdlButton, true);
			injectButton(fw, cmd.x, cmd.y, cmd.sdlButton, false);
			return "OK";
		case HarnessCommand::Type::Scroll:
		{
			auto *e = new MouseEvent(EVENT_MOUSE_SCROLL);
			e->mouse().X = cmd.x;
			e->mouse().Y = cmd.y;
			e->mouse().DeltaX = 0;
			e->mouse().DeltaY = 0;
			e->mouse().Button = 0;
			e->mouse().WheelVertical = cmd.wheelVertical;
			e->mouse().WheelHorizontal = cmd.wheelHorizontal;
			fw.pushEvent(e);
			return "OK";
		}
		case HarnessCommand::Type::Key:
			injectKey(fw, cmd.keyCode, cmd.scanCode, true);
			injectKey(fw, cmd.keyCode, cmd.scanCode, false);
			return "OK";
		case HarnessCommand::Type::KeyDown:
			injectKey(fw, cmd.keyCode, cmd.scanCode, true);
			return "OK";
		case HarnessCommand::Type::KeyUp:
			injectKey(fw, cmd.keyCode, cmd.scanCode, false);
			return "OK";
		case HarnessCommand::Type::Text:
		{
			auto *e = new TextEvent();
			e->text().Input = cmd.text;
			fw.pushEvent(e);
			return "OK";
		}
		case HarnessCommand::Type::Screenshot:
			if (!fw.writeScreenshot(cmd.path))
			{
				return format("ERR screenshot failed ({0})", cmd.path);
			}
			return format("OK {0}", cmd.path);
		case HarnessCommand::Type::Status:
		{
			const auto stage = fw.stageGetCurrent();
			const auto size = fw.displayGetSize();
			auto detail = stage ? stage->harnessDetail() : UString("");
			std::replace(detail.begin(), detail.end(), ' ', '_');
			return format("OK stage={0} w={1} h={2} mouse={3},{4} port={5} detail={6}",
			              demangleStage(stage.get()), size.x, size.y, lastX, lastY, listenPort,
			              detail.empty() ? UString("-") : detail);
		}
		case HarnessCommand::Type::Query:
		{
			const auto handler = getHarnessQueryHandler();
			if (!handler)
			{
				return "ERR no gamestate (query handler not installed yet)";
			}
			UString reply = handler(cmd.text);
			if (reply.empty())
			{
				return format("ERR unknown query \"{0}\"", cmd.text);
			}
			// The reply protocol is strictly one line per command.
			std::replace(reply.begin(), reply.end(), '\n', ' ');
			return format("OK {0}", reply);
		}
		case HarnessCommand::Type::Save:
		{
			// Routed through the same game-layer hook as GS: framework cannot see GameState.
			const auto handler = getHarnessQueryHandler();
			if (!handler)
			{
				return "ERR no gamestate (query handler not installed yet)";
			}
			UString reply = handler("save " + cmd.path);
			if (reply.empty())
			{
				return "ERR save failed";
			}
			std::replace(reply.begin(), reply.end(), '\n', ' ');
			return format("OK {0}", reply);
		}
		case HarnessCommand::Type::UiDump:
		{
			const auto handler = getHarnessUIHandler();
			if (!handler)
			{
				return "ERR no ui handler installed";
			}
			UString reply = handler(cmd.text);
			if (reply.empty())
			{
				return "OK count=0";
			}
			std::replace(reply.begin(), reply.end(), '\n', ' ');
			return format("OK {0}", reply);
		}
		case HarnessCommand::Type::Resize:
		{
			fw.displaySetSize({cmd.x, cmd.y});
			const auto size = fw.displayGetSize();
			return format("OK resized w={0} h={1}", size.x, size.y);
		}
		case HarnessCommand::Type::Action:
		{
			const auto handler = getHarnessActionHandler();
			if (!handler)
			{
				return "ERR no action handler (forms not loaded yet)";
			}
			UString reply = handler(cmd.text, cmd.args);
			if (reply.empty())
			{
				return format("ERR unknown action \"{0}\"", cmd.text);
			}
			std::replace(reply.begin(), reply.end(), '\n', ' ');
			return reply;
		}
		case HarnessCommand::Type::Quit:
			fw.stageQueueCommand({StageCmd::Command::QUIT});
			return "OK quitting";
		case HarnessCommand::Type::Unknown:
			break;
	}
	return format("ERR {0}", cmd.error.empty() ? "unknown" : cmd.error);
}

} // namespace OpenApoc
