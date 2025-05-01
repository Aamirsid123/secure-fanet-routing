#include "ecc-crypto.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <map>
#include <string>
#include <vector>
#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("EccCrypto");

// Helper functions
static void PrintOpenSSLError()
{
    char err_buf[256];
    unsigned long err = ERR_get_error();
    ERR_error_string_n(err, err_buf, sizeof(err_buf));
    NS_LOG_ERROR("OpenSSL Error: " << err_buf);
}

// EccManager implementation
EccManager::EccManager()
{
    NS_LOG_FUNCTION(this);
    // Initialize OpenSSL
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
}

EccManager::~EccManager()
{
    NS_LOG_FUNCTION(this);
    // Cleanup OpenSSL
    EVP_cleanup();
    ERR_free_strings();
}

EccManager&
EccManager::GetInstance()
{
    static EccManager instance;
    return instance;
}

bool
EccManager::InitializeNode(uint32_t nodeId)
{
    NS_LOG_FUNCTION(this << nodeId);
    
    // Generate a key pair for this node
    return GenerateKeyPair(nodeId, false);
}

bool
EccManager::GenerateKeyPair(uint32_t nodeId, bool forceRegen)
{
    NS_LOG_FUNCTION(this << nodeId << forceRegen);
    
    if (!forceRegen && m_keyPairs.find(nodeId) != m_keyPairs.end())
    {
        NS_LOG_INFO("Key pair already exists for node " << nodeId);
        return true;
    }
    
    NS_LOG_INFO("Generating key pair for node " << nodeId);
    
    // Create EC key context using NIST P-256 curve
    EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!ecKey)
    {
        PrintOpenSSLError();
        return false;
    }
    
    // Generate key pair
    if (EC_KEY_generate_key(ecKey) != 1)
    {
        PrintOpenSSLError();
        EC_KEY_free(ecKey);
        return false;
    }
    
    // Store the keypair
    KeyPair keyPair;
    keyPair.privateKey = ecKey;
    
    // Store the key pair
    m_keyPairs[nodeId] = keyPair;
    
    return true;
}

Ptr<Packet>
EccManager::EncryptPacket(Ptr<Packet> packet, uint32_t srcNodeId, uint32_t destNodeId)
{
    NS_LOG_FUNCTION(this << packet << srcNodeId << destNodeId);
    
    // Ensure we have keys for both nodes
    if (!GenerateKeyPair(srcNodeId, false) || !GenerateKeyPair(destNodeId, false))
    {
        NS_LOG_ERROR("Failed to ensure key pairs for encryption");
        return packet; // Return unencrypted as fallback
    }
    
    // Generate or retrieve shared secret with destination
    unsigned char* sharedSecret = nullptr;
    int secretLen = GenerateSharedSecret(srcNodeId, destNodeId, &sharedSecret);
    
    if (secretLen <= 0 || !sharedSecret)
    {
        NS_LOG_ERROR("Failed to generate shared secret for encryption");
        return packet; // Return unencrypted as fallback
    }
    
    // Create encryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        PrintOpenSSLError();
        OPENSSL_free(sharedSecret);
        return packet; // Return unencrypted as fallback
    }
    
    // Generate random IV
    unsigned char iv[16];
    if (RAND_bytes(iv, sizeof(iv)) != 1)
    {
        PrintOpenSSLError();
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(sharedSecret);
        return packet; // Return unencrypted as fallback
    }
    
    // Derive encryption key from shared secret using SHA-256
    unsigned char key[32];
    EVP_Digest(sharedSecret, secretLen, key, nullptr, EVP_sha256(), nullptr);
    
    // Initialize encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1)
    {
        PrintOpenSSLError();
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(sharedSecret);
        return packet; // Return unencrypted as fallback
    }
    
    // Copy packet to buffer
    uint32_t size = packet->GetSize();
    uint8_t* buffer = new uint8_t[size];
    packet->CopyData(buffer, size);
    
    // Allocate output buffer (includes space for padding)
    unsigned char* outBuffer = new unsigned char[size + EVP_CIPHER_CTX_block_size(ctx)];
    int outLen = 0;
    
    // Encrypt
    if (EVP_EncryptUpdate(ctx, outBuffer, &outLen, buffer, size) != 1)
    {
        PrintOpenSSLError();
        delete[] buffer;
        delete[] outBuffer;
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(sharedSecret);
        return packet; // Return unencrypted as fallback
    }
    
    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx, outBuffer + outLen, &finalLen) != 1)
    {
        PrintOpenSSLError();
        delete[] buffer;
        delete[] outBuffer;
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(sharedSecret);
        return packet; // Return unencrypted as fallback
    }
    
    // Total encrypted length
    int encryptedLen = outLen + finalLen;
    
    // Create new packet with header (IV + encrypted data)
    Ptr<Packet> encryptedPacket = Create<Packet>();
    
    // Add IV header
    uint8_t ivHeader[16 + 8]; // IV + source node ID + dest node ID
    memcpy(ivHeader, iv, 16);
    memcpy(ivHeader + 16, &srcNodeId, 4);
    memcpy(ivHeader + 20, &destNodeId, 4);
    
    // Create and add the header
    Ptr<EncryptionHeader> header = Create<EncryptionHeader>(ivHeader, sizeof(ivHeader));
    encryptedPacket->AddHeader(*header);
    
    // Add encrypted payload
    encryptedPacket->AddAtEnd(Create<Packet>(outBuffer, encryptedLen));
    
    // Cleanup
    delete[] buffer;
    delete[] outBuffer;
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_free(sharedSecret);
    
    NS_LOG_INFO("Encrypted packet: original size=" << size << ", encrypted size=" << encryptedPacket->GetSize());
    
    return encryptedPacket;
}

Ptr<Packet>
EccManager::DecryptPacket(Ptr<Packet> packet, uint32_t nodeId)
{
    NS_LOG_FUNCTION(this << packet << nodeId);
    
    // Extract the encryption header
    EncryptionHeader header;
    packet->RemoveHeader(header);
    
    // Get IV, source and destination node IDs from header
    const uint8_t* ivHeader = header.GetIvHeader();
    unsigned char iv[16];
    memcpy(iv, ivHeader, 16);
    
    uint32_t srcNodeId, destNodeId;
    memcpy(&srcNodeId, ivHeader + 16, 4);
    memcpy(&destNodeId, ivHeader + 20, 4);
    
    // Ensure we have keys for both nodes
    if (!GenerateKeyPair(srcNodeId, false) || !GenerateKeyPair(destNodeId, false))
    {
        NS_LOG_ERROR("Failed to ensure key pairs for decryption");
        return packet; // Return encrypted packet as fallback
    }
    
    // Check if we're the intended recipient
    if (nodeId != destNodeId)
    {
        NS_LOG_WARN("Attempting to decrypt a packet not meant for this node");
    }
    
    // Generate or retrieve shared secret with source
    unsigned char* sharedSecret = nullptr;
    int secretLen = GenerateSharedSecret(nodeId, srcNodeId, &sharedSecret);
    
    if (secretLen <= 0 || !sharedSecret)
    {
        NS_LOG_ERROR("Failed to generate shared secret for decryption");
        return packet; // Return encrypted packet as fallback
    }
    
    // Create decryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        PrintOpenSSLError();
        OPENSSL_free(sharedSecret);
        return packet; // Return encrypted packet as fallback
    }
    
    // Derive decryption key from shared secret using SHA-256
    unsigned char key[32];
    EVP_Digest(sharedSecret, secretLen, key, nullptr, EVP_sha256(), nullptr);
    
    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1)
    {
        PrintOpenSSLError();
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(sharedSecret);
        return packet; // Return encrypted packet as fallback
    }
    
    // Copy packet to buffer
    uint32_t size = packet->GetSize();
    uint8_t* buffer = new uint8_t[size];
    packet->CopyData(buffer, size);
    
    // Allocate output buffer
    unsigned char* outBuffer = new unsigned char[size];
    int outLen = 0;
    
    // Decrypt
    if (EVP_DecryptUpdate(ctx, outBuffer, &outLen, buffer, size) != 1)
    {
        PrintOpenSSLError();
        delete[] buffer;
        delete[] outBuffer;
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(sharedSecret);
        return packet; // Return encrypted packet as fallback
    }
    
    int finalLen = 0;
    if (EVP_DecryptFinal_ex(ctx, outBuffer + outLen, &finalLen) != 1)
    {
        PrintOpenSSLError();
        delete[] buffer;
        delete[] outBuffer;
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(sharedSecret);
        return packet; // Return encrypted packet as fallback
    }
    
    // Total decrypted length
    int decryptedLen = outLen + finalLen;
    
    // Create new packet with decrypted data
    Ptr<Packet> decryptedPacket = Create<Packet>(outBuffer, decryptedLen);
    
    // Cleanup
    delete[] buffer;
    delete[] outBuffer;
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_free(sharedSecret);
    
    NS_LOG_INFO("Decrypted packet: encrypted size=" << size << ", decrypted size=" << decryptedPacket->GetSize());
    
    return decryptedPacket;
}

int
EccManager::GenerateSharedSecret(uint32_t localNodeId, uint32_t remoteNodeId, unsigned char** secret)
{
    NS_LOG_FUNCTION(this << localNodeId << remoteNodeId);
    
    // Check if we already have a shared secret for this pair
    auto keyPair = std::make_pair(std::min(localNodeId, remoteNodeId), std::max(localNodeId, remoteNodeId));
    auto it = m_sharedSecrets.find(keyPair);
    if (it != m_sharedSecrets.end())
    {
        // Use cached shared secret
        *secret = (unsigned char*)OPENSSL_malloc(it->second.size());
        if (!*secret)
        {
            NS_LOG_ERROR("Failed to allocate memory for shared secret");
            return -1;
        }
        
        memcpy(*secret, it->second.data(), it->second.size());
        return it->second.size();
    }
    
    // Ensure we have keys for both nodes
    if (m_keyPairs.find(localNodeId) == m_keyPairs.end() || 
        m_keyPairs.find(remoteNodeId) == m_keyPairs.end())
    {
        NS_LOG_ERROR("Missing key pair for node " << 
                    (m_keyPairs.find(localNodeId) == m_keyPairs.end() ? localNodeId : remoteNodeId));
        return -1;
    }
    
    // Get the required keys
    EC_KEY* localPrivKey = m_keyPairs[localNodeId].privateKey;
    EC_KEY* remotePubKey = m_keyPairs[remoteNodeId].privateKey;
    
    // Get the public key from the remote node's key
    const EC_POINT* remotePubKeyPoint = EC_KEY_get0_public_key(remotePubKey);
    if (!remotePubKeyPoint)
    {
        PrintOpenSSLError();
        return -1;
    }
    
    // Compute shared secret
    const EC_GROUP* group = EC_KEY_get0_group(localPrivKey);
    int fieldSize = EC_GROUP_get_degree(group);
    int secretLen = (fieldSize + 7) / 8;
    
    *secret = (unsigned char*)OPENSSL_malloc(secretLen);
    if (!*secret)
    {
        NS_LOG_ERROR("Failed to allocate memory for shared secret");
        return -1;
    }
    
    int len = ECDH_compute_key(*secret, secretLen, remotePubKeyPoint, localPrivKey, nullptr);
    if (len <= 0)
    {
        PrintOpenSSLError();
        OPENSSL_free(*secret);
        *secret = nullptr;
        return -1;
    }
    
    // Cache the shared secret
    std::vector<unsigned char> secretVec(*secret, *secret + len);
    m_sharedSecrets[keyPair] = secretVec;
    
    return len;
}

// EncryptionHeader implementation
EncryptionHeader::EncryptionHeader()
    : m_ivHeader(nullptr)
    , m_ivHeaderSize(0)
{
    NS_LOG_FUNCTION(this);
}

EncryptionHeader::EncryptionHeader(const uint8_t* ivHeader, uint32_t size)
    : m_ivHeader(nullptr)
    , m_ivHeaderSize(size)
{
    NS_LOG_FUNCTION(this << ivHeader << size);
    
    m_ivHeader = new uint8_t[size];
    memcpy(m_ivHeader, ivHeader, size);
}

EncryptionHeader::~EncryptionHeader()
{
    NS_LOG_FUNCTION(this);
    
    if (m_ivHeader)
    {
        delete[] m_ivHeader;
        m_ivHeader = nullptr;
    }
}

TypeId
EncryptionHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EncryptionHeader")
                            .SetParent<Header>()
                            .SetGroupName("Network")
                            .AddConstructor<EncryptionHeader>();
    return tid;
}

TypeId
EncryptionHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
EncryptionHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "EncryptionHeader (size=" << m_ivHeaderSize << ")";
}

uint32_t
EncryptionHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return m_ivHeaderSize + 4; // size + IV data
}

void
EncryptionHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    
    start.WriteHtonU32(m_ivHeaderSize);
    start.Write(m_ivHeader, m_ivHeaderSize);
}

uint32_t
EncryptionHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    
    m_ivHeaderSize = start.ReadNtohU32();
    
    if (m_ivHeader)
    {
        delete[] m_ivHeader;
    }
    
    m_ivHeader = new uint8_t[m_ivHeaderSize];
    start.Read(m_ivHeader, m_ivHeaderSize);
    
    return GetSerializedSize();
}

const uint8_t*
EncryptionHeader::GetIvHeader() const
{
    NS_LOG_FUNCTION(this);
    return m_ivHeader;
}

uint32_t
EncryptionHeader::GetIvHeaderSize() const
{
    NS_LOG_FUNCTION(this);
    return m_ivHeaderSize;
}

} // namespace ns3
