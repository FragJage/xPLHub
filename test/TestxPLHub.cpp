#include "TestxPLHub.h"
#include "Plateforms/Plateforms.h"
#include "xPLLib/Schemas/SchemaObject.h"

using namespace std;

TestxPLHub::TestxPLHub() : TestClass("Hub", this)
{
	addTest("Start", &TestxPLHub::Start);
	addTest("StdConfig", &TestxPLHub::StdConfig);
	addTest("HubFunction", &TestxPLHub::HubFunction);
	addTest("ReConfig", &TestxPLHub::ReConfig);
	addTest("ReLaunch", &TestxPLHub::ReLaunch);
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
    schHb.SetValue("interval", "10");
    schHb.SetValue("port", "3866");
    msg = schHb.ToMessage("fragxpl-one.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3866

    assert(true == Process::find("ModuleTwo"));

    schHb.SetValue("remote-ip", socket.GetAddress());
    schHb.SetValue("interval", "10");
    schHb.SetValue("port", "3867");
    msg = schHb.ToMessage("fragxpl-two.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3866
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3867

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

bool TestxPLHub::ReConfig()
{
    string msg;
    xPL::SchemaObject sch;
    xPL::SchemaObject schCfg(xPL::SchemaObject::cmnd, "config", "response");
    xPL::SchemaObject schHb(xPL::SchemaObject::trig, "hbeat", "app");
    SimpleSockUDP socket;

    schCfg.SetValue("interval", "25");
    schCfg.SetValue("newconf", "test");
    schCfg.SetValue("xplmodule","ModuleOne");
    schCfg.AddValue("xplmodule","ModuleThree");
    schCfg.AddValue("xplmodule","ModuleFour");
    msg = schCfg.ToMessage("fragxpl-test.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);     //Pass hub forward 3866
    msg = SimpleSockUDP::GetLastSend(10);     //Pass hub forward 3867

    Plateforms::delay(251);
    assert(true == Process::find("ModuleOne"));
    assert(false == Process::find("ModuleTwo"));

    Plateforms::delay(251);
    assert(true == Process::find("ModuleThree"));

    schHb.SetValue("remote-ip", socket.GetAddress());
    schHb.SetValue("interval", "10");
    schHb.SetValue("port", "3868");
    msg = schHb.ToMessage("fragxpl-three.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3866
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3868

    assert(true == Process::find("ModuleFour"));

    schHb.SetValue("remote-ip", socket.GetAddress());
    schHb.SetValue("interval", "0");
    schHb.SetValue("port", "3869");
    msg = schHb.ToMessage("fragxpl-four.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3866
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3868
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3869

    return true;
}

bool TestxPLHub::ReLaunch()
{
    string msg;
    xPL::SchemaObject schSensor(xPL::ISchema::trig, "sensor", "basic");
    xPL::SchemaObject schHb(xPL::SchemaObject::trig, "hbeat", "app");
    SimpleSockUDP socket;

    schHb.SetValue("remote-ip", socket.GetAddress());
    schHb.SetValue("interval", "0");
    schHb.SetValue("port", "3870");
    msg = schHb.ToMessage("fragxpl-uman.default", "fragxpl-hub.default");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3866
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3868
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3869
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3870

    Plateforms::delay(6100);

    schSensor.SetValue("type", "temp");
    schSensor.SetValue("device", "room");
    schSensor.SetValue("current", "19");
    msg = schSensor.ToMessage("fragxpl-onewire.test", "*");
    SimpleSockUDP::SetNextRecv(msg);

    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3866
    msg = SimpleSockUDP::GetLastSend(10);       //Pass hub forward 3868
    msg = SimpleSockUDP::GetLastSend(1);
    assert("" == msg);
    assert(true == Process::find("ModuleFour"));

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

    msg = SimpleSockUDP::GetLastSend(20);
    sch.Parse(msg);
    assert("hbeat"==sch.GetClass());
    assert("end"==sch.GetType());
    Plateforms::delay(200);

    return true;
}
