// ecc-adhoc.cc

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/applications-module.h"

#include <iostream>
#include <string>

// Simulating ECC encryption/decryption (mock, real ECC needs heavy crypto libs)
std::string EccEncrypt(const std::string& message) {
    return "ECC_ENCRYPTED(" + message + ")";
}

std::string EccDecrypt(const std::string& encryptedMessage) {
    return encryptedMessage.substr(14, encryptedMessage.size() - 15);
}

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EccAdhocExample");

class SecureEchoApp : public Application
{
public:
    void Setup(Ptr<Socket> socket, Address address, std::string message, uint32_t packets, Time interval);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    std::string m_message;
    uint32_t m_nPackets;
    Time m_interval;
    uint32_t m_sent;
    EventId m_sendEvent;
};

void SecureEchoApp::Setup(Ptr<Socket> socket, Address address, std::string message, uint32_t packets, Time interval)
{
    m_socket = socket;
    m_peer = address;
    m_message = message;
    m_nPackets = packets;
    m_interval = interval;
    m_sent = 0;
}

void SecureEchoApp::StartApplication(void)
{
    m_socket->Connect(m_peer);
    SendPacket();
}

void SecureEchoApp::StopApplication(void)
{
    if (m_socket)
    {
        m_socket->Close();
    }
}

void SecureEchoApp::SendPacket(void)
{
    std::string encrypted = EccEncrypt(m_message);
    Ptr<Packet> packet = Create<Packet>((uint8_t*) encrypted.c_str(), encrypted.length());
    m_socket->Send(packet);

    if (++m_sent < m_nPackets)
    {
        ScheduleTx();
    }
}

void SecureEchoApp::ScheduleTx(void)
{
    m_sendEvent = Simulator::Schedule(m_interval, &SecureEchoApp::SendPacket, this);
}

class SecureEchoReceiver : public Application
{
public:
    void Setup(Ptr<Socket> socket);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;
};

void SecureEchoReceiver::Setup(Ptr<Socket> socket)
{
    m_socket = socket;
}

void SecureEchoReceiver::StartApplication(void)
{
    m_socket->Bind();
    m_socket->SetRecvCallback(MakeCallback(&SecureEchoReceiver::HandleRead, this));
}

void SecureEchoReceiver::StopApplication(void)
{
    if (m_socket)
    {
        m_socket->Close();
    }
}

void SecureEchoReceiver::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    while ((packet = socket->Recv()))
    {
        uint8_t* buffer = new uint8_t[packet->GetSize()];
        packet->CopyData(buffer, packet->GetSize());
        std::string encryptedMessage = std::string((char*)buffer, packet->GetSize());
        std::string decryptedMessage = EccDecrypt(encryptedMessage);
        std::cout << "Received (decrypted): " << decryptedMessage << std::endl;
        delete[] buffer;
    }
}

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(2);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    InternetStackHelper internet;
    OlsrHelper olsr;
    internet.SetRoutingHelper(olsr);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    uint16_t port = 50000;
    Address receiverAddress(InetSocketAddress(interfaces.GetAddress(1), port));
    Ptr<Socket> sourceSocket = Socket::CreateSocket(nodes.Get(0), UdpSocketFactory::GetTypeId());
    Ptr<Socket> sinkSocket = Socket::CreateSocket(nodes.Get(1), UdpSocketFactory::GetTypeId());

    Ptr<SecureEchoApp> app = CreateObject<SecureEchoApp>();
    app->Setup(sourceSocket, receiverAddress, "Hello from Node 0", 5, Seconds(2.0));
    nodes.Get(0)->AddApplication(app);
    app->SetStartTime(Seconds(1.0));
    app->SetStopTime(Seconds(10.0));

    Ptr<SecureEchoReceiver> receiver = CreateObject<SecureEchoReceiver>();
    receiver->Setup(sinkSocket);
    nodes.Get(1)->AddApplication(receiver);
    receiver->SetStartTime(Seconds(0.0));
    receiver->SetStopTime(Seconds(11.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
