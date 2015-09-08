#include "framework/framework.h"
using namespace OpenApoc;

int main(int argc, char *argv[])
{
	std::vector<UString> cmdline;
	for (int i = 1; i < argc; i++)
	{
		cmdline.emplace_back(UString(argv[i]));
	}
	Framework *fw = new Framework(UString(argv[0]), cmdline);
	fw->Run();
	delete fw;
	return 0;
}
