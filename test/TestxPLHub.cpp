#include "TestxPLHub.h"
#include "Plateforms/Plateforms.h"
#include "xPLLib/Schemas/SchemaObject.h"

using namespace std;

TestxPLHub::TestxPLHub() : TestClass("Hub", this)
{
	addTest("Start", &TestxPLHub::Start);
	addTest("StdConfig", &TestxPLHub::StdConfig);
	addTest("HubFunction", &TestxPLHub::HubFunction);
	addTest("Stop", &TestxPLHub::Stop);
	addTest("ReStart", &TestxPLHub::ReStart);
	addTest("ReStop", &TestxPLHub::ReStop);

    if(remove("config")==0)
        cout << termcolor::yellow << "Remove old config file" << endl << termcolor::grey;
}

TestxPLHub::~TestxPLHub()
{
    if(remove("config")!=0)
        cout << termcolor::red << "Unable to remove config file" << endl << termcolor::grey;
}

void TestxPLHub::ThreadStart(xPLHub* pxPLDev)
{
    char exeName[] = "test";
    char confName[] = "config";
    char* argv[2];

    argv[0]= exeName;
    argv[1]= confName;
    pxPLDev->ServiceStart(2, argv);
}

bool TestxPLHub::Start()
{
    string msg;
    xPL::SchemaObject sch;


    thread integrationTest(ThreadStart, &xPLDev);
    integrationTest.detach();

    msg = SimpleSockUDP::GetLastSend(10);
    sch.Parse(msg);
    assert("xPL Hub"==sch.GetValue("appname"));

    return true;
}

bool TestxPLHub::StdConfig()
{
    string msg;
    xPL::SchemaObject sch;
    xPL::SchemaObject schCfg(xPL::SchemaObject::cmnd, "config", "response");
    xPL::SchemaObject schHb(xPL::SchemaObject::trig, "hbeat", "app");
    SimpleSockUDP socket;

    schCfg.SetValue("interval", "25");
    schCfg.SetValue("newconf", "test");
    schCfg.SetValue("xplmodule","ModuleOne");
    schCfg.AddValue("xplmodule","ModuleTwo");
    msg = schCfg.ToMessage("fragxpl-test.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);     //Pass Hbeat message
    msg = SimpleSockUDP::GetLastSend(10);
    sch.Parse(msg);
    assert("25"==sch.GetValue("interval"));
    assert("fragxpl-hub.test"==sch.GetSource());

    Plateforms::delay(505);
    assert(true == Process::find("ModuleOne"));

    schHb.SetValue("remote-ip", socket.GetAddress());
    schHb.SetValue("port", "3866");
    msg = schHb.ToMessage("fragxpl-one.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward

    assert(true == Process::find("ModuleTwo"));

    schHb.SetValue("remote-ip", socket.GetAddress());
    schHb.SetValue("port", "3867");
    msg = schHb.ToMessage("fragxpl-two.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward

    return true;
}

bool TestxPLHub::HubFunction()
{
    string msg;
    xPL::SchemaObject schSensor(xPL::ISchema::trig, "sensor", "basic");
    xPL::SchemaObject sch;

    schSensor.SetValue("type", "temp");
    schSensor.SetValue("device", "room");
    schSensor.SetValue("current", "19");
    msg = schSensor.ToMessage("fragxpl-onewire.test", "*");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);
    sch.Parse(msg);
    assert("temp" == sch.GetValue("type"));
    assert("room" == sch.GetValue("device"));
    assert("19" == sch.GetValue("current"));

    msg = SimpleSockUDP::GetLastSend(10);
    sch.Parse(msg);
    assert("temp" == sch.GetValue("type"));
    assert("room" == sch.GetValue("device"));
    assert("19" == sch.GetValue("current"));

    return true;
}

bool TestxPLHub::Stop()
{
    string msg;
    xPL::SchemaObject sch;

    xPLDev.ServicePause(true);
    Plateforms::delay(800);
    xPLDev.ServicePause(false);
    xPLDev.ServiceStop();

    msg = SimpleSockUDP::GetLastSend(10);     //Pass hbeat message
    sch.Parse(msg);
    assert("hbeat"==sch.GetClass());
    assert("end"==sch.GetType());
    Plateforms::delay(200);
    return true;
}

bool TestxPLHub::ReStart()
{
    string msg;

    thread integrationTest(ThreadStart, &xPLDev);
    integrationTest.detach();

    return true;
}

bool TestxPLHub::ReStop()
{
    string msg;
    xPL::SchemaObject sch;


    xPLDev.ServiceStop();

    msg = SimpleSockUDP::GetLastSend(10);
    sch.Parse(msg);
    assert("hbeat"==sch.GetClass());
    assert("end"==sch.GetType());
    Plateforms::delay(200);

    return true;
}
