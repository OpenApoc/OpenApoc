
#include "framework/framework.h"

int main ( int argc, char* argv[] )
{
	std::vector<std::string> cmdline;
	for (int i = 1; i < argc; i++)
	{
		cmdline.emplace_back(argv[i]);
	}
	OpenApoc::Framework* fw = new OpenApoc::Framework(argv[0], cmdline);
	fw->Run();
	delete fw;
	return 0;
}
