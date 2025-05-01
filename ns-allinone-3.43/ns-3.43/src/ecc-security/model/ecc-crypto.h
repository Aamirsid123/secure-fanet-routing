#ifndef ECC_CRYPTO_H
#define ECC_CRYPTO_H

#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include <openssl/ec.h>
#include <map>
#include <vector>

namespace ns3 {

/**
 * \brief Header for encrypted packets
 */
class EncryptionHeader : public Header, public SimpleRefCount<EncryptionHeader>
{
public:
  /**
   * \brief Get the type ID.
   * \return The object TypeId.
   */
  static TypeId GetTypeId();
  virtual TypeId GetInstanceTypeId() const;

  EncryptionHeader();
  EncryptionHeader(const uint8_t* ivHeader, uint32_t size);
  virtual ~EncryptionHeader();

  // Inherited methods
  virtual void Print(std::ostream& os) const;
  virtual uint32_t GetSerializedSize() const;
  virtual void Serialize(Buffer::Iterator start) const;
  virtual uint32_t Deserialize(Buffer::Iterator start);

  /**
   * \brief Get the IV header
   * \return The IV header
   */
  const uint8_t* GetIvHeader() const;

  /**
   * \brief Get the IV header size
   * \return The IV header size
   */
  uint32_t GetIvHeaderSize() const;

private:
  uint8_t* m_ivHeader;
  uint32_t m_ivHeaderSize;
};

/**
 * \brief ECC security manager
 */
class EccManager
{
  public:
  /**
   * \brief Get the instance of the EccManager
   * \return The instance
   */
  static EccManager& GetInstance();

  /**
   * \brief Encrypt a packet
   * \param packet The packet to encrypt
   * \param srcNodeId The source node ID
   * \param destNodeId The destination node ID
   * \return The encrypted packet
   */
  Ptr<Packet> EncryptPacket(Ptr<Packet> packet, uint32_t srcNodeId, uint32_t destNodeId);

  /**
   * \brief Decrypt a packet
   * \param packet The packet to decrypt
   * \param nodeId The node ID
   * \return The decrypted packet
   */
  Ptr<Packet> DecryptPacket(Ptr<Packet> packet, uint32_t nodeId);
  
  /**
   * \brief Initialize cryptography for a node
   * \param nodeId The node ID to initialize
   * \return True if successful
   */
  bool InitializeNode(uint32_t nodeId);

private:
  /**
   * \brief Structure to hold key pair information
   */
  struct KeyPair {
    EC_KEY* privateKey;
  };

  /**
   * \brief Constructor
   */
  EccManager();
  
  /**
   * \brief Destructor
   */
  ~EccManager();
  
  /**
   * \brief Generate or retrieve a key pair
   * \param nodeId The node ID
   * \param forceRegen Whether to force regeneration
   * \return True if successful
   */
  bool GenerateKeyPair(uint32_t nodeId, bool forceRegen);

//  /**
//    * \brief Initialize cryptography for a node
//    * \param nodeId The node ID to initialize
//    * \return True if successful
//    */
//   bool InitializeNode(uint32_t nodeId);

  /**
   * \brief Generate or retrieve shared secret
   * \param localNodeId The local node ID
   * \param remoteNodeId The remote node ID
   * \param secret The shared secret (output)
   * \return The length of the shared secret or -1 on error
   */
  int GenerateSharedSecret(uint32_t localNodeId, uint32_t remoteNodeId, unsigned char** secret);

  // Member variables
  std::map<uint32_t, KeyPair> m_keyPairs;
  std::map<std::pair<uint32_t, uint32_t>, std::vector<unsigned char>> m_sharedSecrets;
};

} // namespace ns3

#endif /* ECC_CRYPTO_H */