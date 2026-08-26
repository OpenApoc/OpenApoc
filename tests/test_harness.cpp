#include "framework/configfile.h"
#include "framework/harness.h"
#include "tests/test_helpers.h"

using namespace OpenApoc;
using namespace OpenApoc::TestHelpers;

static bool test_parse_click()
{
	HarnessCommand cmd;
	TEST_REQUIRE(parseHarnessCommand("CLICK 10 20", cmd), "parse click");
	TEST_REQUIRE(cmd.type == HarnessCommand::Type::Click, "type");
	TEST_REQUIRE(cmd.x == 10 && cmd.y == 20, "xy");
	TEST_REQUIRE(cmd.sdlButton == 1, "default left");
	TEST_REQUIRE(parseHarnessCommand("CLICK 3 4 right", cmd), "parse right");
	TEST_REQUIRE(cmd.sdlButton == 3, "right button");
	return true;
}

static bool test_parse_key_and_text()
{
	HarnessCommand cmd;
	TEST_REQUIRE(parseHarnessCommand("KEY Escape", cmd), "parse escape");
	TEST_REQUIRE(cmd.type == HarnessCommand::Type::Key, "key type");
	TEST_REQUIRE(cmd.keyCode != 0, "keycode");
	TEST_REQUIRE(parseHarnessCommand("KEY Left Shift", cmd), "parse multi-word key name");
	TEST_REQUIRE(cmd.type == HarnessCommand::Type::Key, "multi-word key type");
	TEST_REQUIRE(cmd.keyCode != 0, "multi-word keycode");
	TEST_REQUIRE(parseHarnessCommand("TEXT hello world", cmd), "parse text");
	TEST_REQUIRE(cmd.type == HarnessCommand::Type::Text, "text type");
	TEST_REQUIRE(cmd.text == "hello world", "text rest");
	return true;
}

static bool test_parse_status_quit_shot()
{
	HarnessCommand cmd;
	TEST_REQUIRE(parseHarnessCommand("STATUS", cmd) && cmd.type == HarnessCommand::Type::Status,
	             "status");
	TEST_REQUIRE(parseHarnessCommand("QUIT", cmd) && cmd.type == HarnessCommand::Type::Quit,
	             "quit");
	TEST_REQUIRE(parseHarnessCommand("SCREENSHOT /tmp/oa.png", cmd), "shot");
	TEST_REQUIRE(cmd.type == HarnessCommand::Type::Screenshot && cmd.path == "/tmp/oa.png", "path");
	return true;
}

static bool test_parse_errors()
{
	HarnessCommand cmd;
	TEST_REQUIRE(!parseHarnessCommand("CLICK no", cmd), "bad click");
	TEST_REQUIRE(!parseHarnessCommand("NOTACOMMAND", cmd), "unknown");
	TEST_REQUIRE(!parseHarnessCommand("GS", cmd), "GS needs a query");
	TEST_REQUIRE(parseHarnessCommand("GS all", cmd) && cmd.type == HarnessCommand::Type::Query,
	             "GS query");
	TEST_REQUIRE(cmd.text == "all", "GS query text");
	TEST_REQUIRE(!parseHarnessCommand("", cmd), "empty");
	return true;
}

static bool test_parse_named_actions()
{
	HarnessCommand cmd;
	TEST_REQUIRE(parseHarnessCommand("CONTROLS", cmd) && cmd.type == HarnessCommand::Type::Action,
	             "controls");
	TEST_REQUIRE(cmd.text == "controls", "controls verb");
	TEST_REQUIRE(parseHarnessCommand("CONTROL BUTTON_NEWGAME", cmd), "control id");
	TEST_REQUIRE(cmd.type == HarnessCommand::Type::Action && cmd.text == "control", "control verb");
	TEST_REQUIRE(cmd.args.size() == 1 && cmd.args[0] == "BUTTON_NEWGAME", "control id arg");
	TEST_REQUIRE(parseHarnessCommand("CONTROL NUM_HUMANS_SLIDER set 8", cmd), "control set");
	TEST_REQUIRE(cmd.args.size() == 3 && cmd.args[1] == "set" && cmd.args[2] == "8", "set args");
	TEST_REQUIRE(parseHarnessCommand("ACTION click BUTTON_NEWGAME", cmd), "action click");
	TEST_REQUIRE(cmd.text == "click" && cmd.args.size() == 1 && cmd.args[0] == "BUTTON_NEWGAME",
	             "action click args");
	TEST_REQUIRE(parseHarnessCommand("HELP", cmd) && cmd.text == "help", "help");
	TEST_REQUIRE(!parseHarnessCommand("CONTROL", cmd), "control needs id");
	TEST_REQUIRE(!parseHarnessCommand("ACTION", cmd), "action needs verb");
	return true;
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	applyDeterministicTestConfig();
	return runTestSuite({
	    {"parse_click", test_parse_click},
	    {"parse_key_and_text", test_parse_key_and_text},
	    {"parse_status_quit_shot", test_parse_status_quit_shot},
	    {"parse_errors", test_parse_errors},
    {"parse_named_actions", test_parse_named_actions},
	});
}
