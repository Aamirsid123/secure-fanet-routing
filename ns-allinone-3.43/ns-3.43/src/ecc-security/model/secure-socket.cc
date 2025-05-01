#include "secure-socket.h"
#include "ecc-crypto.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/inet-socket-address.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SecureSocket");

// SecureSocket implementation
NS_OBJECT_ENSURE_REGISTERED(SecureSocket);

TypeId
SecureSocket::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SecureSocket")
                            .SetParent<Socket>()
                            .SetGroupName("Network")
                            .AddConstructor<SecureSocket>()
                            .AddTraceSource("ReceivedData",
                                           "Data received, encrypted and decrypted",
                                           MakeTraceSourceAccessor(&SecureSocket::m_receivedData),
                                           "ns3::SecureSocket::ReceivedDataTracedCallback");
    return tid;
}

SecureSocket::SecureSocket()
    : m_socket(nullptr)
    , m_nodeId(0)
{
    NS_LOG_FUNCTION(this);
}

SecureSocket::~SecureSocket()
{
    NS_LOG_FUNCTION(this);
    m_socket = nullptr;
}

void
SecureSocket::SetSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_socket = socket;
    m_socket->SetRecvCallback(MakeCallback(&SecureSocket::ForwardUp, this));
}

void
SecureSocket::SetNodeId(uint32_t id)
{
    NS_LOG_FUNCTION(this << id);
    m_nodeId = id;
}

void
SecureSocket::ForwardUp(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Address fromAddress;
    Ptr<Packet> packet = socket->RecvFrom(std::numeric_limits<uint32_t>::max(), 0, fromAddress);
    
    if (packet)
    {
        // Decrypt the packet
        packet = EccManager::GetInstance().DecryptPacket(packet, m_nodeId);
        
        // Forward the decrypted packet up
        m_receivedData(packet, fromAddress);
        NotifyDataRecv();
    }
}

enum Socket::SocketErrno
SecureSocket::GetErrno() const
{
    NS_LOG_FUNCTION(this);
    return m_socket->GetErrno();
}

enum Socket::SocketType
SecureSocket::GetSocketType() const
{
    NS_LOG_FUNCTION(this);
    return m_socket->GetSocketType();
}

Ptr<Node>
SecureSocket::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_socket->GetNode();
}

int
SecureSocket::Bind()
{
    NS_LOG_FUNCTION(this);
    return m_socket->Bind();
}

int
SecureSocket::Bind6()
{
    NS_LOG_FUNCTION(this);
    return m_socket->Bind6();
}

int
SecureSocket::Bind(const Address& address)
{
    NS_LOG_FUNCTION(this << address);
    return m_socket->Bind(address);
}

int
SecureSocket::Close()
{
    NS_LOG_FUNCTION(this);
    return m_socket->Close();
}

int
SecureSocket::ShutdownSend()
{
    NS_LOG_FUNCTION(this);
    return m_socket->ShutdownSend();
}

int
SecureSocket::ShutdownRecv()
{
    NS_LOG_FUNCTION(this);
    return m_socket->ShutdownRecv();
}

int
SecureSocket::Connect(const Address& address)
{
    NS_LOG_FUNCTION(this << address);
    return m_socket->Connect(address);
}

int
SecureSocket::Listen()
{
    NS_LOG_FUNCTION(this);
    return m_socket->Listen();
}

int
SecureSocket::Send(Ptr<Packet> p, uint32_t flags)
{
    NS_LOG_FUNCTION(this << p << flags);
    
    // Get destination node ID from connected address
    Address connectedAddress;
    if (GetPeerName(connectedAddress) != 0)
    {
        NS_LOG_ERROR("Not connected");
        return -1;
    }
    
    uint32_t destNodeId = 0;
    if (InetSocketAddress::IsMatchingType(connectedAddress))
    {
        // This is a simplification - in a real implementation, we would need
        // a mapping from IP address to node ID
        InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(connectedAddress);
        Ipv4Address ipv4Addr = inetAddr.GetIpv4();
        // Extract node ID from last octet of IP address as a simple mapping
        destNodeId = ipv4Addr.Get() & 0xFF;
    }
    else
    {
        NS_LOG_ERROR("Unknown address type");
        return -1;
    }
    
    // Encrypt the packet
    Ptr<Packet> encryptedPacket = EccManager::GetInstance().EncryptPacket(p, m_nodeId, destNodeId);
    
    // Send the encrypted packet
    return m_socket->Send(encryptedPacket, flags);
}

int
SecureSocket::SendTo(Ptr<Packet> p, uint32_t flags, const Address& toAddress)
{
    NS_LOG_FUNCTION(this << p << flags << toAddress);
    
    uint32_t destNodeId = 0;
    if (InetSocketAddress::IsMatchingType(toAddress))
    {
        // This is a simplification - in a real implementation, we would need
        // a mapping from IP address to node ID
        InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(toAddress);
        Ipv4Address ipv4Addr = inetAddr.GetIpv4();
        // Extract node ID from last octet of IP address as a simple mapping
        destNodeId = ipv4Addr.Get() & 0xFF;
    }
    else
    {
        NS_LOG_ERROR("Unknown address type");
        return -1;
    }
    
    // Encrypt the packet
    Ptr<Packet> encryptedPacket = EccManager::GetInstance().EncryptPacket(p, m_nodeId, destNodeId);
    
    // Send the encrypted packet
    return m_socket->SendTo(encryptedPacket, flags, toAddress);
}

Ptr<Packet>
SecureSocket::Recv(uint32_t maxSize, uint32_t flags)
{
    NS_LOG_FUNCTION(this << maxSize << flags);
    
    // We don't use maxSize here because we need to receive the entire
    // encrypted packet to decrypt it. The real size limit will be applied
    // after decryption.
    Ptr<Packet> packet = m_socket->Recv(std::numeric_limits<uint32_t>::max(), flags);
    
    if (packet)
    {
        // Decrypt the packet
        packet = EccManager::GetInstance().DecryptPacket(packet, m_nodeId);
        
        // Apply maxSize limit after decryption
        if (packet->GetSize() > maxSize)
        {
            packet = packet->CreateFragment(0, maxSize);
        }
    }
    
    return packet;
}

Ptr<Packet>
SecureSocket::RecvFrom(uint32_t maxSize, uint32_t flags, Address& fromAddress)
{
    NS_LOG_FUNCTION(this << maxSize << flags);
    
    // We don't use maxSize here because we need to receive the entire
    // encrypted packet to decrypt it. The real size limit will be applied
    // after decryption.
    Ptr<Packet> packet = m_socket->RecvFrom(std::numeric_limits<uint32_t>::max(), flags, fromAddress);
    
    if (packet)
    {
        // Decrypt the packet
        packet = EccManager::GetInstance().DecryptPacket(packet, m_nodeId);
        
        // Apply maxSize limit after decryption
        if (packet->GetSize() > maxSize)
        {
            packet = packet->CreateFragment(0, maxSize);
        }
    }
    
    return packet;
}

uint32_t
SecureSocket::GetTxAvailable() const
{
    NS_LOG_FUNCTION(this);
    // We need to account for encryption overhead
    return m_socket->GetTxAvailable() > 32 ? m_socket->GetTxAvailable() - 32 : 0;
}

uint32_t
SecureSocket::GetRxAvailable() const
{
    NS_LOG_FUNCTION(this);
    // Since we don't know the exact size after decryption, we make a conservative estimate
    return m_socket->GetRxAvailable() > 32 ? m_socket->GetRxAvailable() - 32 : 0;
}

int
SecureSocket::GetSockName(Address& address) const
{
    NS_LOG_FUNCTION(this << address);
    return m_socket->GetSockName(address);
}

int
SecureSocket::GetPeerName(Address& address) const
{
    NS_LOG_FUNCTION(this << address);
    return m_socket->GetPeerName(address);
}

bool
SecureSocket::SetAllowBroadcast(bool allowBroadcast)
{
    NS_LOG_FUNCTION(this << allowBroadcast);
    return m_socket->SetAllowBroadcast(allowBroadcast);
}

bool
SecureSocket::GetAllowBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return m_socket->GetAllowBroadcast();
}

bool
SecureSocket::IsRecvPktInfo() const
{
    NS_LOG_FUNCTION(this);
    return m_socket->IsRecvPktInfo();
}

void
SecureSocket::SetIpTtl(uint8_t ipTtl)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(ipTtl));
    m_socket->SetIpTtl(ipTtl);
}

uint8_t
SecureSocket::GetIpTtl() const
{
    NS_LOG_FUNCTION(this);
    return m_socket->GetIpTtl();
}

void
SecureSocket::SetIpMulticastTtl(uint8_t ipTtl)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(ipTtl));
    // Use attribute-based approach since direct method isn't available
    m_socket->SetAttribute("IpMulticastTtl", UintegerValue(ipTtl));
}

uint8_t
SecureSocket::GetIpMulticastTtl() const
{
    NS_LOG_FUNCTION(this);
    UintegerValue value;
    m_socket->GetAttribute("IpMulticastTtl", value);
    return value.Get();
}

void
SecureSocket::SetIpMulticastIf(int32_t ipIf)
{
    NS_LOG_FUNCTION(this << ipIf);
    // Use attribute-based approach since direct method isn't available
    m_socket->SetAttribute("IpMulticastIf", IntegerValue(ipIf));
}

int32_t
SecureSocket::GetIpMulticastIf() const
{
    NS_LOG_FUNCTION(this);
    IntegerValue value;
    m_socket->GetAttribute("IpMulticastIf", value);
    return value.Get();
}

void
SecureSocket::SetIpMulticastLoop(bool loop)
{
    NS_LOG_FUNCTION(this << loop);
    // Use attribute-based approach since direct method isn't available
    m_socket->SetAttribute("IpMulticastLoop", BooleanValue(loop));
}

bool
SecureSocket::GetIpMulticastLoop() const
{
    NS_LOG_FUNCTION(this);
    BooleanValue value;
    m_socket->GetAttribute("IpMulticastLoop", value);
    return value.Get();
}

void
SecureSocket::SetIpv6HopLimit(uint8_t ipHopLimit)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(ipHopLimit));
    m_socket->SetIpv6HopLimit(ipHopLimit);
}

uint8_t
SecureSocket::GetIpv6HopLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_socket->GetIpv6HopLimit();
}

void
SecureSocket::SetIpv6MulticastHopLimit(uint8_t ipHopLimit)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(ipHopLimit));
    // Use attribute-based approach since direct method isn't available
    m_socket->SetAttribute("Ipv6MulticastHopLimit", UintegerValue(ipHopLimit));
}

uint8_t
SecureSocket::GetIpv6MulticastHopLimit() const
{
    NS_LOG_FUNCTION(this);
    UintegerValue value;
    m_socket->GetAttribute("Ipv6MulticastHopLimit", value);
    return value.Get();
}

void
SecureSocket::SetIpv6MulticastIf(int32_t ipIf)
{
    NS_LOG_FUNCTION(this << ipIf);
    // Use attribute-based approach since direct method isn't available
    m_socket->SetAttribute("Ipv6MulticastIf", IntegerValue(ipIf));
}

int32_t
SecureSocket::GetIpv6MulticastIf() const
{
    NS_LOG_FUNCTION(this);
    IntegerValue value;
    m_socket->GetAttribute("Ipv6MulticastIf", value);
    return value.Get();
}

void
SecureSocket::SetIpv6MulticastLoop(bool loop)
{
    NS_LOG_FUNCTION(this << loop);
    // Use attribute-based approach since direct method isn't available
    m_socket->SetAttribute("Ipv6MulticastLoop", BooleanValue(loop));
}

bool
SecureSocket::GetIpv6MulticastLoop() const
{
    NS_LOG_FUNCTION(this);
    BooleanValue value;
    m_socket->GetAttribute("Ipv6MulticastLoop", value);
    return value.Get();
}

void
SecureSocket::SetRecvPktInfo(bool flag)
{
    NS_LOG_FUNCTION(this << flag);
    m_socket->SetRecvPktInfo(flag);
}

// SecureSocketFactory implementation
NS_OBJECT_ENSURE_REGISTERED(SecureSocketFactory);

TypeId
SecureSocketFactory::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SecureSocketFactory")
                            .SetParent<SocketFactory>()
                            .SetGroupName("Network")
                            .AddConstructor<SecureSocketFactory>();
    return tid;
}

SecureSocketFactory::SecureSocketFactory()
    : m_factory(nullptr)
    , m_nodeId(0)
{
    NS_LOG_FUNCTION(this);
}

SecureSocketFactory::~SecureSocketFactory()
{
    NS_LOG_FUNCTION(this);
    m_factory = nullptr;
}

void
SecureSocketFactory::SetFactory(Ptr<SocketFactory> factory)
{
    NS_LOG_FUNCTION(this << factory);
    m_factory = factory;
}

void
SecureSocketFactory::SetNodeId(uint32_t id)
{
    NS_LOG_FUNCTION(this << id);
    m_nodeId = id;
}

Ptr<Socket>
SecureSocketFactory::CreateSocket()
{
    NS_LOG_FUNCTION(this);
    
    Ptr<Socket> socket = m_factory->CreateSocket();
    Ptr<SecureSocket> secureSocket = CreateObject<SecureSocket>();
    secureSocket->SetSocket(socket);
    secureSocket->SetNodeId(m_nodeId);
    
    return secureSocket;
}

} // namespace ns3
