/*** LICENCE ***************************************************************************************/
/*
  xPPLib - Simple class to manage socket communication TCP or UDP

  This file is part of xPPLib.

    xPPLib is free software : you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    xPPLib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with xPPLib.  If not, see <http://www.gnu.org/licenses/>.
*/
/***************************************************************************************************/

#include "xPLLib/Schemas/SchemaSensor.h"
#include "Sensors.h"
namespace xPL
{

using namespace std;

/**************************************************************************************************************/
/***                                                                                                        ***/
/*** Class Sensors                                                                                           ***/
/***                                                                                                        ***/
/**************************************************************************************************************/
Sensors::Sensors(xPLDevice* xPLDev)
{
    m_xPLDevice = xPLDev;
    m_Log = m_xPLDevice->GetLogHandle();
}

Sensors::~Sensors()
{
	vector<SensorSchema>::iterator itSensor;

	for(itSensor=m_Messages.begin(); itSensor!=m_Messages.end(); ++itSensor)
	{
	    if(itSensor->NeededDelete) delete itSensor->Message;
	}
}

bool Sensors::MsgAnswer(SchemaObject& msg)
{
    if(msg.GetMsgType() != SchemaObject::cmnd)
    {
        LOG_VERBOSE(m_Log) << "Not a command message.";
        LOG_EXIT_OK;
        return false;
    }

    if(msg.GetClass() != "sensor")
    {
        LOG_VERBOSE(m_Log) << "Not a sensor class.";
        LOG_EXIT_OK;
        return false;
    }

    if(msg.GetType() != "request")
    {
        LOG_VERBOSE(m_Log) << "Not a request type";
        LOG_EXIT_OK;
        return false;
    }

    if(msg.GetValue("request") != "current")
    {
        LOG_VERBOSE(m_Log) << "Not a request of current";
        LOG_EXIT_OK;
        return false;
    }

    SchemaObject* pMessage;
    string deviceName;

    deviceName = msg.GetValue("device");
    if(deviceName=="")
    {
        vector<SensorSchema>::iterator itSensor;
        LOG_VERBOSE(m_Log) << "no device so return all devices";

        for(itSensor=m_Messages.begin(); itSensor!=m_Messages.end(); ++itSensor)
        {
            (itSensor->Message)->SetMsgType(ISchema::stat);
            m_xPLDevice->SendxPLMessage(itSensor->Message, msg.GetSource());
        }

        LOG_EXIT_OK;
        return true;
    }

    pMessage = GetMessage(deviceName);
    if(pMessage==nullptr)
    {
        LOG_VERBOSE(m_Log) << "but device '"<<deviceName<<"' not found";
        LOG_EXIT_OK;
        return false;
    }

    LOG_VERBOSE(m_Log) << "send sensor stat message";
    pMessage->SetMsgType(ISchema::stat);
    m_xPLDevice->SendxPLMessage(pMessage, msg.GetSource());
    LOG_EXIT_OK;
    return true;
}

void Sensors::AddMessage(SchemaObject* pMessage)
{
    string deviceName;

	LOG_ENTER;

    deviceName = pMessage->GetValue("device");
	if(GetMessage(deviceName)!=nullptr) RemoveMessage(deviceName);
    m_Messages.emplace_back(pMessage, false);

    pMessage->SetMsgType(ISchema::trig);
    m_xPLDevice->SendxPLMessage(pMessage, "*");

    LOG_EXIT_OK;
}

void Sensors::AddSensorMessage(string device, SchemaSensorTypeUtility::SensorType type, float value)
{
    SchemaSensorBasic* sensorMsg = new SchemaSensorBasic(device, type, value);

	LOG_ENTER;

	if(GetMessage(device)!=nullptr) RemoveMessage(device);
    m_Messages.emplace_back(sensorMsg, true);

    sensorMsg->SetMsgType(ISchema::trig);
    m_xPLDevice->SendxPLMessage(sensorMsg, "*");

    LOG_EXIT_OK;
}

SchemaObject* Sensors::GetMessage(string device)
{
	vector<SensorSchema>::iterator itSensor;

	LOG_ENTER;
	LOG_VERBOSE(m_Log) << "Search " << device << " message";

	for(itSensor=m_Messages.begin(); itSensor!=m_Messages.end(); ++itSensor)
	{
		if((itSensor->Message)->GetValue("device") == device)
		{
            LOG_VERBOSE(m_Log) << "Message found";
		    LOG_EXIT_OK;
			return itSensor->Message;
		}
	}

    LOG_VERBOSE(m_Log) << "Message not found";
    LOG_EXIT_OK;
	return nullptr;
}

void Sensors::RemoveAllMessages()
{
	LOG_ENTER;

	vector<SensorSchema>::iterator itSensor;

	for(itSensor=m_Messages.begin(); itSensor!=m_Messages.end(); ++itSensor)
	{
		if(itSensor->NeededDelete) delete itSensor->Message;
	}

    m_Messages.clear();

    LOG_EXIT_OK;
}

bool Sensors::RemoveMessage(string device)
{
	vector<SensorSchema>::iterator itSensor;

	LOG_ENTER;

	for(itSensor=m_Messages.begin(); itSensor!=m_Messages.end(); ++itSensor)
	{
		if((itSensor->Message)->GetValue("device") == device)
		{
			if(itSensor->NeededDelete) delete itSensor->Message;
			m_Messages.erase(itSensor);
		    LOG_EXIT_OK;
			return true;
		}
	}

    LOG_EXIT_OK;
	return false;
}

bool Sensors::ModifyMessage(string device, string value, string type)
{
    SchemaObject* pMessage;

	LOG_ENTER;

    pMessage = GetMessage(device);
    if(pMessage==nullptr) return false;

    pMessage->SetValue("current", value);
    if(type!="") pMessage->SetValue("type", type);  //Utilisé dans xPLFictif::ConfigChange

    pMessage->SetMsgType(ISchema::trig);
    m_xPLDevice->SendxPLMessage(pMessage, "*");

    LOG_EXIT_OK;
    return true;
}

}
