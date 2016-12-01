#include <iostream>
#include <exception>
#include "xPLHub.h"

int main(int argc, char* argv[])
{
  int res = 0;

  try
  {
    xPLHub xPLDev;
  
	Service* pService = Service::Create("xPLHub", "Hub for xPL protocol", &xPLDev);
  	res = pService->Start(argc, argv);
  	Service::Destroy();
  }
  catch(const exception &e)
  {
      std::cout << e.what();
  }
	return res;
}
