#include "framework/framework.h"
using namespace OpenApoc;

int main ( int argc, char* argv[] )
{
	std::vector<UString> cmdline;
	for (int i = 1; i < argc; i++)
	{
		cmdline.emplace_back(U8Str(argv[i]));
	}
	Framework* fw = new Framework(U8Str(argv[0]), cmdline);
	fw->Run();
	delete fw;
	return 0;
}
