#include <cassert>
#include "UnitTest/UnitTest.h"
#ifdef WIN32
    #include "Thread/mingw.thread.h"
#else
    #include <thread>
#endif
#include "../src/xPLHub.h"

using namespace std;

class TestxPLHub : public TestClass<TestxPLHub>
{
public:
    TestxPLHub();
    ~TestxPLHub();

    static void ThreadStart(xPLHub* pxPLDev);
    bool Start();
    bool StdConfig();
    bool HubFunction();
    bool ReConfig();
    bool ReLaunch();
    bool ClientEnd();
    bool ResetClient();
    bool Stop();
    bool ReStart();
    bool ResetLaunch();
    bool ReStop();

    xPLHub xPLDev;
};
