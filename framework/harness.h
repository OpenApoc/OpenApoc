#pragma once

#include "library/strings.h"
#include <functional>
#include <vector>

namespace OpenApoc
{

class Framework;

// Line protocol on 127.0.0.1 (off unless Framework.Harness.Enable=1).
//
//   CLICK <x> <y> [left|right|middle]
//   MOVE <x> <y>
//   DOWN <x> <y> [left|right|middle]
//   UP <x> <y> [left|right|middle]
//   SCROLL <x> <y> <dy> [dx]
//   KEY <name>
//   KEYDOWN <name>
//   KEYUP <name>
//   TEXT <string>
//   SCREENSHOT <path>
//   RESIZE <width> <height>
//   STATUS
//   GS <query>
//   SAVE <path>
//   CONTROLS
//   CONTROL <id> [click|toggle|set <value>]
//   ACTION <verb> [args...]
//   HELP
//   UI [filter]
//   QUIT
//
// Replies are one line: "OK ..." or "ERR ...".
// CONTROL/ACTION invoke live form widgets by id (no pixel math). CLICK x y remains
// for nameless widgets (map tiles, list rows). Screens still render as usual.

// GS is answered by the game layer. framework/ sits below game/state/ in the link graph
// (OpenApoc_GameState links OpenApoc_Framework, never the reverse), so the harness cannot name a
// GameState type. The game layer installs a handler instead, mirroring how logger.h takes a
// LogFunction. Returns a single line with no trailing newline; an empty return means "no answer".
using HarnessQueryFunction = std::function<UString(const UString &query)>;
void setHarnessQueryHandler(HarnessQueryFunction function);
HarnessQueryFunction getHarnessQueryHandler();

// Named UI / game actions. forms/ installs a handler that walks the currently
// visible forms. Returns a full "OK ..." / "ERR ..." line, or empty for unknown.
using HarnessActionFunction =
    std::function<UString(const UString &verb, const std::vector<UString> &args)>;
void setHarnessActionHandler(HarnessActionFunction function);
HarnessActionFunction getHarnessActionHandler();

// UI is answered by the forms layer, which framework also cannot name (OpenApoc_Forms links
// OpenApoc_Framework, never the reverse). Kept separate from the GameState hook because the two
// have different owners and lifetimes: this one is installed once at startup and never chained.
using HarnessUIFunction = std::function<UString(const UString &filter)>;
void setHarnessUIHandler(HarnessUIFunction function);
HarnessUIFunction getHarnessUIHandler();

struct HarnessCommand
{
	enum class Type
	{
		Click,
		Move,
		Down,
		Up,
		Scroll,
		Key,
		KeyDown,
		KeyUp,
		Text,
		Screenshot,
		Status,
		Query,
		Save,
		Resize,
		UiDump,
		Action,
		Quit,
		Unknown
	};
	Type type = Type::Unknown;
	int x = 0;
	int y = 0;
	int wheelVertical = 0;
	int wheelHorizontal = 0;
	int sdlButton = 1;
	int keyCode = 0;
	int scanCode = 0;
	UString text;
	UString path;
	UString error;
	std::vector<UString> args;
};

bool parseHarnessCommand(const UString &line, HarnessCommand &out);

class Harness
{
  public:
	explicit Harness(int port);
	~Harness();

	Harness(const Harness &) = delete;
	Harness &operator=(const Harness &) = delete;

	bool listening() const;
	int port() const { return listenPort; }
	void poll(Framework &fw);

  private:
	int listenFd = -1;
	int listenPort = 0;
	int lastX = 0;
	int lastY = 0;

	struct Client
	{
		int fd = -1;
		UString buffer;
	};
	std::vector<Client> clients;

	bool bindListen(int port);
	void acceptReady();
	void readClient(Client &c, Framework &fw);
	void closeClient(Client &c);
	UString execute(const HarnessCommand &cmd, Framework &fw);
	void injectMove(Framework &fw, int x, int y, int buttonMask);
	void injectButton(Framework &fw, int x, int y, int sdlButton, bool down);
	void injectKey(Framework &fw, int keyCode, int scanCode, bool down);
	void warpCursor(Framework &fw, int x, int y);
};

} // namespace OpenApoc
