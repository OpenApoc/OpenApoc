
#include "framework.h"

int main ( int argc, char* argv[] )
{
  OpenApoc::Framework* fw = new OpenApoc::Framework();
  DATA = new OpenApoc::Data("./data");
  fw->Run();
  delete fw;
  return 0;
}
