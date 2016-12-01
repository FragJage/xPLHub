#ifndef XPLHUB_H
#define XPLHUB_H

#include<map>
#include<list>
#include "Service/Service.h"
#include "Plateforms/Plateforms.h"
#include "xPLLib/xPLDevCfg.h"

class xPLHub : public Service::IService, public xPL::xPLDevice::IExtension, public xPL::BasicConfig::IConfigure
{
    public:
        xPLHub();
        ~xPLHub();

        bool MsgAnswer(xPL::SchemaObject& msg);
        void Configure();

        void ResetLaunch();

		int ServiceStart(int argc, char* argv[]);
		void ServicePause(bool bPause);
		void ServiceStop();

    private:
        struct HubClient
        {
            unsigned int interval;
            unsigned long ipAddress;
            unsigned short port;
            time_t timeout;
            string source;
            string command;
            processId pid;
        };

        void FindLocalAddress();
        bool AlreadyLaunched(string command);
        void StartLaunch();
        bool ToRelaunch(const HubClient& client);
        void SynchronizeLaunch(unsigned short port);

        std::map<unsigned short, HubClient> m_HubClients;
        std::list<unsigned long> m_LocalAddressList;

        std::list<std::string> m_ToLaunch;
        std::string m_Launching;
        processId m_LaunchPID;
        bool m_LaunchMode;
        time_t m_LaunchTimeout;

        SimpleLog* m_Log;
        xPL::xPLDevCfg m_xPLDevice;
        bool m_bServicePause;
        bool m_bServiceStop;

        SimpleSockUDP m_SenderSock;
};

#endif // XPLHUB_H
