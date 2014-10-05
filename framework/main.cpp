
#include "framework.h"

int main ( int argc, char* argv[] )
{
	OpenApoc::Framework* fw = new OpenApoc::Framework("./data");
	fw->Run();
	delete fw;
	return 0;
}
