#include "secure-socket-helper.h"
#include "secure-socket.h"
#include "ecc-crypto.h"
#include "ecc-key-exchange.h"

#include "ns3/log.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/object-factory.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SecureSocketHelper");

SecureSocketHelper::SecureSocketHelper()
    : m_socketFactoryTid(UdpSocketFactory::GetTypeId())
{
    NS_LOG_FUNCTION(this);
}

SecureSocketHelper::~SecureSocketHelper()
{
    NS_LOG_FUNCTION(this);
}

void
SecureSocketHelper::SetSocketFactoryType(TypeId tid)
{
    NS_LOG_FUNCTION(this << tid.GetName());
    m_socketFactoryTid = tid;
}

void
SecureSocketHelper::Install(NodeContainer container)
{
    NS_LOG_FUNCTION(this);
    for (NodeContainer::Iterator i = container.Begin(); i != container.End(); ++i)
    {
        Install(*i);
    }
}

void
SecureSocketHelper::Install(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    
    // Initialize the node's cryptography
    EccManager::GetInstance().InitializeNode(node->GetId());
    
    // Create the socket factory
    ObjectFactory factory;
    factory.SetTypeId(SecureSocketFactory::GetTypeId());
    Ptr<SecureSocketFactory> secureFactory = factory.Create<SecureSocketFactory>();
    
    // Get the existing socket factory
    Ptr<ns3::SocketFactory> existingFactory = node->GetObject<ns3::SocketFactory>(m_socketFactoryTid);
    if (!existingFactory)
    {
        // The node doesn't have the socket factory yet, create it
        ObjectFactory baseFactory;
        baseFactory.SetTypeId(m_socketFactoryTid);
        existingFactory = baseFactory.Create<ns3::SocketFactory>();
        node->AggregateObject(existingFactory);
    }
    
    // Configure the secure factory
    secureFactory->SetFactory(existingFactory);
    secureFactory->SetNodeId(node->GetId());
    
    // Add the secure socket factory to the node
    node->AggregateObject(secureFactory);
    
    NS_LOG_INFO("Installed secure socket factory on node " << node->GetId());
}

} // namespace ns3