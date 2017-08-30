#include <cassert>
#include "UnitTest/UnitTest.h"
#include <thread>
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
