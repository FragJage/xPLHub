#include <algorithm>
#include "xPLHub.h"
#include "Plateforms/Plateforms.h"

#ifndef _WIN32
#include <netdb.h>      //Pour gethostbyname
#endif

using namespace std;

xPLHub::xPLHub()
{
    m_LaunchMode = false;
    m_LaunchPID = 0;
    m_LaunchTimeout = time(nullptr);
    m_Log = m_xPLDevice.GetLogHandle();

    m_xPLDevice.Initialisation("fragxpl", "hub", "default");
    m_xPLDevice.SetAppName("xPL Hub", "1.1.0.0");
    m_xPLDevice.SetAnswerAllMsg(true);
    m_bServicePause = false;
    m_bServiceStop = false;

    m_xPLDevice.AddExtension(this);
    m_xPLDevice.AddBasicConfig("xplmodule", xPL::ConfigItem::Option, 16);
    m_xPLDevice.SetCallBackConfig(this);

	FindLocalAddress();
}

xPLHub::~xPLHub()
{
    map<unsigned short, HubClient>::iterator it;


    it = m_HubClients.begin();
    while(it != m_HubClients.end())
    {
        if(it->second.command!="") Process::terminate(it->second.pid);
        ++it;
    }
}

bool xPLHub::MsgAnswer(xPL::SchemaObject& msg)
{
    string msgClass;
    string msgType;
    time_t	Time = time(NULL);
    map<unsigned short, HubClient>::iterator it;
    bool toRemove;
    bool startLaunch = false;


    //Update hubClient list for class hbeat/config and type app/end message to local address
    msgClass = msg.GetClass();
    msgType = msg.GetType();


    if((msgClass == "hbeat")||(msgClass == "config"))
    {
        if(msgType == "app")
        {
            unsigned long ip = inet_addr(msg.GetValue("remote-ip").c_str());
            unsigned short port = atoi(msg.GetValue("port").c_str());
            bool newClient = false;

            if(((std::find(m_LocalAddressList.begin(), m_LocalAddressList.end(), ip) != m_LocalAddressList.end()))&&(port!=3865))
            {
                if(m_HubClients[port].port==0) newClient = true;
                m_HubClients[port].ipAddress = ip;
                m_HubClients[port].port = port;
                m_HubClients[port].interval = atoi(msg.GetValue("interval").c_str());
                m_HubClients[port].timeout = Time + (m_HubClients[port].interval * 60*2)+5;
                m_HubClients[port].source = msg.GetSource();

                if((newClient)&&(m_LaunchMode)) SynchronizeLaunch(port);

                if(newClient)
                    LOG_VERBOSE(m_Log) << "New client on port " << port;
                else
                    LOG_VERBOSE(m_Log) << "Refresh client on port " << port;

            }
        }
        if(msgType == "end")
        {
            string source = msg.GetSource();

            it = m_HubClients.begin();
            while(it != m_HubClients.end())
            {
                if(it->second.source==source)
                {
                    LOG_VERBOSE(m_Log) << "End of client on port " << it->second.port;
                    m_HubClients.erase(it);
                    break;
                }
                ++it;
            }
        }
    }

    //Remove dead client from hubClient list
    it = m_HubClients.begin();
    while(it != m_HubClients.end())
    {
        if(it->second.timeout < Time )
        {
            LOG_VERBOSE(m_Log) << "Timeout : removing client on port " << it->second.port;
            startLaunch |= ToRelaunch(it->second);
            it = m_HubClients.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //Forward message to all clients in the list
    it = m_HubClients.begin();
    while(it != m_HubClients.end())
    {
        toRemove = false;
        try
        {
            m_SenderSock.Open(it->second.port, it->second.ipAddress);
            m_SenderSock.Send(msg.ToMessage());
            m_SenderSock.Close();
            LOG_VERBOSE(m_Log) << "Send " << msgClass <<"."<<msgType << " to " << it->second.port;
        }
        catch(const std::exception& e)
        {
            LOG_WARNING(m_Log) << "Unable to send : removing client on port " << it->second.port;
            startLaunch = ToRelaunch(it->second);
            toRemove = true;
        }

        if(toRemove)
            it = m_HubClients.erase(it);
        else
            ++it;
    }

    if(startLaunch) StartLaunch();

    return true;
}

void xPLHub::Configure()
{
    xPL::ConfigItem* pConfigItem;
    unsigned int i;
    unsigned int nb;
    string command;
    set<string> listCommand;
    map<unsigned short, HubClient>::iterator itClient;


    pConfigItem = m_xPLDevice.GetConfigItem("xplmodule");
    if(pConfigItem==nullptr) return;

    nb = pConfigItem->Count();
    if(nb==0) return;

    for(i=0; i<nb; i++)
    {
        command = pConfigItem->GetValue(i);
        //Je ne me rappelle plus à quoi cela servait
        //if((command[1]=='.')||(command[1]=='/')||(command[1]=='~')||(command[1]=='\\'))
        //{
        //    LOG_WARNING(m_Log) << "Unable to launch xplmodule " <<command<< ". It must be on the same directory of xPLHub.";
        //    continue;
        //}
        listCommand.insert(command);
        if(AlreadyLaunched(command)) continue;
        m_ToLaunch.push_back(command);
    }

    //Arrêter les absents
    listCommand.insert("");
    itClient = m_HubClients.begin();
    while(itClient != m_HubClients.end())
    {
        if(listCommand.find((*itClient).second.command) == listCommand.end())
        {
            Process::terminate((*itClient).second.pid);
            itClient = m_HubClients.erase(itClient);
        }
        else
            ++itClient;
    }

    StartLaunch();
}

bool xPLHub::ToRelaunch(const HubClient& client)
{
    if(client.command=="") return false;
    Process::terminate(client.pid);
    m_ToLaunch.push_back(client.command);
    return true;
}

bool xPLHub::AlreadyLaunched(string command)
{
    map<unsigned short, HubClient>::iterator it;

    it = m_HubClients.begin();
    while(it != m_HubClients.end())
    {
        if(it->second.command == command) return true;
        ++it;
    }

    return false;
}

void xPLHub::StartLaunch()
{
    m_Launching = "";
    m_LaunchMode = false;
    if(m_ToLaunch.empty()) return;

    m_Launching = m_ToLaunch.front();
    m_ToLaunch.pop_front();
    m_LaunchPID = Process::launch(m_Launching);
    m_LaunchMode = true;
    m_LaunchTimeout = time(NULL)+10;
    LOG_VERBOSE(m_Log) << "Launch command " << m_Launching;
}

void xPLHub::ResetLaunch()
{
    if(!m_LaunchMode) return;
    if(m_LaunchTimeout > time(NULL)) return;

    m_LaunchMode = false;
    Process::terminate(m_LaunchPID);
    LOG_VERBOSE(m_Log) << "Launch timeout kill " << m_Launching;
    StartLaunch();
}

void xPLHub::SynchronizeLaunch(unsigned short port)
{
    m_LaunchMode = false;
    m_HubClients[port].command = m_Launching;
    m_HubClients[port].pid = m_LaunchPID;
    LOG_VERBOSE(m_Log) << "Launch synchronize " << m_Launching << " with " << m_HubClients[port].source;
    StartLaunch();
}

void xPLHub::FindLocalAddress()
{
    unsigned int i;
	char hostName[128];
    struct hostent* pHostEnt;
    struct in_addr** addressList;


    m_LocalAddressList.clear();
    m_LocalAddressList.push_back(INADDR_LOOPBACK);
	if(gethostname(hostName, 127) != 0) return;

	pHostEnt=gethostbyname(hostName);
	if(pHostEnt == nullptr) return;

    addressList = (struct in_addr **)pHostEnt->h_addr_list;
    for(i=0; addressList[i] != NULL; i++)
    {
        m_LocalAddressList.push_back(addressList[i]->s_addr);
        LOG_VERBOSE(m_Log) << "Local ip address found : " <<inet_ntoa(*addressList[i]);
    }

	return;
}

int xPLHub::ServiceStart(int argc, char* argv[])
{
    m_bServiceStop = false;
    if(argc > 1) m_xPLDevice.SetConfigFileName(argv[1]);

    m_xPLDevice.Open();
    if(m_xPLDevice.GetTCPPort() != 3865)
    {
        LOG_FATAL(m_Log) << "TCP port 3865 is used, another hub is active.";
        return 0;
    }

    while(!m_bServiceStop)
    {
        if(m_bServicePause)
            Plateforms::delay(500);
        else
        {
            m_xPLDevice.WaitRecv(500);
            ResetLaunch();
        }
    }

    m_xPLDevice.Close();
    return 0;
}

void xPLHub::ServicePause(bool bPause)
{
    m_bServicePause = bPause;
}

void xPLHub::ServiceStop()
{
    m_bServiceStop = true;
}
