
#include "framework.h"

int main ( int argc, char* argv[] )
{
  Framework* fw = new Framework();
  DATA = new Data("./data");
  fw->Run();
  delete fw;
  return 0;
}
