
#include "framework.h"

int main ( int argc, char* argv[] )
{
	OpenApoc::Framework* fw = new OpenApoc::Framework(argv[0]);
	fw->Run();
	delete fw;
	return 0;
}
