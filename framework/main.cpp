
#include "framework.h"

int main ( int argc, char* argv[] )
{
  Framework* fw = new Framework();
  fw->Run();
  delete fw;
  return 0;
}
