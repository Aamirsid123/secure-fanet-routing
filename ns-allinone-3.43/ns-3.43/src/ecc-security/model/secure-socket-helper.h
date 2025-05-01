#ifndef SECURE_SOCKET_HELPER_H
#define SECURE_SOCKET_HELPER_H

#include "ns3/node-container.h"
#include "ns3/socket-factory.h"
#include "ns3/ptr.h"

namespace ns3 {

/**
 * \brief Helper class to install secure socket functionality on nodes
 */
class SecureSocketHelper
{
public:
  /**
   * \brief Constructor
   */
  SecureSocketHelper();
  
  /**
   * \brief Destructor
   */
  virtual ~SecureSocketHelper();
  
  /**
   * \brief Install secure socket capabilities on the provided nodes
   * \param container The container of nodes
   */
  void Install(NodeContainer container);
  
  /**
   * \brief Install secure socket capabilities on the provided node
   * \param node The node
   */
  void Install(Ptr<Node> node);
  
  /**
   * \brief Set the underlying socket factory type
   * \param tid The TypeId of the socket factory
   */
  void SetSocketFactoryType(TypeId tid);
  
private:
  TypeId m_socketFactoryTid; //!< The type ID of the socket factory
};

} // namespace ns3

#endif // SECURE_SOCKET_HELPER_H
