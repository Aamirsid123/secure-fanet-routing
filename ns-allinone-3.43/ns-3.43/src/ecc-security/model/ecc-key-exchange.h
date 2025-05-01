#ifndef ECC_KEY_EXCHANGE_H
#define ECC_KEY_EXCHANGE_H

#include "ns3/ptr.h"
#include "ns3/object.h"

#include <openssl/ec.h>
#include <string>
#include <vector>

namespace ns3 {

/**
 * \brief ECC key exchange implementation
 */
class EccKeyExchange : public Object
{
public:
  static TypeId GetTypeId();

  EccKeyExchange();
  virtual ~EccKeyExchange();

  /**
   * \brief Generate a key pair
   * \return True if successful
   */
  bool GenerateKeyPair();

  /**
   * \brief Generate a shared secret with another party
   * \param otherPublicKey The other party's public key
   * \return True if successful
   */
  bool GenerateSharedSecret(const std::vector<uint8_t>& otherPublicKey);

  /**
   * \brief Get the public key
   * \return The public key
   */
  std::vector<uint8_t> GetPublicKey() const;

  /**
   * \brief Get the shared secret
   * \return The shared secret
   */
  std::vector<uint8_t> GetSharedSecret() const;

private:
  EC_KEY* m_ecKey;
  std::vector<uint8_t> m_sharedSecret;
};

} // namespace ns3

#endif /* ECC_KEY_EXCHANGE_H */