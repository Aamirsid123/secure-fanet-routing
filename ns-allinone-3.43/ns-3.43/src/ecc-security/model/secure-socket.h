
#ifndef SECURE_SOCKET_H
#define SECURE_SOCKET_H

#include "ns3/socket.h"
#include "ns3/socket-factory.h"
#include "ns3/ptr.h"
#include "ns3/callback.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Node;
class Packet;

/**
 * \ingroup socket
 *
 * \brief A secure wrapper around a Socket
 *
 * This class implements secure communications using ECC
 */
class SecureSocket : public Socket
{
public:
  static TypeId GetTypeId();

  SecureSocket();
  virtual ~SecureSocket();

  /**
   * \brief Set the underlying socket
   * \param socket the socket to use
   */
  void SetSocket(Ptr<Socket> socket);

  /**
   * \brief Set the node ID
   * \param id the node ID
   */
  void SetNodeId(uint32_t id);

  // From Socket
  virtual enum Socket::SocketErrno GetErrno() const;
  virtual enum Socket::SocketType GetSocketType() const;
  virtual Ptr<Node> GetNode() const;
  virtual int Bind();
  virtual int Bind6();
  virtual int Bind(const Address& address);
  virtual int Close();
  virtual int ShutdownSend();
  virtual int ShutdownRecv();
  virtual int Connect(const Address& address);
  virtual int Listen();
  virtual int Send(Ptr<Packet> p, uint32_t flags);
  virtual int SendTo(Ptr<Packet> p, uint32_t flags, const Address& toAddress);
  virtual Ptr<Packet> Recv(uint32_t maxSize, uint32_t flags);
  virtual Ptr<Packet> RecvFrom(uint32_t maxSize, uint32_t flags, Address& fromAddress);
  virtual uint32_t GetTxAvailable() const;
  virtual uint32_t GetRxAvailable() const;
  virtual int GetSockName(Address& address) const;
  virtual int GetPeerName(Address& address) const;
  virtual bool SetAllowBroadcast(bool allowBroadcast);
  virtual bool GetAllowBroadcast() const;
  virtual bool IsRecvPktInfo() const;
  
  // Methods with updated naming
  virtual void SetIpTtl(uint8_t ipTtl);
  virtual uint8_t GetIpTtl() const;
  virtual void SetIpMulticastTtl(uint8_t ipTtl);
  virtual uint8_t GetIpMulticastTtl() const;
  virtual void SetIpMulticastIf(int32_t ipIf);
  virtual int32_t GetIpMulticastIf() const;
  virtual void SetIpMulticastLoop(bool loop);
  virtual bool GetIpMulticastLoop() const;
  virtual void SetIpv6HopLimit(uint8_t ipHopLimit);
  virtual uint8_t GetIpv6HopLimit() const;
  virtual void SetIpv6MulticastHopLimit(uint8_t ipHopLimit);
  virtual uint8_t GetIpv6MulticastHopLimit() const;
  virtual void SetIpv6MulticastIf(int32_t ipIf);
  virtual int32_t GetIpv6MulticastIf() const;
  virtual void SetIpv6MulticastLoop(bool loop);
  virtual bool GetIpv6MulticastLoop() const;
  virtual void SetRecvPktInfo(bool flag);

private:
  void ForwardUp(Ptr<Socket> socket);

  typedef TracedCallback<Ptr<const Packet>, const Address&> ReceivedDataTracedCallback;
  
  Ptr<Socket> m_socket;
  uint32_t m_nodeId;
  ReceivedDataTracedCallback m_receivedData;
};

/**
 * \ingroup socket
 *
 * \brief A factory to create secure sockets
 */
class SecureSocketFactory : public ns3::SocketFactory
{
public:
  static TypeId GetTypeId();

  SecureSocketFactory();
  virtual ~SecureSocketFactory();

  /**
   * \brief Set the underlying socket factory
   * \param factory the socket factory to use
   */
  void SetFactory(Ptr<SocketFactory> factory);

  /**
   * \brief Set the node ID
   * \param id the node ID
   */
  void SetNodeId(uint32_t id);

  virtual Ptr<Socket> CreateSocket();

private:
  Ptr<SocketFactory> m_factory;
  uint32_t m_nodeId;
};

} // namespace ns3

#endif /* SECURE_SOCKET_H */